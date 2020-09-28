//==============================================================================
// actionmanager.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "syncmacros.h"
#include "actionmanager.h"
#include "action.h"

//Completed actions.
#include "EntityActionIdle.h"
#include "EntityActionListen.h"

#include "UnitActionAvoidCollisionAir.h"
#include "UnitActionAmmoRegen.h"
#include "UnitActionUnderAttack.h"
#include "UnitActionBuilding.h"
#include "UnitActionDOT.h"
#include "UnitActionCapture.h"
#include "UnitActionJoin.h"
#include "UnitActionChangeMode.h"
#include "UnitActionCollisionAttack.h"
#include "UnitActionAreaAttack.h"
#include "UnitActionDeath.h"
#include "UnitActionInfectDeath.h"
#include "UnitActionDetonate.h"
#include "UnitActionGather.h"
#include "UnitActionHonk.h"
#include "UnitActionMines.h"
#include "UnitActionMove.h"
#include "UnitActionMoveAir.h"
#include "UnitActionMoveWarthog.h"
#include "UnitActionMoveGhost.h"
#include "UnitActionPhysics.h"
#include "UnitActionPlayBlockingAnimation.h"   //DCPTODO: Need to figure out if it's okay that this action calls the Squad's notify.
#include "UnitActionRangedAttack.h"
#include "UnitActionShieldRegen.h"
#include "UnitActionSpawnSquad.h"
#include "UnitActionAmbientLifeSpawner.h"
#include "unitactiongarrison.h"
#include "unitactionungarrison.h"
#include "unitactionsecondaryturretattack.h"
#include "unitactionslaveturretattack.h"
#include "unitactionrevealtoteam.h"
#include "unitactionairtrafficcontrol.h"
#include "unitactionhitch.h"
#include "unitactionunhitch.h"
#include "unitactionthrown.h"
#include "unitactiondodge.h"
#include "unitactiondeflect.h"
#include "unitactionplayattachmentanims.h"
#include "unitactionheal.h"
#include "unitactionrevive.h"
#include "unitactionbuff.h"
#include "unitactioninfect.h"
#include "unitactionhotdrop.h"
#include "unitactiontentacledormant.h"
#include "unitactionherodeath.h"
#include "unitactionstasis.h"
#include "unitactionbubbleshield.h"
#include "unitactionbomb.h"
#include "unitactionplasmashieldgen.h"
#include "unitactionjump.h"
#include "unitactionpointblankattack.h"
#include "unitactionroar.h"
#include "unitactionenergyshield.h"
#include "unitactionscaleLOS.h"
#include "unitactionchargedrangedattack.h"
#include "unitactiontowerwall.h"
#include "unitactionaoeheal.h"
#include "unitactioncoreslide.h"
#include "unitactioninfantryenergyshield.h"
#include "unitactiondome.h"
#include "unitactionrage.h"

#include "SquadActionAttack.h"
#include "SquadActionMove.h"
#include "SquadActionWork.h"
#include "SquadActionShieldRegen.h"
#include "SquadActionChangeMode.h"
#include "squadactiontransport.h"
#include "squadactionreinforce.h"
#include "squadactiongarrison.h"
#include "squadactionungarrison.h"
#include "squadactioncarpetbomb.h"
#include "squadactionairstrike.h"
#include "squadactionrepair.h"
#include "squadactionrepairother.h"
#include "squadactionhitch.h"
#include "squadactionunhitch.h"
#include "squadactiondetonate.h"
#include "squadactionwander.h"
#include "squadactioncloak.h"
#include "squadactioncloakdetect.h"
#include "squadactiondaze.h"
#include "squadactioncryo.h"
#include "squadactionjump.h"
#include "squadactionambientlife.h"
#include "squadactionreflectdamage.h"
#include "squadactionspiritbond.h"

#include "platoonactionmove.h"

//Actions that need to be fixed up.
#include "unitactionchangeowner.h"  //Tommy is working on this one.
#include "squadactionplayblockinganimation.h"  //Tommy

BActionManager gActionManager;

GFIMPLEMENTVERSION(BActionManager, 3);

enum
{
   cSaveMarkerBEntityActionIdle                 = 10000,
   cSaveMarkerBEntityActionListen               ,
   cSaveMarkerBUnitActionMove                   ,
   cSaveMarkerBUnitActionMoveAir                ,
   cSaveMarkerBUnitActionMoveWarthog            ,
   cSaveMarkerBUnitActionMoveGhost              ,
   cSaveMarkerBUnitActionRangedAttack           ,
   cSaveMarkerBUnitActionBuilding               ,
   cSaveMarkerBUnitActionDOT                    ,
   cSaveMarkerBUnitActionChangeMode             ,
   cSaveMarkerBUnitActionDeath                  ,
   cSaveMarkerBUnitActionInfectDeath            ,
   cSaveMarkerBUnitActionGarrison               ,
   cSaveMarkerBUnitActionUngarrison             ,
   cSaveMarkerBUnitActionShieldRegen            ,
   cSaveMarkerBUnitActionHonk                   ,
   cSaveMarkerBUnitActionSpawnSquad             ,
   cSaveMarkerBUnitActionCapture                ,
   cSaveMarkerBUnitActionJoin                   ,
   cSaveMarkerBUnitActionChangeOwner            ,
   cSaveMarkerBUnitActionAmmoRegen              ,
   cSaveMarkerBUnitActionPhysics                ,
   cSaveMarkerBUnitActionPlayBlockingAnimation  ,
   cSaveMarkerBUnitActionMines                  ,
   cSaveMarkerBUnitActionDetonate               ,
   cSaveMarkerBUnitActionGather                 ,
   cSaveMarkerBUnitActionCollisionAttack        ,
   cSaveMarkerBUnitActionAreaAttack             ,
   cSaveMarkerBUnitActionUnderAttack            ,
   cSaveMarkerBUnitActionSecondaryTurretAttack  ,
   cSaveMarkerBUnitActionRevealToTeam           ,
   cSaveMarkerBUnitActionAirTrafficControl      ,
   cSaveMarkerBUnitActionHitch                  ,
   cSaveMarkerBUnitActionUnhitch                ,
   cSaveMarkerBUnitActionSlaveTurretAttack      ,
   cSaveMarkerBUnitActionThrown                 ,
   cSaveMarkerBUnitActionDodge                  ,
   cSaveMarkerBUnitActionDeflect                ,
   cSaveMarkerBUnitActionAvoidCollisionAir      ,
   cSaveMarkerBUnitActionPlayAttachmentAnims    ,
   cSaveMarkerBUnitActionHeal                   ,
   cSaveMarkerBUnitActionRevive                 ,
   cSaveMarkerBUnitActionBuff                   ,
   cSaveMarkerBUnitActionInfect                 ,
   cSaveMarkerBUnitActionHotDrop                ,
   cSaveMarkerBUnitActionTentacleDormant        ,
   cSaveMarkerBUnitActionHeroDeath              ,
   cSaveMarkerBUnitActionStasis                 ,
   cSaveMarkerBUnitActionBubbleShield           ,
   cSaveMarkerBUnitActionBomb                   ,
   cSaveMarkerBUnitActionPlasmaShieldGen        ,
   cSaveMarkerBUnitActionJump                   ,
   cSaveMarkerBUnitActionAmbientLifeSpawner     ,
   cSaveMarkerBUnitActionPointBlankAttack       ,
   cSaveMarkerBUnitActionRoar                   ,
   cSaveMarkerBUnitActionEnergyShield           ,
   cSaveMarkerBUnitActionScaleLOS               ,
   cSaveMarkerBUnitActionChargedRangedAttack    ,
   cSaveMarkerBUnitActionTowerWall              ,
   cSaveMarkerBUnitActionAoeHeal                ,
   cSaveMarkerBSquadActionAttack                ,
   cSaveMarkerBSquadActionChangeMode            ,
   cSaveMarkerBSquadActionRepair                ,
   cSaveMarkerBSquadActionRepairOther           ,
   cSaveMarkerBSquadActionShieldRegen           ,
   cSaveMarkerBSquadActionGarrison              ,
   cSaveMarkerBSquadActionUngarrison            ,
   cSaveMarkerBSquadActionTransport             ,
   cSaveMarkerBSquadActionPlayBlockingAnimation ,
   cSaveMarkerBSquadActionMove                  ,
   cSaveMarkerBSquadActionReinforce             ,
   cSaveMarkerBSquadActionWork                  ,
   cSaveMarkerBSquadActionCarpetBomb            ,
   cSaveMarkerBSquadActionAirStrike             ,
   cSaveMarkerBSquadActionHitch                 ,
   cSaveMarkerBSquadActionUnhitch               ,
   cSaveMarkerBSquadActionDetonate              ,
   cSaveMarkerBSquadActionWander                ,
   cSaveMarkerBSquadActionCloak                 ,
   cSaveMarkerBSquadActionCloakDetect           ,
   cSaveMarkerBSquadActionDaze                  ,
   cSaveMarkerBSquadActionJump                  ,
   cSaveMarkerBSquadActionAmbientLife           ,
   cSaveMarkerBSquadActionReflectDamage         ,
   cSaveMarkerBSquadActionCryo                  ,
   cSaveMarkerBPlatoonActionMove                ,
   cSaveMarkerBUnitActionCoreSlide              ,
   cSaveMarkerBUnitActionInfantryEnergyShield   ,
   cSaveMarkerBUnitActionDome                   ,
   cSaveMarkerBSquadActionSpiritBond            ,
   cSaveMarkerBUnitActionRage                   ,
};

//==============================================================================
//==============================================================================
BActionManager::BActionManager()
{
   init();
}

//==============================================================================
//==============================================================================
BActionManager::~BActionManager()
{
   if (mRefCount != 0)
      blogtrace("Some actions are not being released.");
}

//==============================================================================
//==============================================================================
bool BActionManager::init()
{ 
   mRefCount=0; 
   mActionIndex=0; 

   mActionTypes.add("EntityIdle", BAction::cActionTypeEntityIdle);
   mActionTypes.add("EntityListen", BAction::cActionTypeEntityListen);

   mActionTypes.add("UnitMove", BAction::cActionTypeUnitMove);
   mActionTypes.add("UnitMoveAir", BAction::cActionTypeUnitMoveAir);
   mActionTypes.add("UnitMoveWarthog", BAction::cActionTypeUnitMoveWarthog);
   mActionTypes.add("UnitMoveGhost", BAction::cActionTypeUnitMoveGhost);
   mActionTypes.add("UnitRangedAttack", BAction::cActionTypeUnitRangedAttack);
   mActionTypes.add("UnitBuilding", BAction::cActionTypeUnitBuilding);
   mActionTypes.add("UnitDOT", BAction::cActionTypeUnitDOT);
   mActionTypes.add("UnitChangeMode", BAction::cActionTypeUnitChangeMode);
   mActionTypes.add("UnitDeath", BAction::cActionTypeUnitDeath);
   mActionTypes.add("UnitInfectDeath", BAction::cActionTypeUnitInfectDeath);
   mActionTypes.add("UnitGarrison", BAction::cActionTypeUnitGarrison);
   mActionTypes.add("UnitUngarrison", BAction::cActionTypeUnitUngarrison);
   mActionTypes.add("UnitShieldRegen", BAction::cActionTypeUnitShieldRegen);
   mActionTypes.add("UnitHonk", BAction::cActionTypeUnitHonk);
   mActionTypes.add("UnitSpawnSquad", BAction::cActionTypeUnitSpawnSquad);
   mActionTypes.add("UnitCapture", BAction::cActionTypeUnitCapture);
   mActionTypes.add("UnitJoin", BAction::cActionTypeUnitJoin);
   mActionTypes.add("UnitChangeOwner", BAction::cActionTypeUnitChangeOwner);
   mActionTypes.add("UnitAmmoRegen", BAction::cActionTypeUnitAmmoRegen);
   mActionTypes.add("UnitPhysics", BAction::cActionTypeUnitPhysics);
   mActionTypes.add("UnitPlayBlockingAnimation", BAction::cActionTypeUnitPlayBlockingAnimation);
   mActionTypes.add("UnitMines", BAction::cActionTypeUnitMines);
   mActionTypes.add("UnitDetonate", BAction::cActionTypeUnitDetonate);
   mActionTypes.add("UnitGather", BAction::cActionTypeUnitGather);
   mActionTypes.add("UnitCollisionAttack", BAction::cActionTypeUnitCollisionAttack);
   mActionTypes.add("UnitAreaAttack", BAction::cActionTypeUnitAreaAttack);
   mActionTypes.add("UnitUnderAttack", BAction::cActionTypeUnitUnderAttack);
   mActionTypes.add("UnitSecondaryTurretAttack", BAction::cActionTypeUnitSecondaryTurretAttack);
   mActionTypes.add("UnitRevealToTeam", BAction::cActionTypeUnitRevealToTeam);
   mActionTypes.add("UnitAirTrafficControl", BAction::cActionTypeUnitAirTrafficControl);
   mActionTypes.add("UnitHitch", BAction::cActionTypeUnitHitch);
   mActionTypes.add("UnitUnhitch", BAction::cActionTypeUnitUnhitch);
   mActionTypes.add("UnitSlaveTurretAttack", BAction::cActionTypeUnitSlaveTurretAttack);
   mActionTypes.add("UnitThrown", BAction::cActionTypeUnitThrown);
   mActionTypes.add("UnitDodge", BAction::cActionTypeUnitDodge);
   mActionTypes.add("UnitDeflect", BAction::cActionTypeUnitDeflect);
   mActionTypes.add("UnitAvoidCollisionAir", BAction::cActionTypeUnitAvoidCollisionAir);
   mActionTypes.add("UnitPlayAttachmentAnims", BAction::cActionTypeUnitPlayAttachmentAnims);
   mActionTypes.add("UnitHeal", BAction::cActionTypeUnitHeal);
   mActionTypes.add("UnitRevive", BAction::cActionTypeUnitRevive);
   mActionTypes.add("UnitBuff", BAction::cActionTypeUnitBuff);
   mActionTypes.add("UnitInfect", BAction::cActionTypeUnitInfect);
   mActionTypes.add("UnitHotDrop", BAction::cActionTypeUnitHotDrop);
   mActionTypes.add("UnitTentacleDormant", BAction::cActionTypeUnitTentacleDormant);
   mActionTypes.add("UnitHeroDeath", BAction::cActionTypeUnitHeroDeath);
   mActionTypes.add("UnitStasis", BAction::cActionTypeUnitStasis);
   mActionTypes.add("UnitBubbleShield", BAction::cActionTypeUnitBubbleShield);
   mActionTypes.add("UnitBomb", BAction::cActionTypeUnitBomb);
   mActionTypes.add("UnitPlasmaShieldGen", BAction::cActionTypeUnitPlasmaShieldGen);
   mActionTypes.add("UnitJump", BAction::cActionTypeUnitJump);
   mActionTypes.add("UnitAmbientLifeSpawner", BAction::cActionTypeUnitAmbientLifeSpawner);
   mActionTypes.add("UnitJumpGather", BAction::cActionTypeUnitJumpGather);
   mActionTypes.add("UnitJumpGarrison", BAction::cActionTypeUnitJumpGarrison);
   mActionTypes.add("UnitJumpAttack", BAction::cActionTypeUnitJumpAttack);
   mActionTypes.add("UnitPointBlankAttack", BAction::cActionTypeUnitPointBlankAttack);
   mActionTypes.add("UnitRoar", BAction::cActionTypeUnitRoar);
   mActionTypes.add("UnitEnergyShield", BAction::cActionTypeUnitEnergyShield);
   mActionTypes.add("UnitScaleLOS", BAction::cActionTypeUnitScaleLOS);
   mActionTypes.add("UnitChargedRangedAttack", BAction::cActionTypeUnitChargedRangedAttack);
   mActionTypes.add("UnitTowerWall", BAction::cActionTypeUnitTowerWall);
   mActionTypes.add("UnitAoeHeal", BAction::cActionTypeUnitAoeHeal);

   mActionTypes.add("SquadAttack", BAction::cActionTypeSquadAttack);
   mActionTypes.add("SquadChangeMode", BAction::cActionTypeSquadChangeMode);
   mActionTypes.add("SquadRepair", BAction::cActionTypeSquadRepair);
   mActionTypes.add("SquadRepairOther", BAction::cActionTypeSquadRepairOther);
   mActionTypes.add("SquadShieldRegen", BAction::cActionTypeSquadShieldRegen);
   mActionTypes.add("SquadGarrison", BAction::cActionTypeSquadGarrison);
   mActionTypes.add("SquadUngarrison", BAction::cActionTypeSquadUngarrison);
   mActionTypes.add("SquadTransport", BAction::cActionTypeSquadTransport);
   mActionTypes.add("SquadPlayBlockingAnimation", BAction::cActionTypeSquadPlayBlockingAnimation);
   mActionTypes.add("SquadMove", BAction::cActionTypeSquadMove);
   mActionTypes.add("SquadReinforce", BAction::cActionTypeSquadReinforce);
   mActionTypes.add("SquadWork", BAction::cActionTypeSquadWork);
   mActionTypes.add("SquadCarpetBomb", BAction::cActionTypeSquadCarpetBomb);
   mActionTypes.add("SquadAirStrike", BAction::cActionTypeSquadAirStrike);
   mActionTypes.add("SquadHitch", BAction::cActionTypeSquadHitch);
   mActionTypes.add("SquadUnhitch", BAction::cActionTypeSquadUnhitch);
   mActionTypes.add("SquadDetonate", BAction::cActionTypeSquadDetonate);
   mActionTypes.add("SquadWander", BAction::cActionTypeSquadWander);
   mActionTypes.add("SquadCloak", BAction::cActionTypeSquadCloak);
   mActionTypes.add("SquadCloakDetect", BAction::cActionTypeSquadCloakDetect);
   mActionTypes.add("SquadDaze", BAction::cActionTypeSquadDaze);
   mActionTypes.add("SquadJump", BAction::cActionTypeSquadJump);
   mActionTypes.add("SquadAmbientLife", BAction::cActionTypeSquadAmbientLife);
   mActionTypes.add("SquadReflectDamage", BAction::cActionTypeSquadReflectDamage);
   mActionTypes.add("SquadCryo", BAction::cActionTypeSquadCryo);

   mActionTypes.add("PlatoonMove", BAction::cActionTypePlatoonMove);

   mActionTypes.add("UnitCoreSlide", BAction::cActionTypeUnitCoreSlide);
   mActionTypes.add("UnitInfantryEnergyShield", BAction::cActionTypeUnitInfantryEnergyShield);
   mActionTypes.add("UnitDome", BAction::cActionTypeUnitDome);

   mActionTypes.add("SquadSpiritBond", BAction::cActionTypeSquadSpiritBond);
   mActionTypes.add("UnitRage", BAction::cActionTypeUnitRage);

   return(true);
}

//==============================================================================
//==============================================================================
void BActionManager::reset()
{ 
   mRefCount=0; 
   mActionIndex=0; 

   BEntityActionIdle::mFreeList.clear();
   BEntityActionListen::mFreeList.clear();

   BPlatoonActionMove::mFreeList.clear();

   BSquadActionAirStrike::mFreeList.clear();
   BSquadActionAmbientLife::mFreeList.clear();
   BSquadActionAttack::mFreeList.clear();
   BSquadActionCarpetBomb::mFreeList.clear();
   BSquadActionChangeMode::mFreeList.clear();
   BSquadActionCloak::mFreeList.clear();
   BSquadActionCloakDetect::mFreeList.clear();
   BSquadActionCryo::mFreeList.clear();
   BSquadActionDaze::mFreeList.clear();
   BSquadActionDetonate::mFreeList.clear();
   BSquadActionGarrison::mFreeList.clear();
   BSquadActionHitch::mFreeList.clear();
   BSquadActionJump::mFreeList.clear();
   BSquadActionMove::mFreeList.clear();
   BSquadActionPlayBlockingAnimation::mFreeList.clear();
   BSquadActionReflectDamage::mFreeList.clear();
   BSquadActionReinforce::mFreeList.clear();
   BSquadActionRepair::mFreeList.clear();
   BSquadActionRepairOther::mFreeList.clear();
   BSquadActionShieldRegen::mFreeList.clear();
   BSquadActionTransport::mFreeList.clear();
   BSquadActionUngarrison::mFreeList.clear();
   BSquadActionUnhitch::mFreeList.clear();
   BSquadActionWander::mFreeList.clear();
   BSquadActionWork::mFreeList.clear();
   BSquadActionSpiritBond::mFreeList.clear();

   BUnitActionAirTrafficControl::mFreeList.clear();
   BUnitActionAmbientLifeSpawner::mFreeList.clear();
   BUnitActionAmmoRegen::mFreeList.clear();
   BUnitActionAoeHeal::mFreeList.clear();
   BUnitActionAreaAttack::mFreeList.clear();
   BUnitActionAvoidCollisionAir::mFreeList.clear();
   BUnitActionBomb::mFreeList.clear();
   BUnitActionBubbleShield::mFreeList.clear();
   BUnitActionBuff::mFreeList.clear();
   BUnitActionBuilding::mFreeList.clear();
   BUnitActionCapture::mFreeList.clear();
   BUnitActionChangeMode::mFreeList.clear();
   BUnitActionChangeOwner::mFreeList.clear();
   BUnitActionChargedRangedAttack::mFreeList.clear();
   BUnitActionCollisionAttack::mFreeList.clear();
   BUnitActionCoreSlide::mFreeList.clear();
   BUnitActionDeath::mFreeList.clear();
   BUnitActionDeflect::mFreeList.clear();
   BUnitActionDetonate::mFreeList.clear();
   BUnitActionDodge::mFreeList.clear();
   BUnitActionDome::mFreeList.clear();
   BUnitActionDOT::mFreeList.clear();
   BUnitActionEnergyShield::mFreeList.clear();
   BUnitActionGarrison::mFreeList.clear();
   BUnitActionGather::mFreeList.clear();
   BUnitActionHeal::mFreeList.clear();
   BUnitActionHeroDeath::mFreeList.clear();
   BUnitActionHitch::mFreeList.clear();
   BUnitActionHonk::mFreeList.clear();
   BUnitActionHotDrop::mFreeList.clear();
   BUnitActionInfantryEnergyShield::mFreeList.clear();
   BUnitActionInfect::mFreeList.clear();
   BUnitActionInfectDeath::mFreeList.clear();
   BUnitActionJoin::mFreeList.clear();
   BUnitActionJump::mFreeList.clear();
   BUnitActionMines::mFreeList.clear();
   BUnitActionMove::mFreeList.clear();
   BUnitActionMoveAir::mFreeList.clear();
   BUnitActionMoveGhost::mFreeList.clear();
   BUnitActionMoveWarthog::mFreeList.clear();
   BUnitActionPhysics::mFreeList.clear();
   BUnitActionPlasmaShieldGen::mFreeList.clear();
   BUnitActionPlayAttachmentAnims::mFreeList.clear();
   BUnitActionPlayBlockingAnimation::mFreeList.clear();
   BUnitActionPointBlankAttack::mFreeList.clear();
   BUnitActionRangedAttack::mFreeList.clear();
   BUnitActionRevealToTeam::mFreeList.clear();
   BUnitActionRevive::mFreeList.clear();
   BUnitActionRoar::mFreeList.clear();      
   BUnitActionScaleLOS::mFreeList.clear();
   BUnitActionSecondaryTurretAttack::mFreeList.clear();
   BUnitActionShieldRegen::mFreeList.clear();
   BUnitActionSlaveTurretAttack::mFreeList.clear();
   BUnitActionSpawnSquad::mFreeList.clear();
   BUnitActionStasis::mFreeList.clear();
   BUnitActionTentacleDormant::mFreeList.clear();
   BUnitActionThrown::mFreeList.clear();
   BUnitActionTowerWall::mFreeList.clear();
   BUnitActionUnderAttack::mFreeList.clear();
   BUnitActionUngarrison::mFreeList.clear();
   BUnitActionUnhitch::mFreeList.clear();
   BUnitActionRage::mFreeList.clear();

   BClamshellCollisionListener::mFreeList.clear();
   BDetonateCollisionListener::mFreeList.clear();
}

//==============================================================================
//==============================================================================
BActionType BActionManager::getActionType(const char* pName)
{
   BActionType type; 
   if (mActionTypes.find(pName, &type)) 
      return type;
   else 
      return BAction::cActionTypeInvalid;
}

//==============================================================================
//==============================================================================
const char* BActionManager::getActionName(BActionType type)
{
   if (type<0 || type>=mActionTypes.getTags().getNumber())
      return "Unknown";
   return mActionTypes.getTags().get(type).getPtr();
}


//==============================================================================
//==============================================================================
BAction* BActionManager::createAction(BActionType type)
{
   BAction* pReturn=NULL;   

   switch (type)
   {
      //-- entity actions
      case BAction::cActionTypeEntityIdle:
         pReturn = BEntityActionIdle::getInstance();
         break;
	   case BAction::cActionTypeEntityListen:
		   pReturn = BEntityActionListen::getInstance();
         break;
      //-- unit actions
      case BAction::cActionTypeUnitMove:
         pReturn = BUnitActionMove::getInstance();
         break;
      case BAction::cActionTypeUnitMoveAir:
         pReturn = BUnitActionMoveAir::getInstance();
         break;
      case BAction::cActionTypeUnitMoveWarthog:
         pReturn = BUnitActionMoveWarthog::getInstance();
         break;
      case BAction::cActionTypeUnitMoveGhost:
         pReturn = BUnitActionMoveGhost::getInstance();
         break;
      case BAction::cActionTypeUnitRangedAttack:
         pReturn =  BUnitActionRangedAttack::getInstance();
         break;
      case BAction::cActionTypeUnitBuilding:
         pReturn = BUnitActionBuilding::getInstance();
         break;
      case BAction::cActionTypeUnitChangeMode:
         pReturn = BUnitActionChangeMode::getInstance();
         break;
      case BAction::cActionTypeUnitDeath:
         pReturn = BUnitActionDeath::getInstance();
         break;
      case BAction::cActionTypeUnitInfectDeath:
         pReturn = BUnitActionInfectDeath::getInstance();
         break;
      case BAction::cActionTypeUnitHeroDeath:
         pReturn = BUnitActionHeroDeath::getInstance();
         break;
      case BAction::cActionTypeUnitGarrison:
         pReturn = BUnitActionGarrison::getInstance();
         break;
      case BAction::cActionTypeUnitUngarrison:
         pReturn = BUnitActionUngarrison::getInstance();
         break;
      case BAction::cActionTypeUnitShieldRegen:
         pReturn = BUnitActionShieldRegen::getInstance();
         break;
      case BAction::cActionTypeUnitHonk:
         pReturn = BUnitActionHonk::getInstance();
         break;
      case BAction::cActionTypeUnitSpawnSquad:
         pReturn = BUnitActionSpawnSquad::getInstance();
         break;
      case BAction::cActionTypeUnitAmbientLifeSpawner:
         pReturn = BUnitActionAmbientLifeSpawner::getInstance();
         break;
      case BAction::cActionTypeUnitCapture:
         pReturn = BUnitActionCapture::getInstance();
         break;
      case BAction::cActionTypeUnitJoin:
         pReturn = BUnitActionJoin::getInstance();
         break;
      case BAction::cActionTypeUnitChangeOwner:
         pReturn = BUnitActionChangeOwner::getInstance();
         break;
      case BAction::cActionTypeUnitAmmoRegen:
         pReturn = BUnitActionAmmoRegen::getInstance();
         break;
      case BAction::cActionTypeUnitPhysics:
         pReturn = BUnitActionPhysics::getInstance();
         break;
      case BAction::cActionTypeUnitPlayBlockingAnimation:
         pReturn = BUnitActionPlayBlockingAnimation::getInstance();
         break;
      case BAction::cActionTypeUnitGather:
         pReturn = BUnitActionGather::getInstance();
         break;
      case BAction::cActionTypeUnitMines:
         pReturn = BUnitActionMines::getInstance();
         break;
      case BAction::cActionTypeUnitDetonate:
         pReturn = BUnitActionDetonate::getInstance();
         break;
      case BAction::cActionTypeUnitCollisionAttack:
         pReturn = BUnitActionCollisionAttack::getInstance();
         break;
      case BAction::cActionTypeUnitAreaAttack:
         pReturn = BUnitActionAreaAttack::getInstance();
         break;
      case BAction::cActionTypeUnitUnderAttack:
         pReturn = BUnitActionUnderAttack::getInstance();
         break;
      case BAction::cActionTypeUnitSecondaryTurretAttack:
         pReturn =  BUnitActionSecondaryTurretAttack::getInstance();
         break;
      case BAction::cActionTypeUnitSlaveTurretAttack:
         pReturn =  BUnitActionSlaveTurretAttack::getInstance();
         break;
      case BAction::cActionTypeUnitRevealToTeam:
         pReturn = BUnitActionRevealToTeam::getInstance();
         break;
      case BAction::cActionTypeUnitAirTrafficControl:
         pReturn = BUnitActionAirTrafficControl::getInstance();
         break;
      case BAction::cActionTypeUnitHitch:
         pReturn = BUnitActionHitch::getInstance();
         break;
      case BAction::cActionTypeUnitUnhitch:
         pReturn = BUnitActionUnhitch::getInstance();
         break;
      case BAction::cActionTypeUnitThrown:
         pReturn = BUnitActionThrown::getInstance();
         break;
      case BAction::cActionTypeUnitDodge:
         pReturn = BUnitActionDodge::getInstance();
		 break;
      case BAction::cActionTypeUnitDeflect:
         pReturn = BUnitActionDeflect::getInstance();
         break;
      case BAction::cActionTypeUnitAvoidCollisionAir:
         pReturn = BUnitActionAvoidCollisionAir::getInstance();
         break;
      case BAction::cActionTypeUnitPlayAttachmentAnims:
         pReturn = BUnitActionPlayAttachmentAnims::getInstance();
         break;
      case BAction::cActionTypeUnitDOT:
         pReturn = BUnitActionDOT::getInstance();
         break;
      case BAction::cActionTypeUnitHeal:
         pReturn = BUnitActionHeal::getInstance();
         break;
      case BAction::cActionTypeUnitRevive:
         pReturn = BUnitActionRevive::getInstance();
         break;
      case BAction::cActionTypeUnitBuff:
         pReturn = BUnitActionBuff::getInstance();
         break;
      case BAction::cActionTypeUnitInfect:
         pReturn = BUnitActionInfect::getInstance();
         break;
      case BAction::cActionTypeUnitHotDrop:
         pReturn = BUnitActionHotDrop::getInstance();
         break;
      case BAction::cActionTypeUnitTentacleDormant:
         pReturn = BUnitActionTentacleDormant::getInstance();
         break;
      case BAction::cActionTypeUnitStasis:
         pReturn = BUnitActionStasis::getInstance();
         break;
      case BAction::cActionTypeUnitBubbleShield:
         pReturn = BUnitActionBubbleShield::getInstance();
         break;
      case BAction::cActionTypeUnitBomb:
         pReturn = BUnitActionBomb::getInstance();
         break;
      case BAction::cActionTypeUnitPlasmaShieldGen:
         pReturn = BUnitActionPlasmaShieldGen::getInstance();
         break;
      case BAction::cActionTypeUnitJumpAttack:
      case BAction::cActionTypeUnitJumpGarrison:
      case BAction::cActionTypeUnitJumpGather:
      case BAction::cActionTypeUnitJump:
         pReturn = BUnitActionJump::getInstance();
         break;
      case BAction::cActionTypeUnitPointBlankAttack:
         pReturn = BUnitActionPointBlankAttack::getInstance();
         break;
      case BAction::cActionTypeUnitRoar:
         pReturn = BUnitActionRoar::getInstance();
         break;
      case BAction::cActionTypeUnitEnergyShield:
         pReturn = BUnitActionEnergyShield::getInstance();
         break;
      case BAction::cActionTypeUnitScaleLOS:
         pReturn = BUnitActionScaleLOS::getInstance();
         break;
      case BAction::cActionTypeUnitChargedRangedAttack:
         pReturn = BUnitActionChargedRangedAttack::getInstance();
         break;
      case BAction::cActionTypeUnitTowerWall:
         pReturn = BUnitActionTowerWall::getInstance();
         break;
      case BAction::cActionTypeUnitAoeHeal:
         pReturn = BUnitActionAoeHeal::getInstance();
         break;
      case BAction::cActionTypeUnitCoreSlide:
         pReturn = BUnitActionCoreSlide::getInstance();
         break;
      case BAction::cActionTypeUnitInfantryEnergyShield:
         pReturn = BUnitActionInfantryEnergyShield::getInstance();
         break;
      case BAction::cActionTypeUnitDome:
         pReturn = BUnitActionDome::getInstance();
         break;
      case BAction::cActionTypeUnitRage:
         pReturn = BUnitActionRage::getInstance();
         break;

      //-- squad actions
      case BAction::cActionTypeSquadMove:
         pReturn = BSquadActionMove::getInstance();
         break;
      case BAction::cActionTypeSquadAttack:
         pReturn =  BSquadActionAttack::getInstance();
         break;
      case BAction::cActionTypeSquadChangeMode:
         pReturn =  BSquadActionChangeMode::getInstance();
         break;
      case BAction::cActionTypeSquadRepair:
         pReturn = BSquadActionRepair::getInstance();
         break;
      case BAction::cActionTypeSquadRepairOther:
         pReturn = BSquadActionRepairOther::getInstance();
         break;
      case BAction::cActionTypeSquadShieldRegen:
         pReturn = BSquadActionShieldRegen::getInstance();
         break;
      case BAction::cActionTypeSquadGarrison:
         pReturn = BSquadActionGarrison::getInstance();
         break;
      case BAction::cActionTypeSquadUngarrison:
         pReturn = BSquadActionUngarrison::getInstance();
         break;
      case BAction::cActionTypeSquadTransport:
         pReturn = BSquadActionTransport::getInstance();
         break;
      case BAction::cActionTypeSquadPlayBlockingAnimation:
         pReturn = BSquadActionPlayBlockingAnimation::getInstance();
         break;
      case BAction::cActionTypeSquadReinforce:
         pReturn = BSquadActionReinforce::getInstance();
         break;
      case BAction::cActionTypeSquadWork:
         pReturn = BSquadActionWork::getInstance();
         break;
      case BAction::cActionTypeSquadCarpetBomb:
         pReturn = BSquadActionCarpetBomb::getInstance();
         break;
      case BAction::cActionTypeSquadAirStrike:
         pReturn = BSquadActionAirStrike::getInstance();
         break;
      case BAction::cActionTypeSquadHitch:
         pReturn = BSquadActionHitch::getInstance();
         break;
      case BAction::cActionTypeSquadUnhitch:
         pReturn = BSquadActionUnhitch::getInstance();
         break;
      case BAction::cActionTypePlatoonMove:
         pReturn = BPlatoonActionMove::getInstance();
         break;
      case BAction::cActionTypeSquadDetonate:
         pReturn = BSquadActionDetonate::getInstance();
         break;
      case BAction::cActionTypeSquadWander:
         pReturn = BSquadActionWander::getInstance();
         break;
      case BAction::cActionTypeSquadCloak:
         pReturn = BSquadActionCloak::getInstance();
		 break;
      case BAction::cActionTypeSquadCloakDetect:
         pReturn = BSquadActionCloakDetect::getInstance();
         break;
      case BAction::cActionTypeSquadDaze:
         pReturn = BSquadActionDaze::getInstance();
         break;
      case BAction::cActionTypeSquadJump:
         pReturn = BSquadActionJump::getInstance();
         break;
      case BAction::cActionTypeSquadAmbientLife:
         pReturn = BSquadActionAmbientLife::getInstance();
         break;
      case BAction::cActionTypeSquadReflectDamage:
         pReturn = BSquadActionReflectDamage::getInstance();
         break;
      case BAction::cActionTypeSquadCryo:
         pReturn = BSquadActionCryo::getInstance();
         break;
      case BAction::cActionTypeSquadSpiritBond:
         pReturn = BSquadActionSpiritBond::getInstance();
         break;
      default:  BFAIL("Unknown action type in createAction() !");
         break;
   }

   if (!pReturn)
      return(NULL);

   //Init said action.
   pReturn->init();

   //Create ID and stick it into action.
   mActionIndex++;
   BActionID id=CREATEACTIONID(type, mActionIndex);
   pReturn->setID(id);
   mRefCount++;

#ifdef SYNC_UnitAction
   syncUnitActionData("BActionManager::createAction pReturn ID", (int)pReturn->getID());
   syncUnitActionData("BActionManager::createAction pReturn Type", (int)pReturn->getType());
#endif

   return(pReturn);
}

//==============================================================================
//==============================================================================
#define RELEASE_ACTION_INSTANCE(classType,pAction) if (pAction->getOwner()) { pAction->disconnect(); classType::releaseInstance((classType*)pAction); } else classType::mFreeList.release((classType*)pAction);
void BActionManager::releaseAction(BAction* pAction)
{   
   if (!pAction)
      return;   

#ifdef SYNC_UnitAction
   syncUnitActionData("BActionManager::releaseAction pAction ID", (int)pAction->getID());
   syncUnitActionData("BActionManager::releaseAction pAction Type", (int)pAction->getType());
#endif

   if (pAction->getOwner())
      syncUnitData("BActionManager::releaseAction pAction owner ID", (int)pAction->getOwner()->getID());

   BActionType type=pAction->getType();
   switch (type)
   {
      //-- entity actions
      case BAction::cActionTypeEntityIdle:
         RELEASE_ACTION_INSTANCE(BEntityActionIdle, pAction);
         break;
      case BAction::cActionTypeEntityListen:
         RELEASE_ACTION_INSTANCE(BEntityActionListen, pAction);
         break;
         
      //-- unit actions
      case BAction::cActionTypeUnitMove:
         RELEASE_ACTION_INSTANCE(BUnitActionMove, pAction);
         break;
      case BAction::cActionTypeUnitMoveAir:
         RELEASE_ACTION_INSTANCE(BUnitActionMoveAir, pAction);
         break;
      case BAction::cActionTypeUnitMoveWarthog:
         RELEASE_ACTION_INSTANCE(BUnitActionMoveWarthog, pAction);
         break;
      case BAction::cActionTypeUnitMoveGhost:
         RELEASE_ACTION_INSTANCE(BUnitActionMoveGhost, pAction);
         break;
      case BAction::cActionTypeUnitRangedAttack:
         RELEASE_ACTION_INSTANCE(BUnitActionRangedAttack, pAction);
         break;
      case BAction::cActionTypeUnitBuilding:
         RELEASE_ACTION_INSTANCE(BUnitActionBuilding, pAction);
         break;
      case BAction::cActionTypeUnitChangeMode:
         RELEASE_ACTION_INSTANCE(BUnitActionChangeMode, pAction);
         break;
      case BAction::cActionTypeUnitDeath:
         RELEASE_ACTION_INSTANCE(BUnitActionDeath, pAction);
         break;
      case BAction::cActionTypeUnitInfectDeath:
         RELEASE_ACTION_INSTANCE(BUnitActionInfectDeath, pAction);
         break;
      case BAction::cActionTypeUnitHeroDeath:
         RELEASE_ACTION_INSTANCE(BUnitActionHeroDeath, pAction);
         break;
      case BAction::cActionTypeUnitGarrison:
         RELEASE_ACTION_INSTANCE(BUnitActionGarrison, pAction);
         break;
      case BAction::cActionTypeUnitUngarrison:
         RELEASE_ACTION_INSTANCE(BUnitActionUngarrison, pAction);
         break;
      case BAction::cActionTypeUnitShieldRegen:
         RELEASE_ACTION_INSTANCE(BUnitActionShieldRegen, pAction);
         break;
      case BAction::cActionTypeUnitHonk:
         RELEASE_ACTION_INSTANCE(BUnitActionHonk, pAction);
         break;
      case BAction::cActionTypeUnitSpawnSquad:
         RELEASE_ACTION_INSTANCE(BUnitActionSpawnSquad, pAction);
         break;
      case BAction::cActionTypeUnitAmbientLifeSpawner:
         RELEASE_ACTION_INSTANCE(BUnitActionAmbientLifeSpawner, pAction);
         break;
      case BAction::cActionTypeUnitChangeOwner:
         RELEASE_ACTION_INSTANCE(BUnitActionChangeOwner, pAction);
         break;
      case BAction::cActionTypeUnitAmmoRegen:
         RELEASE_ACTION_INSTANCE(BUnitActionAmmoRegen, pAction);
         break;
      case BAction::cActionTypeUnitPlayBlockingAnimation:
         RELEASE_ACTION_INSTANCE(BUnitActionPlayBlockingAnimation, pAction);
         break;
      case BAction::cActionTypeUnitDetonate:
         RELEASE_ACTION_INSTANCE(BUnitActionDetonate, pAction);
         break;
      case BAction::cActionTypeUnitMines:
         RELEASE_ACTION_INSTANCE(BUnitActionMines, pAction);
         break;
      case BAction::cActionTypeUnitPhysics:
         RELEASE_ACTION_INSTANCE(BUnitActionPhysics, pAction);
         break;
      case BAction::cActionTypeUnitGather:
         RELEASE_ACTION_INSTANCE(BUnitActionGather, pAction);
         break;
      case BAction::cActionTypeUnitCapture:
         RELEASE_ACTION_INSTANCE(BUnitActionCapture, pAction);
         break;
      case BAction::cActionTypeUnitJoin:
         RELEASE_ACTION_INSTANCE(BUnitActionJoin, pAction);
         break;
      case BAction::cActionTypeUnitCollisionAttack:
         RELEASE_ACTION_INSTANCE(BUnitActionCollisionAttack, pAction);
         break;
      case BAction::cActionTypeUnitAreaAttack:
         RELEASE_ACTION_INSTANCE(BUnitActionAreaAttack, pAction);
         break;
      case BAction::cActionTypeUnitUnderAttack:
         RELEASE_ACTION_INSTANCE(BUnitActionUnderAttack, pAction);
         break;
      case BAction::cActionTypeUnitSecondaryTurretAttack:
         RELEASE_ACTION_INSTANCE(BUnitActionSecondaryTurretAttack, pAction);
         break;
      case BAction::cActionTypeUnitSlaveTurretAttack:
         RELEASE_ACTION_INSTANCE(BUnitActionSlaveTurretAttack, pAction);
         break;
      case BAction::cActionTypeUnitRevealToTeam:
         RELEASE_ACTION_INSTANCE(BUnitActionRevealToTeam, pAction);
         break;
      case BAction::cActionTypeUnitAirTrafficControl:
         RELEASE_ACTION_INSTANCE(BUnitActionAirTrafficControl, pAction);
         break;
      case BAction::cActionTypeUnitHitch:
         RELEASE_ACTION_INSTANCE(BUnitActionHitch, pAction);
         break;
      case BAction::cActionTypeUnitUnhitch:
         RELEASE_ACTION_INSTANCE(BUnitActionUnhitch, pAction);
         break;
      case BAction::cActionTypeUnitThrown:
         RELEASE_ACTION_INSTANCE(BUnitActionThrown, pAction);
         break;
      case BAction::cActionTypeUnitDodge:
         RELEASE_ACTION_INSTANCE(BUnitActionDodge, pAction);
         break;
      case BAction::cActionTypeUnitDeflect:
         RELEASE_ACTION_INSTANCE(BUnitActionDeflect, pAction);
         break;
      case BAction::cActionTypeUnitAvoidCollisionAir:
         RELEASE_ACTION_INSTANCE(BUnitActionAvoidCollisionAir, pAction);
         break;
      case BAction::cActionTypeUnitPlayAttachmentAnims:
         RELEASE_ACTION_INSTANCE(BUnitActionPlayAttachmentAnims, pAction);
         break;
      case BAction::cActionTypeUnitDOT:
         RELEASE_ACTION_INSTANCE(BUnitActionDOT, pAction);
         break;
      case BAction::cActionTypeUnitHotDrop:
         RELEASE_ACTION_INSTANCE(BUnitActionHotDrop, pAction);
         break;
      case BAction::cActionTypeUnitTentacleDormant:
         RELEASE_ACTION_INSTANCE(BUnitActionTentacleDormant, pAction);
         break;
      case BAction::cActionTypeUnitStasis:
         RELEASE_ACTION_INSTANCE(BUnitActionStasis, pAction);
         break;
      case BAction::cActionTypeUnitBubbleShield:
         RELEASE_ACTION_INSTANCE(BUnitActionBubbleShield, pAction);
         break;
      case BAction::cActionTypeUnitBomb:
         RELEASE_ACTION_INSTANCE(BUnitActionBomb, pAction);
         break;
      case BAction::cActionTypeUnitPlasmaShieldGen:
         RELEASE_ACTION_INSTANCE(BUnitActionPlasmaShieldGen, pAction);
         break;
      case BAction::cActionTypeUnitJumpAttack:
      case BAction::cActionTypeUnitJumpGarrison:
      case BAction::cActionTypeUnitJumpGather:
      case BAction::cActionTypeUnitJump:
         RELEASE_ACTION_INSTANCE(BUnitActionJump, pAction);
         break;
      case BAction::cActionTypeUnitPointBlankAttack:
         RELEASE_ACTION_INSTANCE(BUnitActionPointBlankAttack, pAction);
         break;
      case BAction::cActionTypeUnitRoar:
         RELEASE_ACTION_INSTANCE(BUnitActionRoar, pAction);
         break;
      case BAction::cActionTypeUnitEnergyShield:
         RELEASE_ACTION_INSTANCE(BUnitActionEnergyShield, pAction);
         break;
      case BAction::cActionTypeUnitChargedRangedAttack:
         RELEASE_ACTION_INSTANCE(BUnitActionChargedRangedAttack, pAction);
         break;
      case BAction::cActionTypeUnitTowerWall:
         RELEASE_ACTION_INSTANCE(BUnitActionTowerWall, pAction);
         break;
      case BAction::cActionTypeUnitAoeHeal:
         RELEASE_ACTION_INSTANCE(BUnitActionAoeHeal, pAction);
         break;
      case BAction::cActionTypeUnitCoreSlide:
         RELEASE_ACTION_INSTANCE(BUnitActionCoreSlide, pAction);
         break;
      case BAction::cActionTypeUnitInfantryEnergyShield:
         RELEASE_ACTION_INSTANCE(BUnitActionInfantryEnergyShield, pAction);
         break;
      case BAction::cActionTypeUnitDome:
         RELEASE_ACTION_INSTANCE(BUnitActionDome, pAction);
         break;
      case BAction::cActionTypeUnitRage:
         RELEASE_ACTION_INSTANCE(BUnitActionRage, pAction);
         break;
         
      //-- squad actions
      case BAction::cActionTypeSquadMove:
         RELEASE_ACTION_INSTANCE(BSquadActionMove, pAction);
         break;
      case BAction::cActionTypeSquadAttack:
         RELEASE_ACTION_INSTANCE(BSquadActionAttack, pAction);
         break;
      case BAction::cActionTypeSquadChangeMode:
         RELEASE_ACTION_INSTANCE(BSquadActionChangeMode, pAction);
         break;
      case BAction::cActionTypeSquadRepair:
         RELEASE_ACTION_INSTANCE(BSquadActionRepair, pAction);
         break;
      case BAction::cActionTypeSquadCloak:
         RELEASE_ACTION_INSTANCE(BSquadActionCloak, pAction);
         break;
      case BAction::cActionTypeSquadCloakDetect:
         RELEASE_ACTION_INSTANCE(BSquadActionCloakDetect, pAction);
         break;
      case BAction::cActionTypeSquadDaze:
         RELEASE_ACTION_INSTANCE(BSquadActionDaze, pAction);
         break;
      case BAction::cActionTypeSquadRepairOther:
         RELEASE_ACTION_INSTANCE(BSquadActionRepairOther, pAction);
         break;
      case BAction::cActionTypeSquadReflectDamage:
         RELEASE_ACTION_INSTANCE(BSquadActionReflectDamage, pAction);
         break;
      case BAction::cActionTypeSquadCryo:
         RELEASE_ACTION_INSTANCE(BSquadActionCryo, pAction);
         break;
      case BAction::cActionTypeUnitHeal:
         RELEASE_ACTION_INSTANCE(BUnitActionHeal, pAction);
         break;
      case BAction::cActionTypeUnitRevive:
         RELEASE_ACTION_INSTANCE(BUnitActionRevive, pAction);
         break;
      case BAction::cActionTypeUnitBuff:
         RELEASE_ACTION_INSTANCE(BUnitActionBuff, pAction);
         break;
      case BAction::cActionTypeUnitInfect:
         RELEASE_ACTION_INSTANCE(BUnitActionInfect, pAction);
         break;
      case BAction::cActionTypeUnitScaleLOS:
         RELEASE_ACTION_INSTANCE(BUnitActionScaleLOS, pAction);
         break;
      case BAction::cActionTypeSquadShieldRegen:
         RELEASE_ACTION_INSTANCE(BSquadActionShieldRegen, pAction);
         break;
      case BAction::cActionTypeSquadGarrison:
         RELEASE_ACTION_INSTANCE(BSquadActionGarrison, pAction);
         break;
      case BAction::cActionTypeSquadUngarrison:
         RELEASE_ACTION_INSTANCE(BSquadActionUngarrison, pAction);
         break;
      case BAction::cActionTypeSquadTransport:
         RELEASE_ACTION_INSTANCE(BSquadActionTransport, pAction);
         break;
      case BAction::cActionTypeSquadPlayBlockingAnimation:
         RELEASE_ACTION_INSTANCE(BSquadActionPlayBlockingAnimation, pAction);
         break;
      case BAction::cActionTypeSquadReinforce:
         RELEASE_ACTION_INSTANCE(BSquadActionReinforce, pAction);
         break;
      case BAction::cActionTypeSquadWork:
         RELEASE_ACTION_INSTANCE(BSquadActionWork, pAction);
         break;
      case BAction::cActionTypeSquadCarpetBomb:
         RELEASE_ACTION_INSTANCE(BSquadActionCarpetBomb, pAction);
         break;
      case BAction::cActionTypeSquadAirStrike:
         RELEASE_ACTION_INSTANCE(BSquadActionAirStrike, pAction);
         break;
      case BAction::cActionTypeSquadHitch:
         RELEASE_ACTION_INSTANCE(BSquadActionHitch, pAction);
         break;
      case BAction::cActionTypeSquadUnhitch:
         RELEASE_ACTION_INSTANCE(BSquadActionUnhitch, pAction);
         break;
      case BAction::cActionTypePlatoonMove:
         RELEASE_ACTION_INSTANCE(BPlatoonActionMove, pAction);
         break;
      case BAction::cActionTypeSquadDetonate:
         RELEASE_ACTION_INSTANCE(BSquadActionDetonate, pAction);
         break;
      case BAction::cActionTypeSquadWander:
         RELEASE_ACTION_INSTANCE(BSquadActionWander, pAction);
         break;
      case BAction::cActionTypeSquadJump:
         RELEASE_ACTION_INSTANCE(BSquadActionJump, pAction);
         break;
      case BAction::cActionTypeSquadAmbientLife:
         RELEASE_ACTION_INSTANCE(BSquadActionAmbientLife, pAction);
		 break;
      case BAction::cActionTypeSquadSpiritBond:
         RELEASE_ACTION_INSTANCE(BSquadActionSpiritBond, pAction);
         break;
      default:  BFAIL("Unknown action type in releaseAction() !");
         break;
   }

   mRefCount--;
}

//==============================================================================
//==============================================================================
bool BActionManager::savePtr(BStream* pStream, const BAction* pAction)
{
   bool action = (pAction != NULL);
   GFWRITEVAR(pStream, bool, action);
   if (pAction)
   {
      BActionType type = pAction->getType();
      GFWRITEVAR(pStream, BActionType, type);
      switch (type)
      {
         case BAction::cActionTypeEntityIdle                   : GFWRITEFREELISTITEMPTR(pStream, BEntityActionIdle          , pAction); break;
         case BAction::cActionTypeEntityListen                 : GFWRITEFREELISTITEMPTR(pStream, BEntityActionListen        , pAction); break;
         case BAction::cActionTypeUnitMove                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionMove            , pAction); break;
         case BAction::cActionTypeUnitMoveAir                  : GFWRITEFREELISTITEMPTR(pStream, BUnitActionMoveAir         , pAction); break;
         case BAction::cActionTypeUnitMoveWarthog              : GFWRITEFREELISTITEMPTR(pStream, BUnitActionMoveWarthog     , pAction); break;
         case BAction::cActionTypeUnitMoveGhost                : GFWRITEFREELISTITEMPTR(pStream, BUnitActionMoveGhost       , pAction); break;
         case BAction::cActionTypeUnitRangedAttack             : GFWRITEFREELISTITEMPTR(pStream, BUnitActionRangedAttack    , pAction); break;
         case BAction::cActionTypeUnitBuilding                 : GFWRITEFREELISTITEMPTR(pStream, BUnitActionBuilding        , pAction); break;
         case BAction::cActionTypeUnitDOT                      : GFWRITEFREELISTITEMPTR(pStream, BUnitActionDOT             , pAction); break;
         case BAction::cActionTypeUnitChangeMode               : GFWRITEFREELISTITEMPTR(pStream, BUnitActionChangeMode      , pAction); break;
         case BAction::cActionTypeUnitDeath                    : GFWRITEFREELISTITEMPTR(pStream, BUnitActionDeath           , pAction); break;
         case BAction::cActionTypeUnitInfectDeath              : GFWRITEFREELISTITEMPTR(pStream, BUnitActionInfectDeath     , pAction); break;
         case BAction::cActionTypeUnitGarrison                 : GFWRITEFREELISTITEMPTR(pStream, BUnitActionGarrison        , pAction); break;
         case BAction::cActionTypeUnitUngarrison               : GFWRITEFREELISTITEMPTR(pStream, BUnitActionUngarrison      , pAction); break;
         case BAction::cActionTypeUnitShieldRegen              : GFWRITEFREELISTITEMPTR(pStream, BUnitActionShieldRegen     , pAction); break;
         case BAction::cActionTypeUnitHonk                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionHonk            , pAction); break;
         case BAction::cActionTypeUnitSpawnSquad               : GFWRITEFREELISTITEMPTR(pStream, BUnitActionSpawnSquad      , pAction); break;
         case BAction::cActionTypeUnitCapture                  : GFWRITEFREELISTITEMPTR(pStream, BUnitActionCapture         , pAction); break;
         case BAction::cActionTypeUnitJoin                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionJoin            , pAction); break;
         case BAction::cActionTypeUnitChangeOwner              : GFWRITEFREELISTITEMPTR(pStream, BUnitActionChangeOwner     , pAction); break;
         case BAction::cActionTypeUnitAmmoRegen                : GFWRITEFREELISTITEMPTR(pStream, BUnitActionAmmoRegen       , pAction); break;
         case BAction::cActionTypeUnitPhysics                  : GFWRITEFREELISTITEMPTR(pStream, BUnitActionPhysics         , pAction); break;
         case BAction::cActionTypeUnitPlayBlockingAnimation    : GFWRITEFREELISTITEMPTR(pStream, BUnitActionPlayBlockingAnimation    , pAction); break;
         case BAction::cActionTypeUnitMines                    : GFWRITEFREELISTITEMPTR(pStream, BUnitActionMines           , pAction); break;
         case BAction::cActionTypeUnitDetonate                 : GFWRITEFREELISTITEMPTR(pStream, BUnitActionDetonate        , pAction); break;
         case BAction::cActionTypeUnitGather                   : GFWRITEFREELISTITEMPTR(pStream, BUnitActionGather          , pAction); break;
         case BAction::cActionTypeUnitCollisionAttack          : GFWRITEFREELISTITEMPTR(pStream, BUnitActionCollisionAttack , pAction); break;
         case BAction::cActionTypeUnitAreaAttack               : GFWRITEFREELISTITEMPTR(pStream, BUnitActionAreaAttack      , pAction); break;
         case BAction::cActionTypeUnitUnderAttack              : GFWRITEFREELISTITEMPTR(pStream, BUnitActionUnderAttack     , pAction); break;
         case BAction::cActionTypeUnitSecondaryTurretAttack    : GFWRITEFREELISTITEMPTR(pStream, BUnitActionSecondaryTurretAttack    , pAction); break;
         case BAction::cActionTypeUnitRevealToTeam             : GFWRITEFREELISTITEMPTR(pStream, BUnitActionRevealToTeam    , pAction); break;
         case BAction::cActionTypeUnitAirTrafficControl        : GFWRITEFREELISTITEMPTR(pStream, BUnitActionAirTrafficControl        , pAction); break;
         case BAction::cActionTypeUnitHitch                    : GFWRITEFREELISTITEMPTR(pStream, BUnitActionHitch           , pAction); break;
         case BAction::cActionTypeUnitUnhitch                  : GFWRITEFREELISTITEMPTR(pStream, BUnitActionUnhitch         , pAction); break;
         case BAction::cActionTypeUnitSlaveTurretAttack        : GFWRITEFREELISTITEMPTR(pStream, BUnitActionSlaveTurretAttack        , pAction); break;
         case BAction::cActionTypeUnitThrown                   : GFWRITEFREELISTITEMPTR(pStream, BUnitActionThrown          , pAction); break;
         case BAction::cActionTypeUnitDodge                    : GFWRITEFREELISTITEMPTR(pStream, BUnitActionDodge           , pAction); break;
         case BAction::cActionTypeUnitDeflect                  : GFWRITEFREELISTITEMPTR(pStream, BUnitActionDeflect         , pAction); break;
         case BAction::cActionTypeUnitAvoidCollisionAir        : GFWRITEFREELISTITEMPTR(pStream, BUnitActionAvoidCollisionAir        , pAction); break;
         case BAction::cActionTypeUnitPlayAttachmentAnims      : GFWRITEFREELISTITEMPTR(pStream, BUnitActionPlayAttachmentAnims      , pAction); break;
         case BAction::cActionTypeUnitHeal                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionHeal            , pAction); break;
         case BAction::cActionTypeUnitRevive                   : GFWRITEFREELISTITEMPTR(pStream, BUnitActionRevive          , pAction); break;
         case BAction::cActionTypeUnitBuff                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionBuff            , pAction); break;
         case BAction::cActionTypeUnitInfect                   : GFWRITEFREELISTITEMPTR(pStream, BUnitActionInfect          , pAction); break;
         case BAction::cActionTypeUnitHotDrop                  : GFWRITEFREELISTITEMPTR(pStream, BUnitActionHotDrop         , pAction); break;
         case BAction::cActionTypeUnitTentacleDormant          : GFWRITEFREELISTITEMPTR(pStream, BUnitActionTentacleDormant , pAction); break;
         case BAction::cActionTypeUnitHeroDeath                : GFWRITEFREELISTITEMPTR(pStream, BUnitActionHeroDeath       , pAction); break;
         case BAction::cActionTypeUnitStasis                   : GFWRITEFREELISTITEMPTR(pStream, BUnitActionStasis          , pAction); break;
         case BAction::cActionTypeUnitBubbleShield             : GFWRITEFREELISTITEMPTR(pStream, BUnitActionBubbleShield    , pAction); break;
         case BAction::cActionTypeUnitBomb                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionBomb            , pAction); break;
         case BAction::cActionTypeUnitPlasmaShieldGen          : GFWRITEFREELISTITEMPTR(pStream, BUnitActionPlasmaShieldGen , pAction); break;
         case BAction::cActionTypeUnitJump                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionJump            , pAction); break;
         case BAction::cActionTypeUnitAmbientLifeSpawner       : GFWRITEFREELISTITEMPTR(pStream, BUnitActionAmbientLifeSpawner       , pAction); break;
         case BAction::cActionTypeUnitJumpGather               : GFWRITEFREELISTITEMPTR(pStream, BUnitActionJump            , pAction); break;
         case BAction::cActionTypeUnitJumpGarrison             : GFWRITEFREELISTITEMPTR(pStream, BUnitActionJump            , pAction); break;
         case BAction::cActionTypeUnitJumpAttack               : GFWRITEFREELISTITEMPTR(pStream, BUnitActionJump            , pAction); break;
         case BAction::cActionTypeUnitPointBlankAttack         : GFWRITEFREELISTITEMPTR(pStream, BUnitActionPointBlankAttack, pAction); break;
         case BAction::cActionTypeUnitRoar                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionRoar            , pAction); break;
         case BAction::cActionTypeUnitEnergyShield             : GFWRITEFREELISTITEMPTR(pStream, BUnitActionEnergyShield    , pAction); break;
         case BAction::cActionTypeUnitScaleLOS                 : GFWRITEFREELISTITEMPTR(pStream, BUnitActionScaleLOS        , pAction); break;
         case BAction::cActionTypeUnitChargedRangedAttack      : GFWRITEFREELISTITEMPTR(pStream, BUnitActionChargedRangedAttack      , pAction); break;
         case BAction::cActionTypeUnitTowerWall                : GFWRITEFREELISTITEMPTR(pStream, BUnitActionTowerWall       , pAction); break;
         case BAction::cActionTypeUnitAoeHeal                  : GFWRITEFREELISTITEMPTR(pStream, BUnitActionAoeHeal         , pAction); break;
         case BAction::cActionTypeUnitCoreSlide                : GFWRITEFREELISTITEMPTR(pStream, BUnitActionCoreSlide       , pAction); break;
         case BAction::cActionTypeUnitInfantryEnergyShield     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionInfantryEnergyShield     , pAction); break;
         case BAction::cActionTypeUnitDome                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionDome            , pAction); break;
         case BAction::cActionTypeUnitRage                     : GFWRITEFREELISTITEMPTR(pStream, BUnitActionRage            , pAction); break;

         case BAction::cActionTypeSquadAttack                  : GFWRITEFREELISTITEMPTR(pStream, BSquadActionAttack         , pAction); break;
         case BAction::cActionTypeSquadChangeMode              : GFWRITEFREELISTITEMPTR(pStream, BSquadActionChangeMode     , pAction); break;
         case BAction::cActionTypeSquadRepair                  : GFWRITEFREELISTITEMPTR(pStream, BSquadActionRepair         , pAction); break;
         case BAction::cActionTypeSquadRepairOther             : GFWRITEFREELISTITEMPTR(pStream, BSquadActionRepairOther    , pAction); break;
         case BAction::cActionTypeSquadShieldRegen             : GFWRITEFREELISTITEMPTR(pStream, BSquadActionShieldRegen    , pAction); break;
         case BAction::cActionTypeSquadGarrison                : GFWRITEFREELISTITEMPTR(pStream, BSquadActionGarrison       , pAction); break;
         case BAction::cActionTypeSquadUngarrison              : GFWRITEFREELISTITEMPTR(pStream, BSquadActionUngarrison     , pAction); break;
         case BAction::cActionTypeSquadTransport               : GFWRITEFREELISTITEMPTR(pStream, BSquadActionTransport      , pAction); break;
         case BAction::cActionTypeSquadPlayBlockingAnimation   : GFWRITEFREELISTITEMPTR(pStream, BSquadActionPlayBlockingAnimation   , pAction); break;
         case BAction::cActionTypeSquadMove                    : GFWRITEFREELISTITEMPTR(pStream, BSquadActionMove           , pAction); break;
         case BAction::cActionTypeSquadReinforce               : GFWRITEFREELISTITEMPTR(pStream, BSquadActionReinforce      , pAction); break;
         case BAction::cActionTypeSquadWork                    : GFWRITEFREELISTITEMPTR(pStream, BSquadActionWork           , pAction); break;
         case BAction::cActionTypeSquadCarpetBomb              : GFWRITEFREELISTITEMPTR(pStream, BSquadActionCarpetBomb     , pAction); break;
         case BAction::cActionTypeSquadAirStrike               : GFWRITEFREELISTITEMPTR(pStream, BSquadActionAirStrike      , pAction); break;
         case BAction::cActionTypeSquadHitch                   : GFWRITEFREELISTITEMPTR(pStream, BSquadActionHitch          , pAction); break;
         case BAction::cActionTypeSquadUnhitch                 : GFWRITEFREELISTITEMPTR(pStream, BSquadActionUnhitch        , pAction); break;
         case BAction::cActionTypeSquadDetonate                : GFWRITEFREELISTITEMPTR(pStream, BSquadActionDetonate       , pAction); break;
         case BAction::cActionTypeSquadWander                  : GFWRITEFREELISTITEMPTR(pStream, BSquadActionWander         , pAction); break;
         case BAction::cActionTypeSquadCloak                   : GFWRITEFREELISTITEMPTR(pStream, BSquadActionCloak          , pAction); break;
         case BAction::cActionTypeSquadCloakDetect             : GFWRITEFREELISTITEMPTR(pStream, BSquadActionCloakDetect    , pAction); break;
         case BAction::cActionTypeSquadDaze                    : GFWRITEFREELISTITEMPTR(pStream, BSquadActionDaze           , pAction); break;
         case BAction::cActionTypeSquadJump                    : GFWRITEFREELISTITEMPTR(pStream, BSquadActionJump           , pAction); break;
         case BAction::cActionTypeSquadAmbientLife             : GFWRITEFREELISTITEMPTR(pStream, BSquadActionAmbientLife    , pAction); break;
         case BAction::cActionTypeSquadReflectDamage           : GFWRITEFREELISTITEMPTR(pStream, BSquadActionReflectDamage  , pAction); break;
         case BAction::cActionTypeSquadCryo                    : GFWRITEFREELISTITEMPTR(pStream, BSquadActionCryo           , pAction); break;
         case BAction::cActionTypeSquadSpiritBond              : GFWRITEFREELISTITEMPTR(pStream, BSquadActionSpiritBond     , pAction); break;
         case BAction::cActionTypePlatoonMove                  : GFWRITEFREELISTITEMPTR(pStream, BPlatoonActionMove         , pAction); break;
         default: 
         {
            #ifndef BUILD_FINAL
               BSimString msg;
               msg.format("GameFile Error: no savePtr case for action type %d %s", (int)type, getActionName(type));
               BASSERTM(0, msg.getPtr());
            #endif   
            break;
         }
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BActionManager::loadPtr(BStream* pStream, BAction** ppAction)
{
   BAction* pAction = NULL;
   bool action;
   GFREADVAR(pStream, bool, action);
   if (action)
   {
      BActionType type;
      GFREADVAR(pStream, BActionType, type);
      switch (type)
      {
         case BAction::cActionTypeEntityIdle                   : GFREADFREELISTITEMPTR(pStream, BEntityActionIdle          , pAction); break;
         case BAction::cActionTypeEntityListen                 : GFREADFREELISTITEMPTR(pStream, BEntityActionListen        , pAction); break;
         case BAction::cActionTypeUnitMove                     : GFREADFREELISTITEMPTR(pStream, BUnitActionMove            , pAction); break;
         case BAction::cActionTypeUnitMoveAir                  : GFREADFREELISTITEMPTR(pStream, BUnitActionMoveAir         , pAction); break;
         case BAction::cActionTypeUnitMoveWarthog              : GFREADFREELISTITEMPTR(pStream, BUnitActionMoveWarthog     , pAction); break;
         case BAction::cActionTypeUnitMoveGhost                : GFREADFREELISTITEMPTR(pStream, BUnitActionMoveGhost       , pAction); break;
         case BAction::cActionTypeUnitRangedAttack             : GFREADFREELISTITEMPTR(pStream, BUnitActionRangedAttack    , pAction); break;
         case BAction::cActionTypeUnitBuilding                 : GFREADFREELISTITEMPTR(pStream, BUnitActionBuilding        , pAction); break;
         case BAction::cActionTypeUnitDOT                      : GFREADFREELISTITEMPTR(pStream, BUnitActionDOT             , pAction); break;
         case BAction::cActionTypeUnitChangeMode               : GFREADFREELISTITEMPTR(pStream, BUnitActionChangeMode      , pAction); break;
         case BAction::cActionTypeUnitDeath                    : GFREADFREELISTITEMPTR(pStream, BUnitActionDeath           , pAction); break;
         case BAction::cActionTypeUnitInfectDeath              : GFREADFREELISTITEMPTR(pStream, BUnitActionInfectDeath     , pAction); break;
         case BAction::cActionTypeUnitGarrison                 : GFREADFREELISTITEMPTR(pStream, BUnitActionGarrison        , pAction); break;
         case BAction::cActionTypeUnitUngarrison               : GFREADFREELISTITEMPTR(pStream, BUnitActionUngarrison      , pAction); break;
         case BAction::cActionTypeUnitShieldRegen              : GFREADFREELISTITEMPTR(pStream, BUnitActionShieldRegen     , pAction); break;
         case BAction::cActionTypeUnitHonk                     : GFREADFREELISTITEMPTR(pStream, BUnitActionHonk            , pAction); break;
         case BAction::cActionTypeUnitSpawnSquad               : GFREADFREELISTITEMPTR(pStream, BUnitActionSpawnSquad      , pAction); break;
         case BAction::cActionTypeUnitCapture                  : GFREADFREELISTITEMPTR(pStream, BUnitActionCapture         , pAction); break;
         case BAction::cActionTypeUnitJoin                     : GFREADFREELISTITEMPTR(pStream, BUnitActionJoin            , pAction); break;
         case BAction::cActionTypeUnitChangeOwner              : GFREADFREELISTITEMPTR(pStream, BUnitActionChangeOwner     , pAction); break;
         case BAction::cActionTypeUnitAmmoRegen                : GFREADFREELISTITEMPTR(pStream, BUnitActionAmmoRegen       , pAction); break;
         case BAction::cActionTypeUnitPhysics                  : GFREADFREELISTITEMPTR(pStream, BUnitActionPhysics         , pAction); break;
         case BAction::cActionTypeUnitPlayBlockingAnimation    : GFREADFREELISTITEMPTR(pStream, BUnitActionPlayBlockingAnimation    , pAction); break;
         case BAction::cActionTypeUnitMines                    : GFREADFREELISTITEMPTR(pStream, BUnitActionMines           , pAction); break;
         case BAction::cActionTypeUnitDetonate                 : GFREADFREELISTITEMPTR(pStream, BUnitActionDetonate        , pAction); break;
         case BAction::cActionTypeUnitGather                   : GFREADFREELISTITEMPTR(pStream, BUnitActionGather          , pAction); break;
         case BAction::cActionTypeUnitCollisionAttack          : GFREADFREELISTITEMPTR(pStream, BUnitActionCollisionAttack , pAction); break;
         case BAction::cActionTypeUnitAreaAttack               : GFREADFREELISTITEMPTR(pStream, BUnitActionAreaAttack      , pAction); break;
         case BAction::cActionTypeUnitUnderAttack              : GFREADFREELISTITEMPTR(pStream, BUnitActionUnderAttack     , pAction); break;
         case BAction::cActionTypeUnitSecondaryTurretAttack    : GFREADFREELISTITEMPTR(pStream, BUnitActionSecondaryTurretAttack    , pAction); break;
         case BAction::cActionTypeUnitRevealToTeam             : GFREADFREELISTITEMPTR(pStream, BUnitActionRevealToTeam    , pAction); break;
         case BAction::cActionTypeUnitAirTrafficControl        : GFREADFREELISTITEMPTR(pStream, BUnitActionAirTrafficControl        , pAction); break;
         case BAction::cActionTypeUnitHitch                    : GFREADFREELISTITEMPTR(pStream, BUnitActionHitch           , pAction); break;
         case BAction::cActionTypeUnitUnhitch                  : GFREADFREELISTITEMPTR(pStream, BUnitActionUnhitch         , pAction); break;
         case BAction::cActionTypeUnitSlaveTurretAttack        : GFREADFREELISTITEMPTR(pStream, BUnitActionSlaveTurretAttack        , pAction); break;
         case BAction::cActionTypeUnitThrown                   : GFREADFREELISTITEMPTR(pStream, BUnitActionThrown          , pAction); break;
         case BAction::cActionTypeUnitDodge                    : GFREADFREELISTITEMPTR(pStream, BUnitActionDodge           , pAction); break;
         case BAction::cActionTypeUnitDeflect                  : GFREADFREELISTITEMPTR(pStream, BUnitActionDeflect         , pAction); break;
         case BAction::cActionTypeUnitAvoidCollisionAir        : GFREADFREELISTITEMPTR(pStream, BUnitActionAvoidCollisionAir        , pAction); break;
         case BAction::cActionTypeUnitPlayAttachmentAnims      : GFREADFREELISTITEMPTR(pStream, BUnitActionPlayAttachmentAnims      , pAction); break;
         case BAction::cActionTypeUnitHeal                     : GFREADFREELISTITEMPTR(pStream, BUnitActionHeal            , pAction); break;
         case BAction::cActionTypeUnitRevive                   : GFREADFREELISTITEMPTR(pStream, BUnitActionRevive          , pAction); break;
         case BAction::cActionTypeUnitBuff                     : GFREADFREELISTITEMPTR(pStream, BUnitActionBuff            , pAction); break;
         case BAction::cActionTypeUnitInfect                   : GFREADFREELISTITEMPTR(pStream, BUnitActionInfect          , pAction); break;
         case BAction::cActionTypeUnitHotDrop                  : GFREADFREELISTITEMPTR(pStream, BUnitActionHotDrop         , pAction); break;
         case BAction::cActionTypeUnitTentacleDormant          : GFREADFREELISTITEMPTR(pStream, BUnitActionTentacleDormant , pAction); break;
         case BAction::cActionTypeUnitHeroDeath                : GFREADFREELISTITEMPTR(pStream, BUnitActionHeroDeath       , pAction); break;
         case BAction::cActionTypeUnitStasis                   : GFREADFREELISTITEMPTR(pStream, BUnitActionStasis          , pAction); break;
         case BAction::cActionTypeUnitBubbleShield             : GFREADFREELISTITEMPTR(pStream, BUnitActionBubbleShield    , pAction); break;
         case BAction::cActionTypeUnitBomb                     : GFREADFREELISTITEMPTR(pStream, BUnitActionBomb            , pAction); break;
         case BAction::cActionTypeUnitPlasmaShieldGen          : GFREADFREELISTITEMPTR(pStream, BUnitActionPlasmaShieldGen , pAction); break;
         case BAction::cActionTypeUnitJump                     : GFREADFREELISTITEMPTR(pStream, BUnitActionJump            , pAction); break;
         case BAction::cActionTypeUnitAmbientLifeSpawner       : GFREADFREELISTITEMPTR(pStream, BUnitActionAmbientLifeSpawner       , pAction); break;
         case BAction::cActionTypeUnitJumpGather               : GFREADFREELISTITEMPTR(pStream, BUnitActionJump            , pAction); break;
         case BAction::cActionTypeUnitJumpGarrison             : GFREADFREELISTITEMPTR(pStream, BUnitActionJump            , pAction); break;
         case BAction::cActionTypeUnitJumpAttack               : GFREADFREELISTITEMPTR(pStream, BUnitActionJump            , pAction); break;
         case BAction::cActionTypeUnitPointBlankAttack         : GFREADFREELISTITEMPTR(pStream, BUnitActionPointBlankAttack, pAction); break;
         case BAction::cActionTypeUnitRoar                     : GFREADFREELISTITEMPTR(pStream, BUnitActionRoar            , pAction); break;
         case BAction::cActionTypeUnitEnergyShield             : GFREADFREELISTITEMPTR(pStream, BUnitActionEnergyShield    , pAction); break;
         case BAction::cActionTypeUnitScaleLOS                 : GFREADFREELISTITEMPTR(pStream, BUnitActionScaleLOS        , pAction); break;
         case BAction::cActionTypeUnitChargedRangedAttack      : GFREADFREELISTITEMPTR(pStream, BUnitActionChargedRangedAttack      , pAction); break;
         case BAction::cActionTypeUnitTowerWall                : GFREADFREELISTITEMPTR(pStream, BUnitActionTowerWall       , pAction); break;
         case BAction::cActionTypeUnitAoeHeal                  : GFREADFREELISTITEMPTR(pStream, BUnitActionAoeHeal         , pAction); break;
         case BAction::cActionTypeUnitCoreSlide                : GFREADFREELISTITEMPTR(pStream, BUnitActionCoreSlide       , pAction); break;
         case BAction::cActionTypeUnitInfantryEnergyShield     : GFREADFREELISTITEMPTR(pStream, BUnitActionInfantryEnergyShield     , pAction); break;
         case BAction::cActionTypeUnitDome                     : GFREADFREELISTITEMPTR(pStream, BUnitActionDome            , pAction); break;
         case BAction::cActionTypeUnitRage                     : GFREADFREELISTITEMPTR(pStream, BUnitActionRage            , pAction); break;

         case BAction::cActionTypeSquadAttack                  : GFREADFREELISTITEMPTR(pStream, BSquadActionAttack         , pAction); break;
         case BAction::cActionTypeSquadChangeMode              : GFREADFREELISTITEMPTR(pStream, BSquadActionChangeMode     , pAction); break;
         case BAction::cActionTypeSquadRepair                  : GFREADFREELISTITEMPTR(pStream, BSquadActionRepair         , pAction); break;
         case BAction::cActionTypeSquadRepairOther             : GFREADFREELISTITEMPTR(pStream, BSquadActionRepairOther    , pAction); break;
         case BAction::cActionTypeSquadShieldRegen             : GFREADFREELISTITEMPTR(pStream, BSquadActionShieldRegen    , pAction); break;
         case BAction::cActionTypeSquadGarrison                : GFREADFREELISTITEMPTR(pStream, BSquadActionGarrison       , pAction); break;
         case BAction::cActionTypeSquadUngarrison              : GFREADFREELISTITEMPTR(pStream, BSquadActionUngarrison     , pAction); break;
         case BAction::cActionTypeSquadTransport               : GFREADFREELISTITEMPTR(pStream, BSquadActionTransport      , pAction); break;
         case BAction::cActionTypeSquadPlayBlockingAnimation   : GFREADFREELISTITEMPTR(pStream, BSquadActionPlayBlockingAnimation   , pAction); break;
         case BAction::cActionTypeSquadMove                    : GFREADFREELISTITEMPTR(pStream, BSquadActionMove           , pAction); break;
         case BAction::cActionTypeSquadReinforce               : GFREADFREELISTITEMPTR(pStream, BSquadActionReinforce      , pAction); break;
         case BAction::cActionTypeSquadWork                    : GFREADFREELISTITEMPTR(pStream, BSquadActionWork           , pAction); break;
         case BAction::cActionTypeSquadCarpetBomb              : GFREADFREELISTITEMPTR(pStream, BSquadActionCarpetBomb     , pAction); break;
         case BAction::cActionTypeSquadAirStrike               : GFREADFREELISTITEMPTR(pStream, BSquadActionAirStrike      , pAction); break;
         case BAction::cActionTypeSquadHitch                   : GFREADFREELISTITEMPTR(pStream, BSquadActionHitch          , pAction); break;
         case BAction::cActionTypeSquadUnhitch                 : GFREADFREELISTITEMPTR(pStream, BSquadActionUnhitch        , pAction); break;
         case BAction::cActionTypeSquadDetonate                : GFREADFREELISTITEMPTR(pStream, BSquadActionDetonate       , pAction); break;
         case BAction::cActionTypeSquadWander                  : GFREADFREELISTITEMPTR(pStream, BSquadActionWander         , pAction); break;
         case BAction::cActionTypeSquadCloak                   : GFREADFREELISTITEMPTR(pStream, BSquadActionCloak          , pAction); break;
         case BAction::cActionTypeSquadCloakDetect             : GFREADFREELISTITEMPTR(pStream, BSquadActionCloakDetect    , pAction); break;
         case BAction::cActionTypeSquadDaze                    : GFREADFREELISTITEMPTR(pStream, BSquadActionDaze           , pAction); break;
         case BAction::cActionTypeSquadJump                    : GFREADFREELISTITEMPTR(pStream, BSquadActionJump           , pAction); break;
         case BAction::cActionTypeSquadAmbientLife             : GFREADFREELISTITEMPTR(pStream, BSquadActionAmbientLife    , pAction); break;
         case BAction::cActionTypeSquadReflectDamage           : GFREADFREELISTITEMPTR(pStream, BSquadActionReflectDamage  , pAction); break;
         case BAction::cActionTypeSquadCryo                    : GFREADFREELISTITEMPTR(pStream, BSquadActionCryo           , pAction); break;
         case BAction::cActionTypeSquadSpiritBond              : GFREADFREELISTITEMPTR(pStream, BSquadActionSpiritBond     , pAction); break;
         case BAction::cActionTypePlatoonMove                  : GFREADFREELISTITEMPTR(pStream, BPlatoonActionMove         , pAction); break;
         default:
         {
            #ifndef BUILD_FINAL
               BSimString msg;
               msg.format("GameFile Error: no loadPtr case for action type %d %s", (int)type, getActionName(type));
               BASSERTM(0, msg.getPtr());
            #endif   
            break;
         }
      }
   }
   (*ppAction) = pAction;
   return true;
}

//==============================================================================
//==============================================================================
#define GFWRITEACTIONSHEADER(stream,vartype) GFWRITEFREELISTHEADER(stream, vartype, vartype::mFreeList, uint16, 20000); GFWRITEMARKER(stream, cSaveMarker##vartype); actionTypeCounter++;
bool BActionManager::preSave(BStream* pStream, int saveType) const
{
   int actionTypeCounter = 0;
   GFWRITEACTIONSHEADER(pStream, BEntityActionIdle                 );
   GFWRITEACTIONSHEADER(pStream, BEntityActionListen               );
   GFWRITEACTIONSHEADER(pStream, BUnitActionMove                   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionMoveAir                );
   GFWRITEACTIONSHEADER(pStream, BUnitActionMoveWarthog            );
   GFWRITEACTIONSHEADER(pStream, BUnitActionMoveGhost              );
   GFWRITEACTIONSHEADER(pStream, BUnitActionRangedAttack           );
   GFWRITEACTIONSHEADER(pStream, BUnitActionBuilding               );
   GFWRITEACTIONSHEADER(pStream, BUnitActionDOT                    );
   GFWRITEACTIONSHEADER(pStream, BUnitActionChangeMode             );
   GFWRITEACTIONSHEADER(pStream, BUnitActionDeath                  );
   GFWRITEACTIONSHEADER(pStream, BUnitActionInfectDeath            );
   GFWRITEACTIONSHEADER(pStream, BUnitActionGarrison               );
   GFWRITEACTIONSHEADER(pStream, BUnitActionUngarrison             );
   GFWRITEACTIONSHEADER(pStream, BUnitActionShieldRegen            );
   GFWRITEACTIONSHEADER(pStream, BUnitActionHonk                   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionSpawnSquad             );
   GFWRITEACTIONSHEADER(pStream, BUnitActionCapture                );
   GFWRITEACTIONSHEADER(pStream, BUnitActionJoin                   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionChangeOwner            );
   GFWRITEACTIONSHEADER(pStream, BUnitActionAmmoRegen              );
   GFWRITEACTIONSHEADER(pStream, BUnitActionPhysics                );
   GFWRITEACTIONSHEADER(pStream, BUnitActionPlayBlockingAnimation  );
   GFWRITEACTIONSHEADER(pStream, BUnitActionMines                  );
   GFWRITEACTIONSHEADER(pStream, BUnitActionDetonate               );
   GFWRITEACTIONSHEADER(pStream, BUnitActionGather                 );
   GFWRITEACTIONSHEADER(pStream, BUnitActionCollisionAttack        );
   GFWRITEACTIONSHEADER(pStream, BUnitActionAreaAttack             );
   GFWRITEACTIONSHEADER(pStream, BUnitActionUnderAttack            );
   GFWRITEACTIONSHEADER(pStream, BUnitActionSecondaryTurretAttack  );
   GFWRITEACTIONSHEADER(pStream, BUnitActionRevealToTeam           );
   GFWRITEACTIONSHEADER(pStream, BUnitActionAirTrafficControl      );
   GFWRITEACTIONSHEADER(pStream, BUnitActionHitch                  );
   GFWRITEACTIONSHEADER(pStream, BUnitActionUnhitch                );
   GFWRITEACTIONSHEADER(pStream, BUnitActionSlaveTurretAttack      );
   GFWRITEACTIONSHEADER(pStream, BUnitActionThrown                 );
   GFWRITEACTIONSHEADER(pStream, BUnitActionDodge                  );
   GFWRITEACTIONSHEADER(pStream, BUnitActionDeflect                );
   GFWRITEACTIONSHEADER(pStream, BUnitActionAvoidCollisionAir      );
   GFWRITEACTIONSHEADER(pStream, BUnitActionPlayAttachmentAnims    );
   GFWRITEACTIONSHEADER(pStream, BUnitActionHeal                   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionRevive                 );
   GFWRITEACTIONSHEADER(pStream, BUnitActionBuff                   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionInfect                 );
   GFWRITEACTIONSHEADER(pStream, BUnitActionHotDrop                );
   GFWRITEACTIONSHEADER(pStream, BUnitActionTentacleDormant        );
   GFWRITEACTIONSHEADER(pStream, BUnitActionHeroDeath              );
   GFWRITEACTIONSHEADER(pStream, BUnitActionStasis                 );
   GFWRITEACTIONSHEADER(pStream, BUnitActionBubbleShield           );
   GFWRITEACTIONSHEADER(pStream, BUnitActionBomb                   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionPlasmaShieldGen        );
   GFWRITEACTIONSHEADER(pStream, BUnitActionJump                   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionAmbientLifeSpawner     );
   // cActionTypeUnitJumpGather, cActionTypeUnitJumpGarrison, and cActionTypeUnitJumpAttack use BUnitActionJump action
   actionTypeCounter+=3;
   GFWRITEACTIONSHEADER(pStream, BUnitActionPointBlankAttack       );
   GFWRITEACTIONSHEADER(pStream, BUnitActionRoar                   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionEnergyShield           );
   GFWRITEACTIONSHEADER(pStream, BUnitActionScaleLOS               );
   GFWRITEACTIONSHEADER(pStream, BUnitActionChargedRangedAttack    );
   GFWRITEACTIONSHEADER(pStream, BUnitActionTowerWall              );
   GFWRITEACTIONSHEADER(pStream, BUnitActionAoeHeal                );
   GFWRITEACTIONSHEADER(pStream, BSquadActionAttack                );
   GFWRITEACTIONSHEADER(pStream, BSquadActionChangeMode            );
   GFWRITEACTIONSHEADER(pStream, BSquadActionRepair                );
   GFWRITEACTIONSHEADER(pStream, BSquadActionRepairOther           );
   GFWRITEACTIONSHEADER(pStream, BSquadActionShieldRegen           );
   GFWRITEACTIONSHEADER(pStream, BSquadActionGarrison              );
   GFWRITEACTIONSHEADER(pStream, BSquadActionUngarrison            );
   GFWRITEACTIONSHEADER(pStream, BSquadActionTransport             );
   GFWRITEACTIONSHEADER(pStream, BSquadActionPlayBlockingAnimation );
   GFWRITEACTIONSHEADER(pStream, BSquadActionMove                  );
   GFWRITEACTIONSHEADER(pStream, BSquadActionReinforce             );
   GFWRITEACTIONSHEADER(pStream, BSquadActionWork                  );
   GFWRITEACTIONSHEADER(pStream, BSquadActionCarpetBomb            );
   GFWRITEACTIONSHEADER(pStream, BSquadActionAirStrike             );
   GFWRITEACTIONSHEADER(pStream, BSquadActionHitch                 );
   GFWRITEACTIONSHEADER(pStream, BSquadActionUnhitch               );
   GFWRITEACTIONSHEADER(pStream, BSquadActionDetonate              );
   GFWRITEACTIONSHEADER(pStream, BSquadActionWander                );
   GFWRITEACTIONSHEADER(pStream, BSquadActionCloak                 );
   GFWRITEACTIONSHEADER(pStream, BSquadActionCloakDetect           );
   GFWRITEACTIONSHEADER(pStream, BSquadActionDaze                  );
   GFWRITEACTIONSHEADER(pStream, BSquadActionJump                  );
   GFWRITEACTIONSHEADER(pStream, BSquadActionAmbientLife           );
   GFWRITEACTIONSHEADER(pStream, BSquadActionReflectDamage         );
   GFWRITEACTIONSHEADER(pStream, BSquadActionCryo                  );
   GFWRITEACTIONSHEADER(pStream, BPlatoonActionMove                );
   GFWRITEACTIONSHEADER(pStream, BUnitActionCoreSlide              );
   GFWRITEACTIONSHEADER(pStream, BUnitActionInfantryEnergyShield   );
   GFWRITEACTIONSHEADER(pStream, BUnitActionDome                   );
   GFWRITEACTIONSHEADER(pStream, BSquadActionSpiritBond            );
   GFWRITEACTIONSHEADER(pStream, BUnitActionRage                   );
   BASSERT(actionTypeCounter==BAction::cActionTypeInvalid);
   return true;
}

//==============================================================================
//==============================================================================
#define GFREADACTIONSHEADER(stream,vartype) GFREADFREELISTHEADER(stream, vartype, vartype::mFreeList, uint16, 20000); GFREADMARKER(stream, cSaveMarker##vartype); actionTypeCounter++;
bool BActionManager::preLoad(BStream* pStream, int saveType)
{
   int actionTypeCounter = 0;
   GFREADACTIONSHEADER(pStream, BEntityActionIdle                 );
   GFREADACTIONSHEADER(pStream, BEntityActionListen               );
   GFREADACTIONSHEADER(pStream, BUnitActionMove                   );
   GFREADACTIONSHEADER(pStream, BUnitActionMoveAir                );
   GFREADACTIONSHEADER(pStream, BUnitActionMoveWarthog            );
   GFREADACTIONSHEADER(pStream, BUnitActionMoveGhost              );
   GFREADACTIONSHEADER(pStream, BUnitActionRangedAttack           );
   GFREADACTIONSHEADER(pStream, BUnitActionBuilding               );
   GFREADACTIONSHEADER(pStream, BUnitActionDOT                    );
   GFREADACTIONSHEADER(pStream, BUnitActionChangeMode             );
   GFREADACTIONSHEADER(pStream, BUnitActionDeath                  );
   GFREADACTIONSHEADER(pStream, BUnitActionInfectDeath            );
   GFREADACTIONSHEADER(pStream, BUnitActionGarrison               );
   GFREADACTIONSHEADER(pStream, BUnitActionUngarrison             );
   GFREADACTIONSHEADER(pStream, BUnitActionShieldRegen            );
   GFREADACTIONSHEADER(pStream, BUnitActionHonk                   );
   GFREADACTIONSHEADER(pStream, BUnitActionSpawnSquad             );
   GFREADACTIONSHEADER(pStream, BUnitActionCapture                );
   GFREADACTIONSHEADER(pStream, BUnitActionJoin                   );
   GFREADACTIONSHEADER(pStream, BUnitActionChangeOwner            );
   GFREADACTIONSHEADER(pStream, BUnitActionAmmoRegen              );
   GFREADACTIONSHEADER(pStream, BUnitActionPhysics                );
   GFREADACTIONSHEADER(pStream, BUnitActionPlayBlockingAnimation  );
   GFREADACTIONSHEADER(pStream, BUnitActionMines                  );
   GFREADACTIONSHEADER(pStream, BUnitActionDetonate               );
   GFREADACTIONSHEADER(pStream, BUnitActionGather                 );
   GFREADACTIONSHEADER(pStream, BUnitActionCollisionAttack        );
   GFREADACTIONSHEADER(pStream, BUnitActionAreaAttack             );
   GFREADACTIONSHEADER(pStream, BUnitActionUnderAttack            );
   GFREADACTIONSHEADER(pStream, BUnitActionSecondaryTurretAttack  );
   GFREADACTIONSHEADER(pStream, BUnitActionRevealToTeam           );
   GFREADACTIONSHEADER(pStream, BUnitActionAirTrafficControl      );
   GFREADACTIONSHEADER(pStream, BUnitActionHitch                  );
   GFREADACTIONSHEADER(pStream, BUnitActionUnhitch                );
   GFREADACTIONSHEADER(pStream, BUnitActionSlaveTurretAttack      );
   GFREADACTIONSHEADER(pStream, BUnitActionThrown                 );
   GFREADACTIONSHEADER(pStream, BUnitActionDodge                  );
   GFREADACTIONSHEADER(pStream, BUnitActionDeflect                );
   GFREADACTIONSHEADER(pStream, BUnitActionAvoidCollisionAir      );
   GFREADACTIONSHEADER(pStream, BUnitActionPlayAttachmentAnims    );
   GFREADACTIONSHEADER(pStream, BUnitActionHeal                   );
   GFREADACTIONSHEADER(pStream, BUnitActionRevive                 );
   GFREADACTIONSHEADER(pStream, BUnitActionBuff                   );
   GFREADACTIONSHEADER(pStream, BUnitActionInfect                 );
   GFREADACTIONSHEADER(pStream, BUnitActionHotDrop                );
   GFREADACTIONSHEADER(pStream, BUnitActionTentacleDormant        );
   GFREADACTIONSHEADER(pStream, BUnitActionHeroDeath              );
   GFREADACTIONSHEADER(pStream, BUnitActionStasis                 );
   GFREADACTIONSHEADER(pStream, BUnitActionBubbleShield           );
   GFREADACTIONSHEADER(pStream, BUnitActionBomb                   );
   GFREADACTIONSHEADER(pStream, BUnitActionPlasmaShieldGen        );
   GFREADACTIONSHEADER(pStream, BUnitActionJump                   );
   GFREADACTIONSHEADER(pStream, BUnitActionAmbientLifeSpawner     );
   // cActionTypeUnitJumpGather, cActionTypeUnitJumpGarrison, and cActionTypeUnitJumpAttack use BUnitActionJump action
   actionTypeCounter+=3;
   GFREADACTIONSHEADER(pStream, BUnitActionPointBlankAttack       );
   GFREADACTIONSHEADER(pStream, BUnitActionRoar                   );
   GFREADACTIONSHEADER(pStream, BUnitActionEnergyShield           );
   GFREADACTIONSHEADER(pStream, BUnitActionScaleLOS               );
   GFREADACTIONSHEADER(pStream, BUnitActionChargedRangedAttack    );
   GFREADACTIONSHEADER(pStream, BUnitActionTowerWall              );
   GFREADACTIONSHEADER(pStream, BUnitActionAoeHeal                );
   GFREADACTIONSHEADER(pStream, BSquadActionAttack                );
   GFREADACTIONSHEADER(pStream, BSquadActionChangeMode            );
   GFREADACTIONSHEADER(pStream, BSquadActionRepair                );
   GFREADACTIONSHEADER(pStream, BSquadActionRepairOther           );
   GFREADACTIONSHEADER(pStream, BSquadActionShieldRegen           );
   GFREADACTIONSHEADER(pStream, BSquadActionGarrison              );
   GFREADACTIONSHEADER(pStream, BSquadActionUngarrison            );
   GFREADACTIONSHEADER(pStream, BSquadActionTransport             );
   GFREADACTIONSHEADER(pStream, BSquadActionPlayBlockingAnimation );
   GFREADACTIONSHEADER(pStream, BSquadActionMove                  );
   GFREADACTIONSHEADER(pStream, BSquadActionReinforce             );
   GFREADACTIONSHEADER(pStream, BSquadActionWork                  );
   GFREADACTIONSHEADER(pStream, BSquadActionCarpetBomb            );
   GFREADACTIONSHEADER(pStream, BSquadActionAirStrike             );
   GFREADACTIONSHEADER(pStream, BSquadActionHitch                 );
   GFREADACTIONSHEADER(pStream, BSquadActionUnhitch               );
   GFREADACTIONSHEADER(pStream, BSquadActionDetonate              );
   GFREADACTIONSHEADER(pStream, BSquadActionWander                );
   GFREADACTIONSHEADER(pStream, BSquadActionCloak                 );
   GFREADACTIONSHEADER(pStream, BSquadActionCloakDetect           );
   GFREADACTIONSHEADER(pStream, BSquadActionDaze                  );
   GFREADACTIONSHEADER(pStream, BSquadActionJump                  );
   GFREADACTIONSHEADER(pStream, BSquadActionAmbientLife           );
   GFREADACTIONSHEADER(pStream, BSquadActionReflectDamage         );
   GFREADACTIONSHEADER(pStream, BSquadActionCryo                  );
   GFREADACTIONSHEADER(pStream, BPlatoonActionMove                );
   GFREADACTIONSHEADER(pStream, BUnitActionCoreSlide              );
   GFREADACTIONSHEADER(pStream, BUnitActionInfantryEnergyShield   );
   GFREADACTIONSHEADER(pStream, BUnitActionDome                   );

   if (BActionManager::mGameFileVersion >= 2)
   {
      GFREADACTIONSHEADER(pStream, BSquadActionSpiritBond            );
   }
   if (BActionManager::mGameFileVersion >= 3)
   {
      GFREADACTIONSHEADER(pStream, BUnitActionRage            );
   }
   return true;
}

//==============================================================================
//==============================================================================
#define GFWRITEACTIONSDATA(stream,savetype,vartype) GFWRITEFREELISTDATA(stream, savetype, vartype, vartype::mFreeList, uint16, 20000); GFWRITEMARKER(stream, cSaveMarker##vartype); 
bool BActionManager::save(BStream* pStream, int saveType) const
{
   GFWRITEACTIONSDATA(pStream, saveType, BEntityActionIdle                 );
   GFWRITEACTIONSDATA(pStream, saveType, BEntityActionListen               );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionMove                   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionMoveAir                );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionMoveWarthog            );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionMoveGhost              );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionRangedAttack           );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionBuilding               );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionDOT                    );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionChangeMode             );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionDeath                  );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionInfectDeath            );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionGarrison               );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionUngarrison             );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionShieldRegen            );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionHonk                   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionSpawnSquad             );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionCapture                );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionJoin                   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionChangeOwner            );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionAmmoRegen              );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionPhysics                );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionPlayBlockingAnimation  );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionMines                  );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionDetonate               );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionGather                 );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionCollisionAttack        );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionAreaAttack             );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionUnderAttack            );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionSecondaryTurretAttack  );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionRevealToTeam           );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionAirTrafficControl      );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionHitch                  );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionUnhitch                );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionSlaveTurretAttack      );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionThrown                 );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionDodge                  );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionDeflect                );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionAvoidCollisionAir      );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionPlayAttachmentAnims    );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionHeal                   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionRevive                 );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionBuff                   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionInfect                 );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionHotDrop                );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionTentacleDormant        );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionHeroDeath              );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionStasis                 );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionBubbleShield           );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionBomb                   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionPlasmaShieldGen        );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionJump                   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionAmbientLifeSpawner     );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionPointBlankAttack       );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionRoar                   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionEnergyShield           );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionScaleLOS               );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionChargedRangedAttack    );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionTowerWall              );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionAoeHeal                );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionAttack                );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionChangeMode            );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionRepair                );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionRepairOther           );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionShieldRegen           );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionGarrison              );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionUngarrison            );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionTransport             );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionPlayBlockingAnimation );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionMove                  );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionReinforce             );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionWork                  );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionCarpetBomb            );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionAirStrike             );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionHitch                 );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionUnhitch               );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionDetonate              );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionWander                );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionCloak                 );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionCloakDetect           );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionDaze                  );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionJump                  );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionAmbientLife           );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionReflectDamage         );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionCryo                  );
   GFWRITEACTIONSDATA(pStream, saveType, BPlatoonActionMove                );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionCoreSlide              );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionInfantryEnergyShield   );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionDome                   );
   GFWRITEACTIONSDATA(pStream, saveType, BSquadActionSpiritBond            );
   GFWRITEACTIONSDATA(pStream, saveType, BUnitActionRage                   );
   return true;
}

//==============================================================================
//==============================================================================
#define GFREADACTIONSDATA(stream,savetype,vartype) GFREADFREELISTDATA(stream, savetype, vartype, vartype::mFreeList, uint16, 20000); GFREADMARKER(stream, cSaveMarker##vartype); 
bool BActionManager::load(BStream* pStream, int saveType)
{
   GFREADACTIONSDATA(pStream, saveType, BEntityActionIdle                 );
   GFREADACTIONSDATA(pStream, saveType, BEntityActionListen               );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionMove                   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionMoveAir                );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionMoveWarthog            );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionMoveGhost              );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionRangedAttack           );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionBuilding               );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionDOT                    );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionChangeMode             );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionDeath                  );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionInfectDeath            );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionGarrison               );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionUngarrison             );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionShieldRegen            );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionHonk                   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionSpawnSquad             );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionCapture                );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionJoin                   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionChangeOwner            );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionAmmoRegen              );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionPhysics                );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionPlayBlockingAnimation  );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionMines                  );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionDetonate               );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionGather                 );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionCollisionAttack        );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionAreaAttack             );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionUnderAttack            );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionSecondaryTurretAttack  );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionRevealToTeam           );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionAirTrafficControl      );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionHitch                  );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionUnhitch                );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionSlaveTurretAttack      );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionThrown                 );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionDodge                  );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionDeflect                );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionAvoidCollisionAir      );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionPlayAttachmentAnims    );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionHeal                   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionRevive                 );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionBuff                   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionInfect                 );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionHotDrop                );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionTentacleDormant        );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionHeroDeath              );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionStasis                 );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionBubbleShield           );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionBomb                   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionPlasmaShieldGen        );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionJump                   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionAmbientLifeSpawner     );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionPointBlankAttack       );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionRoar                   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionEnergyShield           );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionScaleLOS               );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionChargedRangedAttack    );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionTowerWall              );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionAoeHeal                );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionAttack                );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionChangeMode            );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionRepair                );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionRepairOther           );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionShieldRegen           );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionGarrison              );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionUngarrison            );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionTransport             );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionPlayBlockingAnimation );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionMove                  );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionReinforce             );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionWork                  );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionCarpetBomb            );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionAirStrike             );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionHitch                 );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionUnhitch               );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionDetonate              );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionWander                );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionCloak                 );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionCloakDetect           );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionDaze                  );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionJump                  );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionAmbientLife           );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionReflectDamage         );
   GFREADACTIONSDATA(pStream, saveType, BSquadActionCryo                  );
   GFREADACTIONSDATA(pStream, saveType, BPlatoonActionMove                );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionCoreSlide              );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionInfantryEnergyShield   );
   GFREADACTIONSDATA(pStream, saveType, BUnitActionDome                   );

   if (BActionManager::mGameFileVersion >= 2)
   {
      GFREADACTIONSDATA(pStream, saveType, BSquadActionSpiritBond            );
   }
   if (BActionManager::mGameFileVersion >= 3)
   {
      GFREADACTIONSDATA(pStream, saveType, BUnitActionRage                 );
   }
   return true;
}

//==============================================================================
//==============================================================================
uint BActionManager::getActionIndex(const BAction* pAction) const
{
   uint index = UINT_MAX;
   bool retval = false;
   if (pAction)
   {
      BActionType type = pAction->getType();
      switch (type)
      {
         case BAction::cActionTypeEntityIdle                   : retval = BEntityActionIdle          ::mFreeList.getIndex((BEntityActionIdle          *) pAction, index); break;
         case BAction::cActionTypeEntityListen                 : retval = BEntityActionListen        ::mFreeList.getIndex((BEntityActionListen        *) pAction, index); break;
         case BAction::cActionTypeUnitMove                     : retval = BUnitActionMove            ::mFreeList.getIndex((BUnitActionMove            *) pAction, index); break;
         case BAction::cActionTypeUnitMoveAir                  : retval = BUnitActionMoveAir         ::mFreeList.getIndex((BUnitActionMoveAir         *) pAction, index); break;
         case BAction::cActionTypeUnitMoveWarthog              : retval = BUnitActionMoveWarthog     ::mFreeList.getIndex((BUnitActionMoveWarthog     *) pAction, index); break;
         case BAction::cActionTypeUnitMoveGhost                : retval = BUnitActionMoveGhost       ::mFreeList.getIndex((BUnitActionMoveGhost       *) pAction, index); break;
         case BAction::cActionTypeUnitRangedAttack             : retval = BUnitActionRangedAttack    ::mFreeList.getIndex((BUnitActionRangedAttack    *) pAction, index); break;
         case BAction::cActionTypeUnitBuilding                 : retval = BUnitActionBuilding        ::mFreeList.getIndex((BUnitActionBuilding        *) pAction, index); break;
         case BAction::cActionTypeUnitDOT                      : retval = BUnitActionDOT             ::mFreeList.getIndex((BUnitActionDOT             *) pAction, index); break;
         case BAction::cActionTypeUnitChangeMode               : retval = BUnitActionChangeMode      ::mFreeList.getIndex((BUnitActionChangeMode      *) pAction, index); break;
         case BAction::cActionTypeUnitDeath                    : retval = BUnitActionDeath           ::mFreeList.getIndex((BUnitActionDeath           *) pAction, index); break;
         case BAction::cActionTypeUnitInfectDeath              : retval = BUnitActionInfectDeath     ::mFreeList.getIndex((BUnitActionInfectDeath     *) pAction, index); break;
         case BAction::cActionTypeUnitGarrison                 : retval = BUnitActionGarrison        ::mFreeList.getIndex((BUnitActionGarrison        *) pAction, index); break;
         case BAction::cActionTypeUnitUngarrison               : retval = BUnitActionUngarrison      ::mFreeList.getIndex((BUnitActionUngarrison      *) pAction, index); break;
         case BAction::cActionTypeUnitShieldRegen              : retval = BUnitActionShieldRegen     ::mFreeList.getIndex((BUnitActionShieldRegen     *) pAction, index); break;
         case BAction::cActionTypeUnitHonk                     : retval = BUnitActionHonk            ::mFreeList.getIndex((BUnitActionHonk            *) pAction, index); break;
         case BAction::cActionTypeUnitSpawnSquad               : retval = BUnitActionSpawnSquad      ::mFreeList.getIndex((BUnitActionSpawnSquad      *) pAction, index); break;
         case BAction::cActionTypeUnitCapture                  : retval = BUnitActionCapture         ::mFreeList.getIndex((BUnitActionCapture         *) pAction, index); break;
         case BAction::cActionTypeUnitJoin                     : retval = BUnitActionJoin            ::mFreeList.getIndex((BUnitActionJoin            *) pAction, index); break;
         case BAction::cActionTypeUnitChangeOwner              : retval = BUnitActionChangeOwner     ::mFreeList.getIndex((BUnitActionChangeOwner     *) pAction, index); break;
         case BAction::cActionTypeUnitAmmoRegen                : retval = BUnitActionAmmoRegen       ::mFreeList.getIndex((BUnitActionAmmoRegen       *) pAction, index); break;
         case BAction::cActionTypeUnitPhysics                  : retval = BUnitActionPhysics         ::mFreeList.getIndex((BUnitActionPhysics         *) pAction, index); break;
         case BAction::cActionTypeUnitPlayBlockingAnimation    : retval = BUnitActionPlayBlockingAnimation    ::mFreeList.getIndex((BUnitActionPlayBlockingAnimation    *) pAction, index); break;
         case BAction::cActionTypeUnitMines                    : retval = BUnitActionMines           ::mFreeList.getIndex((BUnitActionMines           *) pAction, index); break;
         case BAction::cActionTypeUnitDetonate                 : retval = BUnitActionDetonate        ::mFreeList.getIndex((BUnitActionDetonate        *) pAction, index); break;
         case BAction::cActionTypeUnitGather                   : retval = BUnitActionGather          ::mFreeList.getIndex((BUnitActionGather          *) pAction, index); break;
         case BAction::cActionTypeUnitCollisionAttack          : retval = BUnitActionCollisionAttack ::mFreeList.getIndex((BUnitActionCollisionAttack *) pAction, index); break;
         case BAction::cActionTypeUnitAreaAttack               : retval = BUnitActionAreaAttack      ::mFreeList.getIndex((BUnitActionAreaAttack      *) pAction, index); break;
         case BAction::cActionTypeUnitUnderAttack              : retval = BUnitActionUnderAttack     ::mFreeList.getIndex((BUnitActionUnderAttack     *) pAction, index); break;
         case BAction::cActionTypeUnitSecondaryTurretAttack    : retval = BUnitActionSecondaryTurretAttack    ::mFreeList.getIndex((BUnitActionSecondaryTurretAttack    *) pAction, index); break;
         case BAction::cActionTypeUnitRevealToTeam             : retval = BUnitActionRevealToTeam    ::mFreeList.getIndex((BUnitActionRevealToTeam    *) pAction, index); break;
         case BAction::cActionTypeUnitAirTrafficControl        : retval = BUnitActionAirTrafficControl        ::mFreeList.getIndex((BUnitActionAirTrafficControl        *) pAction, index); break;
         case BAction::cActionTypeUnitHitch                    : retval = BUnitActionHitch           ::mFreeList.getIndex((BUnitActionHitch           *) pAction, index); break;
         case BAction::cActionTypeUnitUnhitch                  : retval = BUnitActionUnhitch         ::mFreeList.getIndex((BUnitActionUnhitch         *) pAction, index); break;
         case BAction::cActionTypeUnitSlaveTurretAttack        : retval = BUnitActionSlaveTurretAttack        ::mFreeList.getIndex((BUnitActionSlaveTurretAttack        *) pAction, index); break;
         case BAction::cActionTypeUnitThrown                   : retval = BUnitActionThrown          ::mFreeList.getIndex((BUnitActionThrown          *) pAction, index); break;
         case BAction::cActionTypeUnitDodge                    : retval = BUnitActionDodge           ::mFreeList.getIndex((BUnitActionDodge           *) pAction, index); break;
         case BAction::cActionTypeUnitDeflect                  : retval = BUnitActionDeflect         ::mFreeList.getIndex((BUnitActionDeflect         *) pAction, index); break;
         case BAction::cActionTypeUnitAvoidCollisionAir        : retval = BUnitActionAvoidCollisionAir        ::mFreeList.getIndex((BUnitActionAvoidCollisionAir        *) pAction, index); break;
         case BAction::cActionTypeUnitPlayAttachmentAnims      : retval = BUnitActionPlayAttachmentAnims      ::mFreeList.getIndex((BUnitActionPlayAttachmentAnims      *) pAction, index); break;
         case BAction::cActionTypeUnitHeal                     : retval = BUnitActionHeal            ::mFreeList.getIndex((BUnitActionHeal            *) pAction, index); break;
         case BAction::cActionTypeUnitRevive                   : retval = BUnitActionRevive          ::mFreeList.getIndex((BUnitActionRevive          *) pAction, index); break;
         case BAction::cActionTypeUnitBuff                     : retval = BUnitActionBuff            ::mFreeList.getIndex((BUnitActionBuff            *) pAction, index); break;
         case BAction::cActionTypeUnitInfect                   : retval = BUnitActionInfect          ::mFreeList.getIndex((BUnitActionInfect          *) pAction, index); break;
         case BAction::cActionTypeUnitHotDrop                  : retval = BUnitActionHotDrop         ::mFreeList.getIndex((BUnitActionHotDrop         *) pAction, index); break;
         case BAction::cActionTypeUnitTentacleDormant          : retval = BUnitActionTentacleDormant ::mFreeList.getIndex((BUnitActionTentacleDormant *) pAction, index); break;
         case BAction::cActionTypeUnitHeroDeath                : retval = BUnitActionHeroDeath       ::mFreeList.getIndex((BUnitActionHeroDeath       *) pAction, index); break;
         case BAction::cActionTypeUnitStasis                   : retval = BUnitActionStasis          ::mFreeList.getIndex((BUnitActionStasis          *) pAction, index); break;
         case BAction::cActionTypeUnitBubbleShield             : retval = BUnitActionBubbleShield    ::mFreeList.getIndex((BUnitActionBubbleShield    *) pAction, index); break;
         case BAction::cActionTypeUnitBomb                     : retval = BUnitActionBomb            ::mFreeList.getIndex((BUnitActionBomb            *) pAction, index); break;
         case BAction::cActionTypeUnitPlasmaShieldGen          : retval = BUnitActionPlasmaShieldGen ::mFreeList.getIndex((BUnitActionPlasmaShieldGen *) pAction, index); break;
         case BAction::cActionTypeUnitJump                     : retval = BUnitActionJump            ::mFreeList.getIndex((BUnitActionJump            *) pAction, index); break;
         case BAction::cActionTypeUnitAmbientLifeSpawner       : retval = BUnitActionAmbientLifeSpawner       ::mFreeList.getIndex((BUnitActionAmbientLifeSpawner       *) pAction, index); break;
         case BAction::cActionTypeUnitJumpGather               : retval = BUnitActionJump            ::mFreeList.getIndex((BUnitActionJump            *) pAction, index); break;
         case BAction::cActionTypeUnitJumpGarrison             : retval = BUnitActionJump            ::mFreeList.getIndex((BUnitActionJump            *) pAction, index); break;
         case BAction::cActionTypeUnitJumpAttack               : retval = BUnitActionJump            ::mFreeList.getIndex((BUnitActionJump            *) pAction, index); break;
         case BAction::cActionTypeUnitPointBlankAttack         : retval = BUnitActionPointBlankAttack::mFreeList.getIndex((BUnitActionPointBlankAttack*) pAction, index); break;
         case BAction::cActionTypeUnitRoar                     : retval = BUnitActionRoar            ::mFreeList.getIndex((BUnitActionRoar            *) pAction, index); break;
         case BAction::cActionTypeUnitEnergyShield             : retval = BUnitActionEnergyShield    ::mFreeList.getIndex((BUnitActionEnergyShield    *) pAction, index); break;
         case BAction::cActionTypeUnitScaleLOS                 : retval = BUnitActionScaleLOS        ::mFreeList.getIndex((BUnitActionScaleLOS        *) pAction, index); break;
         case BAction::cActionTypeUnitChargedRangedAttack      : retval = BUnitActionChargedRangedAttack      ::mFreeList.getIndex((BUnitActionChargedRangedAttack      *) pAction, index); break;
         case BAction::cActionTypeUnitTowerWall                : retval = BUnitActionTowerWall       ::mFreeList.getIndex((BUnitActionTowerWall       *) pAction, index); break;
         case BAction::cActionTypeUnitAoeHeal                  : retval = BUnitActionAoeHeal         ::mFreeList.getIndex((BUnitActionAoeHeal         *) pAction, index); break;
         case BAction::cActionTypeUnitCoreSlide                : retval = BUnitActionCoreSlide       ::mFreeList.getIndex((BUnitActionCoreSlide       *) pAction, index); break;
         case BAction::cActionTypeUnitInfantryEnergyShield     : retval = BUnitActionInfantryEnergyShield     ::mFreeList.getIndex((BUnitActionInfantryEnergyShield     *) pAction, index); break;
         case BAction::cActionTypeUnitDome                     : retval = BUnitActionDome            ::mFreeList.getIndex((BUnitActionDome            *) pAction, index); break;
         case BAction::cActionTypeUnitRage                     : retval = BUnitActionRage            ::mFreeList.getIndex((BUnitActionRage            *) pAction, index); break;

         case BAction::cActionTypeSquadAttack                  : retval = BSquadActionAttack         ::mFreeList.getIndex((BSquadActionAttack         *) pAction, index); break;
         case BAction::cActionTypeSquadChangeMode              : retval = BSquadActionChangeMode     ::mFreeList.getIndex((BSquadActionChangeMode     *) pAction, index); break;
         case BAction::cActionTypeSquadRepair                  : retval = BSquadActionRepair         ::mFreeList.getIndex((BSquadActionRepair         *) pAction, index); break;
         case BAction::cActionTypeSquadRepairOther             : retval = BSquadActionRepairOther    ::mFreeList.getIndex((BSquadActionRepairOther    *) pAction, index); break;
         case BAction::cActionTypeSquadShieldRegen             : retval = BSquadActionShieldRegen    ::mFreeList.getIndex((BSquadActionShieldRegen    *) pAction, index); break;
         case BAction::cActionTypeSquadGarrison                : retval = BSquadActionGarrison       ::mFreeList.getIndex((BSquadActionGarrison       *) pAction, index); break;
         case BAction::cActionTypeSquadUngarrison              : retval = BSquadActionUngarrison     ::mFreeList.getIndex((BSquadActionUngarrison     *) pAction, index); break;
         case BAction::cActionTypeSquadTransport               : retval = BSquadActionTransport      ::mFreeList.getIndex((BSquadActionTransport      *) pAction, index); break;
         case BAction::cActionTypeSquadPlayBlockingAnimation   : retval = BSquadActionPlayBlockingAnimation   ::mFreeList.getIndex((BSquadActionPlayBlockingAnimation   *) pAction, index); break;
         case BAction::cActionTypeSquadMove                    : retval = BSquadActionMove           ::mFreeList.getIndex((BSquadActionMove           *) pAction, index); break;
         case BAction::cActionTypeSquadReinforce               : retval = BSquadActionReinforce      ::mFreeList.getIndex((BSquadActionReinforce      *) pAction, index); break;
         case BAction::cActionTypeSquadWork                    : retval = BSquadActionWork           ::mFreeList.getIndex((BSquadActionWork           *) pAction, index); break;
         case BAction::cActionTypeSquadCarpetBomb              : retval = BSquadActionCarpetBomb     ::mFreeList.getIndex((BSquadActionCarpetBomb     *) pAction, index); break;
         case BAction::cActionTypeSquadAirStrike               : retval = BSquadActionAirStrike      ::mFreeList.getIndex((BSquadActionAirStrike      *) pAction, index); break;
         case BAction::cActionTypeSquadHitch                   : retval = BSquadActionHitch          ::mFreeList.getIndex((BSquadActionHitch          *) pAction, index); break;
         case BAction::cActionTypeSquadUnhitch                 : retval = BSquadActionUnhitch        ::mFreeList.getIndex((BSquadActionUnhitch        *) pAction, index); break;
         case BAction::cActionTypeSquadDetonate                : retval = BSquadActionDetonate       ::mFreeList.getIndex((BSquadActionDetonate       *) pAction, index); break;
         case BAction::cActionTypeSquadWander                  : retval = BSquadActionWander         ::mFreeList.getIndex((BSquadActionWander         *) pAction, index); break;
         case BAction::cActionTypeSquadCloak                   : retval = BSquadActionCloak          ::mFreeList.getIndex((BSquadActionCloak          *) pAction, index); break;
         case BAction::cActionTypeSquadCloakDetect             : retval = BSquadActionCloakDetect    ::mFreeList.getIndex((BSquadActionCloakDetect    *) pAction, index); break;
         case BAction::cActionTypeSquadDaze                    : retval = BSquadActionDaze           ::mFreeList.getIndex((BSquadActionDaze           *) pAction, index); break;
         case BAction::cActionTypeSquadJump                    : retval = BSquadActionJump           ::mFreeList.getIndex((BSquadActionJump           *) pAction, index); break;
         case BAction::cActionTypeSquadAmbientLife             : retval = BSquadActionAmbientLife    ::mFreeList.getIndex((BSquadActionAmbientLife    *) pAction, index); break;
         case BAction::cActionTypeSquadReflectDamage           : retval = BSquadActionReflectDamage  ::mFreeList.getIndex((BSquadActionReflectDamage  *) pAction, index); break;
         case BAction::cActionTypeSquadCryo                    : retval = BSquadActionCryo           ::mFreeList.getIndex((BSquadActionCryo           *) pAction, index); break;
         case BAction::cActionTypeSquadSpiritBond              : retval = BSquadActionSpiritBond     ::mFreeList.getIndex((BSquadActionSpiritBond     *) pAction, index); break;

         case BAction::cActionTypePlatoonMove                  : retval = BPlatoonActionMove         ::mFreeList.getIndex((BPlatoonActionMove         *) pAction, index); break;
      }
   }

   if (retval)
      return index;

   return UINT_MAX;
}

//==============================================================================
// entity.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "actionmanager.h"
#include "entity.h"
#include "entityactionidle.h"
#include "entityactionlisten.h"
#include "world.h"
#include "TerrainSimRep.h" 
#include "protoobject.h"
#include "obstructionmanager.h"
#include "action.h"
#include "objectmanager.h"
#include "physicsinfo.h"
#include "physicsinfomanager.h"
#include "spring.h"
#include "tactic.h"
#include "configsgame.h"
#include "simhelper.h"
#include "syncmacros.h"
#include "unitactionphysics.h"
#include "UnitActionCollisionAttack.h"
#include "weapontype.h"
#include "unitactionbuilding.h"
#include "alert.h"
#include "protosquad.h"

// xphysics
#include "physics.h"
#include "physicselephantaction.h"
#include "physicsgrizzlyaction.h"
#include "physicscobraaction.h"
#include "physicswolverineaction.h"
#include "physicswarthogaction.h"
#include "physicsghostaction.h"
#include "physicshoverflightaction.h"
#include "physicsscorpionaction.h"
#include "Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "physicsobjectblueprint.h"
#include "physicsCollision.h"
#include "physicsgroundvehicleaction.h"

//xsystem
#include "config.h"

// Render
#include "debugprimitives.h"
#include "render.h"

#include "visual.h"
#include "grannyinstance.h"

GFIMPLEMENTVERSION(BEntity, 2);
enum 
{
   cSaveMarkerEntity1=10000,
};

DWORD cColor[] = {cDWORDGreen, cDWORDBlue, cDWORDRed};

//#define DEBUG_INVALID_FLOATS

#ifdef DEBUG_INVALID_FLOATS
   static BVector debugVec=cOriginVector;
   static BEntity* pDebugEntity=NULL;
#endif


#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BEntityRef::createDebugString(BSimString &debugString)
{
   BSimString refTypeName;
   switch (mType)
   {
      case cTypeTrainLimitBuilding: { refTypeName = "TrainLimitBuilding"; break; }
      case cTypeTrainLimitUnit: { refTypeName = "TrainLimitUnit"; break; }
      case cTypeTrainLimitSquad: { refTypeName = "TrainLimitSquad"; break; }
      case cTypeGatherBuilding: { refTypeName = "GatherBuilding"; break; }
      case cTypeGatherResource: { refTypeName = "GatherResource"; break; }
      case cTypeGatherUnit: { refTypeName = "GatherUnit"; break; }
      case cTypeGatherTarget: { refTypeName = "GatherTarget"; break; }
      case cTypeGatherParent: { refTypeName = "GatherParent"; break; }
      case cTypeGatherChild: { refTypeName = "GatherChild"; break; }
      case cTypeBlockerChild: { refTypeName = "BlockerChild"; break; }
      case cTypeContainUnit: { refTypeName = "ContainUnit"; break; }
      case cTypeAttachObject: { refTypeName = "AttachObject"; break; }
      case cTypeAttachedToObject: { refTypeName = "AttachedToObject"; break; }
      case cTypeCapturingUnit: { refTypeName = "CapturingUnit"; break; }
      case cTypeCapturingPlayer: { refTypeName = "CapturingPlayer"; break; }
      case cTypeBuiltByUnit: { refTypeName = "BuiltByUnit"; break; }
      case cTypeAssociatedParkingLot: { refTypeName = "AssociatedParkingLot"; break; }
      case cTypeParkingLotRef: { refTypeName = "ParkingLotRef"; break; }
      case cTypeAssociatedSettlement: { refTypeName = "AssociatedSettlement"; break; }
      case cTypeAssociatedBase: { refTypeName = "AssociatedBase"; break; }
      case cTypeHitchUnit: { refTypeName = "HitchUnit"; break; }
      case cTypeHitchedToUnit: { refTypeName = "HitchedToUnit"; break; }
      case cTypeBuildQueueChild: { refTypeName = "BuildQueueChild"; break; }
      case cTypeBuildQueueParent: { refTypeName = "BuildQueueParent"; break; }
      case cTypeParentSocket: { refTypeName = "ParentSocket"; break; }
      case cTypeSocketPlug: { refTypeName = "SocketPlug"; break; }
      case cTypeAssociatedSocket: { refTypeName = "AssociatedSocket"; break; }
      case cTypeAssociatedBuilding: { refTypeName = "AssociatedBuilding"; break; }
      case cTypeAssociatedUnit: { refTypeName = "AssociatedUnit"; break; }
      case cTypeAssociatedFoundation: { refTypeName = "AssociatedFoundation"; break; }
      case cTypeStickedProjectile:  { refTypeName = "StickedProjectile"; break; }
      case cTypeTrainLock:  { refTypeName = "TrainLock"; break; }
      case cTypeEffect:  { refTypeName = "Effect"; break; }
      case cTypeAssociatedObject: { refTypeName = "AssociatedObject"; break; }

      default: { refTypeName = "???"; break; }
   }
   debugString.format("Type=%-20s EntityID=%-13s Data1=%d Data2=%d", refTypeName.getPtr(), mID.getDebugString().getPtr(), mData1, mData2);
}
#endif

//==============================================================================
// BEntity::BEntity
//==============================================================================
BEntity::BEntity() :
   mID(0)
{
}

//==============================================================================
// BEntity::~BEntity
//==============================================================================
BEntity::~BEntity() 
{
}

//==============================================================================
// BEntity::updatePreAsync
//==============================================================================
bool BEntity::updatePreAsync(float elapsedTime)
{
   return true;
}

//==============================================================================
// BEntity::updateAsync
//==============================================================================
bool BEntity::updateAsync(float elapsedTime)
{
   return true;
}

//==============================================================================
// BEntity::update
//==============================================================================
bool BEntity::update(float elapsedTime)
{
   //SCOPEDSAMPLE(BEntity_update);
   #ifdef _DEBUG
   if (!getFlagValid())
   {
      BFAIL("Invalid unit being updated.");
      return false;
   }
   #endif

   //Don't update if we're not supposed to.
   if (getFlagDestroy())
      return (false);
 
   bool firstUpdate=getFlagFirstUpdate();
   if (firstUpdate)
   {
      setFlagFirstUpdate(false);
      //if (!isMoving())
      //   setTargetLocation(mPosition);
   }

   bool bWasMoving = isMoving();

   //-- Update our actions
   mActions.update(elapsedTime);

   if((isMoving() || bWasMoving || getFlagFlyingHeightFixup()) && !getFlagUseMaxHeight())
      tieToGround();

   // jce [9/2/2008] -- Experiment: forcing this to update here since I believe there cases where
   // the isMoving() flag is true but setPosition is not getting called, so lastMoveTime wasn't getting
   // updated.  If isMoving is true but lastMoveTime is not updating, it causes movement to break.
   if(isMoving())
      setLastMoveTime(gWorld->getGametime());

   if(getFlagMoved() && getFlagIsBuilt())
      updateObstruction();

   return (true);
}


//==============================================================================
// BEntity::getPlayer
//==============================================================================
BPlayer* BEntity::getPlayer( void )
{
   return gWorld->getPlayer(mPlayerID); 
}

//==============================================================================
// BEntity::getPlayer
//==============================================================================
const BPlayer* BEntity::getPlayer( void ) const
{
   return gWorld->getPlayer(mPlayerID); 
}

//==============================================================================
//==============================================================================
void BEntity::debugRender()
{
   // Tie position to terrain.
   BVector pos = mPosition;
   gTerrainSimRep.getHeight(pos, true);

   BMatrix matrix;
   BSimHelper::calculateDebugRenderMatrix(pos, mForward, mUp, mRight, 1.0f, matrix);

   //Entity movement indicator.
   /*if (getClassType() == cClassTypeUnit)
   {
      if (isMoving())
         gpDebugPrimitives->addDebugCircle(matrix, getObstructionRadius(), cDWORDGreen);
      else
         gpDebugPrimitives->addDebugCircle(matrix, getObstructionRadius(), cDWORDWhite);
   }*/
   
   #ifndef BUILD_FINAL
   if (!gConfig.isDefined(cConfigRenderSimDebugNoBoxes))
   {
      //Entity position.
      DWORD color = cDWORDBlack;
      float radius = 1.0f;
      switch (getClassType())
      {
         case cClassTypeSquad:
            color = cDWORDOrange;
            radius = 2.0f;
            break;
         case cClassTypePlatoon:
            color = cDWORDGreen;
            radius = 4.0f;
            break;
      }
      if (color != cDWORDBlack)
         gpDebugPrimitives->addDebugBox(matrix, radius, color);
   }
   if (gConfig.isDefined(cConfigRenderSimDebugSquadCircles) && (getClassType() == cClassTypeSquad))
   {
      if (getNumberChildren() > 1)
         gpDebugPrimitives->addDebugCircle(matrix, getObstructionRadius(), cDWORDOrange);
      else
      {
         BOPQuadHull quadHull;
         getObstructionHull(quadHull);
         BVector ugh[4];
         for (uint i=0; i < 4; i++)
            ugh[i].set(quadHull.mPoint[i].mX, 0.0f, quadHull.mPoint[i].mZ);
         gpDebugPrimitives->addDebugLine(ugh[0], ugh[1], cDWORDOrange, cDWORDOrange);
         gpDebugPrimitives->addDebugLine(ugh[1], ugh[2], cDWORDOrange, cDWORDOrange);
         gpDebugPrimitives->addDebugLine(ugh[2], ugh[3], cDWORDOrange, cDWORDOrange);
         gpDebugPrimitives->addDebugLine(ugh[3], ugh[0], cDWORDOrange, cDWORDOrange);
      }
   }
   #endif

   //Actions.
   mActions.debugRender();

   //Orientation.
   //matrix.multTranslate(0.0f, 0.125f, 0.0f);
   //gpDebugPrimitives->addDebugArrow(matrix, BVector(0.4f, 0.4f, 0.4f), cColor[getPlayerID()]);   
}


//==============================================================================
//==============================================================================
void BEntity::handleKillBase(bool bKillImmediately)
{
   // Alert the player
   const BPlayer* pPlayer = getPlayer();
   BASSERT(pPlayer);
   pPlayer->getAlertManager()->createBaseDestructionAlert(getPosition(), mID);

   BUnit* pUnit = getUnit();
   if (pUnit && pUnit->getProtoObject()->getFlagKillChildObjectsOnDeath())
   {
      // Reveal base if it's visible
      long numTeams = gWorld->getNumberTeams();
      for (long teamID = 1; teamID < numTeams; teamID++)
      {
         if (pUnit->isVisible(teamID))
         {
            gWorld->createRevealer(teamID, pUnit->getPosition(), pUnit->getObstructionRadius(), 0);
         }
      }

      BPlayer* pPlayer = pUnit->getPlayer();
      bool recycled = pUnit->getFlagRecycled();
      uint numEntityRefs = getNumberEntityRefs();
      for (uint i=0; i<numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 5589
         const BEntityRef* pRef = getEntityRefByIndex(i);
//--
         if (pRef && pRef->mID != mID)
         {
            if (pRef->mType == BEntityRef::cTypeAssociatedBuilding || pRef->mType == BEntityRef::cTypeAssociatedSocket || pRef->mType == BEntityRef::cTypeAssociatedParkingLot)
            {
               BUnit* pAssocItem = gWorld->getUnit(pRef->mID);
               if (pAssocItem && pAssocItem->isAlive())
               {
                  if (recycled)
                  {
                     pAssocItem->setFlagRecycled(true);

                     // Refund portion of building cost
                     const BProtoObject* pProtoObject = pAssocItem->getProtoObject();
                     if (pPlayer && pProtoObject)
                     {
                        BCost cost;
                        pProtoObject->getCost(pPlayer, &cost, -1);
                        if (!pAssocItem->getFlagBuildingConstructionPaused())
                        {
                           if (pAssocItem->getFlagBuilt())
                              cost *= gDatabase.getRecyleRefundRate();
                           float curHp = pAssocItem->getHitpoints();
                           float maxHp = pProtoObject->getHitpoints();
                           if (curHp < maxHp)
                           {
                              float pctDamaged = curHp / maxHp;
                              cost *= pctDamaged;
                           }
                        }
                        pPlayer->refundCost(&cost);
                     }
                  }

                  // Reveal associated items if the base is visible
                  long numTeams = gWorld->getNumberTeams();
                  for (long teamID = 1; teamID < numTeams; teamID++)
                  {
                     if (pUnit->isVisible(teamID))
                     {
                        pAssocItem->makeVisible(teamID);
                        gWorld->createRevealer(teamID, pAssocItem->getPosition(), Math::Max(pAssocItem->getObstructionRadius(), 1.0f), 0);
                     }
                  }
                  pAssocItem->setKilledByID(pUnit->getKilledByID());
                  pAssocItem->kill(bKillImmediately);
               }
            }
         }
      }
   }
   else
   {
      uint numEntityRefs = getNumberEntityRefs();
      for (uint i=0; i<numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 5590
         const BEntityRef* pRef = getEntityRefByIndex(i);
//--
         if (pRef)
         {
            if (pRef->mType == BEntityRef::cTypeAssociatedBuilding || pRef->mType == BEntityRef::cTypeAssociatedSocket)
            {
               BUnit* pAssocBldgOrSocket = gWorld->getUnit(pRef->mID);
               if (pAssocBldgOrSocket)
                  pAssocBldgOrSocket->removeEntityRef(BEntityRef::cTypeAssociatedBase, mID);
            }
         }
      }
   }

   bool selfParkingLot = false;
//-- FIXING PREFIX BUG ID 5591
   const BEntityRef* pParkingLotRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedParkingLot);
//--
   if (pParkingLotRef)
   {
      if (pParkingLotRef->mID == mID)
         selfParkingLot = true;
      else
         setAssociatedParkingLot(cInvalidObjectID);
   }

//-- FIXING PREFIX BUG ID 5592
   const BEntityRef* pSettlementRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedSettlement);
//--
   if (pSettlementRef)
   {
      BUnit* pSettlement = gWorld->getUnit(pSettlementRef->mID);
      if (pSettlement)
      {
         pSettlement->removeEntityRef(BEntityRef::cTypeAssociatedBase, mID);
         if (selfParkingLot)
            pSettlement->setAssociatedParkingLot(cInvalidObjectID);
      }
   }
}


//==============================================================================
//==============================================================================
void BEntity::handleKillSettlement()
{
//-- FIXING PREFIX BUG ID 5596
   const BEntityRef* pParkingLotRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedParkingLot);
//--
   if (pParkingLotRef)
   {
      BUnit* pParkingLot = gWorld->getUnit(pParkingLotRef->mID);
      if (pParkingLot)
         pParkingLot->removeEntityRef(BEntityRef::cTypeAssociatedSettlement, mID);
   }

//-- FIXING PREFIX BUG ID 5597
   const BEntityRef* pBaseRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedBase);
//--
   if (pBaseRef)
   {
      BUnit* pBase = gWorld->getUnit(pBaseRef->mID);
      if (pBase)
         pBase->removeEntityRef(BEntityRef::cTypeAssociatedSettlement, mID);
   }

   uint numEntityRefs = getNumberEntityRefs();
   for (uint i=0; i<numEntityRefs; i++)
   {
//-- FIXING PREFIX BUG ID 5595
      const BEntityRef* pRef = getEntityRefByIndex(i);
//--
      if (pRef)
      {
         if (pRef->mType == BEntityRef::cTypeAssociatedBuilding || pRef->mType == BEntityRef::cTypeAssociatedSocket)
         {
            BUnit* pAssocBldgOrSocket = gWorld->getUnit(pRef->mID);
            if (pAssocBldgOrSocket)
               pAssocBldgOrSocket->removeEntityRef(BEntityRef::cTypeAssociatedSettlement, mID);
         }
      }
   }
}


//==============================================================================
//==============================================================================
BEntityID BEntity::getAssociatedParkingLot() const
{
//-- FIXING PREFIX BUG ID 5598
   const BEntityRef* pPLRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedParkingLot);
//--
   if (pPLRef)
      return (pPLRef->mID);
   else
      return (cInvalidObjectID);
}


//==============================================================================
//==============================================================================
BEntityID BEntity::getAssociatedSettlement() const
{
//-- FIXING PREFIX BUG ID 5599
   const BEntityRef* pSettlementRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedSettlement);
//--
   if (pSettlementRef)
      return (pSettlementRef->mID);
   else
      return (cInvalidObjectID);
}


//==============================================================================
//==============================================================================
BEntityID BEntity::getAssociatedBase() const
{
//-- FIXING PREFIX BUG ID 5600
   const BEntityRef* pBaseRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedBase);
//--
   if (pBaseRef)
      return (pBaseRef->mID);
   else
      return (cInvalidObjectID);
}


//==============================================================================
// BEntity::kill
//==============================================================================
void BEntity::kill(bool bKillImmediately)
{
   if(mpEntityRefs)
   {
      setFlagEntityRefsLocked(true);

      const BUnit* pUnit = this->getUnit();

      bool isBase = (pUnit && pUnit->isType(gDatabase.getOTIDBase()));
      if (isBase)
         handleKillBase(bKillImmediately);
      else if (pUnit && pUnit->isType(gDatabase.getOTIDSettlement()))
         handleKillSettlement();

      uint numEntityRefs = getNumberEntityRefs();
      for (uint i=0; i<numEntityRefs; i++)
      {
         BEntityRef ref = (*mpEntityRefs)[i];

         if (ref.mID != mID)
         {
            // Immediately kill the referenced entity in these cases.
            if (ref.mType == BEntityRef::cTypeGatherChild      || 
                ref.mType == BEntityRef::cTypeBlockerChild     || 
                ref.mType == BEntityRef::cTypeTrainLimitSquad  ||
                ref.mType == BEntityRef::cTypeTrainLimitUnit   ||
                ref.mType == BEntityRef::cTypeBuildQueueChild)
            {
               BEntity* pReferencedEntity = gWorld->getEntity(ref.mID);
               if(pReferencedEntity)
               {
                  #ifdef SYNC_Unit
                  syncUnitData("BEntity::kill mID", ref.mID)
                  #endif
                  pReferencedEntity->kill(bKillImmediately);
                  continue;
               }
            }

            // Kill empty associated sockets.
            if (isBase && ref.mType == BEntityRef::cTypeAssociatedSocket)
            {
               BEntity* pSocket = gWorld->getEntity(ref.mID);
               if(pSocket && !pSocket->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug))
               {
                  #ifdef SYNC_Unit
                  syncUnitData("BEntity::kill mID", ref.mID)
                  #endif
                  pSocket->kill(bKillImmediately);
                  continue;
               }
            }

            // Kill empty parent socket if not linked to a base.
            if (!isBase && ref.mType == BEntityRef::cTypeParentSocket)
            {
               BEntity* pSocket = gWorld->getEntity(ref.mID);
               if(pSocket && !pSocket->getFirstEntityRefByType(BEntityRef::cTypeAssociatedBase))
               {
//-- FIXING PREFIX BUG ID 5601
                  const BUnit* pSocketUnit = pSocket->getUnit();
//--
                  if (!pSocketUnit || !pSocketUnit->getProtoObject()->getFlagPermanentSocket())
                  {
                     #ifdef SYNC_Unit
                     syncUnitData("BEntity::kill mID", ref.mID)
                     #endif
                     pSocket->kill(bKillImmediately);
                     continue;
                  }
               }
            }

            // If the reference type is to the parking lot, decrement the parking lots ref count, then kill if it's 1 (just the settlement).
            if (ref.mType == BEntityRef::cTypeAssociatedParkingLot)
            {
               BEntity* pParkingLot = gWorld->getEntity(ref.mID);
               if (pParkingLot)
               {
                  int refCount = 0;
                  BEntityRef* pRef = pParkingLot->getFirstEntityRefByType(BEntityRef::cTypeParkingLotRef);
                  if (pRef)
                  {
                     pRef->mData1--;
                     refCount = pRef->mData1;
                  }
                  if (refCount <= 1)
                  {
//-- FIXING PREFIX BUG ID 5602
                     const BEntityRef* pSettlementRef = pParkingLot->getFirstEntityRefByType(BEntityRef::cTypeAssociatedSettlement);
//--
                     if (pSettlementRef)
                     {
                        BUnit* pSettlement = gWorld->getUnit(pSettlementRef->mID);
                        if (pSettlement && pSettlement != this)
                           pSettlement->removeEntityRef(BEntityRef::cTypeAssociatedParkingLot, pParkingLot->getID());
                     }
                     #ifdef SYNC_Unit
                     syncUnitData("BEntity::kill mID", ref.mID)
                     #endif
                     pParkingLot->kill(bKillImmediately);
                  }
               }
               continue;
            }

            // Do other custom cleanup for each entity ref type...
            long removeType=-1;
            long removeType2=-1;
            switch(ref.mType)
            {
               case BEntityRef::cTypeTrainLimitBuilding  : removeType=BEntityRef::cTypeTrainLimitUnit; removeType2=BEntityRef::cTypeTrainLimitSquad; break;
               case BEntityRef::cTypeTrainLimitSquad     :
               case BEntityRef::cTypeTrainLimitUnit      : removeType=BEntityRef::cTypeTrainLimitBuilding; break;

               case BEntityRef::cTypeGatherBuilding      : removeType=BEntityRef::cTypeGatherResource; break;
               case BEntityRef::cTypeGatherResource      : removeType=BEntityRef::cTypeGatherBuilding; break;

               case BEntityRef::cTypeGatherUnit          : removeType=BEntityRef::cTypeGatherTarget; break;
               case BEntityRef::cTypeGatherTarget        : removeType=BEntityRef::cTypeGatherUnit; break;

               case BEntityRef::cTypeGatherParent        : removeType=BEntityRef::cTypeGatherChild; break;

               // break the relationship between the building and it's parent socket
               case BEntityRef::cTypeParentSocket        : removeType=BEntityRef::cTypeSocketPlug; break;
               case BEntityRef::cTypeSocketPlug          : removeType=BEntityRef::cTypeParentSocket; break;

               // break the relationship between the building around the base and the socket of the base.
               case BEntityRef::cTypeAssociatedSettlement: 
               case BEntityRef::cTypeAssociatedBase      : removeType=BEntityRef::cTypeAssociatedBuilding; removeType2=BEntityRef::cTypeAssociatedSocket; break;
               case BEntityRef::cTypeAssociatedUnit      : removeType=BEntityRef::cTypeAssociatedUnit; break;
               case BEntityRef::cTypeAssociatedObject    : removeType=BEntityRef::cTypeAssociatedObject; break;
               case BEntityRef::cTypeAssociatedFoundation : removeType=BEntityRef::cTypeAssociatedFoundation; break;
                  
               case BEntityRef::cTypeBuildQueueParent    : removeType=BEntityRef::cTypeBuildQueueChild; break;
            }

            if(removeType != -1)
            {
               BEntity* pEntity=gWorld->getEntity(ref.mID);
               if(pEntity)
               {
                  pEntity->removeEntityRef(removeType, mID);
                  if (removeType2 != -1)
                     pEntity->removeEntityRef(removeType2, mID);

                  if (getProtoObject()->getFlagKillChildObjectsOnDeath())
                  {
                     const BEntityRef* pRef = pEntity->getFirstEntityRefByType(BEntityRef::cTypeBuiltByUnit);
                     if (pRef && pRef->mID == mID)
                     {
                        // if this object really is a child of me, and I want to kill my objects, do that now
                        pEntity->kill(bKillImmediately);
                     }
                  }
               }
            }
         }
      }

      // Finish up.
      mpEntityRefs->clear();
      setFlagEntityRefsLocked(false);
   }
}

//==============================================================================
// BEntity::destroy
//==============================================================================
void BEntity::destroy()
{
   setFlagDestroy(true);
}


//==============================================================================
// BEntity::setPosition
//==============================================================================
void BEntity::setPosition(const BVector pos, bool overridePhysics)
{
#ifdef DEBUG_INVALID_FLOATS
   if(!Math::IsValidFloat(pos.x) || !Math::IsValidFloat(pos.y) || !Math::IsValidFloat(pos.z))
   {
      debugVec = pos;
      pDebugEntity=this;
   }
#endif

   if (mpPhysicsObject && (overridePhysics || mpPhysicsObject->isKeyframed()))
      mpPhysicsObject->setPosition(pos);

   // Update last move time.
   // TOMMYTODO: Unfortunately setPosition is called for everything right now so need to check
   // if it really moved.  Remove this check once setPosition isn't called as much.
   // jce [5/6/2008] -- nuking this out in _MOVE4... naively it doesn't seem to get called except on moving stuff.
   // There is Badness(tm) if something is flagged moving but isn't updating it's lastMoveTime since the pather/collisions
   // just think it's ok to move through this.
   #ifndef _MOVE4
   if (!mPosition.almostEqual(pos))
   #endif
   // jce [9/2/2008] -- move this to ::update to bullet-proof it.  Basically this time needs to be updated every frame the isMoving flag is
   // set.  However, there appear to be times when this flag is set but setPosition isn't called, which is causing
   // issues for movement.
      //setLastMoveTime(gWorld->getGametime());

   mPosition = pos;

   setFlagMoved(true);
}

//==============================================================================
// BEntity::setRotation
//==============================================================================
void BEntity::setRotation(const BMatrix rot, bool overridePhysics)
{
#ifdef DEBUG_INVALID_FLOATS
   if(!Math::IsValidFloat(rot.r[2].x) || !Math::IsValidFloat(rot.r[2].y) || !Math::IsValidFloat(rot.r[2].z))
   {
      debugVec.set(rot.r[2].x, rot.r[2].y, rot.r[2].z);
      pDebugEntity=this;
   }
#endif

   if (mpPhysicsObject && (overridePhysics || mpPhysicsObject->isKeyframed()))
   {
      D3DMATRIX d3dMatrix;
      rot.getD3DXMatrix(d3dMatrix);
      BPhysicsMatrix mtx(d3dMatrix);
      mpPhysicsObject->setRotation(mtx);
   }

   BVector tempVector;
   rot.getForward(tempVector);
   setForward(tempVector);

   rot.getRight(tempVector);
   setRight(tempVector);

   rot.getUp(tempVector);
   setUp(tempVector);
}

//==============================================================================
// BEntity::setForward
//==============================================================================
void BEntity::setForward(const BVector forward)
{
#ifdef DEBUG_INVALID_FLOATS
   if(!Math::IsValidFloat(forward.x) || !Math::IsValidFloat(forward.y) || !Math::IsValidFloat(forward.z) || forward.x > 1.1f || forward.y > 1.1f || forward.z > 1.1f)
   {
      debugVec=forward;
      pDebugEntity=this;
   }
#endif
   mForward=forward;
   //BASSERT(Math::fAbs(mForward.length() - 1.0f) <= 0.0001f);
}

//==============================================================================
// BEntity::setUp
//==============================================================================
void BEntity::setUp(const BVector up)
{ 
#ifdef DEBUG_INVALID_FLOATS
   if(!Math::IsValidFloat(up.x) || !Math::IsValidFloat(up.y) || !Math::IsValidFloat(up.z) || up.x > 1.1f || up.y > 1.1f || up.z > 1.1f)
   {
      debugVec=up;
      pDebugEntity=this;
   }
#endif
   mUp=up;
   //BASSERT(Math::fAbs(mUp.length() - 1.0f) <= 0.0001f);
}

//==============================================================================
// BEntity::setRight
//==============================================================================
void BEntity::setRight(const BVector right)
{ 
#ifdef DEBUG_INVALID_FLOATS
   if(!Math::IsValidFloat(right.x) || !Math::IsValidFloat(right.y) || !Math::IsValidFloat(right.z) || right.x > 1.1f || right.y > 1.1f || right.z > 1.1f)
   {
      debugVec=right;
      pDebugEntity=this;
   }
#endif
   mRight=right;
   //BASSERT(Math::fAbs(mRight.length() - 1.0f) <= 0.0001f);
}

//==============================================================================
// BEntity::setVelocity
//==============================================================================
void BEntity::setVelocity(const BVector velocity)
{
#ifdef DEBUG_INVALID_FLOATS
   if(!Math::IsValidFloat(velocity.x) || !Math::IsValidFloat(velocity.y) || !Math::IsValidFloat(velocity.z))
   {
      debugVec=velocity;
      pDebugEntity=this;
   }
#endif
   mVelocity = velocity;
}

//==============================================================================
// BEntity::moveForward
//==============================================================================
void BEntity::moveForward(float distance)
{
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::moveForward", mPosition + (distance * mForward));
   #endif
   setPosition(mPosition + (distance * mForward));
}

//==============================================================================
// BEntity::moveRight
//==============================================================================
void BEntity::moveRight(float distance)
{
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::moveRight", mPosition + (distance * mRight));
   #endif
   setPosition(mPosition + (distance * mRight));
}

//==============================================================================
// BEntity::moveUp
//==============================================================================
void BEntity::moveUp(float distance)
{
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::moveUp", mPosition + (distance * mUp));
   #endif
   setPosition(mPosition + (distance * mUp));
}

//==============================================================================
// BEntity::moveWorldForward
//==============================================================================
void BEntity::moveWorldForward(float distance)
{
   BVector dir;
   if (fabs(mForward.y) < fabs(mUp.y))
      dir = BVector(mForward.x, 0.0f, mForward.z);
   else
      dir=BVector(mUp.x, 0.0f, mUp.z);
   dir.normalize();
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::moveWorldForward", mPosition + (distance * dir));
   #endif
   setPosition(mPosition + (distance * dir));
}

//==============================================================================
// BEntity::moveWorldRight
//==============================================================================
void BEntity::moveWorldRight(float distance)
{
   BVector dir(mRight.x, 0.0f, mRight.z);
   dir.normalize();

   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::moveWorldRight", mPosition + (distance * dir));
   #endif
   setPosition(mPosition + (distance * dir));
}

//==============================================================================
// BEntity::moveWorldUp
//==============================================================================
void BEntity::moveWorldUp(float distance)
{
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::moveWorldUp", mPosition + (distance * cYAxisVector));
   #endif
   setPosition(mPosition + (distance * cYAxisVector));
}

//==============================================================================
// BEntity::moveRelative
//==============================================================================
void BEntity::moveRelative(const BVector offset)
{
   BVector newPos(mPosition);
   newPos += (offset.z * mForward);
   newPos += (offset.y * mUp);
   newPos += (offset.x * mRight);

   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::moveRelative", newPos);
   #endif
   setPosition(newPos);
}

//==============================================================================
//==============================================================================
void BEntity::beginMove() 
{
   setFlagMoving(true);
}

//==============================================================================
//==============================================================================
void BEntity::endMove() 
{
   if (!getFlagMoving())
      return;   

  setFlagMoving(false);

  updateObstruction();

   //DCP 11/19/08: Nothing listens for this, so let's not send it.
   //sendEvent(mID, mID, BEntity::cEventMoveEnd, 0 );
}

//==============================================================================
//==============================================================================
void BEntity::startMove()
{
   setFlagMoving(true);
   updateObstruction();
}

//==============================================================================
//==============================================================================
void BEntity::stopMove()
{
   setFlagMoving(false);
   updateObstruction();
}

//==============================================================================
// BEntity::stop
//==============================================================================
void BEntity::stop( void ) 
{
   if (getFlagMoving())
      endMove();

   sendEvent(mID, mID, BEntity::cEventStopped, 0 );
}

//==============================================================================
// BEntity::settle
//==============================================================================
void BEntity::settle( void ) 
{
   stop();
   tieToGround();
}


//==============================================================================
//==============================================================================
bool BEntity::isIdle() const
{
   return (getActionByTypeConst(BAction::cActionTypeEntityIdle) != NULL);
}


//==============================================================================
//==============================================================================
DWORD BEntity::getIdleDuration() const
{
   const BEntityActionIdle* pIdleAction = static_cast<const BEntityActionIdle*>(getActionByTypeConst(BAction::cActionTypeEntityIdle));
   if (pIdleAction)
      return (pIdleAction->getIdleDuration());
   else
      return (0);
}


//==============================================================================
// BEntity::tieToGround
//==============================================================================
void BEntity::tieToGround( void )
{
   if (!getFlagTiesToGround())
      return;

   if (getFlagPhysicsControl())
      return;

   //-- tie to ground
   if (getFlagFlying() && mYDisplacement > 0)
   {
      const float lookAheadScale = 1.5f;
      const float maxHeightGainPerSecond = 6.0f;
      const float minAltitude = Math::Min(3.0f, mYDisplacement);
      float maxHeightGain = maxHeightGainPerSecond * gWorld->getLastUpdateLengthFloat();
      float y, newHeight;

      gTerrainSimRep.getCameraHeightRaycast(mPosition, y, true);

      if (getFlagMoving() && getFlagFlying())
      {
         float forwardY;

         gTerrainSimRep.getCameraHeightRaycast(mPosition + mYDisplacement * lookAheadScale * mForward, forwardY, true);
         newHeight = Math::Max(y, forwardY) + mYDisplacement;
      }
      else
         newHeight = y + mYDisplacement;

      float diff = newHeight - mPosition.y;
      float diffA = Math::fAbs(diff);

      if (diffA > cFloatCompareEpsilon)
      {
         setFlagFlyingHeightFixup(true);
         diff = Math::Clamp(diff, -maxHeightGain, maxHeightGain);
      }
      else
         setFlagFlyingHeightFixup(false);

      mPosition.y = Math::Max(y + minAltitude, mPosition.y + diff);
      setFlagMoved(true);

      #ifdef DEBUG_INVALID_FLOATS
      if (!Math::IsValidFloat(mPosition.y))
      {
         debugVec=mPosition;
         pDebugEntity=this;
      }
      #endif
   }
   else if (getUnit() && getUnit()->getProtoObject()->getFlagOrientUnitWithGround())
   {
      // Save off axial vectors before they are altered in orientWithGround
      // Use them as references for later filtering
      BVector vecRight = mRight;
      BVector vecUp = mUp;
      BVector vecForward = mForward;

      orientWithGround();
      if ((vecUp.y != 1.0f) && getUnit()->getProtoObject()->getFlagFilterOrient())
         filterPitchAndRoll( vecRight, vecForward, 0.1f );
   }
   else if (getSquad() && getSquad()->getProtoObject() && (getSquad()->getProtoObject()->getMovementType() == cMovementTypeAir))
   {
      // Do nothing for air squads controlled by physics.  Their physics flight action will set the squad height.
      // For non-physics flying squads, set the height using the flight heights in the sim rep.  This is what BUnitActionMoveAir does.
      if (!getSquad()->isSquadAPhysicsVehicle())
      {
         if (gTerrainSimRep.flightHeightsLoaded())
            gTerrainSimRep.getFlightHeightRaycast(mPosition, mPosition.y, true, true);
         else
            gTerrainSimRep.getCameraHeight(mPosition, true);
      }
   }
   else
   {
      gTerrainSimRep.getHeightRaycast(mPosition, mPosition.y, true);
      setFlagMoved(true);
   }

   return;  
}


//==============================================================================
// BEntity::orientWithGround
//==============================================================================
void BEntity::orientWithGround( void )
{
   int numContactPoints = 4; // This will eventually be determined by model data, but for now, its 4

   // Use the contact points' ground height to determine the pitch and roll for the entity
   // (Zero or One contact point will require finding the polygon normal)
   switch (numContactPoints)
   {   
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      default:

         BVector groundY[4];
         groundY[0] = mPosition + 1.25f * mRight + 2.25f * mForward;
         groundY[1] = mPosition + 1.25f * mRight - 2.25f * mForward;
         groundY[2] = mPosition - 1.25f * mRight + 2.25f * mForward;
         groundY[3] = mPosition - 1.25f * mRight - 2.25f * mForward;

         for (int i=0; i<4; i++)
            gTerrainSimRep.getHeightRaycast(groundY[i], groundY[i].y, true);

         // When using model data, we will have to find opposing corner points - for now we know they are p0 - p3 and p1 - p2
         // FIXME: Find opposing corner points (Add X and Z offsets for each contact point - highest to lowest is one pair)

         float meanHeightOfDiagonal[2];
         meanHeightOfDiagonal[0] = 0.5f * (groundY[0].y + groundY[3].y);
         meanHeightOfDiagonal[1] = 0.5f * (groundY[1].y + groundY[2].y);

         int highDiagonal;
         BVector highDiagonalSegment;
         if (meanHeightOfDiagonal[0] >= meanHeightOfDiagonal[1])
         {
            highDiagonal = 0;
            highDiagonalSegment = groundY[0] - groundY[3];
         }
         else
         {
            highDiagonal = 1;
            highDiagonalSegment = groundY[1] - groundY[2];
         }

         // Mirror one of the ground contact points not in the high diagonal to the opposite side of the entity's center.
         // Then use the average of this and the remaining point to create a point which along with the high diagonal,
         // defines the orientation plane of the entity. (This presumes symmetrical point layout - probably good enough for our needs)
         BVector referencePoint1, referencePoint2;
         BVector mirroredPoint;
         BVector averagedPoint;
         if (highDiagonal == 0)
         {
            referencePoint1 = groundY[1];
            referencePoint2 = groundY[2];
         }
         else
         {
            referencePoint1 = groundY[0];
            referencePoint2 = groundY[3];
         }

         mirroredPoint = 2.0f * mPosition - referencePoint1;
         averagedPoint = 0.5f * (mirroredPoint + referencePoint2);

         BVector referenceSegment = averagedPoint - mPosition;
         BVector newNormal = highDiagonalSegment.cross(referenceSegment);
         if (newNormal.y < 0.0f) // If pointing down, flip the vector
            newNormal.scale(-1.0f);

         newNormal.normalize();

         // Set the new entity position (once again assuming symmetrical contact points at least for now)
         mPosition.y = 0.25f * (groundY[0].y + groundY[1].y + groundY[2].y + groundY[3].y);
         setFlagMoved(true);

         #ifdef DEBUG_INVALID_FLOATS
         if (!Math::IsValidFloat(mPosition.y))
         {
            debugVec=mPosition;
            pDebugEntity=this;
         }
         #endif

         BVector temp;
         temp = newNormal.cross(mForward);
         mForward = temp.cross(newNormal);
         mForward.normalize();

         mUp = newNormal;
         calcRight();

         break;
   }
}


//==============================================================================
// BEntity::filterPitchAndRoll
//==============================================================================
void BEntity::filterPitchAndRoll(BVector &vecRight, BVector &vecForward, float blendRate)
{
   // Blend the right and forward axes, get a new up axis from their cross product,
   // Cross the up and forward vectors to get a right vector orthogonal to both.
   // Normalize the vectors and set the entity's axes back to the blended state.

   BVector rightBlend = blendRate * mRight + (1.0f - blendRate) * vecRight;
   BVector forwardBlend = blendRate * mForward + (1.0f - blendRate) * vecForward;
   BVector upBlend = forwardBlend.cross(rightBlend);
   rightBlend = upBlend.cross(forwardBlend);

   rightBlend.normalize();
   forwardBlend.normalize();
   upBlend.normalize();

   setRight(rightBlend);
   setForward(forwardBlend);
   setUp(upBlend);
}


//==============================================================================
// BEntity::yaw
//==============================================================================
void BEntity::yaw(float angle)
{
   BMatrix tempMatrix;
   tempMatrix.makeRotateArbitrary(angle, mUp);

   // forward
   BVector tempVector;
   tempMatrix.transformVector(mForward, tempVector);
   tempVector.normalize();

   // right
   BVector tempVector2;
   tempVector2.assignCrossProduct(mUp, tempVector);
   tempVector2.normalize();

   tempMatrix.makeOrient(tempVector, mUp, tempVector2);
   setRotation(tempMatrix);
}

//==============================================================================
// BEntity::pitch
//==============================================================================
void BEntity::pitch(float angle)
{
   BMatrix tempMatrix;
   tempMatrix.makeRotateArbitrary(angle, mRight);

   // forward
   BVector tempVector;
   tempMatrix.transformVector(mForward, tempVector);
   tempVector.normalize();

   // up
   BVector tempVector2;
   tempMatrix.transformVector(mUp, tempVector2);
   tempVector2.normalize();

   tempMatrix.makeOrient(tempVector, tempVector2, mRight);
   setRotation(tempMatrix);
}

//==============================================================================
// BEntity::roll
//==============================================================================
void BEntity::roll(float angle)
{
   BMatrix tempMatrix;
   tempMatrix.makeRotateArbitrary(angle, mForward);

   // up
   BVector tempVector;
   tempMatrix.transformVector(mUp, tempVector);
   tempVector.normalize();

   // right
   BVector tempVector2;
   tempVector2.assignCrossProduct(tempVector, mForward);
   tempVector2.normalize();

   tempMatrix.makeOrient(mForward, tempVector, tempVector2);
   setRotation(tempMatrix);
}

//==============================================================================
// BEntity::yawWorld
//==============================================================================
void BEntity::yawWorld(float angle)
{
   BMatrix rot;
   rot.makeRotateY(angle);

   // Forward
   BVector tempVector;
   rot.transformVector(mForward, tempVector);
   tempVector.normalize();

   // Up
   BVector tempVector2;
   rot.transformVector(mUp, tempVector2);
   tempVector2.normalize();

   // Right
   BVector tempVector3;
   tempVector3.assignCrossProduct(tempVector2, tempVector);
   tempVector3.normalize();

   BMatrix tempMatrix;
   tempMatrix.makeOrient(tempVector, tempVector2, tempVector3);
   setRotation(tempMatrix);
}

//==============================================================================
// BEntity::yawWorldAbout
//==============================================================================
void BEntity::yawWorldAbout(float angle, const BVector point)
{
   BMatrix rot;
   rot.makeTranslate(-point.x, 0.0f, -point.z);
   rot.multRotateY(angle);
   rot.multTranslate(point.x, 0.0f, point.z);

   BVector tempVector;
   rot.transformVector(mForward, tempVector);
   tempVector.normalize();
   setForward(tempVector);

   rot.transformVector(mUp, tempVector);
   tempVector.normalize();
   setUp(tempVector);

   rot.transformVector(mRight, tempVector);
   tempVector.normalize();
   setRight(tempVector);

   BVector newPos;
   rot.transformVectorAsPoint(mPosition, newPos);
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::yawWorldAbout", newPos);
   #endif
   setPosition(newPos);
}


//==============================================================================
// BEntity::yawWorldRelative
//==============================================================================
void BEntity::yawWorldRelative(BEntity *pRefObj, float angle, const BVector point)
{
   if (pRefObj == NULL)
      return;

   BMatrix rot;
   rot.makeTranslate(-point.x, 0.0f, -point.z);
   rot.multRotateY(angle);
   rot.multTranslate(point.x, 0.0f, point.z);

   rot.transformVector(pRefObj->getForward(), mForward);
   mForward.normalize();

   rot.transformVector(pRefObj->getUp(), mUp);
   mUp.normalize();

   calcRight();

   BVector newPos;
   rot.transformVectorAsPoint(pRefObj->mPosition, newPos);
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::yawWorldRelative", newPos);
   #endif
   setPosition(newPos);
}

//==============================================================================
// BEntity::pitchWorld
//==============================================================================
void BEntity::pitchWorld(float angle)
{
   BMatrix rot;
   rot.makeRotateX(angle);

   // Forward
   BVector tempVector;
   rot.transformVector(mForward, tempVector);
   tempVector.normalize();

   // Up
   BVector tempVector2;
   rot.transformVector(mUp, tempVector2);
   tempVector2.normalize();

   // Right
   BVector tempVector3;
   tempVector3.assignCrossProduct(tempVector2, tempVector);
   tempVector3.normalize();

   BMatrix tempMatrix;
   tempMatrix.makeOrient(tempVector, tempVector2, tempVector3);
   setRotation(tempMatrix);
}

//==============================================================================
// BEntity::pitchWorldAbout
//==============================================================================
void BEntity::pitchWorldAbout(float angle, const BVector point)
{
   BMatrix rot;
   rot.makeTranslate(-point.x, -point.y, -point.z);
   rot.multRotateArbitrary(angle, mRight);
   rot.multTranslate(point.x, point.y, point.z);

   BVector tempVector;
   rot.transformVector(mForward, tempVector);
   tempVector.normalize();
   setForward(tempVector);

   rot.transformVector(mUp, tempVector);
   tempVector.normalize();
   setUp(tempVector);

   rot.transformVector(mRight, tempVector);
   tempVector.normalize();
   setRight(tempVector);

   BVector newPos;
   rot.transformVectorAsPoint(mPosition, newPos);
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::pitchWorldAbout", newPos);
   #endif
   setPosition(newPos);
}

//==============================================================================
// BEntity::rollWorld
//==============================================================================
void BEntity::rollWorld(float angle)
{
   BMatrix rot;
   rot.makeRotateZ(angle);

   // Up
   BVector tempVector;
   rot.transformVector(mUp, tempVector);
   tempVector.normalize();

   // Forward
   BVector tempVector2;
   rot.transformVector(mForward, tempVector2);
   tempVector2.normalize();

   // Right
   BVector tempVector3;
   tempVector3.assignCrossProduct(tempVector, tempVector2);
   tempVector3.normalize();

   BMatrix tempMatrix;
   tempMatrix.makeOrient(tempVector2, tempVector, tempVector3);
   setRotation(tempMatrix);
}

//==============================================================================
// BEntity::calcRight
//==============================================================================
void BEntity::calcRight()
{
   BVector tempVector;
   tempVector.assignCrossProduct(mUp, mForward);
   tempVector.normalize();
   setRight(tempVector);
}

//==============================================================================
// BEntity::calcForward
//==============================================================================
void BEntity::calcForward()
{
   BVector tempVector;
   tempVector.assignCrossProduct(mRight, mUp);
   tempVector.normalize();
   setForward(tempVector);
}

//==============================================================================
// BEntity::calcUp
//==============================================================================
void BEntity::calcUp()
{
   BVector tempVector;
   tempVector.assignCrossProduct(mForward, mRight);
   tempVector.normalize();
   setUp(tempVector);
}

//==============================================================================
// BEntity::setWorldMatrix
//==============================================================================
void BEntity::setWorldMatrix(const BMatrix& matrix)
{
   BVector v;
   matrix.getTranslation(v);
   #ifdef SYNC_Unit
      if (isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BEntity::setWorldMatrix", v);
   #endif
   setPosition(v);
   setRotation(matrix);
}

//==============================================================================
// BEntity::getWorldMatrix
//==============================================================================
void BEntity::getWorldMatrix(BMatrix& matrix) const
{
   matrix.makeOrient(mForward, mUp, mRight);
   matrix.setTranslation(mPosition);
}

//==============================================================================
// BEntity::getInvWorldMatrix
//==============================================================================
void BEntity::getInvWorldMatrix(BMatrix& matrix) const
{
   getWorldMatrix(matrix);
   matrix.invert();
}

//==============================================================================
//==============================================================================
void BEntity::init()
{
   mVelocity = cOriginVector;
   mPosition = cOriginVector;
   mForward = cZAxisVector;
   mUp = cYAxisVector;
   mRight = cXAxisVector;
   mpObstructionNode = NULL;
   mpPhysicsObject = NULL;
   //mpPhantom = NULL;
   mpEntityRefs = NULL;
   mpPhysicsObject = NULL;
   mParentID = cInvalidObjectID;
   mYDisplacement = 0.0f;
   mpProtoObject = NULL;

   clearFlags();
   
   setPlayerID(cInvalidPlayerID);
   
   //-- Actions
   mActions.clearActions();

   mObstructionRadiusX = -1.0f;
   mObstructionRadiusY = -1.0f;
   mObstructionRadiusZ = -1.0f;
   
   // jce [9/29/2008] -- Make sure the last move time is set appropriately to the time when this entity
   // was created.  Before it was being left as zero for things that never moved, which breaks pathing since
   // it relies on this time being accurate.
   setLastMoveTime(gWorld->getGametime());


   #ifndef BUILD_FINAL
   mEntityName=NULL;
   #endif
}


//==============================================================================
//==============================================================================
void BEntity::setAssociatedParkingLot(BEntityID parkingLotID)
{
   BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedParkingLot);
   if (pRef)
   {
      BEntity* pOldParkingLot = gWorld->getEntity(pRef->mID);
      if (pOldParkingLot)
      {
         short count=0;
         BEntityRef* pRef2 = pOldParkingLot->getFirstEntityRefByType(BEntityRef::cTypeParkingLotRef);
         if (pRef2)
         {
            pRef2->mData1--;
            count=pRef2->mData1;
         }
         if (count==0)
            pOldParkingLot->kill(false);
      }
      if (parkingLotID == cInvalidObjectID)
      {
         removeEntityRef(BEntityRef::cTypeAssociatedParkingLot, pRef->mID);
         return;
      }
      else
         pRef->mID = parkingLotID;
   }
   else
   {
      if (parkingLotID == cInvalidObjectID)
         return;
      addEntityRef(BEntityRef::cTypeAssociatedParkingLot, parkingLotID, 0, 0);
   }

   BEntity* pNewParkingLot = gWorld->getEntity(parkingLotID);
   if (pNewParkingLot)
   {      
      BEntityRef* pRef2 = pNewParkingLot->getFirstEntityRefByType(BEntityRef::cTypeParkingLotRef);
      if (pRef2)
         pRef2->mData1++;
      else
         pNewParkingLot->addEntityRef(BEntityRef::cTypeParkingLotRef, cInvalidObjectID, 1, 0);
   }
}


//==============================================================================
//==============================================================================
void BEntity::setAssociatedSettlement(BEntityID settlementID)
{
   BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedSettlement);
   if (pRef)
   {
      if (settlementID != cInvalidObjectID)
         pRef->mID = settlementID;
      else
         removeEntityRef(BEntityRef::cTypeAssociatedSettlement, pRef->mID);
   }
   else
   {
      if (settlementID != cInvalidObjectID)
         addEntityRef(BEntityRef::cTypeAssociatedSettlement, settlementID, 0, 0);
   }
}


//==============================================================================
//==============================================================================
void BEntity::setAssociatedBase(BEntityID baseID)
{
   BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedBase);
   if (pRef)
   {
      if (baseID != cInvalidObjectID)
         pRef->mID = baseID;
      else
         removeEntityRef(BEntityRef::cTypeAssociatedBase, pRef->mID);
   }
   else
   {
      if (baseID != cInvalidObjectID)
         addEntityRef(BEntityRef::cTypeAssociatedBase, baseID, 0, 0);
   }
}


//==============================================================================
//==============================================================================
void BEntity::setParentSocket(BEntityID parentSocketID)
{
   BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeParentSocket);
   if (pRef)
   {
      if (parentSocketID != cInvalidObjectID)
         pRef->mID = parentSocketID;
      else
         removeEntityRef(BEntityRef::cTypeParentSocket, pRef->mID);
   }
   else
   {
      if (parentSocketID != cInvalidObjectID)
         addEntityRef(BEntityRef::cTypeParentSocket, parentSocketID, 0, 0);
   }
}


//==============================================================================
//==============================================================================
void BEntity::setSocketPlug(BEntityID socketPlugID)
{
   BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeSocketPlug);
   if (pRef)
   {
      if (socketPlugID != cInvalidObjectID)
         pRef->mID = socketPlugID;
      else
         removeEntityRef(BEntityRef::cTypeSocketPlug, pRef->mID);
   }
   else
   {
      if (socketPlugID != cInvalidObjectID)
         addEntityRef(BEntityRef::cTypeSocketPlug, socketPlugID, 0, 0);
   }
}


//==============================================================================
//==============================================================================
void BEntity::addAssociatedSocket(BEntityID socketID)
{
   uint numEntityRefs = getNumberEntityRefs();
   for (uint i=0; i<numEntityRefs; i++)
   {
      // Avoid adding a duplicate reference to the associated socket.
//-- FIXING PREFIX BUG ID 5613
      const BEntityRef* pRef = getEntityRefByIndex(i);
//--
      if (pRef && pRef->mType == BEntityRef::cTypeAssociatedSocket && pRef->mID == socketID)
         return;
   }

   if (socketID != cInvalidObjectID)
      addEntityRef(BEntityRef::cTypeAssociatedSocket, socketID, 0, 0);
}


//==============================================================================
//==============================================================================
void BEntity::removeAssociatedSocket(BEntityID socketID)
{
   removeAllMatchingEntityRefs(BEntityRef::cTypeAssociatedSocket, socketID, 0, 0);
}


//==============================================================================
//==============================================================================
void BEntity::addAssociatedBuilding(BEntityID buildingID)
{
   uint numEntityRefs = getNumberEntityRefs();
   for (uint i=0; i<numEntityRefs; i++)
   {
      // Avoid adding a duplicate reference to the associated building.
//-- FIXING PREFIX BUG ID 5614
      const BEntityRef* pRef = getEntityRefByIndex(i);
//--
      if (pRef && pRef->mType == BEntityRef::cTypeAssociatedBuilding && pRef->mID == buildingID)
         return;
   }

   if (buildingID != cInvalidObjectID)
      addEntityRef(BEntityRef::cTypeAssociatedBuilding, buildingID, 0, 0);
}


//==============================================================================
//==============================================================================
void BEntity::removeAssociatedBuilding(BEntityID buildingID)
{
   removeAllMatchingEntityRefs(BEntityRef::cTypeAssociatedBuilding, buildingID, 0, 0);
}

//==============================================================================
//==============================================================================
void BEntity::addAssociatedFoundation(BEntityID socketID)
{
   uint numEntityRefs = getNumberEntityRefs();
   for (uint i=0; i<numEntityRefs; i++)
   {
      // Avoid adding a duplicate reference to the associated socket.
      //-- FIXING PREFIX BUG ID 5613
      const BEntityRef* pRef = getEntityRefByIndex(i);
      //--
      if (pRef && pRef->mType == BEntityRef::cTypeAssociatedFoundation&& pRef->mID == socketID)
         return;
   }

   if (socketID != cInvalidObjectID)
      addEntityRef(BEntityRef::cTypeAssociatedFoundation, socketID, 0, 0);
}


//==============================================================================
//==============================================================================
void BEntity::removeAssociatedFoundation(BEntityID socketID)
{
   removeAllMatchingEntityRefs(BEntityRef::cTypeAssociatedFoundation, socketID, 0, 0);
}

//==============================================================================
//==============================================================================
void BEntity::onRelease()
{
   mActions.clearActions();

   setFlagValid(false);

   if (!gWorldReset)
      deleteObstruction();

   if (mpEntityRefs)
   {
      delete mpEntityRefs;
      mpEntityRefs=NULL;
   }

   releasePhysicsObject();
}

//==============================================================================
// BEntity::getForwardVelocity
//==============================================================================
float BEntity::getForwardVelocity( void ) const
{
  BMatrix mat;
  mat.makeOrient(mForward, mUp, mRight);
  BVector dir;
  mat.transformVector(mVelocity, dir);
  return dir.x;
}

//==============================================================================
// Check and see if position is outside playable bounds
//==============================================================================
bool BEntity::isOutsidePlayableBounds(bool forceCheckWorldBoundaries) const
{ 
   return(gWorld->isOutsidePlayableBounds(mPosition, forceCheckWorldBoundaries)); 
}

//==============================================================================
//==============================================================================
void BEntity::onAcquire()
{
   init();
}

//==============================================================================
// BEntity::addEventListener
//==============================================================================
bool BEntity::addEventListener(IEventListener*  pListener)
{
   // Bomb check
   if (!pListener)
      return(false);

   // Check if we already have this action and use that one.  Otherwise create the action.
   BEntityActionListen*  pAction = (BEntityActionListen*)getActionByType(BAction::cActionTypeEntityListen);
   if (!pAction)
   {
      pAction = (BEntityActionListen*)gActionManager.createAction(BAction::cActionTypeEntityListen);
      if (!pAction)
         return(false);
      addAction(pAction);
   }

   // We have the action now, add the event listener.
   BASSERT(pAction);
   pAction->addEventListener(pListener);
   return(true);
}

//==============================================================================
// BEntity::removeEventListener
//==============================================================================
void BEntity::removeEventListener(IEventListener*  pListener)
{
   // Bomb check
   if (!pListener)
      return;
   BEntityActionListen*  pAction = (BEntityActionListen*)getActionByType(BAction::cActionTypeEntityListen);
   if (pAction)
      pAction->removeEventListener(pListener);
}

//==============================================================================
// BEntity::addAction
// mrh 6/23/06 - Release the action if we're unable to add it, because nobody 
// checks the return value to handle it later.
//==============================================================================
bool BEntity::addAction(BAction *pAction, BSimOrder* pOrder)
{
   if (!pAction)
      return (false);

#ifdef SYNC_UnitAction
   syncUnitActionData("BEntity::addAction pActionID", (int)pAction->getID());
#endif

   bool addActionResult=mActions.addAction(pAction, this, pOrder);
   if (!addActionResult)
      gActionManager.releaseAction(pAction);

#ifndef BUILD_FINAL
   if (addActionResult && mActions.getNumberActions() > 16)
   {
      BSimString msg;
      long classType = getClassType();
      if (classType == cClassTypeObject || classType == cClassTypeUnit)
      {
         const BProtoObject* pProto = ((BObject*)this)->getProtoObject();
         if (pProto)
            msg.format("Too many actions %d, Object %s, ID %d", mActions.getNumberActions(), pProto->getName().getPtr(), mID.asLong());
      }
      else if (classType == cClassTypeSquad)
      {
         const BProtoSquad* pProto = ((BSquad*)this)->getProtoSquad();
         if (pProto)
            msg.format("Too many actions %d, Squad %s, ID %d", mActions.getNumberActions(), pProto->getName().getPtr(), mID.asLong());
      }
      if (msg.isEmpty())
         msg.format("Too many actions %d, Entity Type %d, ID %d", mActions.getNumberActions(), classType, mID.asLong());

      uint actionCounts[BAction::cActionTypeInvalid];
      for (uint i=0; i<BAction::cActionTypeInvalid; i++)
         actionCounts[i]=0;
      uint numActions = (uint)mActions.getNumberActions();
      for (uint i=0; i<numActions; i++)
      {
         const BAction* pAction = mActions.getAction(i);
         if (pAction)
         {
            int actionType = pAction->getType();
            if (actionType >= 0 && actionType < BAction::cActionTypeInvalid)
               actionCounts[actionType]++;
         }
      }
      BSimString actionStr;
      for (uint i=0; i<BAction::cActionTypeInvalid; i++)
      {
         if (actionCounts[i] > 0)
         {
            const char* pActionName = gActionManager.getActionName(i);
            actionStr.format(", %s %d", (pActionName ? pActionName : "Unknown"), actionCounts[i]);
            msg.append(actionStr);
         }
      }

      BASSERTM(0, msg.getPtr());
   }
#endif

   return(addActionResult);
}

//==============================================================================
//==============================================================================
bool BEntity::removeAction(BAction *pAction)
{
   if (!pAction)
      return (false);

#ifdef SYNC_UnitAction
   syncUnitActionData("BEntity::removeAction pActionID", (int)pAction->getID());
#endif

   //-- disconnect() is called in the action list
   return mActions.removeAction(pAction);
}

//==============================================================================
//==============================================================================
void BEntity::removeActions(bool ignorePersistent)
{
   if (ignorePersistent)
      mActions.clearNonPersistentActions();
   else
      mActions.clearActions();
}

//==============================================================================
//==============================================================================
bool BEntity::removeActionByID(long id)
{
   if (id == cInvalidActionID)
      return (false);

   BAction* pAction=mActions.getActionByID(id);
   if(!pAction)
      return false;
   return removeAction(pAction);
}

//==============================================================================
//==============================================================================
BAction* BEntity::findActionByID( long id )
{
   return mActions.getActionByID(id);
}

//==============================================================================
//==============================================================================
bool BEntity::hasPersistentMoveAction() const
{
   for (long i=0; i < mActions.getNumberActions(); i++)
   {
      const BAction* pAction=mActions.getAction(i);
      if (!pAction || !pAction->getFlagPersistent())
         continue;
      if ((pAction->getType() == BAction::cActionTypeUnitMove) ||
         (pAction->getType() == BAction::cActionTypeUnitMoveAir) ||
         (pAction->getType() == BAction::cActionTypeUnitMoveWarthog) ||
         (pAction->getType() == BAction::cActionTypeUnitMoveGhost))
         return (true);
   }
   return (false);
}


//==============================================================================
//==============================================================================
BEntity* BEntity::getParent() const
{
   return(gWorld->getEntity(mParentID));
}


//==============================================================================
// BEntity::sendEvent
//==============================================================================
void BEntity::sendEvent(BEntityID targetID,  BEntityID senderID, DWORD eventType, DWORD data, DWORD data2/*=0*/)
{
   if (targetID == mID)
   {
      notify(eventType, senderID, data, data2);
      return;
   }

   BEntity *pTarget = gWorld->getObjectManager()->getEntity(targetID);
   if (!pTarget)
      return;

   pTarget->notify(eventType, senderID, data, data2);   
}

//==============================================================================
// BEntity::notify
//==============================================================================
void BEntity::notify(DWORD eventType, BEntityID sender, DWORD data, DWORD data2)
{
   //-- first notify all the actions
   mActions.notify(eventType, sender, data, data2);

   // Handle switch player event
   if (eventType == cEventSwitchToPlayerID)
   {
      setPlayerID(data);

      BEntity *pEntity = gWorld->getEntity(getParentID());
      if (pEntity)
         pEntity->setPlayerID(data);
   }

   //WMJ 
   //-- only tell the parent if we sent the event
   if (sender != mID)
      return;

   //-- then the parent
   if (getParentID() == cInvalidObjectID)
      return;

   BEntity *pParent = gWorld->getObjectManager()->getEntity(getParentID());
   if (!pParent)
      return;

   pParent->notify(eventType, sender, data, data2);
}

//==============================================================================
//==============================================================================
float BEntity::calculateXZDistance(BVector point) const
{
   return(calculateXZDistance(mPosition, point));
}

//==============================================================================
//==============================================================================
float BEntity::calculateXZDistanceSqr(BVector point) const
{
   return(calculateXZDistanceSqr(mPosition, point));
}

//==============================================================================
//==============================================================================
float BEntity::calculateXZDistance(const BEntity *pTarget) const
{
   return(calculateXZDistance(mPosition, pTarget));
}

//==============================================================================
//==============================================================================
float BEntity::calculateXZDistance(BVector myTempPos, BVector point) const
{
   if (isEverMobile())   
      return calcDistRadiusToPoint(myTempPos, point);
   else
      return calcDistObstructionToPoint(myTempPos, point);
}

//==============================================================================
//==============================================================================
float BEntity::calculateXZDistanceSqr(BVector myTempPos, BVector point) const
{
   if (isEverMobile())      
      return calcDistRadiusToPointSqr(myTempPos, point);
   else
      return calcDistObstructionToPointSqr(myTempPos, point);
}

//==============================================================================
//==============================================================================
float BEntity::calculateXZDistance(BVector myTempPos, const BEntity *pTarget) const
{
   BASSERT(pTarget);
   
   if (isEverMobile())
   {
      if (pTarget->isEverMobile())
         return calcDistRadiusToRadius(myTempPos, pTarget);
      else
         return calcDistRadiusToObstruction(myTempPos, pTarget);
   }
   else
   {
      if (pTarget->isEverMobile())
         return calcDistObstructionToRadius(myTempPos, pTarget);
      else
         return calcDistObstructionToObstruction(myTempPos, pTarget);
   }
}


//==============================================================================
//==============================================================================
float BEntity::calculateXZDistance(BSimTarget &target)
{
   // If there is a target entity, get distance to that.  Otherwise just use the point.
   BEntity *entity = gWorld->getEntity(target.getID());
   float dist;
   if(entity)
      dist = calculateXZDistance(entity);
   else
      dist = calculateXZDistance(target.getPosition());
   
   return(dist);
}


//==============================================================================
// BEntity::createPhysicsObject
//==============================================================================
bool BEntity::createPhysicsObject(long physicsInfoID, const BPhysicsObjectBlueprintOverrides* pBPOverrides, bool completeOnInactivePhysics, bool bIsPhysicsReplacement)
{
   BPhysicsInfo *pInfo = gPhysicsInfoManager.get(physicsInfoID, true);
   if (!pInfo)
      return (false);

   return createPhysicsObject(pInfo, pBPOverrides, completeOnInactivePhysics, bIsPhysicsReplacement);
}

//==============================================================================
// BEntity::createPhysicsObject
//==============================================================================
bool BEntity::createPhysicsObject(const BPhysicsInfo* pInfo, const BPhysicsObjectBlueprintOverrides* pBPOverrides, bool completeOnInactivePhysics, bool bIsPhysicsReplacement)
{
   bool fromSave = gSaveGame.isLoading();

   BPhysicsWorld *physicsWorld = gWorld->getPhysicsWorld();
   if (!physicsWorld)
      return (false);

   if (!pInfo)
      return false;

   #ifdef SYNC_Unit
      syncUnitActionData("BEntity::createPhysicsObject pInfo", pInfo->getFilename());
   #endif

   if (mpPhysicsObject)
      return (false);

   BPhysicsMatrix rot;
   rot.makeIdentity();
   rot.setForward(mForward);
   rot.setUp(mUp);
   rot.setRight(mRight);

   
   bool bIsFixed = pInfo->isFixed();

   // Phantom
   /*if (pInfo->isPhantom() && getObject())
   {
      float minObsXZ = Math::Min(getProtoObject()->getObstructionRadiusX(), getProtoObject()->getObstructionRadiusZ());
      BVector min = BVector(-minObsXZ, 0.0f, -minObsXZ) + mPosition;
      BVector max = BVector(minObsXZ, getProtoObject()->getObstructionRadiusY() * 2.0f, minObsXZ) + mPosition;

      mpPhantom = new BPhantom(physicsWorld, min, max, &gPhantomListener, physicsWorld->getPhantomCollisionFilterInfo(), (int) this);
   }
   // Clamshell physics object
   else*/
   if (pInfo->isClamshell())
   {
      // Get blueprints
      BPhysicsObjectBlueprint *upperBP = gPhysics->getPhysicsObjectBlueprintManager().get(pInfo->getBlueprintID(BPhysicsInfo::cUpper));
      BPhysicsObjectBlueprint *lowerBP = gPhysics->getPhysicsObjectBlueprintManager().get(pInfo->getBlueprintID(BPhysicsInfo::cLower));
      BPhysicsObjectBlueprint *pelvisBP = gPhysics->getPhysicsObjectBlueprintManager().get(pInfo->getBlueprintID(BPhysicsInfo::cPelvis));
      if (!upperBP || !lowerBP || !pelvisBP)
         return false;

      #ifdef SYNC_Unit
         if (getProtoObject())
         {
            syncUnitActionData("BEntity::createPhysicsObject (clamshell) name", getProtoObject()->getName());
         }
         else
         {
            syncUnitActionData("BEntity::createPhysicsObject (clamshell) PID", getProtoID());
         }
      #endif
      mpPhysicsObject = new BClamshellPhysicsObject(physicsWorld, *upperBP, *lowerBP, *pelvisBP, mPosition,
                                                    pInfo->getCenterOffset(), rot, pInfo->getUpperHeightOffset(), pInfo->getLowerHeightOffset(),
                                                    (DWORD)this, false, pBPOverrides);
   }
   // Regular physics object
   else
   {
      // Get blueprint.
      BPhysicsObjectBlueprint *bp = gPhysics->getPhysicsObjectBlueprintManager().get(pInfo->getBlueprintID(0));
      if(!bp)
         return(false);
      #ifdef SYNC_Unit
         if (getProtoObject())
         {
            syncUnitActionData("BEntity::createPhysicsObject name", getProtoObject()->getName());
         }
         else
         {
            syncUnitActionData("BEntity::createPhysicsObject PID", getProtoID());
         }
      #endif

      mpPhysicsObject = new BPhysicsObject(physicsWorld, *bp, mPosition, pInfo->getCenterOffset(), rot, (DWORD)this, bIsFixed, pBPOverrides);
   }
   if(!mpPhysicsObject)
      return(false);

   // No rigid body means this is useless.
   if(!mpPhysicsObject->getRigidBody())
   {
      delete mpPhysicsObject;
      mpPhysicsObject = NULL;
      return(false);
   }

   // Release phantom if there is one since we have a legitimate RB physics object now
   /*if (mpPhantom)
   {
      delete mpPhantom;
      mpPhantom = NULL;
   }*/

   if(!bIsFixed)
   {
      BUnitActionPhysics* pAction = NULL;
      if (fromSave)
         pAction = (BUnitActionPhysics*)getActionByType(BAction::cActionTypeUnitPhysics);
      else
      {
         // Add persistent physics action when not dealing with a building
         pAction = reinterpret_cast<BUnitActionPhysics*>(gActionManager.createAction(BAction::cActionTypeUnitPhysics));
         if (pAction)
         {
            pAction->setFlagCompleteOnInactivePhysics(completeOnInactivePhysics);
            pAction->setFlagPersistent(true);
            addAction(pAction);
         }
      }

      if(getClassType() == cClassTypeUnit || getClassType() == cClassTypeObject)
      {
         //-- Push in the physicsinfo ID
         if (pAction)
            pAction->setPhysicsInfoID(pInfo->getID());
      }

      // Velocities to 0.
      mpPhysicsObject->setLinearVelocity(cOriginVector);
      mpPhysicsObject->setAngularVelocity(cOriginVector);
   }

   mpPhysicsObject->setInfoID(pInfo->getID());

   // Add to world.
   mpPhysicsObject->addToWorld();

   // Don't tie to ground.
   //setTieToGround(false);

   // MS 8/23/2005: 18065
   //mpPhysicsObject->getPosition(mPreviousPosition.mPosition);

   if (!fromSave)
      startPhysics();

   // Callback.
   // WMJ Can't do this because we are updating physics in another thread.  Will have to implement
   // delayed callback mechanism
   //setUpdateCallback(callbackFunction, pUserData);
   /*
   if (pInfo->isVehicle())
   {
      mpPhysicsObject->enableDeactivation(false);
      mpPhysicsObject->setCollisionFilterInfo(physicsWorld->getTestVehicleCollisionInfo());
      //mPhysicsVehicleID = physicsWorld->createTestVehicle(mpPhysicsObject);
      startPhysics();
   }
   */

   // Create and add vehicle physics action - make physics always active for now
   // TODO - we should allow the physics to activate / deactivate without killing it off.
   // This would potentially help perf.
   setupVehiclePhysics();

   if(bIsFixed)
   {
      mpPhysicsObject->setCollisionFilterInfo(gWorld->getPhysicsWorld()->getBuildingCollisionFilterInfo());
      mpPhysicsObject->updateCollisionFilter();
   }

   // Set physics replacement flag
   if (!fromSave)
      setFlagIsPhysicsReplacement(bIsPhysicsReplacement);

   return (true);
}

//==============================================================================
// BEntity::createPhysicsObjectDirect
//==============================================================================
bool BEntity::createPhysicsObjectDirect(const BPhysicsObjectParams &params, bool completeOnInactivePhysics)
{
   bool fromSave = gSaveGame.isLoading();

   BPhysicsWorld *physicsWorld = gWorld->getPhysicsWorld();
   if (!physicsWorld)
      return (false);

   if (mpPhysicsObject)
      return (false);

   #ifdef SYNC_Unit
      if (getProtoObject())
      {
         syncUnitActionData("BEntity::createPhysicsObjectDirect name", getProtoObject()->getName());
      }
      else
      {
         syncUnitActionData("BEntity::createPhysicsObjectDirect PID", getProtoID());
      }
   #endif

   mpPhysicsObject = new BPhysicsObject(physicsWorld, params);

   if(!mpPhysicsObject)
      return(false);

   // No rigid body means this is useless.
   if(!mpPhysicsObject->getRigidBody())
   {
      delete mpPhysicsObject;
      mpPhysicsObject = NULL;
      return(false);
   }

   if (!fromSave)
   {
      // Add persistent physics action
      BUnitActionPhysics* pAction = reinterpret_cast<BUnitActionPhysics*>(gActionManager.createAction(BAction::cActionTypeUnitPhysics));
      if (pAction)
      {
         pAction->setFlagCompleteOnInactivePhysics(completeOnInactivePhysics);
         pAction->setFlagPersistent(true);
         addAction(pAction);
      }
   }

   // Velocities to 0.
   mpPhysicsObject->setLinearVelocity(cOriginVector);
   mpPhysicsObject->setAngularVelocity(cOriginVector);

   // Add to world.
   mpPhysicsObject->addToWorld();

   // Don't tie to ground.
   //setTieToGround(false);

   // MS 8/23/2005: 18065
   //mpPhysicsObject->getPosition(mPreviousPosition.mPosition);

   if (!fromSave)
      startPhysics();

   return (true);
}


//==============================================================================
// BEntity::releasePhysicsObject
//==============================================================================
void BEntity::releasePhysicsObject( void ) 
{
   // Kill physics object.
   if (mpPhysicsObject)
   {
      BUnitActionPhysics* pAction = reinterpret_cast<BUnitActionPhysics*>(getActionByType(BAction::cActionTypeUnitPhysics));
      if (pAction)
         pAction->setState(BAction::cStateDone);

      delete mpPhysicsObject;
      mpPhysicsObject = NULL;
   }

   // Delete phantom if there is one
   /*if (mpPhantom)
   {
      delete mpPhantom;
      mpPhantom = NULL;
   }*/
}

//==============================================================================
//==============================================================================
void BEntity::setupVehiclePhysics()
{
   if (!mpPhysicsObject)
      return;

   if (getClassType() != cClassTypeUnit)
      return;

//-- FIXING PREFIX BUG ID 5621
   const BPhysicsInfo* pInfo = gPhysicsInfoManager.get(mpPhysicsObject->getInfoID(), true);
//--
   if (!pInfo)
      return;

   if (!pInfo->isVehicle())
      return;

   // Create physics action
   // Determine proper action based on the vehicle type.  Any actions derived from
   // BPhysicsGroundVehicleAction should not collide with the terrain as this is
   // specifically done inside the action.
   bool collideWithTerrain = true;
   hkpAction* pAction = NULL;
   if (pInfo->getVehicleType() == BPhysicsInfo::cWarthog)
      pAction = new BPhysicsWarthogAction(mpPhysicsObject->getRigidBody(), this, false);
   else if (pInfo->getVehicleType() == BPhysicsInfo::cGhost)
      pAction = new BPhysicsGhostAction(mpPhysicsObject->getRigidBody(), this);
   else if ((pInfo->getVehicleType() == BPhysicsInfo::cScorpion) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cGrizzly) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cElephant) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cRhino) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cReactor))
   {
      pAction = new BPhysicsScorpionAction(mpPhysicsObject->getRigidBody(), this, pInfo);
      collideWithTerrain = false;
   }
   else if (pInfo->getVehicleType() == BPhysicsInfo::cCobra)
   {
      pAction = new BPhysicsCobraAction(mpPhysicsObject->getRigidBody(), this, pInfo);
      collideWithTerrain = false;
   }
   else if (pInfo->getVehicleType() == BPhysicsInfo::cGremlin)
   {
      BPhysicsCobraAction* pCobraAction = new BPhysicsCobraAction(mpPhysicsObject->getRigidBody(), this, pInfo);
      pCobraAction->setFlagGremlin(true);
      pAction = pCobraAction;
      collideWithTerrain = false;
   }
   else if (pInfo->getVehicleType() == BPhysicsInfo::cWolverine)
   {
      pAction = new BPhysicsWolverineAction(mpPhysicsObject->getRigidBody(), this, pInfo);
      collideWithTerrain = false;
   }
   else if (pInfo->getVehicleType() == BPhysicsInfo::cGround)
   {
      pAction = new BPhysicsGroundVehicleAction(mpPhysicsObject->getRigidBody(), this, pInfo);
      collideWithTerrain = false;
   }
   else if ((pInfo->getVehicleType() == BPhysicsInfo::cHawk) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cHornet) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cVulture) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cBanshee) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cVampire) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cSentinel))
      pAction = new BPhysicsHoverFlightAction(mpPhysicsObject->getRigidBody(), this);
   else if (pInfo->getVehicleType() == BPhysicsInfo::cChopper)
   {
      pAction = new BPhysicsWarthogAction(mpPhysicsObject->getRigidBody(), this, true);
   }

   if (pAction)
   {
      gWorld->getPhysicsWorld()->getHavokWorld()->addAction(pAction);
      pAction->removeReference();
   }
   mpPhysicsObject->addHavokCollisionListener(&gVehicleCollisionListener);

   // Set vehicle collision filter
   if (collideWithTerrain)
      mpPhysicsObject->setCollisionFilterInfo(gWorld->getPhysicsWorld()->getVehicleCollisionFilterInfo());
   else
      mpPhysicsObject->setCollisionFilterInfo(gWorld->getPhysicsWorld()->getNoTerrainVehicleCollisionFilterInfo());

   /*
   if (pInfo->getVehicleType() == BPhysicsInfo::cElephant)
   {
      mpPhysicsObject->getRigidBody()->setShape(mpPhysicsObject->getRigidBody()->getCollidable()->getShape());
      return(true);
   }
   */

   // MPB TODO - This is a hacked convex shape from the havok vehicle demo.  We need to adjust this
   // and make it data driven.
   // Set hacked temporary vehcile shape

   if ((pInfo->getVehicleType() == BPhysicsInfo::cHawk) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cHornet) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cVulture) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cBanshee) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cVampire) ||
      (pInfo->getVehicleType() == BPhysicsInfo::cSentinel))
   {
      mpPhysicsObject->getRigidBody()->setShape(mpPhysicsObject->getRigidBody()->getCollidable()->getShape());
   }
   else
   {
      mpPhysicsObject->getRigidBody()->setShape(gWorld->getPhysicsWorld()->getVehicleShape());
   }
}

//==============================================================================
//==============================================================================
BVector BEntity::getPhysicsVelocity()  const
{
   if (!mpPhysicsObject)
      return(cOriginVector);
   BVector foo;
   mpPhysicsObject->getLinearVelocity(foo);
   return(foo);
}

//==============================================================================
//==============================================================================
BVector BEntity::getPhysicsForward() const
{
   if (!mpPhysicsObject)
      return(cZAxisVector);
   BVector foo;
   BPhysicsMatrix bar;
   mpPhysicsObject->getRotation(bar);
   bar.getForward(foo);
   return(foo);
}

//==============================================================================
//==============================================================================
BVector BEntity::getPhysicsPosition()  const
{
   if (!mpPhysicsObject)
      return(cOriginVector);
   BVector foo;
   mpPhysicsObject->getPosition(foo);
   return(foo);
}

//==============================================================================
// Set the physics key framing, and optionally reset the physics
//==============================================================================
void BEntity::setPhysicsKeyFramed(bool keyFramed, bool resetPhysics /*= false*/)
{
   BPhysicsObject* pPhysicsObject = getPhysicsObject();
   if (pPhysicsObject)
   {
      pPhysicsObject->setKeyframed(keyFramed);      
      setFlagPhysicsControl(!keyFramed);

      if (resetPhysics)
      {
         pPhysicsObject->setLinearVelocity(cOriginVector);
         pPhysicsObject->setAngularVelocity(cOriginVector);
      }
   }
}

//==============================================================================
// See if the physics is being key framed
//==============================================================================
bool BEntity::getPhysicsKeyFramed()
{
   bool result = false;
//-- FIXING PREFIX BUG ID 5626
   const BPhysicsObject* pPhysicsObject = getPhysicsObject();
//--
   if (pPhysicsObject)
   {
      result = pPhysicsObject->isKeyframed();
   }

   return (result);
}

//==============================================================================
//==============================================================================
bool BEntity::canMove(bool allowAutoUnlock) const
{
   if ((getDesiredVelocity() < cFloatCompareEpsilon) ||
      (getMaxVelocity() < cFloatCompareEpsilon) ||
      !isMobile())
      return (false);        

   if (isLockedDown() && !allowAutoUnlock && !canAutoUnlock())
      return (false);

   //GTG.
   return (true);
}

//==============================================================================
//==============================================================================
bool BEntity::canAutoUnlock() const
{
   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject)
   {
      if (pProtoObject->getAutoLockDownType() == cAutoLockAndUnlock || pProtoObject->getAutoLockDownType() == cAutoUnlockOnly)
         return (true);
   }
   return (false);
}

//==============================================================================
//==============================================================================
bool BEntity::canAutoLock() const
{
   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject)
   {
      if (pProtoObject->getAutoLockDownType() == cAutoLockAndUnlock || pProtoObject->getAutoLockDownType() == cAutoLockOnly)
         return (true);
   }
   return (false);
}

//==============================================================================
//==============================================================================
const BProtoObject* BEntity::getProtoObjectForSquad() const
{
   // This replicates the functionality of BSquad::getProtoObjectID but returns the
   // leader unit's cached proto object rather than requiring looking it up from the player.
   // If the BSquad::getProtoObjectID changes then this might need to change too.
   BDEBUG_ASSERT(isClassType(cClassTypeSquad));
   const BSquad* pSquad = reinterpret_cast<const BSquad*>(this);
   if (pSquad->getFlagProtoSquad())
   {
//-- FIXING PREFIX BUG ID 5627
      const BUnit* pLeaderUnit = pSquad->getLeaderUnit();
//--
      if (pLeaderUnit)
         return (pLeaderUnit->getProtoObject());
   }
   else
   {
      BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
      if (pPlayer)
         return (pPlayer->getProtoObject(getProtoID()));
   }
   return NULL;
}

#ifdef BUILD_DEBUG
//==============================================================================
//==============================================================================
void BEntity::verifyProtoObject() const
{
   // Confirm cached proto object pointer is up to date
   const BPlayer* pPlayer = getPlayer();
   if (pPlayer)
   {
      BASSERT(mpProtoObject == pPlayer->getProtoObject(getProtoID()));
   }
}
#endif

//==============================================================================
//==============================================================================
float BEntity::calcDistRadiusToPoint(BVector myTempPos, BVector vPoint) const
{

   float fDist = 0.0f;
   fDist = myTempPos.xzDistance(vPoint);
   float fRadius1 = getObstructionRadius();
   fDist -= fRadius1;

   // Don't be negative.
   if(fDist<0.0f)
      return(0.0f);

   return fDist;
}

//==============================================================================
//==============================================================================
float BEntity::calcDistObstructionToPoint(BVector myTempPos, BVector vPoint) const
{
   // If I dont have an obstruction, use the radius..
   const BOPObstructionNode* obs = getObstructionNode();
   if (!obs)
   {
      obs = getChildObstructionNode();
      if(!obs)
         return calcDistRadiusToPoint(myTempPos, vPoint);
   }

   const BOPQuadHull *hull1 = obs->getHull();
   if (!hull1)
      return calcDistRadiusToPoint(myTempPos, vPoint);

   float fDist = hull1->distance(vPoint);

   // Don't be negative.
   if(fDist<0.0f)
      return(0.0f);

   return fDist;
}

//==============================================================================
//==============================================================================
float BEntity::calcDistRadiusToRadius(BVector myTempPos, const BEntity *pTarget) const
{
   BASSERT(pTarget);

   float fDist = 0.0f;   
   fDist = myTempPos.xzDistance(pTarget->getPosition());

   fDist -= (getObstructionRadius() + pTarget->getObstructionRadius());

   // Don't be negative.
   if(fDist<0.0f)
      return(0.0f);

   return fDist;
}

//==============================================================================
//==============================================================================
float BEntity::calcDistRadiusToObstruction(BVector myTempPos, const BEntity *pTarget) const
{
   const BOPObstructionNode *pNode = pTarget->getObstructionNode();
   if (!pNode)
   {
      pNode = pTarget->getChildObstructionNode();
      if(!pNode)
         return calcDistRadiusToRadius(myTempPos, pTarget);
   }

   float fDist = 0.0f;

   const BOPQuadHull *pHull = pNode->getHull();
   if(!pHull)
      calcDistRadiusToRadius(myTempPos, pTarget);
   
   fDist = pHull->distance(myTempPos);
   fDist -= getObstructionRadius();

   // Don't be negative.
   if(fDist<0.0f)
      return(0.0f);

   return fDist;
}
//==============================================================================
//==============================================================================
float BEntity::calcDistObstructionToRadius(BVector myTempPos, const BEntity *pTarget) const
{
   // If I don't have an obstruction, use the radius..
   const BOPObstructionNode *obs = getObstructionNode();
   if (!obs)
   {
      obs = getChildObstructionNode();
      if(!obs)
         return calcDistRadiusToRadius(myTempPos, pTarget);
   }

   const BOPQuadHull *hull1 = obs->getHull();
   if (!hull1)
      return calcDistRadiusToRadius(myTempPos, pTarget);

   float fDist = hull1->distance(pTarget->getPosition());

   fDist -= pTarget->getObstructionRadius();

   // Don't be negative.
   if(fDist<0.0f)
      return(0.0f);

   return fDist;
}

//==============================================================================
//==============================================================================
float BEntity::calcDistObstructionToObstruction(BVector myTempPos, const BEntity *pTarget) const
{
   // If I dont have an obstruction, use the radius..
   const BOPObstructionNode* obs = getObstructionNode();
   if (!obs)
   {
      obs = getChildObstructionNode();
      if(!obs)
         return calcDistRadiusToObstruction(myTempPos, pTarget);
   }

   const BOPQuadHull *hull1 = obs->getHull();
   if (!hull1)
      return calcDistRadiusToObstruction(myTempPos, pTarget);

   // If the target unit doesn't have an obstruction, we default to
   // calcDistRadiusToRadius..
   const BOPObstructionNode* obs2 = pTarget->getObstructionNode();
   if (!obs2)
   {
      obs2 = pTarget->getChildObstructionNode();
      if(!obs2)
         return calcDistObstructionToRadius(myTempPos, pTarget);
   }

   const BOPQuadHull* hull2 = obs2->getHull();
   //float fDist = hull1->distance((const BConvexHull &)hull2);
   float fDist = hull1->distance(hull2); 

   // Don't be negative.
   if(fDist<0.0f)
      return(0.0f); 

   return fDist;
}

//==============================================================================
//==============================================================================
float BEntity::calcDistRadiusToPointSqr(BVector myTempPos, BVector vPoint) const
{
   float fDist = calcDistRadiusToPoint(myTempPos, vPoint);
   return  fDist * fDist;
}

//==============================================================================
//==============================================================================
float BEntity::calcDistObstructionToPointSqr(BVector myTempPos, BVector vPoint) const
{
   // If I dont have an obstruction, use the radius..
   const BOPObstructionNode* obs = getObstructionNode();
   if (!obs)
   {
      obs = getChildObstructionNode();
      if(!obs)
         return calcDistRadiusToPointSqr(myTempPos, vPoint);
   }

   const BOPQuadHull *hull1 = obs->getHull();
   if (!hull1)
      return calcDistRadiusToPointSqr(myTempPos, vPoint);

   float fDist = hull1->distanceSqr(vPoint);

   // Don't be negative.
   if(fDist<0.0f)
      return(0.0f);

   return fDist;
}

//==============================================================================
// BEntity::deleteObstruction
//==============================================================================
void BEntity::deleteObstruction()
{
   if (mpObstructionNode)
   {
      gObsManager.deleteObstruction(mpObstructionNode);
      mpObstructionNode = NULL;
   }
}

//==============================================================================
//==============================================================================
void BEntity::createObstruction(bool playerOwnsObstruction)
{
   //-- this should be set in BObject::initProtoObject()
   BASSERT(mObstructionRadiusX != -1.0f);
   BASSERT(mObstructionRadiusY != -1.0f);
   BASSERT(mObstructionRadiusZ != -1.0f);

   //-- create an obstruction
   mpObstructionNode = gObsManager.getNewObstructionNode();
   BASSERT(mpObstructionNode);


   gObsManager.resetObstructionNode(mpObstructionNode);

  
   // Update Position
   if (getFlagRotateObstruction())
   {
      gObsManager.fillOutRotatedPosition(mpObstructionNode, mPosition.x, mPosition.z, mObstructionRadiusX, 
         mObstructionRadiusZ, mForward.x, mForward.z);
   }
   else
   {
      gObsManager.fillOutNonRotatedPosition(mpObstructionNode, mPosition.x, mPosition.z, mObstructionRadiusX, 
         mObstructionRadiusZ);
   }

   // Get our new obstruction type since this can change.
   long obsType = computeObstructionType();


   // Fill out additional data
   if (mID.getType() == cClassTypeDopple)
      mpObstructionNode->mType = BObstructionManager::cObsNodeTypeDoppleganger;
   else if (mID.getType() == cClassTypeSquad)
      mpObstructionNode->mType = BObstructionManager::cObsNodeTypeSquad;
   else
      mpObstructionNode->mType = BObstructionManager::cObsNodeTypeUnit;

   if(playerOwnsObstruction)
   {
      mpObstructionNode->mProperties |= BObstructionManager::cObsPropertyMultiPlayerOwned;
      mpObstructionNode->mPlayerID = (BYTE) getPlayerID();
   }

   mpObstructionNode->mObject = this;			// This overwrites the first half of mEntity!!!
   mpObstructionNode->mEntityID = mID.asLong();

   // Put it into the quadtree
   gObsManager.installObjectObstruction(mpObstructionNode, obsType);
}

//==============================================================================
//==============================================================================
void BEntity::updateObstruction()
{
   if (mpObstructionNode)
   {
      SCOPEDSAMPLE(BEntityUpdateObstruction);
    
      // Update Position
      if (getFlagRotateObstruction())
      {
         gObsManager.fillOutRotatedPosition(mpObstructionNode, mPosition.x, mPosition.z, mObstructionRadiusX, 
            mObstructionRadiusZ, mForward.x, mForward.z);
      }
      else
      {
         gObsManager.fillOutNonRotatedPosition(mpObstructionNode, mPosition.x, mPosition.z, mObstructionRadiusX, 
            mObstructionRadiusZ);
      }

      // Get our new obstruction type since this can change.
      // (virtual function)
      long obsType = computeObstructionType();

      // Make sure type is valid since obstruction manager uses it to index into an array
      if (obsType < 0)
         return;

      // Update ourselves in the ob mgr.
      // Note: This will take care of the quad tree updating.  This function also 
      // handles moving of the obstruction from one quad tree to another, so call
      // it even if you're just changing from collideable to non-collideable, say.
      gObsManager.updateObstructionLocation(mpObstructionNode, obsType);
   }   
   setFlagMoved(false);
}


//==============================================================================
//==============================================================================
bool BEntity::isCollisionEnabledWithEntity(BEntity* pEntity, bool collideWithMoving)
{
   // Bomb check.
   if (!pEntity)
      return (false);
   // Moving units are always non-collidable
   if (pEntity->isMoving() && !collideWithMoving)
      return (false);

   //==============================================================================
   // Otherwise, check hit-and-run mode hit object types.
   // The naming / logic is a bit confusing, but if a unit is "hittable" by a
   // hit-and-run weapon that means we want to path through it.  Pathing through
   // it requires it to not be collidable, so a "hittable" object won't return
   // collisions.
//-- FIXING PREFIX BUG ID 5639
   const BSquad* pPathingSquad = NULL;
//--
   BUnit* pPathingUnit = NULL;
   if (isClassType(BEntity::cClassTypeSquad))
   {
      pPathingSquad = getSquad();
      pPathingUnit = gWorld->getUnit(pPathingSquad->getLeader());
   }
   else if (isClassType(BEntity::cClassTypeUnit))
   {
      pPathingUnit = getUnit();
      pPathingSquad = pPathingUnit->getParentSquad();
   }
   else
   {
      return (false);
   }

   // Check to see if entity is bowlable or rammable, and thus should be ignored
   if (pPathingUnit && pPathingSquad && (pPathingSquad->getSquadMode() == BSquadAI::cModeHitAndRun))
   {
      // Only bowl/ram enemy units
//-- FIXING PREFIX BUG ID 5638
      const BUnitActionCollisionAttack* pBowlAction = reinterpret_cast<BUnitActionCollisionAttack*>(pPathingUnit->getActionByType(BAction::cActionTypeUnitCollisionAttack));
//--
      BASSERT(pBowlAction);
      if (pBowlAction && pBowlAction->isBowlableOrRammable(pEntity))
         return false;
   }

   //==============================================================================
   // Buildings that aren't being rammed are collidable
   if ((pEntity->getProtoObject() != NULL) && (pEntity->getProtoObject()->getObjectClass() == cObjectClassBuilding))
      return (true);

   //==============================================================================
   // Check air stuff
//-- FIXING PREFIX BUG ID 5640
   const BSquad* pObsSquad = NULL;
//--
   BUnit* pObsUnit = NULL;
   if (pEntity->isClassType(BEntity::cClassTypeSquad))
   {
      pObsSquad = pEntity->getSquad();
      pObsUnit = gWorld->getUnit(pObsSquad->getChild(0));
   }
   else if (pEntity->isClassType(BEntity::cClassTypeUnit))
   {
      pObsUnit = pEntity->getUnit();
      pObsSquad = pObsUnit->getParentSquad();
   }

   if ((pPathingUnit && pPathingSquad && (pPathingUnit->getProtoObject()->getMovementType() == cMovementTypeAir)) &&
      (pObsUnit && pObsSquad && (pObsUnit->getProtoObject()->getMovementType() != cMovementTypeAir)))
      return (false);

   if ((pPathingUnit && pPathingSquad && (pPathingUnit->getProtoObject()->getMovementType() != cMovementTypeAir)) &&
      (pObsUnit && pObsSquad && (pObsUnit->getProtoObject()->getMovementType() == cMovementTypeAir)))
      return (false);

   // Not ignored by anything above, so collision is enabled
   return (true);
}


//==============================================================================
// BEntity::getEntityRefByIndex
//==============================================================================
BEntityRef* BEntity::getEntityRefByIndex(uint index) const
{
   if ((index >= 0) && (index < getNumberEntityRefs()))
      return(&(*mpEntityRefs)[index]);
   else
      return(NULL);
}

//==============================================================================
// BEntity::getFirstEntityRefByType
//==============================================================================
BEntityRef* BEntity::getFirstEntityRefByType(long type) const
{
   long numEntityRefs = getNumberEntityRefs();
   for (long i=0; i<numEntityRefs; i++)
   {
      BEntityRef *pEntityRef = getEntityRefByIndex(i);
      if (pEntityRef && pEntityRef->mType == type)
         return(pEntityRef);
   }
   return(NULL);
}

//==============================================================================
// BEntity::getFirstEntityRefByID
//==============================================================================
BEntityRef* BEntity::getFirstEntityRefByID(long type, BEntityID id) const
{
   long numEntityRefs = getNumberEntityRefs();
   for (long i=0; i<numEntityRefs; i++)
   {
      BEntityRef *pEntityRef = getEntityRefByIndex(i);
      if (pEntityRef && pEntityRef->mType == type && pEntityRef->mID == id)
         return(pEntityRef);
   }
   return(NULL);
}

//==============================================================================
// BEntity::getFirstEntityRefByData1
//==============================================================================
BEntityRef* BEntity::getFirstEntityRefByData1(long type, short data1) const
{
   long numEntityRefs = getNumberEntityRefs();
   for (long i=0; i<numEntityRefs; i++)
   {
      BEntityRef *pEntityRef = getEntityRefByIndex(i);
      if (pEntityRef && pEntityRef->mType == type && pEntityRef->mData1 == data1)
         return(pEntityRef);
   }
   return(NULL);
}

//==============================================================================
// BEntity::getFirstEntityRefByData2
//==============================================================================
BEntityRef* BEntity::getFirstEntityRefByData2(long type, BYTE data2) const
{
   long numEntityRefs = getNumberEntityRefs();
   for (long i=0; i<numEntityRefs; i++)
   {
      BEntityRef *pEntityRef = getEntityRefByIndex(i);
      if (pEntityRef && pEntityRef->mType == type && pEntityRef->mData2 == data2)
         return(pEntityRef);
   }
   return(NULL);
}

//==============================================================================
// BEntity::addEntityRef
//==============================================================================
BEntityRef* BEntity::addEntityRef(short type, BEntityID id, short data1, BYTE data2)
{
   if (!mpEntityRefs || getFlagEntityRefsLocked())
   {
      mpEntityRefs = new BEntityRefArray;
      if (!mpEntityRefs)
         return NULL;
   }
   BEntityRef ref(type, id, data1, data2);
   uint index = mpEntityRefs->add(ref);
#ifndef BUILD_FINAL
   if (type != BEntityRef::cTypeContainUnit && index > 100)
   {
      // ajl 11/12/08 - Attempt to figure out where we are adding lots of entity refs
      BSimString msg;
      msg.format("Too many entity refs: Count %u, Ref type %d, Entity %d (%s), data1 %d, data2 %d", index+1, (int)type, id.asLong(), (mEntityName ? mEntityName->getPtr() : ""), (int)data1, (int)data2);
      BASSERTM(0, msg.getPtr());
   }
#endif
   return getEntityRefByIndex(index);
}

//==============================================================================
// BEntity::removeEntityRef
//==============================================================================
void BEntity::removeEntityRef(long type, BEntityID id)
{
   if (!mpEntityRefs || getFlagEntityRefsLocked())
      return;
   uint count = (uint)mpEntityRefs->getNumber();
   for (uint i = 0; i < count; i++)
   {
      BEntityRef ref = (*mpEntityRefs)[i];
      if ((ref.mID == id) && (ref.mType == type))
      {
         // ajl 12/4/07 - change to keep list ordered
         mpEntityRefs->removeIndex(i);
         /*
         uint endIndex = count - 1;
         if (i != endIndex)
            (*mpEntityRefs)[i] = (*mpEntityRefs)[endIndex];
         mpEntityRefs->resize(endIndex);
         */
         return;
      }
   }
}

//==============================================================================
// BEntity::removeEntityRefByData1
//==============================================================================
void BEntity::removeEntityRefByData1(long type, short data1)
{
   if (!mpEntityRefs || getFlagEntityRefsLocked())
      return;
   uint count = (uint)mpEntityRefs->getNumber();
   for (uint i = 0; i < count; i++)
   {
      BEntityRef ref = (*mpEntityRefs)[i];
      if ((ref.mType == type) && (ref.mData1 == data1))
      {
         // ajl 12/4/07 - change to keep list ordered
         mpEntityRefs->removeIndex(i);
         /*
         uint endIndex = count - 1;
         if (i != endIndex)
            (*mpEntityRefs)[i] = (*mpEntityRefs)[endIndex];
         mpEntityRefs->resize(endIndex);
         */
         return;
      }
   }
}

//==============================================================================
// BEntity::removeEntityRefByData2
//==============================================================================
void BEntity::removeEntityRefByData2(long type, BYTE data2)
{
   if (!mpEntityRefs || getFlagEntityRefsLocked())
      return;
   uint count = (uint)mpEntityRefs->getNumber();
   for (uint i = 0; i < count; i++)
   {
      BEntityRef ref = (*mpEntityRefs)[i];
      if ((ref.mType == type) && (ref.mData2 == data2))
      {
         // ajl 12/4/07 - change to keep list ordered
         mpEntityRefs->removeIndex(i);
         /*
         uint endIndex = count - 1;
         if (i != endIndex)
            (*mpEntityRefs)[i] = (*mpEntityRefs)[endIndex];
         mpEntityRefs->resize(endIndex);
         */
         return;
      }
   }
}

//==============================================================================
// BEntity::removeEntityRefByType
//==============================================================================
void BEntity::removeEntityRefByType(long type)
{
   if (!mpEntityRefs || getFlagEntityRefsLocked())
      return;
   uint count = (uint)mpEntityRefs->getNumber();
   for (uint i = 0; i < count; i++)
   {
      BEntityRef ref = (*mpEntityRefs)[i];
      if (ref.mType == type)
      {
         mpEntityRefs->removeIndex(i);
         return;
      }
   }
}

//==============================================================================
//==============================================================================
void BEntity::removeAllMatchingEntityRefs(long type, BEntityID id, short data1, BYTE data2)
{
   if (!mpEntityRefs || getFlagEntityRefsLocked())
      return;

   uint i = 0;
   while (i < mpEntityRefs->getSize())
   {
//-- FIXING PREFIX BUG ID 5641
      const BEntityRef& ref = mpEntityRefs->get(i);
//--
      if (ref.mType == type && ref.mID == id && ref.mData1 == data1 && ref.mData2 == data2)
         mpEntityRefs->removeIndex(i);
      else
         i++;
   }
}

//==============================================================================
// BEntity::removeEntityRef
//==============================================================================
void BEntity::removeEntityRef(long index)
{
   if(!mpEntityRefs || getFlagEntityRefsLocked())
      return;
   long count=mpEntityRefs->getNumber();
   if(index<0 || index>=count)
      return;
   // ajl 12/4/07 - change to keep list ordered
   mpEntityRefs->removeIndex(index);
   /*
   long endIndex=count-1;
   if(index!=endIndex)
      (*mpEntityRefs)[index]=(*mpEntityRefs)[endIndex];
   mpEntityRefs->resize(endIndex);
   */
}


//==============================================================================
// BEntity::getTeamID
//==============================================================================
BTeamID BEntity::getTeamID() const
{
   //FIXME AJL 6/9/06 - Try adding team ID to mID (like player ID) so that we don't have to deref the player
   return const_cast<BEntity*>(this)->getPlayer()->getTeamID();
}

//==============================================================================
// BEntity::getTeam
//==============================================================================
BTeam* BEntity::getTeam()
{
   return getPlayer()->getTeam();
}

//==============================================================================
// BEntity::getNumGatherUnits
//==============================================================================
long BEntity::getNumGatherUnits(void) const
{
   long numGatherUnits = 0;
   long numEntityRefs = mpEntityRefs->getNumber();
   for (long i = 0; i < numEntityRefs; i++)
   {
      BEntityRef *pEntityRef = getEntityRefByIndex(i);
      if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeGatherUnit)
         numGatherUnits++;
   }

   return numGatherUnits;
}

//==============================================================================
// BEntity::getObstructionHull
//==============================================================================
void BEntity::getObstructionHull(BOPQuadHull &obstructionHull) const
{
   //If we have an obstruction hull, just use that.
   const BOPObstructionNode* pObsNode=getObstructionNode();
   if (!pObsNode)
      pObsNode=getChildObstructionNode();
   if (pObsNode && pObsNode->getHull())
   {
      obstructionHull.expandFrom(pObsNode->getHull(), 0.0f);
      return;
   }

   //Old "wrong" code (with fixed winding).
   float x=mPosition.x;
   float z=mPosition.z;
   float obsRadius = getObstructionRadius();
   obstructionHull.mX1 = x - obsRadius;
   obstructionHull.mZ1 = z - obsRadius;
   obstructionHull.mX2 = x - obsRadius;
   obstructionHull.mZ2 = z + obsRadius;
   obstructionHull.mX3 = x + obsRadius;
   obstructionHull.mZ3 = z + obsRadius;
   obstructionHull.mX4 = x + obsRadius;
   obstructionHull.mZ4 = z - obsRadius;
   obstructionHull.mRadius = obsRadius;
   obstructionHull.mNextNode = NULL;
   obstructionHull.mRotation = 0;
   obstructionHull.mBoundingIndicies = 0;
   obstructionHull.mIdxMinX = 0;
   obstructionHull.mIdxMinZ = 1;
   obstructionHull.mIdxMaxX = 6;
   obstructionHull.mIdxMaxZ = 7;
   obstructionHull.mSequence = 0;
   obstructionHull.mUnused1 = 0;
}

//==============================================================================
//==============================================================================
bool BEntity::isPassable(BVector position, const BEntityIDArray& ignoreList) const
{
   //SCOPEDSAMPLE(BEntity_isPassable)
   //Make the ignore list.
   BEntityIDArray actualIgnoreList;
   for (uint i=0; i < ignoreList.getSize(); i++)
   {
      BEntity* pEntity=gWorld->getEntity(ignoreList[i]);
      if (pEntity->getClassType() == BEntity::cClassTypeSquad)
      {
//-- FIXING PREFIX BUG ID 5644
         const BSquad* pSquad=reinterpret_cast<BSquad*>(pEntity);
//--
         for (uint j=0; j < pSquad->getNumberChildren(); j++)
            actualIgnoreList.add(pSquad->getChild(j));
      }
      else
         actualIgnoreList.add(ignoreList[i]);
   }

   //Collision options.  Check for any terrain passability or unit (moving or not).
   long lObOptions = BObstructionManager::cIsNewTypeAllCollidableUnits;

   if (getProtoObject() != NULL)
   {
      if (getProtoObject()->getMovementType() == cMovementTypeLand)
         lObOptions |= BObstructionManager::cIsNewTypeBlockLandUnits;
      else if (getProtoObject()->getMovementType() == cMovementTypeFlood)
         lObOptions |= BObstructionManager::cIsNewTypeBlockFloodUnits;
      else if (getProtoObject()->getMovementType() == cMovementTypeScarab)
         lObOptions |= BObstructionManager::cIsNewTypeBlockScarabUnits;
   }

   if (getFlagFlying())
      lObOptions = 0;
   long lObNodeType = BObstructionManager::cObsNodeTypeAll;
 
   //Clear the obstructions list.
   BObstructionNodePtrArray collisionObs;
   collisionObs.setNumber(0);
   // Do the ObMgr check.
   if (gObsManager.begin(BObstructionManager::cBeginEntity, mID.asLong(), getClassType(), lObOptions, 
      lObNodeType, 0, cDefaultRadiusSofteningFactor, &actualIgnoreList, canJump()))
   {
      gObsManager.findObstructions(position, true, true, collisionObs);
      gObsManager.end();
   }
   
   //if (gObsManager.testObstructions(position, getObstructionRadius(), 0.0f, lObOptions, lObNodeType, mPlayerID))
   if (collisionObs.getSize() > 0)
      return (false);
   return (true);
}

//==============================================================================
// BEntity::validateFacing
//==============================================================================
bool BEntity::validateFacing(BVector targetPosition, float maxAngle)
{
   BVector targetDir=targetPosition-mPosition;
   if(targetDir!=cOriginVector)
   {
      targetDir.normalize();
      float targetAngle=targetDir.getAngleAroundY();

      float forwardAngle=mForward.getAngleAroundY();

      float angleDiff=Math::fAbs(forwardAngle-targetAngle);
      if(angleDiff>cPi)
         angleDiff=Math::fAbs(angleDiff-cTwoPi);

      if(angleDiff>maxAngle)
         return false;
   }

   return true;
}

//==============================================================================
// BEntity::getChildObstructionNode
//==============================================================================
const BOPObstructionNode* BEntity::getChildObstructionNode(void) const
{
   //-- For buildings will return the obstruction node of the squad's building
   //DJBFIXME: How terrible is this for perf? Is there a more elegant solution?
   const BSquad *pSquad = NULL;
   if (getClassType() == BEntity::cClassTypeSquad)      
       pSquad = reinterpret_cast<const BSquad*>(this);
   if(pSquad)
   {
      if(pSquad->getNumberChildren() == 1)
      {
         BEntityID id = pSquad->getChild(0);
         const BEntity *pChild = gWorld->getEntity(id);
         if(pChild)
            return(pChild->getObstructionNode());
      }
   }
  
   return NULL;
}

//==============================================================================
// BEntity::isSameUnitOrSquad
//==============================================================================
bool BEntity::isSameUnitOrSquad(const BEntity* pEntity)
{
   BASSERT(pEntity);

   //-- Simple check, are we the same entity?
   if(mID == pEntity->getID())
      return true;

   if(this->isClassType(BEntity::cClassTypeUnit)) //-- We're a unit and 
   {
      if(pEntity->isClassType(BEntity::cClassTypeSquad))
      {
         if(this->getUnit()->getParentID() == pEntity->getID())
            return true;
      }
   }

   if(this->isClassType(BEntity::cClassTypeSquad))
   {
      if(pEntity->isClassType(BEntity::cClassTypeUnit))
      {
         if(mID == pEntity->getUnit()->getParentID())
            return true;
      }
   }

   return false;
}

//==============================================================================
// 
//==============================================================================
void BEntity::queryEntityID(BEntityID killedByID, BPlayerID& killerPlayerID, long& killerProtoID, long& killerProtoSquadID)
{
   if(killerPlayerID != cInvalidPlayerID && killerProtoID != cInvalidProtoID && killerProtoSquadID != cInvalidProtoSquadID)
      return;

   // determine the killer of this unit
   killerPlayerID = cInvalidPlayerID;
   killerProtoID = cInvalidProtoID;
   killerProtoSquadID = cInvalidProtoID;

   if (killedByID != cInvalidObjectID)
   {
      BEntity* pKillerEntity = gWorld->getEntity(killedByID.asLong());
      if (pKillerEntity)
      {
         switch (pKillerEntity->getClassType())
         {
            case BEntity::cClassTypeObject:
            case BEntity::cClassTypeUnit:
               {
                  BObject* pObject = pKillerEntity->getObject();
                  if (pObject)
                  {
                     killerPlayerID = pObject->getPlayerID();
                     killerProtoID = pObject->getProtoID();
                     BUnit* pKillerUnit = pKillerEntity->getUnit();
                     if (pKillerUnit)
                     {
                        BSquad* pKillerSquad = pKillerUnit->getParentSquad();
                        if (pKillerSquad)
                           killerProtoSquadID = pKillerSquad->getProtoSquadID();
                     }
                  }
               }
               break;
            case BEntity::cClassTypeSquad:
               {
                  BSquad* pSquad = pKillerEntity->getSquad();
                  if (pSquad)
                  {
                     killerPlayerID = pSquad->getPlayerID();
                     killerProtoID = pSquad->getProtoObjectID();
                     killerProtoSquadID = pSquad->getProtoSquadID();
                  }
               }
               break;
            case BEntity::cClassTypeProjectile:
               {
                  // Unparented leader power projectiles use built-in protoIDs.
                  BProjectile* pProj = pKillerEntity->getProjectile();
                  if (pProj && pProj->getFlagFromLeaderPower())
                  {
                     killerPlayerID = pProj->getPlayerID();
                     killerProtoID = gDatabase.getPOIDLeaderPowerCaster();
                     killerProtoSquadID = gDatabase.getPSIDLeaderPowerCaster();
                  }
               }
               break;
            }
      }
   }
}

//==============================================================================
// BEntity::canJump
// Test whether this is entity should be allowed access to the special jump
// paths.  If it is a platoon, all children must be warthog units.
//==============================================================================
bool BEntity::canJump() const 
{
   bool allCanJump = false;

   if (getClassType() == BEntity::cClassTypePlatoon)
   {
      const BPlatoon *pPlatoon = reinterpret_cast<const BPlatoon*>(this);
      for (uint i=0; i < pPlatoon->getNumberChildren(); i++ )
      {
         BEntityID eid = pPlatoon->getChild(i);
         const BEntity* pChildEntity = gWorld->getEntity(eid);

         const BProtoObject* pProto = pChildEntity ? pChildEntity->getProtoObject() : NULL;

//-- FIXING PREFIX BUG ID 5646
         const BPhysicsInfo* pPhysInfo = pProto ? gPhysicsInfoManager.get(pProto->getPhysicsInfoID(), true) : NULL;
//--

         if(pPhysInfo && (pPhysInfo->getVehicleType() == BPhysicsInfo::cWarthog))
            allCanJump = true;  //need at least one to possibly be true
         else
         {
            allCanJump = false; //if any can't jump (or don't exist), the whole platoon can't
            break;
         }
      }
   }
   else
   {
      const BProtoObject* pProto = pProto = getProtoObject();
      if(pProto)
      {
//-- FIXING PREFIX BUG ID 5647
         const BPhysicsInfo* pPhysInfo = gPhysicsInfoManager.get(pProto->getPhysicsInfoID(), true);
//--
         if(pPhysInfo)
         {
            if(pPhysInfo->getVehicleType() == BPhysicsInfo::cWarthog)
               allCanJump = true;
         }
      }
   }

   return allCanJump;
}

#ifndef BUILD_FINAL
static const uint cMaxTextBufSize=2048;
//==============================================================================
//==============================================================================
void BEntity::debug(const char* pMsg, ...)
{
   char buf[cMaxTextBufSize];
   va_list arglist;
   va_start( arglist, pMsg );
   StringCchVPrintf(buf, sizeof(buf), pMsg, arglist);
   va_end(arglist);
   
   gConsole.output(cChannelSim, "%s: %s", mID.getDebugString().getPtr(), buf);
}
#endif

//==============================================================================
//==============================================================================
void BEntity::clearFlags()
{
   //-- flags   
   mFlagCollidable=false;
   mFlagMoving=false;
   mFlagDestroy=false;
   mFlagFirstUpdate=true;
   mFlagTiesToGround=true;
   mFlagUseMaxHeight=false;
   mFlagPhysicsControl=false;
   mFlagRotateObstruction=false;
   mFlagFlying=false;
   mFlagValid=true;
   mFlagNonMobile=false;
   mFlagLockedDown=false;
   mFlagEntityRefsLocked=false;
   mFlagFlyingHeightFixup=false;
   mFlagGarrisoned=false;
   mFlagPassiveGarrisoned=false;
   mFlagAttached=false;
   mFlagMoved=true;
   mFlagTeleported=false;
   mFlagInSniper=false;
   mFlagIsBuilt=true;
   mFlagHasSounds=false;
   mFlagHitched=false;
   mFlagSprinting=false;
   mFlagRecovering=false;
   mFlagHitched = false;
   mFlagInCover = false;
   mFlagSelectable = true;
   mFlagUngarrisonValid = true;
   mFlagGarrisonValid = true;
   mFlagIsPhysicsReplacement = false;
   mFlagIsDoneBuilding = false;

#ifndef BUILD_FINAL
   mFlagIsTriggered = false;
#endif
}

//==============================================================================
//==============================================================================
bool BEntity::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVECTOR(pStream, mUp);
   GFWRITEVECTOR(pStream, mForward);
   GFWRITEVECTOR(pStream, mVelocity);
   GFWRITECLASS(pStream, saveType, mActions);
   long count=mActions.getNumberActions();
   for (long i=0; i<count; i++)
   {
      const BAction* pAction = mActions.getAction((uint)i);
      BASSERT(pAction);
      BASSERT((const_cast<BAction*>(pAction))->getOwner()==this);
   }

   bool haveRefs = (mpEntityRefs!=NULL);
   GFWRITEVAR(pStream, bool, haveRefs);
   if (haveRefs)
      GFWRITEARRAY(pStream, BEntityRef, *mpEntityRefs, uint16, 1000);

   //BEntityID mID;
   GFWRITEVAR(pStream, BEntityID, mID);

   GFWRITEVAR(pStream, BPlayerID, mPlayerID);

   //BProtoObject* mpProtoObject;
   //BOPObstructionNode* mpObstructionNode;

   GFWRITEVAR(pStream, BEntityID, mParentID);

   //BPhysicsObject* mpPhysicsObject;

   GFWRITEVAR(pStream, float, mYDisplacement);
   GFWRITEVAR(pStream, float, mObstructionRadiusX);
   GFWRITEVAR(pStream, float, mObstructionRadiusY);
   GFWRITEVAR(pStream, float, mObstructionRadiusZ);

   //BSimString* mEntityName;

   GFWRITEBITBOOL(pStream, mFlagCollidable);
   GFWRITEBITBOOL(pStream, mFlagMoving);
   GFWRITEBITBOOL(pStream, mFlagDestroy);
   GFWRITEBITBOOL(pStream, mFlagFirstUpdate);
   GFWRITEBITBOOL(pStream, mFlagTiesToGround);
   GFWRITEBITBOOL(pStream, mFlagUseMaxHeight);
   GFWRITEBITBOOL(pStream, mFlagPhysicsControl);
   GFWRITEBITBOOL(pStream, mFlagRotateObstruction);
   GFWRITEBITBOOL(pStream, mFlagFlying);
   GFWRITEBITBOOL(pStream, mFlagValid);
   GFWRITEBITBOOL(pStream, mFlagNonMobile);
   GFWRITEBITBOOL(pStream, mFlagLockedDown);
   GFWRITEBITBOOL(pStream, mFlagEntityRefsLocked);
   GFWRITEBITBOOL(pStream, mFlagFlyingHeightFixup);
   GFWRITEBITBOOL(pStream, mFlagGarrisoned);
   GFWRITEBITBOOL(pStream, mFlagPassiveGarrisoned);
   GFWRITEBITBOOL(pStream, mFlagAttached);
   GFWRITEBITBOOL(pStream, mFlagMoved);
   GFWRITEBITBOOL(pStream, mFlagTeleported);
   GFWRITEBITBOOL(pStream, mFlagInSniper);
   GFWRITEBITBOOL(pStream, mFlagIsBuilt);
   GFWRITEBITBOOL(pStream, mFlagHasSounds);
   GFWRITEBITBOOL(pStream, mFlagHitched);
   GFWRITEBITBOOL(pStream, mFlagSprinting);
   GFWRITEBITBOOL(pStream, mFlagRecovering);
   GFWRITEBITBOOL(pStream, mFlagInCover);
   GFWRITEBITBOOL(pStream, mFlagSelectable);
   GFWRITEBITBOOL(pStream, mFlagUngarrisonValid);
   GFWRITEBITBOOL(pStream, mFlagGarrisonValid);
   GFWRITEBITBOOL(pStream, mFlagIsPhysicsReplacement);
   GFWRITEBITBOOL(pStream, mFlagIsDoneBuilding);

   GFWRITEMARKER(pStream, cSaveMarkerEntity1);
   return true;
}

//==============================================================================
//==============================================================================
bool BEntity::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mPosition);
   GFREADVECTOR(pStream, mUp);
   GFREADVECTOR(pStream, mForward);
   calcRight();
   GFREADVECTOR(pStream, mVelocity);
   GFREADCLASS(pStream, saveType, mActions);

   bool haveRefs;
   GFREADVAR(pStream, bool, haveRefs);
   if (haveRefs)
   {
      if (!mpEntityRefs)
      {
         mpEntityRefs = new BEntityRefArray;
         if (!mpEntityRefs)
            return false;
      }
      if (mGameFileVersion < 2)
         GFREADARRAY(pStream, BEntityRef, *mpEntityRefs, uint8, 250)
      else
         GFREADARRAY(pStream, BEntityRef, *mpEntityRefs, uint16, 1000)
   }

   //BEntityID mID;
   BEntityID entityID;
   GFREADVAR(pStream, BEntityID, entityID);
   BASSERT(mID==entityID);

   GFREADVAR(pStream, BPlayerID, mPlayerID);

   //BProtoObject* mpProtoObject;
   //BOPObstructionNode* mpObstructionNode;

   GFREADVAR(pStream, BEntityID, mParentID);

   //BPhysicsObject* mpPhysicsObject;

   GFREADVAR(pStream, float, mYDisplacement);
   GFREADVAR(pStream, float, mObstructionRadiusX);
   GFREADVAR(pStream, float, mObstructionRadiusY);
   GFREADVAR(pStream, float, mObstructionRadiusZ);

   //BSimString* mEntityName;

   GFREADBITBOOL(pStream, mFlagCollidable);
   GFREADBITBOOL(pStream, mFlagMoving);
   GFREADBITBOOL(pStream, mFlagDestroy);
   GFREADBITBOOL(pStream, mFlagFirstUpdate);
   GFREADBITBOOL(pStream, mFlagTiesToGround);
   GFREADBITBOOL(pStream, mFlagUseMaxHeight);
   GFREADBITBOOL(pStream, mFlagPhysicsControl);
   GFREADBITBOOL(pStream, mFlagRotateObstruction);
   GFREADBITBOOL(pStream, mFlagFlying);
   GFREADBITBOOL(pStream, mFlagValid);
   GFREADBITBOOL(pStream, mFlagNonMobile);
   GFREADBITBOOL(pStream, mFlagLockedDown);
   GFREADBITBOOL(pStream, mFlagEntityRefsLocked);
   GFREADBITBOOL(pStream, mFlagFlyingHeightFixup);
   GFREADBITBOOL(pStream, mFlagGarrisoned);
   GFREADBITBOOL(pStream, mFlagPassiveGarrisoned);
   GFREADBITBOOL(pStream, mFlagAttached);
   GFREADBITBOOL(pStream, mFlagMoved);
   GFREADBITBOOL(pStream, mFlagTeleported);
   GFREADBITBOOL(pStream, mFlagInSniper);
   GFREADBITBOOL(pStream, mFlagIsBuilt);
   GFREADBITBOOL(pStream, mFlagHasSounds);
   GFREADBITBOOL(pStream, mFlagHitched);
   GFREADBITBOOL(pStream, mFlagSprinting);
   GFREADBITBOOL(pStream, mFlagRecovering);
   GFREADBITBOOL(pStream, mFlagInCover);
   GFREADBITBOOL(pStream, mFlagSelectable);
   GFREADBITBOOL(pStream, mFlagUngarrisonValid);
   GFREADBITBOOL(pStream, mFlagGarrisonValid);
   GFREADBITBOOL(pStream, mFlagIsPhysicsReplacement);
   GFREADBITBOOL(pStream, mFlagIsDoneBuilding);

   GFREADMARKER(pStream, cSaveMarkerEntity1);
   return true;
}

//==============================================================================
//==============================================================================
bool BEntity::postLoad(int saveType)
{
   if (!mActions.postLoad(saveType))
      return false;
   return true;
}

//==============================================================================
//==============================================================================
bool BEntity::savePtr(BStream* pStream) const
{
   GFWRITEVAR(pStream, BEntityID, mID);
   return true;
}

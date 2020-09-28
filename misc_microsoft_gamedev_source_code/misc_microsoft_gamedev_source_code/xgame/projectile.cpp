//==============================================================================
// projectile.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "projectile.h"
#include "syncmacros.h"
#include "protoobject.h"
#include "config.h"
#include "configsgame.h"
#include "visual.h"
#include "world.h"
#include "worldsoundmanager.h"
#include "movementhelper.h"
#include "unit.h"
#include "tactic.h"
#include "damagehelper.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "minimap.h"
#include "visiblemap.h"
#include "physics.h"
#include "physicsInfoManager.h"
#include "physicsInfo.h"
#include "math\collision.h"
#include "camera.h"
#include "commands.h"
#include "commandmanager.h"
#include "game.h"
#include "uimanager.h"
#include "protoimpacteffect.h"
#include "grannyinstance.h"
#include "terraineffect.h"
#include "terraineffectmanager.h"
#include "power.h"
#include "powermanager.h"
#include "unitactionchargedrangedattack.h"
#include "unitactionpointblankattack.h"
#include "simhelper.h"
#include "achievementmanager.h"

//#define DEBUG_PROJECTILE_PATH

GFIMPLEMENTVERSION(BProjectile, 4);

static const float scTerrainCollisionOffset = 0.25f; 
static const float scVelocityScaleFactor = 10.0f;
static const float scTerrainSegCheckEarlyOutMaxDistance = 8.0f;
static const float scTerrainSegCheckBeamIncrementDistance = scTerrainSegCheckEarlyOutMaxDistance * 0.9f;
static const float scTerrainSegCheckEarlyOutMaxDistanceSqr = scTerrainSegCheckEarlyOutMaxDistance * scTerrainSegCheckEarlyOutMaxDistance;
static const float scTerrainSegCheckEarlyOutMinHeight = 0.25f;

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BPerturbanceData, 4, &gSimHeap);
IMPLEMENT_FREELIST(BStickData, 4, &gSimHeap);


//==============================================================================
//==============================================================================
BProjectile::~BProjectile()
{
   mActions.clearActions();
   if (mpPerturbanceData)
   {
      BPerturbanceData::releaseInstance(mpPerturbanceData);
      mpPerturbanceData = NULL;
   }
   if (mpStickData)
   {
      BStickData::releaseInstance(mpStickData);
      mpStickData = NULL;
   }
}

//==============================================================================
// BProjectile::update
//==============================================================================
bool BProjectile::update( float elapsedTime ) 
{
   //SCOPEDSAMPLE(BProjectile_update);  
   #ifdef SYNC_Projectile
      syncProjectileData("BProjectile::update mID", mID.asLong());
      syncProjectileData("BProjectile::update name", getProtoObject()->getName());
      syncProjectileData("BProjectile::update mPosition", mPosition);
   #endif

      
   // if we are marked for destruction let object handle destruction and return
   if (getFlagDestroy())
   {
      #ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigDebugProjectiles))
            gpDebugPrimitives->addDebugSphere(mPosition, 0.05f, cDWORDYellow, BDebugPrimitives::cCategoryNone, 1.0f);
      #endif
      return BObject::update(elapsedTime);
   }

   if (getFlagBeam())
   {
      return updateBeam(elapsedTime);
   }


   bool explode = false;
   bool finished = true;
   BEntityID unitHitID = cInvalidObjectID;

   switch(mMotionState)
   {
      case cStateFlying:
         {
            //SCOPEDSAMPLE(BProjectile_update_cStateFlying);  
            BVector oldVelocity = mVelocity;
            if (getFlagFirstUpdate())
            {
               BDEBUG_ASSERTM(!XMVector3IsNaN(mPosition), "BProjectile::update - initial position is invalid.");
               BDEBUG_ASSERTM(!XMVector3IsNaN(mVelocity), "BProjectile::update - initial velocity is invalid.");

               // SLB: No more revealers
               // Revealer
               //if (getParentID().isValid() && mTargetObjectID.isValid())
               //{
               //   BObject *pAttacker = gWorld->getObject(getParentID());
               //   BObject *pTarget = gWorld->getObject(mTargetObjectID);
               //   // mrh 7/17/06 - NOTE: Put this all within a check for attacker because god powers can shoot projectiles and damage units w/o an attacker, which asploded this.
               //   if (pAttacker && pTarget)
               //   {
               //      BUnit *pAttackerUnit = pAttacker->getUnit();
               //      BDEBUG_ASSERT(pAttackerUnit);
               //      if (pAttackerUnit->isGarrisoned())
               //      {
               //         if (!pAttackerUnit->isPositionVisible(pTarget->getTeamID()))
               //            gWorld->createRevealer(pTarget->getTeamID(), pAttackerUnit->getPosition(), gDatabase.getAttackRevealerLOS(), gDatabase.getAttackRevealerLifespan());
               //      }
               //      else if (!pAttackerUnit->isVisible(pTarget->getTeamID()))
               //         gWorld->createRevealer(pTarget->getTeamID(), pAttackerUnit->getPosition(), gDatabase.getAttackRevealerLOS(), gDatabase.getAttackRevealerLifespan());
               //   }
               //}

               // Set starting velocity
               if (mAcceleration <= cFloatCompareEpsilon)
               {
                  if (!getFlagIsAffectedByGravity())
                     mVelocity.scale(getDesiredVelocity());
               }
               else
                  mVelocity.scale(mStartingVelocity);

               mInitialProjectilePosition = mPosition;
               mInitialTargetPosition = getTargetLocation();
               mFollowGroundHeight = 5.0f;
               float simGridHeight;
               if (gTerrainSimRep.getHeightRaycast(mPosition, simGridHeight, true))
                  mFollowGroundHeight = Math::Clamp(mPosition.y - simGridHeight, 0.0f, 5.0f);
               beginMove();

               //-- update the visual on the first frame and bail so that projectiles update the visuals correctly with their first/initial position. 
               //-- if we don't do this visuals will have incorrect starting points.
               if (!BObject::update(elapsedTime))
                  return false;

               return true;
            }

            // Handle tracking delay
            if (getFlagTrackingDelay())
            {
               bool startTracking = false;
               if(gWorld->getGametime() >= mTrackingStartTime)
               {
                  // timer expired, so we're good to go
                  startTracking = true;
               }
               else
               {
                  // if we're "close" to the unit after factoring in our velocity, start tracking early
                  static const float cMinTrackingDuration = 0.4f; // we have at least this much time (in seconds) granted for tracking
                  const BEntity* pTargetEntity = gWorld->getEntity(mTargetObjectID);
                  if(pTargetEntity && pTargetEntity->getPosition().distance(getPosition()) < cMinTrackingDuration * getVelocity().length())
                     startTracking = true;
               }

               if(startTracking)
               {
                  setFlagTrackingDelay(false);
                  setFlagTracking(true);
               }
            }


            // If the unit is dead - turn off tracking   
            if (mTargetObjectID != cInvalidObjectID)
            {
//-- FIXING PREFIX BUG ID 1619
               const BUnit* pTargetUnit = gWorld->getUnit(mTargetObjectID);
//--
               BProjectile* pTargetProj = gWorld->getProjectile(mTargetObjectID); // We can now track flares, which are projectiles
               if (!pTargetProj && (!pTargetUnit || !pTargetUnit->isAlive()))
                  setFlagTracking(false);
            }

            // Handle acceleration
            if (mAcceleration > cFloatCompareEpsilon)
            {
               BASSERT(mFuel > cFloatCompareEpsilon);
               float currentVelocity = mVelocity.length();
               float desiredVelocity = getDesiredVelocity();
               if (currentVelocity < desiredVelocity)
               {
                  float newVelocity = Math::Min(currentVelocity + Math::Min(elapsedTime, mFuel) * mAcceleration, desiredVelocity);
                  mVelocity = XMVectorScale(XMVector3Normalize(oldVelocity), newVelocity);
               }
               else
                  mAcceleration = 0.0f;
            }

//-- FIXING PREFIX BUG ID 1626
            const BEntity* pTargetEntity = NULL;
//--

            // Handle flares
            if (mTargetObjectID != cInvalidObjectID)
            {
               BUnit* pTargetUnit = gWorld->getUnit(mTargetObjectID);
               if (pTargetUnit)
               {
                  BUnitActionPointBlankAttack* pAction = static_cast<BUnitActionPointBlankAttack*>(pTargetUnit->getActionByType(BAction::cActionTypeUnitPointBlankAttack));

                  if (pAction && pAction->getNumFlares() > 0)
                  {
                     BEntityID newTarget = pAction->getFlare();
                     if (newTarget != cInvalidObjectID)
                     {
                        mTargetObjectID = newTarget;
                     }
                  }
               }
            }
            
            // Handle Active Scanning
            if ((mTargetObjectID != cInvalidObjectID) && (mActiveScanChance > cFloatCompareEpsilon) && (getRandDistribution(cSimRand) <= mActiveScanChance))
            {      
               //SCOPEDSAMPLE(BProjectile_cStateFlying_activeScanning);  
               pTargetEntity = gWorld->getEntity(mTargetObjectID);
               if (pTargetEntity)
               {
                  BUnit* pOwnerUnit = gWorld->getUnit(getParentID());
                  if (pOwnerUnit)
                  {
                     BEntityIDArray* pMultiTargets = pOwnerUnit->getMultiTargets();
                     uint numTargets = (uint)pMultiTargets->getNumber();
                     if (numTargets <= 10)
                     {
                        bool newTargetFound = false;
                        BEntityIDArray results(0, 10);       
                        BUnitQuery query(pTargetEntity->getPosition(), mActiveScanRadiusScale * pTargetEntity->getObstructionRadius(), true);
                        gWorld->getUnitsInArea(&query, &results);
                        uint numUnits = (uint)results.getNumber();
                        for (uint i = 0; i < numUnits; i++)
                        {
//-- FIXING PREFIX BUG ID 1620
                           const BUnit* pUnit = gWorld->getUnit(results[i]);
//--
                           if (!pUnit)
                           {
                              continue;
                           }

                           if (results[i] == mTargetObjectID)
                           {
                              continue;
                           }
                           
                           if (pMultiTargets->find(results[i]) != cInvalidIndex)
                           {
                              continue;
                           }

                           if (pTargetEntity->getProtoID() == pUnit->getProtoID())
                           {
                              mTargetObjectID = results[i];
                              mActiveScanChance = 0.0f;
                              pMultiTargets->uniqueAdd(results[i]);
                              newTargetFound = true;
                              break;
                           }            
                        }

                        if (!newTargetFound && (numTargets > 0))
                        {
                           // Pick random target from multi target list
                           int index = getRandRange(cSimRand, 0, (numTargets - 1));
                           mTargetObjectID = pMultiTargets->get(index);
                           mActiveScanChance = 0.0f;
                        }
                     }
                  }
               }
            }

            //-- adjust target position   
            if( mTargetObjectID != cInvalidObjectID )
            {
               pTargetEntity = gWorld->getEntity( mTargetObjectID );
               if (pTargetEntity)
               {
                  BVector targetVelocity = pTargetEntity->isMoving() ? pTargetEntity->getVelocity() : XMVectorZero();

                  // If valid hit zone
                  BVector hitZonePos;
                  BUnit*  pTargetUnit = gWorld->getUnit( mTargetObjectID );            
                  if( pTargetUnit && pTargetUnit->getHitZonePosition( mHitZoneIndex, hitZonePos ) )
                  {                  
                     BVector targetPos = hitZonePos + mTargetOffset;

                     // Track intercept if we're close to the target
                     BVector targetingLead = XMVectorZero();
                     if( getFlagTracking() )
                     {
                        float d = targetPos.distance( mPosition );
                        if( d <= gDatabase.getTrackInterceptDistance() )
                        {
                           float   leadWeight   =  XMScalarCos( ( d / gDatabase.getTrackInterceptDistance() ) * XM_PIDIV2 );
                           float   t            =  d / mVelocity.length();               
                           BVector newTargetPos =  targetPos + targetVelocity * t; // Halwes 1/29/2007 - Hit zones are using the entity's velocity
                           targetingLead        =  newTargetPos - targetPos;
                           float   d2           =  newTargetPos.distance( mPosition );
                           targetingLead        *= leadWeight * d2 / d;

                           mFlagInterceptDist = true;
                        }
                     }

                     setTargetLocation( hitZonePos + mTargetOffset + targetingLead);
                     #ifdef SYNC_Projectile
                        syncProjectileData( "BProjectile::update targetLocation", mTargetLocation );
                     #endif
                  }
                  // If NO valid hit zone
                  else
                  {
                     BVector targetPos = pTargetEntity->getPosition() + mTargetOffset;

                     // Track intercept if we're close to the target
                     BVector targetingLead = XMVectorZero();
                     if (getFlagTracking())
                     {
                        float d = targetPos.distance(mPosition);
                        if (d <= gDatabase.getTrackInterceptDistance())
                        {
                           float leadWeight = XMScalarCos((d / gDatabase.getTrackInterceptDistance()) * XM_PIDIV2);
                           float t = d / mVelocity.length();
                           BVector newTargetPos = targetPos + targetVelocity * t;
                           targetingLead = newTargetPos - targetPos;
                           float d2 = newTargetPos.distance(mPosition);
                           targetingLead *= leadWeight * d2 / d;

                           mFlagInterceptDist = true;
                        }
                     }

                     setTargetLocation(pTargetEntity->getPosition() + mTargetOffset + targetingLead);
                     #ifdef SYNC_Projectile
                        syncProjectileData("BProjectile::update targetLocation", mTargetLocation);
                     #endif
                  }
               }
            }   

            // Handle fuel
            if (mFlagTestFuel)
            {
               mFuel -= elapsedTime;
               if (mFuel <= cFloatCompareEpsilon)
               {
                  setFlagTumbling(true);
                  setFlagIsAffectedByGravity(true);
                  setFlagTracking(false);
                  setFlagTrackingDelay(false);
                  setFlagPerturbed(false);
                  mFuel = 0.0f;
                  mAcceleration = 0.0f;
                  if (mpPerturbanceData)
                     mpPerturbanceData->mPerturbanceChance = 0.0f;
                  mFlagTestFuel = false;
                  initTumbleVector();
               }
            }

            // Thou shall not perturb until clear of thine launcher
            if (!mFlagClearedLauncher)
            {
//-- FIXING PREFIX BUG ID 1621
               const BUnit* pUnit = gWorld->getUnit(getParentID());
//--
               if (pUnit)
               {
                  float radius = pUnit->getObstructionRadius();
                  radius *= radius;         
                  if (radius <= pUnit->calculateXZDistanceSqr(getPosition()))
                  {
                     mFlagClearedLauncher = true;
                     attemptFlares();
                  }
               }
               else
               {
                  mFlagClearedLauncher = true;
               }      
            }



            //-- handle perturbance
            BVector perturbance;
            perturbance.zero();
            if (mFlagClearedLauncher && !mFlagInterceptDist)
            {
               if (getFlagPerturbed())
               {
                  BASSERT(mpPerturbanceData);
                  bool perturbanceDone = mpPerturbanceData->updatePerturbance(elapsedTime, perturbance);
                  if (perturbanceDone)
                  {
                     setFlagPerturbed(false);
                     mPerturbOnce = false;
                  }
               }
               else
               {
                  float perturbanceRoll = getRandDistribution(cSimRand);

                  if (mPerturbOnce || (mpPerturbanceData && (perturbanceRoll <= mpPerturbanceData->mPerturbanceChance)))
                  {            
                     float velocityRatio = mVelocity.length() / getDesiredVelocity();
                     mpPerturbanceData->startPerturbance(mPerturbOnce, velocityRatio);
                     setFlagPerturbed(true);
                  }
               }
            }



            BVector facing = mForward;
            BVector prev = mPosition;
            BMovementHelper helper;
            helper.doProjectileMovement(this, perturbance, elapsedTime, getFlagIsAffectedByGravity(), getFlagTracking(), mGravity, mTurnRate, facing, 1.0f);
            BDEBUG_ASSERTM(!XMVector3IsNaN(mPosition), "BProjectile::update - new position is invalid.");

            // Are we there yet?
            if (getFlagAirBurst() || getFlagTracking())
            {
               // If we are close and target is past the 3-9 line, we're there.
               if (mPosition.distance(mTargetLocation) < 10.0f)
               {
                  BVector trajectory = XMVectorSubtract(mPosition, prev);
                  trajectory.normalize();
                  BVector vecToTarget = XMVectorSubtract(mTargetLocation, mPosition);
                  vecToTarget.normalize();
                  float dot = trajectory.dot(vecToTarget);
                  if (dot < 0.0f) // past the 3-9 line
                  {
                     if (getFlagAirBurst())
                        explode = true;
                     if (getFlagTracking())
                        setFlagTracking(false);
                  }
               }
            }

            //-- handle tumbling
            if (getFlagTumbling())
               setForward(XMVector3TransformNormal(mForward, XMMatrixRotationRollPitchYawFromVector(XMVectorScale(mTumblingVector, elapsedTime))));
            else
            {
               // Make forward vector match the projectile's velocity
               BVector forward=facing;
               if(forward.safeNormalize())
                  setForward(forward);
            }
            calcRight();
            calcUp();





            #ifdef SYNC_Projectile
               syncProjectileData("BProjectile::update mVelocity", mVelocity);
            #endif

            #ifdef DEBUG_PROJECTILE_PATH
               gpDebugPrimitives->addDebugLine(prev, mPosition, cDWORDWhite, cDWORDWhite);
            #endif

            bool closeToTarget = (mTargetObjectID == cInvalidObjectID) ? true : (mInitialProjectilePosition.distanceSqr(mPosition) >= mInitialProjectilePosition.distanceSqr(getTargetLocation()) * 0.95f);

            // Tracking height fixup
            const BObject *pObject = gWorld->getObject(mTargetObjectID);
            if (getFlagTracking() && pObject && !pObject->getFlagFlying())
            {
               float cameraGridHeight;
               float simGridHeight;
               if (gTerrainSimRep.getCameraHeightRaycast(mPosition, cameraGridHeight, true) && gTerrainSimRep.getHeightRaycast(mPosition, simGridHeight, true))
               {
                  //float fixedHeight = Math::Max(cameraGridHeight, simGridHeight) + mFollowGroundHeight;
                  float fixedHeight = simGridHeight + mFollowGroundHeight; //DJB - Removed the camera height since this can be much higher than the simrep. Given the true LOS changes, I don't think this will ever show up anyway. If it does, we can apply a look-ahead solution to smooth out projectile ground avoidance movement.
                  if (fixedHeight > mPosition.y)
                  {
                     float xzDistance = mPosition.xzDistance(getTargetLocation());
                     float speed = mVelocity.length();
                     float fixedHeightDistance = speed * (Math::Clamp(mTurnRate, 90.0f, 360.0f) / 360.0f);

                     if (fixedHeightDistance <= xzDistance)
                        mPosition.y = fixedHeight;
                  }
               }
            }

            // Did we collide with any units?
            BVector next = mPosition;
            // If projectile is dodgeable or deflectable, add an extra 1 second of movement to it
            // to check for potential collisions (and trigger dodging/deflecting)
            if (mpDamageInfo && mpDamageInfo->getDodgeable())
            {
               BVector trajectory = next - prev;
               trajectory.normalize();
               trajectory *= (mVelocity.length());
               next += trajectory;
            }
            static BObstructionNodePtrArray obsResults;
            obsResults.resize(0);
            BSimHelper::getUnitObstructionsAlongSegment(prev, next, obsResults);
            long numResults = obsResults.getNumber();


            // BTK/AJL 10/12/06 - Hack to stop projectiles from sticking to targets and not blowing up
            if (numResults == 0 && !getFlagFirstUpdate() && !getFlagExplodeOnTimer() && mPosition.distanceSqr(mTargetLocation) <= 0.01f)
            {
               explode = true;
               if (pObject)
                  unitHitID = pObject->getID();
            }

            // CJS 9-3-08 - Hack so that missiles chasing after flares will blow up
            if (numResults == 0)
            {
               uint type = (uint) mTargetObjectID.getType();
               if (type == BEntity::cClassTypeProjectile)
               {
                  BProjectile* pProj = gWorld->getProjectile(mTargetObjectID);
                  if (pProj)
                  {
                     float rad = pProj->getObstructionRadius();
                     if (mPosition.distanceSqr(mTargetLocation) < rad * rad)
                     {
                        explode = true;
                        unitHitID = mTargetObjectID;
                     }
                  }
               }
            }

            if (!explode)
            {
               for (long i = 0; i < numResults; i++)
               {
                  const BOPObstructionNode* pObstructionNode = obsResults[i];
                  if ((pObstructionNode == NULL) || (pObstructionNode->mObject == NULL))
                     continue;

                  BUnit* pUnit = pObstructionNode->mObject->getUnit();
                  if (!pUnit)
                     continue;

                  bool externalShield = pUnit->isExternalShield();

                  const BProtoObject* pProtoObject = pUnit->getProtoObject();

                  // Disregard units with no protoobjects
                  if (!pProtoObject)
                     continue;

                  // Don't hurt ourselves unless self damage is enabled
                  if(pUnit->getID() == getParentID() && !getFlagSelfDamage())
                     continue;

                  // HACK: If the unit is flagged as not hittable, continue
                  if (pUnit->getFlagUnhittable())
                     continue;

                  // Disregard dead
                  if (!pUnit->isAlive())
                     continue;

                  // Disregard neutrals
                  if (pProtoObject->getFlagNeutral())
                     continue;

                  // If the target dodged, ignore htem
                  if (mFlagIngoreTargetCollisions && pUnit->getID() == mTargetObjectID)
                     continue;
                  
                  if(!pProtoObject->getFlagIsProjectileObstructable())
                  {
                     if (!externalShield && !closeToTarget && pUnit != pTargetEntity && !getFlagSelfDamage())
                        continue;

                     if(!pProtoObject->isType(gDatabase.getOTIDCover()))
                     {                     
                        bool hitsFriendly = (mpProtoAction && mpProtoAction->getFriendlyFire());
                        if (!hitsFriendly && !gWorld->getPlayer(getPlayerID())->isEnemy(pUnit->getPlayerID()) && !pUnit->getPlayer()->isGaia())
                           continue;
                     }
                  }

                  if (!getFlagCollidesWithAllUnits() && pProtoObject->getFlagTargetsFootOfUnit())
                     continue;
                  float intersectDistSqr = 0.0f;
                  long intersectAttachmentHandle = -1;
                  long intersectBoneHandle = -1;
                  BVector intersectBoneSpacePos;
                  BVector intersectBoneSpaceDir;
                  BVector trajectory = XMVectorSubtract(mPosition, prev);
                  bool collided = false;         
                  BBoundingBox obb;

                  if (externalShield)
                  {
                     const BVector pos = pUnit->getPosition();
                     float radiusX = pUnit->getObstructionRadiusX();
                     float radiusY = pUnit->getObstructionRadiusY();
                     float radiusZ = pUnit->getObstructionRadiusZ();
                     BVector maxXZ = BVector(pos.x + radiusX, pos.y, pos.z + radiusZ);
                     float radiusSq = pos.distanceSqr(maxXZ);
                     float maxY = pos.y + radiusY;
                     float minY = pos.y - radiusY;
                     bool wallShield = pUnit->isType(gDatabase.getOTIDWallShield()) || pUnit->isType(gDatabase.getOTIDBaseShield());
                     // Verify the projectile didn't start inside the shield
                     if (!wallShield && (mInitialProjectilePosition.y <= maxY) && (mInitialProjectilePosition.y >= minY))
                     {
                        float initDistSq = pos.distanceSqr(mInitialProjectilePosition);
                        if (initDistSq <= radiusSq)
                        {
                           continue;
                        }
                     }

                     // Is the projectile in the right vertical range to collide with the external shield (shield is not perfect sphere)
                     if ((mPosition.y <= maxY) && (mPosition.y >= minY))
                     {
                        float distSqr = pos.distanceSqr(mPosition);
                        // Is the projectile within the horizontal radius
                        if (distSqr <= radiusSq)
                        {
                           collided = true;
                        }
                     }

                     if (collided && wallShield)
                     {
                        //XXXHalwes - 8/4/2008 - Wallshield art is setup so that the unit's right direction is the correct forward facing direction.
                        BVector wallShieldDir = pUnit->getRight();
                        wallShieldDir.normalize();
                        BVector startDir = XMVectorSubtract(mInitialProjectilePosition, pos);
                        startDir.normalize();
                        BVector currentDir = XMVectorSubtract(mPosition, pos);
                        currentDir.normalize();
                        float angle = startDir.dot(wallShieldDir);
                        // We started on the front side
                        if (angle > 0.0f)
                        {
                           collided = (currentDir.dot(wallShieldDir) <= 0.0f);
                        }
                        // We started on the back side
                        else
                        {
                           collided = (currentDir.dot(wallShieldDir) >= 0.0f);
                        }
                     }
                  }
                  else if ((pUnit->getID() == mTargetObjectID) && pUnit->getHitZoneOBB(mHitZoneIndex, obb))
                  {
                     // Check for and trigger potential dodging / deflecting
                     if (!mFlagCheckedForDeflect && mpDamageInfo && ((mpDamageInfo->getDodgeable() && pUnit->canDodge()) || (mpDamageInfo->getDeflectable() && pUnit->canDeflect()) ||
                         (mpDamageInfo->getSmallArmsDeflectable() && pUnit->canDeflectSmallArms())))
                     {
                        BVector futureTrajectory = XMVectorSubtract(next, prev);
                        bool futureCollide = obb.raySegmentIntersects(prev, futureTrajectory, true, NULL, intersectDistSqr);
                        if (futureCollide)
                        {
                           mFlagCheckedForDeflect = true;
                           if ((pUnit->canDeflect() || pUnit->canDeflectSmallArms()) && pUnit->doDeflect(getID(), futureTrajectory, mDamage))
                           {
                              // Clear the deflect action's target
                              pUnit->resetDeflectID();
                              mDeflectingUnitID = pUnit->getID();
                           }
                           else if (pUnit->canDodge())
                           {
                              pUnit->doDodge(getID(), futureTrajectory);
                           }
                        }
                        #ifndef BUILD_FINAL
                           if (gConfig.isDefined(cConfigDebugDodge))
                              gpDebugPrimitives->addDebugLine(prev, prev + futureTrajectory, futureCollide ? cDWORDGreen : cDWORDRed, futureCollide ? cDWORDGreen : cDWORDRed, BDebugPrimitives::cCategoryNone, 0.5f);
                        #endif
                     }

                     // Ignore collisions if the unit is dodging this projectile
                     // and if the unit is an aircraft, ignore collisions if it is dodging anything
                     if (pUnit->getProtoObject()->getMovementType() == cMovementTypeAir)
                     {
                        if (pUnit->isDodging())
                        {
                           if (getFlagTracking())
                              setFlagTracking(false);
                           continue;
                        }
                     }
                     else if (pUnit->isDodging(getID()))
                     {
                        mFlagIngoreTargetCollisions = true;
                        continue;
                     }

                     // Collision check
                     collided = obb.raySegmentIntersects(prev, trajectory, true, NULL, intersectDistSqr);
                  }
                  else
                  {
                     // Check for and trigger potential dodging / deflecting
                     if (!mFlagCheckedForDeflect && mpDamageInfo && ((mpDamageInfo->getDodgeable() && pUnit->canDodge()) || (mpDamageInfo->getDeflectable() && pUnit->canDeflect()) ||
                         (mpDamageInfo->getSmallArmsDeflectable() && pUnit->canDeflectSmallArms())))
                     {
                        BVector futureTrajectory = XMVectorSubtract(next, prev);
                        bool futureCollide = pUnit->getSimBoundingBox()->raySegmentIntersects(prev, futureTrajectory, true, NULL, intersectDistSqr);
                        //bool futureCollide = pUnit->getVisualBoundingBox()->raySegmentIntersects(prev, futureTrajectory, true, NULL, intersectDistSqr);
                        if (futureCollide)
                        {
                           mFlagCheckedForDeflect = true;
                           if ((pUnit->canDeflect() || pUnit->canDeflectSmallArms()) && pUnit->doDeflect(getID(), futureTrajectory, mDamage))
                           {
                              // Clear the deflect action's target
                              pUnit->resetDeflectID();
                              mDeflectingUnitID = pUnit->getID();
                           }
                           else if (pUnit->canDodge())
                           {
                              pUnit->doDodge(getID(), futureTrajectory);
                           }
                        }

                        #ifndef BUILD_FINAL
                           if (gConfig.isDefined(cConfigDebugDodge))
                              gpDebugPrimitives->addDebugLine(prev, prev + futureTrajectory, futureCollide ? cDWORDGreen : cDWORDRed, futureCollide ? cDWORDGreen : cDWORDRed, BDebugPrimitives::cCategoryNone, 0.5f);
                        #endif

                     }

                     // Ignore collisions if the unit is dodging this projectile
                     // and if the unit is an aircraft, ignore collisions if it is dodging anything
                     if (pProtoObject->getMovementType() == cMovementTypeAir)
                     {
                        if (pUnit->isDodging())
                        {
                           if (getFlagTracking())
                              setFlagTracking(false);
                           continue;
                        }
                     }
                     else if (pUnit->isDodging(getID()))
                     {
                        mFlagIngoreTargetCollisions = true;
                        continue;
                     }

                     // Box collision check
                     collided = pUnit->getSimBoundingBox()->raySegmentIntersectsFirst(prev, trajectory, true, NULL, intersectDistSqr);
                     //collided = pUnit->getVisualBoundingBox()->raySegmentIntersects(prev, trajectory, true, NULL, intersectDistSqr);
                  }

                  if (!collided)
                     continue;


                  // Use the more acurate physics shape collision for buildings that have it.
                  //
//-- FIXING PREFIX BUG ID 1625
                  const BPhysicsObject* pPhysicsObject = pUnit->getPhysicsObject();
//--
                  if(pPhysicsObject)
                  {
                     BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pPhysicsObject->getInfoID(), true);

                     if (pInfo && (!pInfo->isVehicle() || pInfo->isAircraft()))
                     {
                        BVector normal;
                        collided = pPhysicsObject->raySegmentIntersects(prev, trajectory, true, intersectDistSqr, &normal);
                        /*if (collided)
                        {
                           mRight = normal;
                        }*/
                     }
                  }

                  if (!collided)
                     continue;


                  // Mesh boxes collision check
                  // Do finer collision testing if we are dealing with a sticky projectile.
                  if(getFlagIsSticky())
                  {
                     BVisual *pVisual = pUnit->getVisual();

                     BMatrix unitWorldMat;
                     pUnit->getWorldMatrix(unitWorldMat);

                     if(pVisual && (pVisual->raySegmentIntersects(prev, trajectory, true, unitWorldMat, NULL, intersectDistSqr, intersectAttachmentHandle, intersectBoneHandle, intersectBoneSpacePos, intersectBoneSpaceDir)))
                     {
                        collided = true;
                     }
                     else
                     {
                        collided = false;
                     }
                  }

                  if (!collided)
                     continue;

                  //-- Update the projectile position to the point of impact
                  trajectory.normalize();
                  if (!externalShield)
                  {
                     trajectory.scale(sqrt(intersectDistSqr) + 0.01f); //-- Add a little bit so the projectile is inside target when it explodes, guaranteeing 0.0 distance when calcing AOE.
                     mPosition = prev + trajectory;
                  }

                  // Check for deflection
                  // [6/20/08 CJS] This duplicate line looks like an oops to me...
                  //if (mDeflectingUnitID == pUnit->getID())


                  if (mDeflectingUnitID == pUnit->getID())
                  {
                     if(!mFlagDeflected)
                     {
                        deflect();
                        mPosition = prev;
                     }
                     continue;
                  }

                  if (getFlagHideOnImpact())
                  {
                     createImpactEffect();
                     setFlagNoRender(true);
                  }

                  if(!getFlagExplodeOnTimer() && !getFlagExpireOnTimer())
                  {
                     explode = true;
                     unitHitID = pUnit->getID();
                  }
                  else
                  {
                     // Count the flame effects on the target unit
                     int numFlames = 0;
                     uint numEntityRefs = pUnit->getNumberEntityRefs();
                     for (uint i=0; i<numEntityRefs; i++)
                     {

//-- FIXING PREFIX BUG ID 1623
                        const BEntityRef* pRef = pUnit->getEntityRefByIndex(i);
//--
                        if (pRef && pRef->mID != pUnit->getID())
                        {
                           if (pRef->mType == BEntityRef::cTypeStickedProjectile)
                           {
//-- FIXING PREFIX BUG ID 1622
                              const BObject* pStickedProj = gWorld->getObject(pRef->mID);
//--
                              if (pStickedProj && pStickedProj->getProtoObject()->getFlagIsFlameEffect())
                                 numFlames++;
                           }
                        }
                     }

                     // If already at the flame limit for this target type, destroy the oldest one
                     int maxFlames = pUnit->getProtoObject()->getMaxFlameEffects();
                     if (maxFlames == -1)
                     {
                        const BProtoObject* pProtoObj = pUnit->getProtoObject();
                        maxFlames = gDatabase.getBurningEffectLimitByProtoObject(pProtoObj);
                     }

                     if (numFlames >= maxFlames)
                     {
                        for (uint i=0; i<numEntityRefs; i++)
                        {
//-- FIXING PREFIX BUG ID 1624
                           const BEntityRef* pRef = pUnit->getEntityRefByIndex(i);
//--
                           if (pRef && pRef->mID != pUnit->getID())
                           {
                              if (pRef->mType == BEntityRef::cTypeStickedProjectile)
                              {
                                 BObject* pStickedProj = gWorld->getObject(pRef->mID);
                                 if (pStickedProj && pStickedProj->getProtoObject()->getFlagIsFlameEffect())
                                 {
                                    pStickedProj->kill(true);
                                 }
                              }
                           }
                        }
                     }

                     if(!getFlagIsSticky())
                     {
                        mMotionState = cStatePhysicsTumbling;

                        long physicsInfoID = getProtoObject()->getPhysicsReplacementInfoID();
                        BASSERTM(physicsInfoID != -1, "Projectile wants to go into tumbling state but does not have a PhysicsReplacementInfo");

                        if(physicsInfoID != -1)
                        {
                           createPhysicsObject(physicsInfoID, NULL, false, true);

                           BVector velocity = mForward;
                           velocity.scale(scVelocityScaleFactor);
#ifdef SYNC_Projectile
                           syncProjectileData("BProjectile::update setLinearVelocity", velocity);
#endif
                           getPhysicsObject()->setLinearVelocity(velocity);
                        }
                     }
                     else
                     {
                        mMotionState = cStateSticked;

                        // Get stick data instance if needed
                        if (!mpStickData)
                        {
                           mpStickData = BStickData::getInstance();
                           BASSERT(mpStickData);
                        }

                        if (mpStickData)
                        {
                           mpStickData->mStickedToUnitID = pUnit->getID();
                           mpStickData->mStickedToAttachmentHandle = intersectAttachmentHandle;
                           mpStickData->mStickedToBoneHandle = intersectBoneHandle;
                           mpStickData->mStickedModelSpacePosition = intersectBoneSpacePos;
                           mpStickData->mStickedModelSpaceDirection = intersectBoneSpaceDir;

   #ifdef SYNC_Projectile
                           syncProjectileData("BProjectile::update mMotionState", mMotionState);
                           syncProjectileData("BProjectile::update mStickedToUnitID", mpStickData->mStickedToUnitID.asLong());
                           syncProjectileData("BProjectile::update mStickedModelSpacePosition", mpStickData->mStickedModelSpacePosition);
                           syncProjectileData("BProjectile::update mStickedModelSpaceDirection", mpStickData->mStickedModelSpaceDirection);
   #endif      
                        }

                        if (getFlagExpireOnTimer())
                        {
                           bool killed;
                           doDamage(killed, pUnit->getID());
                        }
                        
                        if(getFlagIsNeedler())
                        {
                           // Add sticked projectile to units' entityRef list
                           pUnit->addEntityRef(BEntityRef::cTypeStickedProjectile, getID(), getProtoID(), 0);
                           mFlagAddedStickRef=true;

                           // Change anim state to remove trail
                           setAnimationState(BObjectAnimationState::cAnimationStateDeath);
                        }

                        if (getProtoObject()->getFlagIsFlameEffect())
                        {
                           pUnit->addEntityRef(BEntityRef::cTypeStickedProjectile, getID(), getProtoID(), 0);
                           mFlagAddedStickRef=true;
                        }
                     }
                  }

                  break;
               }
            }

            if (!explode && (closeToTarget || getFlagTracking() || getFlagTumbling()))
            {
               // Did we collide with the terrain?
               BVector intersectionPt;
               bool terrainHit = fastCheckHitTerrain(prev, mPosition, intersectionPt);
               if (terrainHit)
               {
                  mPosition = intersectionPt;
                  mPosition.y += scTerrainCollisionOffset;
               }

               //-- Blow up
               if (terrainHit)
               {
                  if (getFlagHideOnImpact())
                  {
                     createImpactEffect();
                     setFlagNoRender(true);
                  }

                  if(!getFlagExplodeOnTimer() && !getFlagExpireOnTimer())
                  {
                     explode = true;
                  }
                  else
                  {
                     if(!getFlagIsSticky())
                     {
                        long physicsInfoID = getProtoObject()->getPhysicsReplacementInfoID();
                        BASSERTM(physicsInfoID != -1, "Projectile wants to go into tumbling state but does not have a PhysicsReplacementInfo");

                        if(physicsInfoID != -1)
                        {
                           createPhysicsObject(physicsInfoID, NULL, false, true);

                           BVector velocity = mForward;
                           velocity.y *= -1;
                           velocity.scale(scVelocityScaleFactor);
#ifdef SYNC_Projectile
                           syncProjectileData("BProjectile::update setLinearVelocity", velocity);
#endif
                           getPhysicsObject()->setLinearVelocity(velocity);
                        }
                     }
                     else
                     {
                        mMotionState = cStateRest;
                        if (getFlagExpireOnTimer()) // Set up for lingering visual effects (like flames)
                        {
                           mSurfaceImpactType = gTerrainSimRep.getTileType(mPosition);
                           //CLM ignore crystal and plasma types
                           if(mSurfaceImpactType != 0x0E && mSurfaceImpactType != 0x0F)
                           {
                              const BProtoObject *bpo = getProtoObject();
                              if(bpo->getImpactDecal())
                                 gWorld->createImpactTerrainDecal(mPosition,bpo->getImpactDecal());
                           }

                        }
                     }
                  }
               }
            }

         }
         break;

      case cStatePhysicsTumbling:
         {

            //SCOPEDSAMPLE(BProjectile_update_cStatePhysicsTumbling);  


         }
         break;

      case cStateSticked:
         {
            //SCOPEDSAMPLE(BProjectile_update_cStateSticked);  
            if (mpStickData)
            {
               BUnit* pStickedToUnit = gWorld->getUnit(mpStickData->mStickedToUnitID);

               if(pStickedToUnit)
               {
                  BMatrix unitWorldMat;
                  pStickedToUnit->getWorldMatrix(unitWorldMat);

                  BVisual *pUnitVisual = pStickedToUnit->getVisual();
                  if (pUnitVisual && (pUnitVisual->mModelAsset.mType == cVisualAssetGrannyModel))
                  {
                     if(mpStickData->mStickedToAttachmentHandle != -1)
                     {
                        BVisualItem *pAttachmentVisual = pUnitVisual->getAttachment(mpStickData->mStickedToAttachmentHandle, NULL);

                        if(pAttachmentVisual && (pAttachmentVisual->mModelAsset.mType == cVisualAssetGrannyModel))
                        {
                           BMatrix attachmentMat = pAttachmentVisual->mMatrix;
                           unitWorldMat.mult(attachmentMat, unitWorldMat);

                           if(mpStickData->mStickedToBoneHandle != -1)
                           {
                              BGrannyInstance* pGrannyInstance = (BGrannyInstance *) pAttachmentVisual->mpInstance;

                              if(pGrannyInstance)
                              {
                                 BMatrix boneMat;
                                 if(pGrannyInstance->getBone(mpStickData->mStickedToBoneHandle, NULL, &boneMat, NULL, &unitWorldMat))
                                 {
                                    unitWorldMat = boneMat;
                                 }
                                 else
                                 {
                                    explode = true;
                                 }
#ifdef SYNC_Projectile
                                 syncProjectileCode("BProjectile::update cStateSticked 1");
#endif      
                              }
                           }
                        }
                     }
                     else
                     {
                        if(mpStickData->mStickedToBoneHandle != -1)
                        {
                           BGrannyInstance* pGrannyInstance = (BGrannyInstance *) pUnitVisual->mpInstance;

                           if(pGrannyInstance)
                           {
                              BMatrix boneMat;
                              if(pGrannyInstance->getBone(mpStickData->mStickedToBoneHandle, NULL, &boneMat, NULL, &unitWorldMat))
                              {
                                 unitWorldMat = boneMat;
                              }
                              else
                              {
                                 explode = true;
                              }

#ifdef SYNC_Projectile
                              syncProjectileCode("BProjectile::update cStateSticked 2");
#endif      
                           }
                        }
                     }
                  }

                  unitWorldMat.transformVectorAsPoint(mpStickData->mStickedModelSpacePosition, mPosition);
                  unitWorldMat.transformVector(mpStickData->mStickedModelSpaceDirection, mForward);
                  mForward.normalize();
                  calcRight();
                  calcUp();

                  if (mpDamageInfo && mpDamageInfo->getReapplyTime() > 0.0f && mpDamageInfo->getApplyTime() > 0.0f)
                  {
                     mApplicationTimer += elapsedTime;
                     mReapplicationTimer += elapsedTime;

                     if (mApplicationTimer > mpDamageInfo->getApplyTime())
                     {
                        explode = true;
                        finished = true;
                     }
                     else if (mReapplicationTimer > mpDamageInfo->getReapplyTime())
                     {
                        mReapplicationTimer = 0.0f;
                        explode = true;
                        finished = false;
                     }
                  }

#ifdef SYNC_Projectile
                  syncProjectileData("BProjectile::update mPosition", mPosition);
                  syncProjectileData("BProjectile::update mForward", mForward);
#endif      
               }
            }
         }
         break;

      case cStateRest:
         {
            if (mpDamageInfo && mpDamageInfo->getReapplyTime() > 0.0f && mpDamageInfo->getApplyTime() > 0.0f)
            {
               mApplicationTimer += elapsedTime;
               mReapplicationTimer += elapsedTime;

               if (mApplicationTimer > mpDamageInfo->getApplyTime())
               {
                  explode = true;
                  finished = true;
               }
               else if (mReapplicationTimer > mpDamageInfo->getReapplyTime())
               {
                  mReapplicationTimer = 0.0f;
                  explode = true;
                  finished = false;
               }
            }
         }
         break;
   }

   mUnitHitID = unitHitID;

   if(getFlagExplodeOnTimer())
   {
      // We have a lifespan and we are expired, so go away.
      if (gWorld->getGametime() >= mLifespanExpiration)
      {
         unitHitID = getStickedToUnitID();
         explode = true;
      }
   }

   bool expire = false;
   if(getFlagExplodeOnTimer())
   {
      // We have a lifespan and we are expired, so go away.
      if (gWorld->getGametime() >= mLifespanExpiration)
      {
         unitHitID = getStickedToUnitID();
         expire = true;
      }

   }

   if(getFlagDetonate())
   {
      unitHitID = getStickedToUnitID();
      explode = true;
   }


   bool done = false;
   if (explode && !mFlagDeflected)
   {
      //-- Do our damage
      BEntityID unitID = unitHitID;

      // only notify the power if we're finished
      BPower* pPower = (finished) ? gWorld->getPowerManager()->getPowerByID(mOwningPowerID) : NULL;
      BEntityIDArray killedUnits, damagedUnits;

      bool killed;

      // If target is a projectile, just kill it
      uint type = (uint) unitID.getType();
      if (type == BEntity::cClassTypeProjectile)
      {
         BProjectile* pProj = gWorld->getProjectile(unitID);
         pProj->kill(false);
         killed = true;
      }
      else
      {
         if (pPower || mFlagFromLeaderPower)
            doDamage(killed, unitID, &killedUnits, &damagedUnits);
         else 
            doDamage(killed, unitID);
      }

      if (finished)
      {
         BUnit* pUnit = gWorld->getUnit(unitID);

         if(pUnit)
         {
            const BProtoObject *pProto = pUnit->getProtoObject();
            if (pProto)
               mSurfaceImpactType = pProto->getSurfaceType();
         }
         else
         {
            // we missed, we need to see whether we're too high to add a decal to the terrain
            float terrainHeight;
            if (gTerrainSimRep.getHeightRaycast(mPosition, terrainHeight, true))
            {
               float maxHeight = 1.0f;
               gConfig.get(cConfigMaxProjectileHeightForDecal, &maxHeight);

               if ((mPosition.y - terrainHeight) <= maxHeight)
               {
                  mSurfaceImpactType = gTerrainSimRep.getTileType(mPosition);
                  //CLM ignore crystal and plasma types
                  if(mSurfaceImpactType != 0x0E && mSurfaceImpactType != 0x0F)
                  {
                  const BProtoObject *bpo = getProtoObject();
                  if(bpo->getImpactDecal())
                     gWorld->createImpactTerrainDecal(mPosition,bpo->getImpactDecal());
                  }
               }
            }
         }

         //-- Destroy ourself
         #ifdef SYNC_Projectile
            syncProjectileData("BProjectile::update mID", mID.asLong());
            syncProjectileData("BProjectile::update killed", killed);
            syncProjectileData("BProjectile::update unitID", unitID.asLong());
         #endif      

         if (pPower)
            pPower->projectileImpact(getID(), killedUnits, damagedUnits);

         done = true;
      }
   }

   BEntityID stickedToUnitID = getStickedToUnitID();
   if (!mFlagDeflected && (explode || expire) && (stickedToUnitID != -1))
   {
      if (getFlagIsNeedler())
      {
         // Detonate all sticky projectile entity ref with the same protoID
         BUnit* pStickedToUnit = gWorld->getUnit(stickedToUnitID);
         if(pStickedToUnit)
         {
            // First remove this projectile's entityref from unit's list
            pStickedToUnit->removeEntityRef(BEntityRef::cTypeStickedProjectile, getID());
            mFlagAddedStickRef=false;

            // Now loop through all remaining entity refs of this type and have them 
            // detonate.
            int numEntityRefs = pStickedToUnit->getNumberEntityRefs();
            for (int i=numEntityRefs; i>=0; i--)
            {
//-- FIXING PREFIX BUG ID 1628
               const BEntityRef* pEntityRef = pStickedToUnit->getEntityRefByIndex(i);
//--
               if (pEntityRef && 
                  (pEntityRef->mType == BEntityRef::cTypeStickedProjectile) &&
                  (pEntityRef->mData1 == getProtoID()))
               {
                  BProjectile* pProjectile = gWorld->getProjectile(pEntityRef->mID);
                  if (pProjectile)
                  {
                     pProjectile->setFlagDetonate(true);
                  }
               }
            }
         }
      }
      else if (mFlagSingleStick)
      {
         BUnit* pStickedToUnit = gWorld->getUnit(stickedToUnitID);
         if(pStickedToUnit)
         {
            pStickedToUnit->removeEntityRef(BEntityRef::cTypeStickedProjectile, getID());
            mFlagAddedStickRef=false;
         }
      }
   }




   #ifndef BUILD_FINAL
      if(gConfig.isDefined(cConfigDebugProjectiles))
         gpDebugPrimitives->addDebugSphere(mPosition, 0.05f, cDWORDYellow, BDebugPrimitives::cCategoryNone, 1.0f);
   #endif

   // All projectile movement needs to be completed before object::update can be called
   // (unless object is marked for destruction) because object updates the visuals 
   // which need the correct world matrix.  If this is not done, all visuals will lag one frame behind
   if (!BObject::update(elapsedTime))
   {
      #ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigDebugProjectiles))
            gpDebugPrimitives->addDebugSphere(mPosition, 0.05f, cDWORDYellow, BDebugPrimitives::cCategoryNone, 1.0f);
      #endif

      if (done)
      {
         kill(false);
         launchTriggerScript(unitHitID, mPosition);
      }

      return (false);
   }

   if (done)
   {
      kill(false);
      launchTriggerScript(unitHitID, mPosition);
   }

   return (true);
}

//==============================================================================
// BProjectile::attemptFlares()
//==============================================================================
void BProjectile::attemptFlares()
{
   if (!getProtoObject() || !getProtoObject()->getFlagTracking())
      return;

   BUnit* pTargetUnit = gWorld->getUnit(mTargetObjectID);
   if (pTargetUnit)
   {
      BUnitActionPointBlankAttack* pAction = static_cast<BUnitActionPointBlankAttack*>(pTargetUnit->getActionByType(BAction::cActionTypeUnitPointBlankAttack));

      if (pAction)
      {
         pAction->attemptLaunchFlares(this);
      }
   }
}

//==============================================================================
// BProjectile::launchTriggerScript()
//==============================================================================
void BProjectile::launchTriggerScript(BEntityID unitHitID, BVector hitLocation) const
{
   if (mpDamageInfo)
   {
      BSimString triggerScript;
      mpDamageInfo->getTriggerScript(triggerScript);
      if (!triggerScript.isEmpty())
      {
         BTriggerCommand* pCommand = (BTriggerCommand*) gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (pCommand)
         {
            pCommand->setSenders(1, &mPlayerID);
            pCommand->setSenderType(BCommand::cPlayer);
            pCommand->setRecipientType(BCommand::cPlayer);
            pCommand->setType(BTriggerCommand::cTypeActivateTriggerScript);
            pCommand->setTriggerScript(triggerScript);
            pCommand->setInputUnit(unitHitID);
            pCommand->setInputLocation(hitLocation);
            gWorld->getCommandManager()->addCommandToExecute(pCommand);
         }
      }
   }
}

//==============================================================================
// BProjectile::updateBeamMatrices()
//==============================================================================
void BProjectile::updateBeamMatrices()
{
   static bool gRenderDebug = false;
   if (gRenderDebug)
   {
      gpDebugPrimitives->addDebugSphere(mPosition, 1.0f, cDWORDGreen, BDebugPrimitives::cCategoryNone);
      gpDebugPrimitives->addDebugLine(mPosition, mTargetLocation, cDWORDGreen, cDWORDRed);
      gpDebugPrimitives->addDebugSphere(mTargetLocation, 1.0f, cDWORDRed, BDebugPrimitives::cCategoryNone);
   }

   BMatrix mtx, boneMatrix;
   mtx.makeIdentity();
   boneMatrix.makeIdentity();
   getBeamLaunchPosition(&mPosition, &boneMatrix);

   //mForward = boneMatrix.getForward();
   //mRight   = boneMatrix.getRight();

   BObject* pBeamHead = gWorld->getObject(mBeamHeadID);
   if (pBeamHead)
   {
      pBeamHead->setPosition(mPosition);     
      mtx.setTranslation(mPosition);
      BVisual* pVisual = pBeamHead->getVisual();
      if (pVisual)
         pVisual->updateWorldMatrix(mtx,NULL);
   }

   // Get collision location - loop through all units + terrain that the beam intersects and choose the closest intersection point
   float intersectDistSqr = 0.0f;
   bool collided = false;
   float minIntersectDistSqr = FLT_MAX;
   BVector collisionLocation = mTargetLocation;
   BVector beamOrigin = mPosition;
   BVector beamVector = mTargetLocation - mPosition;
   BPlayer* pPlayer = getPlayer();
   BASSERTM(pPlayer, "Trying to update a beam projectile without a valid player!");

   // Collide with units
   if (getFlagBeamCollideWithUnits())
   {
      static BObstructionNodePtrArray obsResults;
      obsResults.resize(0);
      BSimHelper::getUnitObstructionsAlongSegment(mPosition, mTargetLocation, obsResults);
      long numUnits = obsResults.getNumber();

      for (long i = 0; i < numUnits; i++)
      {
         const BOPObstructionNode* pObstructionNode = obsResults[i];
         if ((pObstructionNode == NULL) || (pObstructionNode->mObject == NULL))
            continue;

         BUnit* pUnit = pObstructionNode->mObject->getUnit();
         if (!pUnit || !pUnit->isAlive() || pUnit->getFlagDown() || pUnit->getID() == getParentID() || gDatabase.getPOIDPhysicsThrownObject() == pUnit->getProtoID())     // don't shoot downed heroes or ourself
            continue;

         // check to make sure they're an enemy
         BPlayerID unitPlayerID = pUnit->getPlayerID();
         const BProtoObject* pUnitProtoObject = pUnit->getProtoObject();
         if (mPlayerID == unitPlayerID || !pUnitProtoObject || pUnitProtoObject->getFlagNeutral() || !pPlayer->isEnemy(unitPlayerID))
            continue;

         // Use the more acurate physics shape collision for buildings that have it.
         //
         BPhysicsObject* pPhysicsObject = pUnit->getPhysicsObject();
         if(pPhysicsObject)
         {
            BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pPhysicsObject->getInfoID(), true);

            if (pInfo && (!pInfo->isVehicle() || pInfo->isAircraft()))
            {
               bool testCollided = pPhysicsObject->raySegmentIntersects(beamOrigin, beamVector, true, intersectDistSqr);
               if (testCollided)
               {
                  collided = true;
                  if (intersectDistSqr < minIntersectDistSqr)
                     minIntersectDistSqr = intersectDistSqr;
               }
            }
         }

         // Box collision check
         bool testCollided = pUnit->getSimBoundingBox()->raySegmentIntersects(beamOrigin, beamVector, true, NULL, intersectDistSqr);
         if (testCollided)
         {
            collided = true;
            if (intersectDistSqr < minIntersectDistSqr)
               minIntersectDistSqr = intersectDistSqr;
         }
      }
   }

   // Check terrain collision too
   if (getFlagBeamCollideWithTerrain())
   {
      // VAT: 11/15/08: check smaller segments optimized for fast check hit terrain for speed
      BVector beamIncrement = (mTargetLocation - beamOrigin);
      BVector startCheck = beamOrigin;
      float beamTotalDistance = beamIncrement.length();
      float beamDistanceChecked = 0.0f;
      if (beamIncrement.safeNormalize())
      {
         beamIncrement *= scTerrainSegCheckBeamIncrementDistance;

         BVector endCheck = startCheck + beamIncrement;
         BVector terrainIntersectionPt = mTargetLocation;

         while (beamDistanceChecked < beamTotalDistance)
         {
            // add in the distance we'll gain from doing this check
            beamDistanceChecked += scTerrainSegCheckBeamIncrementDistance;
            
            // if our checked distance passed our total distance, 
            // we've passed the end point and need to clamp it off
            if (beamDistanceChecked > beamTotalDistance)
               endCheck = mTargetLocation;

            // do a fast check along this segment to see if a longer check is necessary
            if (fastCheckHitTerrain(startCheck, endCheck, terrainIntersectionPt, true))
            {
               // the fast check recommended a more detailed check - perform it from beam origin
               if (fastCheckHitTerrain(beamOrigin, mTargetLocation, terrainIntersectionPt))
               {
                  float terrainIntersectDistSqr = terrainIntersectionPt.distanceSqr(beamOrigin);
                  if (terrainIntersectDistSqr < minIntersectDistSqr)
                  {
                     collided = true;
                     minIntersectDistSqr = terrainIntersectDistSqr;
                  }
               }

               // even if we didn't update the minIntersecDistSqr, we bail, because
               // that means we've hit something else closer, and don't need to 
               // continue checking the rest of the beam against the terrain
               break;
            }

            // shift the segment we're checking along
            startCheck += beamIncrement;
            endCheck += beamIncrement;
         }
      }
   }

   // If beam collided with units or terrain, update the collision location
   if (collided)
   {
      beamVector.safeNormalize();
      collisionLocation = beamOrigin + beamVector * sqrtf(minIntersectDistSqr);
   }


   // Update beam tail and visual
   BObject* pBeamTail = gWorld->getObject(mBeamTailID);
   if (pBeamTail)
   {
      pBeamTail->setPosition(collisionLocation);
      mtx.setTranslation(collisionLocation);
      BVisual* pVisual = pBeamTail->getVisual();
      if (pVisual)
         pVisual->updateWorldMatrix(mtx,NULL);
   }

   BVisual* pVisual = getVisual();
   if (pVisual)
   {
      mtx.setTranslation(collisionLocation);  
      pVisual->updateSecondaryWorldMatrix(mtx);
   }
}

//==============================================================================
// BProjectile::updateBeam()
//==============================================================================
bool BProjectile::updateBeam(float elapsedTime)
{   
   updateBeamMatrices();

   //ground scorch mark
   //CLM - This should have a more elegant stall system to it, don't need to generate one every frame..
   if(mBeamDecalCreateStall%2==0)
   {
      mBeamDecalCreateStall=0;

      //CLM ignore crystal and plasma types
      if(mSurfaceImpactType != 0x0E && mSurfaceImpactType != 0x0F)
      {
         const BProtoObject *bpo = getProtoObject();
         if(bpo->getImpactDecal())
            gWorld->createImpactTerrainDecal(mTargetLocation,bpo->getImpactDecal());
      }
   }
   
   mBeamDecalCreateStall++;

      
   // All projectile movement needs to be completed before object::update can be called
   // (unless object is marked for destruction) because object updates the visuals 
   // which need the correct world matrix.  If this is not done, all visuals will lag one frame behind
   if (!BObject::update(elapsedTime))
   {
#ifndef BUILD_FINAL
      if(gConfig.isDefined(cConfigDebugProjectiles))
         gpDebugPrimitives->addDebugSphere(mPosition, 0.05f, cDWORDYellow, BDebugPrimitives::cCategoryNone, 1.0f);
#endif
      return (false);
   }


   static bool bDoDamage = true;
   bool bDead;
   if (bDoDamage)
      doDamage(bDead, cInvalidObjectID);

   return true;
}

//==============================================================================
// BProjectile::init
//==============================================================================
void BProjectile::init( void )
{
   //-- call the base func
   BObject::init(); 
   //--Flags
   mFlagIsAffectedByGravity=false;
   mFlagTrackingDelay=false;
   mFlagTracking=false;
   mFlagPerturbed=false;
   mFlagTumbling=false;
   mFlagDoLogging=false;
   mFlagBeam=false;
   mFlagCollidesWithAllUnits=true;
   mFlagClearedLauncher = false;
   mFlagInterceptDist = false;
   mFlagTestFuel = false;
   mFlagExplodeOnTimer=false;
   mFlagExpireOnTimer=false;
   mFlagIsSticky=false;
   mFlagDetonate=false;
   mFlagIsNeedler=false;
   mFlagSingleStick=false;
   mFlagAddedStickRef=false;
   mFlagTargetAlreadyDazed=false;
   mFlagCheckedForDeflect=false;
   mFlagDeflected=false;
   mFlagSelfDamage=false;
   mFlagAirBurst=false;
   mFlagHideOnImpact=false;
   mFlagStaticPosition=false;
   mFlagFromLeaderPower=false;
   mFlagIngoreTargetCollisions=false;
   mFlagBeamCollideWithUnits=false;
   mFlagBeamCollideWithTerrain=false;

   // State
   mMotionState = cStateFlying;

   setFlagVisibility(true);
   setFlagLOS(true);
   mParentID = cInvalidObjectID;
   mpProtoAction = NULL;
   mSurfaceImpactType = (byte)-1;
   mDamage = 0.0f;
   mpDamageInfo = NULL;
   mTargetOffset.zero();
   mGravity = 0.0f;
   mInitialTargetPosition = XMVectorZero();
   mInitialProjectilePosition = XMVectorZero();;
   mFuel = 0.0f;
   mStartingVelocity = 0.0f;
   mAcceleration = 0.0f;
   mTrackingStartTime = 0;
   mTurnRate = 0.0f;
   mpPerturbanceData = NULL;
   mpStickData = NULL;
   mPerturbOnce = false;
   mActiveScanChance = 0.0f;
   mActiveScanRadiusScale = 0.0f;
   mOwningPowerID = -1;

   mTumblingVector.zero();
   mHitZoneIndex = -1;

   mTargetLocation.zero();
   mTargetObjectID=cInvalidObjectID;

   mLaunchPointHandle = -1;
   mAttachmentHandle = -1;
   mBoneHandle = -1;

   mBeamHeadID = cInvalidObjectID;
   mBeamTailID = cInvalidObjectID;
   mBeamDecalCreateStall =0;

   mApplicationTimer = 0.0f;
   mReapplicationTimer = 0.0f;

   mDeflectingUnitID = cInvalidObjectID;
   mUnitHitID = cInvalidObjectID;
}
   

//==============================================================================
// BProjectile::destroy
//==============================================================================
void BProjectile::destroy()
{
#ifdef SYNC_Projectile
   syncProjectileData("BProjectile::destroy mID", mID.asLong())
#endif

   BObject* pBeamHead = gWorld->getObject(mBeamHeadID);
   if (pBeamHead)
      pBeamHead->kill(true);

   BObject* pBeamTail = gWorld->getObject(mBeamTailID);
   if (pBeamTail)
      pBeamTail->kill(true);

   // Release perturbance data
   if (mpPerturbanceData)
   {
      BPerturbanceData::releaseInstance(mpPerturbanceData);
      mpPerturbanceData = NULL;
   }

   // Release stick data
   if (mpStickData)
   {
      BStickData::releaseInstance(mpStickData);
      mpStickData = NULL;
   }

   BObject::destroy();
}

//==============================================================================
// BProjectile::kill
//==============================================================================
void BProjectile::kill(bool bKillImmediately)
{
#ifdef SYNC_Projectile
   syncProjectileData("BProjectile::kill mID", mID.asLong());
#endif

#ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigDebugProjectiles))
      gpDebugPrimitives->addDebugSphere(mPosition, 0.1f, cDWORDRed, BDebugPrimitives::cCategoryNone, 1.0f);
#endif

   if (!getFlagHideOnImpact())
      createImpactEffect();

   stop();

   if (mFlagAddedStickRef)
   {
      BUnit* pStickedToUnit = gWorld->getUnit(getStickedToUnitID());
      if(pStickedToUnit)
      {
         pStickedToUnit->removeEntityRef(BEntityRef::cTypeStickedProjectile, getID());
         mFlagAddedStickRef=false;
      }
   }

   destroy();
}

//==============================================================================
// BProjectile::doDamage
//==============================================================================
float BProjectile::doDamage( bool &dead, BEntityID unitHit, BEntityIDArray* pKilledUnits, BEntityIDArray* pDamagedUnits )
{
   dead = false;
   if(mpDamageInfo)
   {
      if(mpDamageInfo->getAOERadius() > cFloatCompareEpsilon)
      {      
         #ifdef SYNC_Projectile
            syncProjectileData("BProjectile::doDamage mID", mID.asLong());
            syncProjectileData("BProjectile::doDamage AOERadius", mpDamageInfo->getAOERadius());
            syncProjectileData("BProjectile::doDamage mDamage", mDamage);
            syncProjectileData("BProjectile::doDamage mPosition", mPosition);
         #endif

         // if no killed array was passed in, use this one, since we need one to process locally
         BEntityIDArray unitsKilled;
         BEntityIDArray* pKilledUnitsInternal = (pKilledUnits) ? pKilledUnits : &unitsKilled;

         //DJBPERF: Using damageInfo pointer right now to get all the neccesary AOE and weapontype information.
         //We could store all of this on the projectile itself if this will help perf.
         BVector damagePosition = mPosition;
         if (mFlagBeam)
         {
/*          // MSC: Do damage at the target location. There are many times the tail of the beam will collide with some unit it cannot attack, and then deal no damage to the unit it is trying to attack.
            // Yes, this can look goofy, but actually damaging the thing you're shooting is good.
            BObject* pBeamTail = gWorld->getObject(mBeamTailID);
            if (pBeamTail)
               damagePosition = pBeamTail->getPosition();
            else
*/
               damagePosition = mTargetLocation;
         }

         BEntityID parentID = getParentID();
         // If no parentID and is a leader power projectile, use the projectile's id as the parent.  This gets used for stats tracking later
         if (mFlagFromLeaderPower && parentID == cInvalidObjectID)
            parentID = getID();
         float damageDealt = BDamageHelper::doAreaEffectDamage(mPlayerID, getTeamID(), parentID, mpDamageInfo, mDamage, damagePosition, getID(), mVelocity, pKilledUnitsInternal, unitHit, mHitZoneIndex, getFlagDoLogging(), pDamagedUnits);
         
         // SLB: No more revealers
         // Reveal the location of the projectile to the attacker.
         //long simX=getSimX();
         //long simZ=getSimZ();
         //if (simX>=0 && simZ>=0 && simX<gVisibleMap.getMaxXTiles() && simZ<gVisibleMap.getMaxZTiles())
         //{
         //   if (!(gVisibleMap.getVisibility(simX, simZ) & gVisibleMap.getTeamFogOffMask(getTeamID())))
         //      gWorld->createRevealer(getTeamID(), getPosition(), gDatabase.getAttackedRevealerLOS(), gDatabase.getAttackedRevealerLifespan()); 
         //}

         #ifdef SYNC_Projectile
            syncProjectileData("BProjectile::doDamage damageDealt", damageDealt);
         #endif

         //-- Did we kill our primary target?
         if(mTargetObjectID != cInvalidObjectID)
         {
            if(pKilledUnitsInternal->find(mTargetObjectID) >= 0)
               dead = true;
         }

         return damageDealt;
      }
      else if(unitHit != cInvalidObjectID)
      { 
         #ifdef SYNC_Projectile
            syncProjectileData("BProjectile::doDamage mID", mID.asLong());
            syncProjectileData("BProjectile::doDamage target", unitHit.asLong());
            syncProjectileData("BProjectile::doDamage mDamage", mDamage);
            syncProjectileData("BProjectile::doDamage mPosition", mPosition);
         #endif

         // Halwes - 3/3/2008 - Projectiles need to hit cover objects, but don't do any direct damage as per design
         bool cover = false;
//-- FIXING PREFIX BUG ID 1632
         const BUnit* pUnitHit = gWorld->getUnit(unitHit);
//--
         if (pUnitHit)
         {
            const BProtoObject* pUnitHitProtoObject = pUnitHit->getProtoObject();
            if (pUnitHitProtoObject && pUnitHitProtoObject->isType(gDatabase.getOTIDCover()))
            {
               cover = true;
            }
         }

         bool isDead = false;
         float damageDealt = 0.0f;
         if (!cover)
         {
            BEntityID parentID = getParentID();
            // If no parentID and is a leader power projectile, use the projectile's id as the parent.  This gets used for stats tracking later
            if (mFlagFromLeaderPower && parentID == cInvalidObjectID)
               parentID = getID();
            damageDealt = BDamageHelper::doDamageWithWeaponType(mPlayerID, getTeamID(), unitHit, mpDamageInfo, mDamage, mpDamageInfo->getWeaponType(), true, mVelocity, 1.0f, mInitialProjectilePosition, getID(), parentID, &isDead, NULL, mHitZoneIndex, getFlagDoLogging());
         }

         // SLB: No more revealers
         // Reveal the location of the projectile to the attacker.
         //long simX=getSimX();
         //long simZ=getSimZ();
         //if (simX>=0 && simZ>=0 && simX<gVisibleMap.getMaxXTiles() && simZ<gVisibleMap.getMaxZTiles())
         //{
         //   if (!(gVisibleMap.getVisibility(simX, simZ) & gVisibleMap.getTeamFogOffMask(getTeamID())))
         //      gWorld->createRevealer(getTeamID(), getPosition(), gDatabase.getAttackedRevealerLOS(), gDatabase.getAttackedRevealerLifespan());
         //}

         #ifdef SYNC_Projectile
            syncProjectileData("BProjectile::doDamage damageDealt", damageDealt);
         #endif

         if (unitHit == mTargetObjectID)
            dead = isDead;

         if (pKilledUnits && isDead)
            pKilledUnits->add(unitHit);
         else if (pDamagedUnits && !isDead)
            pDamagedUnits->add(unitHit);

         return damageDealt;
      } 
   }

   
   return 0.0f;
}

//==============================================================================
// BProjectile::createImpactEffect
//==============================================================================
void BProjectile::createImpactEffect(void)
{
   BObjectCreateParms parms;
   parms.mPlayerID = getPlayerID();
   parms.mPosition = getPosition();
   parms.mRight = cXAxisVector;
   parms.mForward = cZAxisVector;

   bool specialDamageControllingEffect = false;
   BDamageAreaOverTimeInstance* pDAOTInst = NULL;

   //-- Create the visual impact effects
   if (mpProtoAction)
   {
      int impactProtoID = mpProtoAction->getImpactEffectProtoID();
      if (impactProtoID != -1)
      {
         // Create shockwave damage associated with impact effect
         if (getProtoObject() && getProtoObject()->getTactic())
         {
            BProtoAction* pShockwaveAction = getProtoObject()->getTactic()->getShockwaveAction();
            if (mpProtoAction->getDoShockwaveAction() && pShockwaveAction)
            {
               // Get new instance
               pDAOTInst = BDamageAreaOverTimeInstance::getInstance();
               if (pDAOTInst)
               {
                  // Init
                  bool result = pDAOTInst->init(gWorld->getGametime(), pShockwaveAction , getPlayerID(), getTeamID(), cInvalidObjectID);

                  // If init'd successfully, setup callback data for impact effect
                  if (result)
                  {
                     specialDamageControllingEffect = true;
                  }
                  // otherwise, release the bad instance
                  else
                  {
                     BDamageAreaOverTimeInstance::releaseInstance(pDAOTInst);
                  }
               }
            }
         }


         if( !specialDamageControllingEffect )
         {
            //If we aren't a special impact effect that deals damage, return if we shouldn't be seen.
            BTeamID teamID = gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID();
            if( isVisible(teamID) == false )
               return;
         }

         // Create impact effect
         const BProtoImpactEffect* pImpactData = gDatabase.getProtoImpactEffectFromIndex(impactProtoID);
         //BASSERT(pImpactData);    // asserting is good, but not crashing is better
         if (!pImpactData)
            return;

         BTerrainEffect* pTerrainEffect = gTerrainEffectManager.getTerrainEffect(pImpactData->mTerrainEffectIndex, true);
         if (pTerrainEffect)
         {
            BObject* pImpactObject = NULL;
            BUnit* pUnitHit = gWorld->getUnit(mUnitHitID);
            if (pUnitHit && pUnitHit->isType(gDatabase.getOTIDWallShield()))
            {
               parms.mForward = pUnitHit->getForward();
               parms.mRight = pUnitHit->getRight();
            }
            else if (pUnitHit && pUnitHit->isType(gDatabase.getOTIDBaseShield()))
            {
               //gpDebugPrimitives->addDebugLine(prev, prev + normal * 5.0f, cDWORDBlue, cDWORDBlue, BDebugPrimitives::cCategoryNone, 5.0f);

               /*gpDebugPrimitives->addDebugLine(mPosition, mPosition + mRight * 5.0f, cDWORDRed, cDWORDRed, BDebugPrimitives::cCategoryNone, 5.0f);
               gpDebugPrimitives->addDebugLine(mPosition, mPosition + mForward * 5.0f, cDWORDBlue, cDWORDBlue, BDebugPrimitives::cCategoryNone, 5.0f);*/

               parms.mForward = mRight;
               parms.mRight = mForward;
            }
            pTerrainEffect->instantiateEffect(mSurfaceImpactType, mpProtoAction->getImpactEffectSize(), parms.mPosition, parms.mForward, true, parms.mPlayerID, pImpactData->mLifespan, getFlagVisibleToAll(), cVisualDisplayPriorityCombat, &pImpactObject);
            if (specialDamageControllingEffect && pImpactObject && pDAOTInst)
            {
               pDAOTInst->setObjectID(pImpactObject->getID());
            }
         }


         // Create surface impact effect
//-- FIXING PREFIX BUG ID 1633
         const BUnit* pTargetUnit = gWorld->getUnit(mTargetObjectID);
//--
         bool bFlyingTarget = false;
         if (pTargetUnit && pTargetUnit->getFlagFlying())
            bFlyingTarget = true;

         if (mSurfaceImpactType != -1 && !bFlyingTarget)
         {
            const BProtoImpactEffect* pImpactData = gDatabase.getProtoImpactEffectFromIndex(impactProtoID);
            BTerrainEffect* pTerrainEffect = gTerrainEffectManager.getTerrainEffect(gDatabase.getSurfaceTypeImpactEffect(), true);
            if (pTerrainEffect && pImpactData)
            {
               pTerrainEffect->instantiateEffect(mSurfaceImpactType, mpProtoAction->getImpactEffectSize(), parms.mPosition, parms.mForward, true, parms.mPlayerID, pImpactData->mLifespan, getFlagVisibleToAll(), cVisualDisplayPriorityNormal, NULL);
            }
         }
      }
   }

   //-- Play our impact sound
   if (mSurfaceImpactType != -1 )
   {
      BImpactSoundInfo soundInfo;
      bool result = getProtoObject()->getImpactSoundCue(mSurfaceImpactType, soundInfo);
      if (result && (soundInfo.mSoundCue != cInvalidCueIndex))
         gWorld->getWorldSoundManager()->addSound(mPosition, soundInfo.mSoundCue, true, cInvalidCueIndex, soundInfo.mCheckSoundRadius, true);            
   }

   //-- Rumble controller and shake camera
   if (mpProtoAction)
      mpProtoAction->doImpactRumbleAndCameraShake(BRumbleEvent::cTypeImpact, parms.mPosition, false, mID);
}

//==============================================================================
// BProjectile::initFromProtoObject
//==============================================================================
bool BProjectile::initFromProtoObject(const BProtoObject* pProto, BObjectCreateParms& parms)
{
   mStartingVelocity = pProto->getStartingVelocity();
   mFuel = pProto->getFuel();
   //If we don't have any fuel, we really don't want any acceleration in a projectile
   //given how the code works.
   mAcceleration = (mFuel > cFloatCompareEpsilon) ? pProto->getAcceleration() : 0.0f;
   mTrackingStartTime = gWorld->getGametime() + pProto->getTrackingDelay();
   mTurnRate = pProto->getTurnRate();
   mPerturbOnce = pProto->getFlagPertubOnce();
   if (pProto->getFlagPertubOnce() || (pProto->getPerturbanceChance() > 0.0f))
   {
      mpPerturbanceData = BPerturbanceData::getInstance();
      if (mpPerturbanceData)
      {
         mpPerturbanceData->mPerturbanceChance = pProto->getPerturbanceChance();
         mpPerturbanceData->mPerturbanceVelocity = pProto->getPerturbanceVelocity();
         mpPerturbanceData->mPerturbanceMinTime = pProto->getPerturbanceMinTime();
         mpPerturbanceData->mPerturbanceMaxTime = pProto->getPerturbanceMaxTime();
         mpPerturbanceData->mInitialPerturbanceVelocity = pProto->getInitialPerturbanceVelocity();
         mpPerturbanceData->mInitialPerturbanceMinTime = pProto->getInitialPerturbanceMinTime();
         mpPerturbanceData->mInitialPerturbanceMaxTime = pProto->getInitialPerturbanceMaxTime();
      }
   }
   else
      mpPerturbanceData = NULL;
   mpStickData = NULL;
   mActiveScanChance = pProto->getActiveScanChance();
   mActiveScanRadiusScale = pProto->getActiveScanRadius();

   setFlagIsAffectedByGravity(pProto->getFlagIsAffectedByGravity());                                      
   setFlagTrackingDelay(pProto->getFlagTracking());
   mFlagTestFuel = pProto->getFlagTracking();
   setFlagBeam(pProto->getFlagIsBeam());
   setFlagExplodeOnTimer(pProto->getFlagExplodeOnTimer());    
   setFlagExpireOnTimer(pProto->getFlagExpireOnTimer());    
   setFlagIsSticky(pProto->getFlagIsSticky());       
   setFlagIsNeedler(pProto->getFlagIsNeedler());       
   setFlagSelfDamage(pProto->getFlagSelfDamage());
   mFlagSingleStick = pProto->getFlagSingleStick();
   setFlagHideOnImpact(pProto->getFlagHideOnImpact());

   // init the two extra objects for beams
   if (getFlagBeam())
   {
      mTargetLocation = parms.mForward;
      mBeamHeadID = initBeamFromProtoObject(pProto->getBeamHead(), mPosition);      
      mBeamTailID = initBeamFromProtoObject(pProto->getBeamTail(), mTargetLocation);      
   }

   // if the created by player id is set, use it for the player id, 
   // and the player id for the color x form
   if (parms.mCreatedByPlayerID != cInvalidPlayerID)
   {
      setPlayerID(parms.mCreatedByPlayerID);
      setColorPlayerID(parms.mPlayerID);
   }

   // if we tumble by default, then set us up to tumble
   if (pProto->getFlagProjectileTumbles())
   {
      setFlagTumbling(true);
      initTumbleVector();
   }

   bool retVal = BObject::initFromProtoObject(pProto, parms);

   if (getFlagBeam())
      updateBeamMatrices();

   return retVal;
}

//=============================================================================
//=============================================================================
BEntityID BProjectile::initBeamFromProtoObject(long protoObjectID, BVector pos)
{
   BObjectCreateParms parms;
   parms.mPlayerID = getPlayerID();
   parms.mPosition = pos;
   parms.mRight = cXAxisVector;
   parms.mForward = cZAxisVector;
   parms.mProtoObjectID = protoObjectID;   
   parms.mIgnorePop = true;
   parms.mNoTieToGround = true;
   parms.mPhysicsReplacement = false;
   parms.mStartBuilt=true;

   BEntityID id = cInvalidObjectID;
   BObject* pObject = gWorld->createObject(parms);
   if (pObject)
   {
      id = pObject->getID();
      pObject->setFlagNoWorldUpdate(true);
   }

   return id;
}

//=============================================================================
// BProjectile::updateVisibleLists
//=============================================================================
void BProjectile::updateVisibleLists(void)
{
   DWORD newVisibility = getNewVisibility();
   BObject *pObject = gWorld->getObject(mTargetObjectID);
   if (pObject)
      newVisibility |= (0x10001 << pObject->getTeamID()); // Force projectiles to always be visible to the team we're attacking
   updateVisibility(newVisibility);

   const BUser * const user = gUserManager.getUser(BUserManager::cPrimaryUser);

   //-- reveal minimap
   if (getTeamID() == user->getTeamID())
   {
      float los = getLOS();
      if (los > cFloatCompareEpsilon)
      {
         //if (!gConfig.isDefined(cConfigFlashGameUI))
         //   gMiniMap.reveal(getPosition(), los);
         //else
         gUIManager->revealMinimap(getPosition(), los);
      }
   }
}

//=============================================================================
// void BProjectile::getBeamLaunchPosition
//=============================================================================
void BProjectile::getBeamLaunchPosition(BVector* position, BMatrix* pMatrix) const
{
   if (getParentID() == cInvalidObjectID)
      return;

   BUnit* pUnit = gWorld->getUnit(getParentID());
   if (!pUnit)
      return;

         
   if (mBoneHandle!=-1)
   {
      if (mAttachmentHandle!=-1)
      {
         BMatrix attachmentMat;
         
//-- FIXING PREFIX BUG ID 1640
         const BVisualItem* pVisualItem=pUnit->getVisual()->getAttachment(mAttachmentHandle, &attachmentMat);
//--
         if (pVisualItem)
         {
            BMatrix unitMat;
            pUnit->getWorldMatrix(unitMat);

            BMatrix mat;
            mat.mult(attachmentMat, unitMat);

            if (pVisualItem->getBone(mBoneHandle, position, NULL, NULL, &mat))
            {
               if (pMatrix)
                  *pMatrix = mat;
               return;
            }
         }
      }
      else
      {
//-- FIXING PREFIX BUG ID 1641
         const BVisualItem* pVisualItem=pUnit->getVisual();
//--
         if (pVisualItem)
         {
            BMatrix mat;
            pUnit->getWorldMatrix(mat);
            if (pVisualItem->getBone(mBoneHandle, position, NULL, NULL, &mat))
            {
               if (pMatrix)
                  *pMatrix = mat;
               return;
            }
         }
      }
   }

   if (mLaunchPointHandle != -1)
   {  
//-- FIXING PREFIX BUG ID 1642
      const BVisual* pVisual=pUnit->getVisual();
//--
      if (pVisual)
      {
         BVector launchPoint;
         if (pVisual->getPointPosition(cActionAnimationTrack, mLaunchPointHandle, launchPoint))
         {
            BMatrix mat;
            pUnit->getWorldMatrix(mat);
            BVector foo;
            mat.transformVectorAsPoint(launchPoint, foo);
            *position=foo;
            if (pMatrix)
               pMatrix->makeIdentity();
            return;
         }
      }
   }

   // FIXME AJL 9/18/06 - Visual center is going out of sync too, so disabling until we fix it.
   //pos = pUnit->getVisualCenter();
//    *position= pUnit->getPosition();
//    position->y+=pUnit->getProtoObject()->getObstructionRadiusY();   

   // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
   if(getFlagStaticPosition() == false)
      *position = pUnit->getSimCenter();

   if (pMatrix)
      pMatrix->makeIdentity();
}

//=============================================================================
//=============================================================================
bool BProjectile::stickToTarget(BVector startPos, BUnit* pUnit)
{
   if (!pUnit)
      return false;

   if (!getFlagIsSticky())
      return false;

   if (mFlagSingleStick && pUnit->getFirstEntityRefByData1(BEntityRef::cTypeStickedProjectile, getProtoID()) != NULL)
      return false;

   BVisual* pVisual = pUnit->getVisual();
   if (!pVisual)
      return false;

   BMatrix unitWorldMat;
   pUnit->getWorldMatrix(unitWorldMat);

   BVector trajectory = pUnit->getSimCenter() - startPos;

   float intersectDistSqr = 0.0f;
   long intersectAttachmentHandle = -1;
   long intersectBoneHandle = -1;
   BVector intersectBoneSpacePos;
   BVector intersectBoneSpaceDir;

   if (pVisual && pVisual->raySegmentIntersects(startPos, trajectory, true, unitWorldMat, NULL, intersectDistSqr, intersectAttachmentHandle, intersectBoneHandle, intersectBoneSpacePos, intersectBoneSpaceDir))
   {
      mMotionState = cStateSticked;
      if (!mpStickData)
      {
         mpStickData = BStickData::getInstance();
         BASSERT(mpStickData);
      }

      if (mpStickData)
      {
         mpStickData->mStickedToUnitID = pUnit->getID();
         mpStickData->mStickedToAttachmentHandle = intersectAttachmentHandle;
         mpStickData->mStickedToBoneHandle = intersectBoneHandle;
         mpStickData->mStickedModelSpacePosition = intersectBoneSpacePos;
         mpStickData->mStickedModelSpaceDirection = intersectBoneSpaceDir;
         if (mFlagSingleStick)
         {
            pUnit->addEntityRef(BEntityRef::cTypeStickedProjectile, getID(), getProtoID(), 0);
            mFlagAddedStickRef=true;
         }
      }
      return true;
   }

   return false;
}

//==============================================================================
// BProjectile::notify
//==============================================================================

void BProjectile::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   BObject::notify(eventType, senderID, data, data2);

   switch (eventType)
   {
   case cEventTargetDazed:
      BEntityID target = (BEntityID)data;
      BUnit* pUnit = gWorld->getUnit(mTargetObjectID);

      syncUnitActionData("BProjectile::notify mID", mID.asLong());
      syncUnitActionData("BProjectile::notify senderID", senderID.asLong());
      syncUnitActionData("BProjectile::notify mTargetObjectID", mTargetObjectID.asLong());

      if (pUnit)
      {
         syncUnitActionCode("BProjectile::notify pUnit");
//-- FIXING PREFIX BUG ID 1645
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         if (pSquad)
         {          
            syncUnitActionCode("BProjectile::notify pSquad");
            if (target == pSquad->getID())
            {
               syncUnitActionCode("BProjectile::notify mFlagTargetAlreadyDazed");
               mFlagTargetAlreadyDazed = true;
            }
         }
      }
      break;
   }
}

//==============================================================================
// BProjectile::initTumble
//==============================================================================
void BProjectile::initTumbleVector()
{
   const float tumblingScale = gDatabase.getProjectileTumbleRate();
   float x = ((getRandDistribution(cSimRand) * 2.0f) - 1.0f) * tumblingScale;
   float y = ((getRandDistribution(cSimRand) * 2.0f) - 1.0f) * tumblingScale;
   float z = ((getRandDistribution(cSimRand) * 2.0f) - 1.0f) * tumblingScale;
   mTumblingVector.set(x, y, z);
}


//==============================================================================
// BProjectile::deflect
//==============================================================================
void BProjectile::deflect()
{   
   // Adjust velocity vector (randomly upward)
   float currentSpeed = mVelocity.length();   
   //-- Calc deflection dir
   BUnit* pUnit = gWorld->getUnit(mDeflectingUnitID);
   BVector awayDir = getPosition() - pUnit->getVisualCenter();
   awayDir.y = 0;
   awayDir.normalize();

   //-- Generate a random left right of 22 degrees
   awayDir.rotateXZ(getRandRangeFloat(cSimRand, -cPiOver8, cPiOver8));

   //-- Generate a random up/down
   awayDir.y = getRandRangeFloat(cSimRand, -1, 1);  
   awayDir.safeNormalize();

   mVelocity = awayDir * currentSpeed;

   //-- Set the projectile to face its velocity dir
   mForward = awayDir;
   mForward.normalize();
   calcRight();
   calcUp();

   // Play a "deflect" sound
   BCueIndex deflectSound = gSoundManager.getCueIndexByEnum(BSoundManager::cSoundDeflect);
   gWorld->getWorldSoundManager()->addSound(this, -1, deflectSound, true, cInvalidCueIndex, true, true);
   
   //setFlagIsAffectedByGravity(true);
   setFlagTracking(false);
   setFlagTrackingDelay(false);
   setFlagPerturbed(false);
   mFuel = 0.0f;
   mAcceleration = 0.0f;
   if (mpPerturbanceData)
      mpPerturbanceData->mPerturbanceChance = 0.0f;
   mFlagTestFuel = false;
   //initTumbleVector();
   mFlagDeflected = true;
}

//=============================================================================
//=============================================================================
bool BProjectile::fastCheckHitTerrain(const BVector& startPt, const BVector& endPt, BVector& intersectionPt, bool onlyHeightApproximation)
{
   // VAT: 11/15/08: This gets called A LOT - early out if at all possible
   // since a seg intersect with the terrain sim rep is damn expensive
   if (startPt.distanceSqr(endPt) < scTerrainSegCheckEarlyOutMaxDistanceSqr)
   {
      // check to see if both points are sufficiently above or below the terrain. 
      // if so, we don't need to do the more expensive seg intersect check 
      float startPtHeight = 0.0f;
      float endPtHeight = 0.0f;
      bool startPtAboveTerrain = (gTerrainSimRep.getHeightRaycast(startPt, startPtHeight, true) && ((startPt.y - startPtHeight) > scTerrainSegCheckEarlyOutMinHeight));
      bool endPtAboveTerrain = (gTerrainSimRep.getHeightRaycast(endPt, endPtHeight, true) && ((endPt.y - endPtHeight) > scTerrainSegCheckEarlyOutMinHeight));

      // both sufficiently above the terrain, no hit
      if (startPtAboveTerrain && endPtAboveTerrain)
         return false;

      // both below the terrain, return the end pt height as the intersection point
      if (!startPtAboveTerrain && !endPtAboveTerrain)
      {
         intersectionPt = endPt;
         intersectionPt.y = endPtHeight;
         return true;
      }
   }

   // if this check requested only height checks, return true
   // as a recommendation to perform a more detailed check
   if (onlyHeightApproximation)
   {
      intersectionPt = endPt;
      return true;
   }

   return gTerrainSimRep.segmentIntersects(startPt, endPt, intersectionPt);
}

//=============================================================================
//=============================================================================
bool BProjectile::save(BStream* pStream, int saveType) const
{
   if (!BObject::save(pStream, saveType))
      return false;

   GFWRITEVECTOR(pStream, mTumblingVector);
   GFWRITEVECTOR(pStream, mTargetOffset);
   GFWRITEVECTOR(pStream, mInitialProjectilePosition);
   GFWRITEVECTOR(pStream, mInitialTargetPosition);
   GFWRITEVECTOR(pStream, mTargetLocation);
   GFWRITEVAR(pStream, BEntityID, mTargetObjectID);
   GFWRITEVAR(pStream, BEntityID, mBeamHeadID);
   GFWRITEVAR(pStream, BEntityID, mBeamTailID);
   GFWRITEVAR(pStream, BEntityID, mDeflectingUnitID);
   GFWRITEVAR(pStream, BEntityID, mUnitHitID);
   GFWRITEVAR(pStream, float, mDamage);
   GFWRITEPROTOACTIONPTR(pStream, (BProtoAction*)mpDamageInfo);
   GFWRITEPROTOACTIONPTR(pStream, mpProtoAction);
   GFWRITEVAR(pStream, float, mGravity);
   GFWRITEVAR(pStream, float, mStartingVelocity);
   GFWRITEVAR(pStream, float, mFuel);
   GFWRITEVAR(pStream, float, mAcceleration);
   GFWRITEVAR(pStream, DWORD, mTrackingStartTime);
   GFWRITEVAR(pStream, float, mTurnRate);
   GFWRITEFREELISTITEMPTR(pStream, BPerturbanceData, mpPerturbanceData);
   GFWRITEFREELISTITEMPTR(pStream, BStickData, mpStickData);
   GFWRITEVAR(pStream, float, mActiveScanChance);
   GFWRITEVAR(pStream, float, mActiveScanRadiusScale);
   GFWRITEVAR(pStream, float, mFollowGroundHeight);
   GFWRITEVAR(pStream, long, mHitZoneIndex);
   GFWRITEVAR(pStream, long, mLaunchPointHandle);
   GFWRITEVAR(pStream, long, mAttachmentHandle);
   GFWRITEVAR(pStream, long, mBoneHandle);
   GFWRITEVAR(pStream, long, mMotionState);
   GFWRITEVAR(pStream, BPowerID, mOwningPowerID);
   GFWRITEVAR(pStream, byte, mSurfaceImpactType);
   GFWRITEVAR(pStream, byte, mBeamDecalCreateStall);
   GFWRITEVAR(pStream, float, mApplicationTimer);
   GFWRITEVAR(pStream, float, mReapplicationTimer);
   GFWRITEBITBOOL(pStream, mFlagIsAffectedByGravity);
   GFWRITEBITBOOL(pStream, mFlagTrackingDelay);
   GFWRITEBITBOOL(pStream, mFlagTracking);
   GFWRITEBITBOOL(pStream, mFlagPerturbed);
   GFWRITEBITBOOL(pStream, mFlagTumbling);
   GFWRITEBITBOOL(pStream, mFlagDoLogging);
   GFWRITEBITBOOL(pStream, mFlagBeam);
   GFWRITEBITBOOL(pStream, mPerturbOnce);
   GFWRITEBITBOOL(pStream, mFlagClearedLauncher);
   GFWRITEBITBOOL(pStream, mFlagInterceptDist);
   GFWRITEBITBOOL(pStream, mFlagTestFuel);
   GFWRITEBITBOOL(pStream, mFlagCollidesWithAllUnits);
   GFWRITEBITBOOL(pStream, mFlagExplodeOnTimer);
   GFWRITEBITBOOL(pStream, mFlagExpireOnTimer);
   GFWRITEBITBOOL(pStream, mFlagIsSticky);
   GFWRITEBITBOOL(pStream, mFlagDetonate);
   GFWRITEBITBOOL(pStream, mFlagIsNeedler);
   GFWRITEBITBOOL(pStream, mFlagSingleStick);
   GFWRITEBITBOOL(pStream, mFlagAddedStickRef);
   GFWRITEBITBOOL(pStream, mFlagTargetAlreadyDazed);
   GFWRITEBITBOOL(pStream, mFlagCheckedForDeflect);
   GFWRITEBITBOOL(pStream, mFlagDeflected);
   GFWRITEBITBOOL(pStream, mFlagSelfDamage);
   GFWRITEBITBOOL(pStream, mFlagAirBurst);
   GFWRITEBITBOOL(pStream, mFlagHideOnImpact);
   GFWRITEBITBOOL(pStream, mFlagStaticPosition);
   GFWRITEBITBOOL(pStream, mFlagFromLeaderPower);
   GFWRITEBITBOOL(pStream, mFlagIngoreTargetCollisions);
   GFWRITEBITBOOL(pStream, mFlagBeamCollideWithUnits);
   GFWRITEBITBOOL(pStream, mFlagBeamCollideWithTerrain);

   return true;
}

//=============================================================================
//=============================================================================
bool BProjectile::load(BStream* pStream, int saveType)
{
   if (!BObject::load(pStream, saveType))
      return false;

   GFREADVECTOR(pStream, mTumblingVector);
   GFREADVECTOR(pStream, mTargetOffset);
   GFREADVECTOR(pStream, mInitialProjectilePosition);
   GFREADVECTOR(pStream, mInitialTargetPosition);
   GFREADVECTOR(pStream, mTargetLocation);
   GFREADVAR(pStream, BEntityID, mTargetObjectID);
   if (BProjectile::mGameFileVersion < 3 || BProjectile::mGameFileVersion >= 4)
   {
      GFREADVAR(pStream, BEntityID, mBeamHeadID);
      GFREADVAR(pStream, BEntityID, mBeamTailID);
   }
   GFREADVAR(pStream, BEntityID, mDeflectingUnitID);
   GFREADVAR(pStream, BEntityID, mUnitHitID);
   GFREADVAR(pStream, float, mDamage);

   BProtoAction* pDamageInfo = NULL;
   GFREADPROTOACTIONPTR(pStream, pDamageInfo);
   mpDamageInfo = pDamageInfo;

   BProtoAction* pProtoAction = NULL;
   GFREADPROTOACTIONPTR(pStream, pProtoAction);
   mpProtoAction = pProtoAction;

   GFREADVAR(pStream, float, mGravity);
   GFREADVAR(pStream, float, mStartingVelocity);
   GFREADVAR(pStream, float, mFuel);
   GFREADVAR(pStream, float, mAcceleration);
   GFREADVAR(pStream, DWORD, mTrackingStartTime);
   GFREADVAR(pStream, float, mTurnRate);
   GFREADFREELISTITEMPTR(pStream, BPerturbanceData, mpPerturbanceData);
   GFREADFREELISTITEMPTR(pStream, BStickData, mpStickData);
   GFREADVAR(pStream, float, mActiveScanChance);
   GFREADVAR(pStream, float, mActiveScanRadiusScale);
   GFREADVAR(pStream, float, mFollowGroundHeight);
   GFREADVAR(pStream, long, mHitZoneIndex);
   GFREADVAR(pStream, long, mLaunchPointHandle);
   GFREADVAR(pStream, long, mAttachmentHandle);
   GFREADVAR(pStream, long, mBoneHandle);
   GFREADVAR(pStream, long, mMotionState);
   GFREADVAR(pStream, BPowerID, mOwningPowerID);
   GFREADVAR(pStream, byte, mSurfaceImpactType);
   GFREADVAR(pStream, byte, mBeamDecalCreateStall);
   GFREADVAR(pStream, float, mApplicationTimer);
   GFREADVAR(pStream, float, mReapplicationTimer);
   GFREADBITBOOL(pStream, mFlagIsAffectedByGravity);
   GFREADBITBOOL(pStream, mFlagTrackingDelay);
   GFREADBITBOOL(pStream, mFlagTracking);
   GFREADBITBOOL(pStream, mFlagPerturbed);
   GFREADBITBOOL(pStream, mFlagTumbling);
   GFREADBITBOOL(pStream, mFlagDoLogging);
   GFREADBITBOOL(pStream, mFlagBeam);
   GFREADBITBOOL(pStream, mPerturbOnce);
   GFREADBITBOOL(pStream, mFlagClearedLauncher);
   GFREADBITBOOL(pStream, mFlagInterceptDist);
   GFREADBITBOOL(pStream, mFlagTestFuel);
   GFREADBITBOOL(pStream, mFlagCollidesWithAllUnits);
   GFREADBITBOOL(pStream, mFlagExplodeOnTimer);
   GFREADBITBOOL(pStream, mFlagExpireOnTimer);
   GFREADBITBOOL(pStream, mFlagIsSticky);
   GFREADBITBOOL(pStream, mFlagDetonate);
   GFREADBITBOOL(pStream, mFlagIsNeedler);
   GFREADBITBOOL(pStream, mFlagSingleStick);
   GFREADBITBOOL(pStream, mFlagAddedStickRef);
   GFREADBITBOOL(pStream, mFlagTargetAlreadyDazed);
   GFREADBITBOOL(pStream, mFlagCheckedForDeflect);
   GFREADBITBOOL(pStream, mFlagDeflected);
   GFREADBITBOOL(pStream, mFlagSelfDamage);
   GFREADBITBOOL(pStream, mFlagAirBurst);
   GFREADBITBOOL(pStream, mFlagHideOnImpact);
   GFREADBITBOOL(pStream, mFlagStaticPosition);
   GFREADBITBOOL(pStream, mFlagFromLeaderPower);
   GFREADBITBOOL(pStream, mFlagIngoreTargetCollisions);
   if (BProjectile::mGameFileVersion > 1)
   {
      GFREADBITBOOL(pStream, mFlagBeamCollideWithUnits);
      GFREADBITBOOL(pStream, mFlagBeamCollideWithTerrain);
   }

   return true;
}

//=============================================================================
//=============================================================================
bool BProjectile::postLoad(int saveType)
{
   /*
   if (getFlagBeam())
   {
      mBeamHeadID = initBeamFromProtoObject(getProtoObject()->getBeamHead(), mPosition);      
      mBeamTailID = initBeamFromProtoObject(getProtoObject()->getBeamTail(), mTargetLocation);      
   }
   */
   return BObject::postLoad(saveType);
}

//=============================================================================
//=============================================================================
bool BPerturbanceData::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPerturbanceVector);
   GFWRITEVAR(pStream, float, mPerturbanceChance);
   GFWRITEVAR(pStream, float, mPerturbanceVelocity);
   GFWRITEVAR(pStream, float, mPerturbanceMinTime);
   GFWRITEVAR(pStream, float, mPerturbanceMaxTime);
   GFWRITEVAR(pStream, float, mPerturbanceDuration);
   GFWRITEVAR(pStream, float, mPerturbanceTimer);
   GFWRITEVAR(pStream, float, mInitialPerturbanceVelocity);
   GFWRITEVAR(pStream, float, mInitialPerturbanceMinTime);
   GFWRITEVAR(pStream, float, mInitialPerturbanceMaxTime);
   return true;
}

//=============================================================================
//=============================================================================
bool BPerturbanceData::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mPerturbanceVector);
   GFREADVAR(pStream, float, mPerturbanceChance);
   GFREADVAR(pStream, float, mPerturbanceVelocity);
   GFREADVAR(pStream, float, mPerturbanceMinTime);
   GFREADVAR(pStream, float, mPerturbanceMaxTime);
   GFREADVAR(pStream, float, mPerturbanceDuration);
   GFREADVAR(pStream, float, mPerturbanceTimer);
   GFREADVAR(pStream, float, mInitialPerturbanceVelocity);
   GFREADVAR(pStream, float, mInitialPerturbanceMinTime);
   GFREADVAR(pStream, float, mInitialPerturbanceMaxTime);
   return true;
}

//=============================================================================
//=============================================================================
bool BStickData::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mStickedModelSpacePosition);
   GFWRITEVECTOR(pStream, mStickedModelSpaceDirection);
   GFWRITEVAR(pStream, BEntityID, mStickedToUnitID);
   GFWRITEVAR(pStream, long, mStickedToAttachmentHandle);
   GFWRITEVAR(pStream, long, mStickedToBoneHandle);
   return true;
}

//=============================================================================
//=============================================================================
bool BStickData::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mStickedModelSpacePosition);
   GFREADVECTOR(pStream, mStickedModelSpaceDirection);
   GFREADVAR(pStream, BEntityID, mStickedToUnitID);
   GFREADVAR(pStream, long, mStickedToAttachmentHandle);
   GFREADVAR(pStream, long, mStickedToBoneHandle);
   return true;
}

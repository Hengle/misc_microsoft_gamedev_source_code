//==============================================================================
// physicsobject.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "physicsobject.h"
#include "physicsworld.h"
#include "physicsobjectblueprint.h"
#include "physics.h"
#include "..\xgameRender\debugprimitives.h"
//#include "renderer.h"
#include "shape.h"
#include "constraint.h"
#include "physicslistener.h"
#include "..\xgame\syncmacros.h"

//==============================================================================
// Statics
BPointerList<BPhysicsObject> BPhysicsObject::mPhysicsObjects;

//==============================================================================
// Defines
const float cFixedUpdate = 0.0125f;

#if 0
//==============================================================================
// BPhysicsObject::BPhysicsObject
//==============================================================================
BPhysicsObject::BPhysicsObject(BPhysicsWorld *world) : 
   mWorld(world),
   mphkRigidBody(NULL), 
   mbDeleteRigidBodyOnDestruction(true),
   mpUserData(NULL),
   mShapeID(-1),
   mInfoID(-1),
   mbRegisteredForCollisionCallback(false),
   mbFixed(false),
   mbBreakable(false),
   mbLoadedKeyframed(false)
{
   mListHandle = mPhysicsObjects.addToTail(this);
} 

//==============================================================================
// BPhysicsObject::BPhysicsObject
//==============================================================================
BPhysicsObject::BPhysicsObject(BPhysicsWorld *world, hkpRigidBody *phkRigidBody, bool deleteOnDestruction /*=true*/) : 
   mWorld(world),
   mbDeleteRigidBodyOnDestruction(deleteOnDestruction),
   mpUserData(NULL),
   mShapeID(-1),
   mInfoID(-1),
   mbRegisteredForCollisionCallback(false),
   mbFixed(false),
   mbBreakable(false),
   mbLoadedKeyframed(false)
{
   BASSERT(phkRigidBody != NULL);
   mphkRigidBody = NULL;
   setRigidBody(phkRigidBody);
   mListHandle = mPhysicsObjects.addToTail(this);
   
} 
#endif

//==============================================================================
// BPhysicsObject::BPhysicsObject
//==============================================================================
BPhysicsObject::BPhysicsObject(BPhysicsWorld *world, const BPhysicsObjectBlueprint &blueprint, const BVector &position, const BVector &centerOffset, const BPhysicsMatrix &rotation, 
                              DWORD userdata, bool fixed, const BPhysicsObjectBlueprintOverrides *pOverrides /*=NULL*/) : 
   mCenterOffset(0.0f, 0.0f, 0.0f),
   mCenterOfMassOffset(0.0f, 0.0f, 0.0f),
   mShapeHalfExtents(0.5f, 0.5f, 0.5f),
   mWorld(world),
   mphkRigidBody(NULL), 
   mbDeleteRigidBodyOnDestruction(true),
   mpUserData(NULL),
   mShapeID(-1),
   mInfoID(-1),
   mbRegisteredForCollisionCallback(false),
   mbFixed(false),
   mbBreakable(false),
   mbLoadedKeyframed(false)
{
   BPhysicsObjectParams params;

   long shapeID = -1;

   // Get shape.
   if (pOverrides && pOverrides->getFlag(BPhysicsObjectBlueprintOverrides::cFlagValidShape))
   {
      shapeID = pOverrides->getShapeID();
   }
   else
   {
      shapeID = blueprint.getShape();
   }

   hkpShape *pHavokShape = NULL;
   
   BShape *shape = gPhysics->getShapeManager().get(shapeID);
   //--
   //-- if we don't have a valid shape, then we can try to create a simple box around the shape
   //--
   bool newShapeCreated = false;
   if(!shape || !shape->getHavokShape())
   {
      if (  pOverrides && 
            pOverrides->getFlag(BPhysicsObjectBlueprintOverrides::cFlagValidHalfExtents) && 
            (pOverrides->getHalfExtents().length() > cFloatCompareEpsilon))
      {
         //-- is this specified in the override
         BShape shape;
         shape.allocateBox(pOverrides->getHalfExtents());
         shape.addReference();
         pHavokShape = shape.getHavokShape();
         params.shapeHalfExtents = pOverrides->getHalfExtents();
         newShapeCreated = true;
      }
      else if (blueprint.getHalfExtents().length() > cFloatCompareEpsilon)
      {
         //-- perhaps it is specified in the blueprint itself
         BShape shape;
         shape.allocateBox(blueprint.getHalfExtents());
         shape.addReference();
         pHavokShape = shape.getHavokShape();
         params.shapeHalfExtents = blueprint.getHalfExtents();
         newShapeCreated = true;
      }
      else
      {
         //-- ok, we are going to give up
         BASSERT(false);
      }
   }
   else
   {
      //-- we had a valid shape ID passed in, so this can just be used
      //-- to get our Havok shape.
      pHavokShape = shape->getHavokShape();
   }
   //--
   //--

   mShapeID = shapeID;

   
   //-- basic motion properties
   params.position = position;
   params.rotation = rotation;
   params.centerOffset = centerOffset;
   params.centerOfMassOffset  = blueprint.getCenterOfMassOffset();
   params.pHavokShape = pHavokShape;
   params.restitution = blueprint.getRestitution();
   params.friction = blueprint.getFriction();
   params.angularDamping = blueprint.getAngularDamping();
   params.linearDamping = blueprint.getLinearDamping();
   params.mass = blueprint.getMass();

   //-- finally we set the collision filter layer
   if (pOverrides && pOverrides->getFlag(BPhysicsObjectBlueprintOverrides::cFlagValidCollisionFilter))
   {
      params.collisionFilterInfo = pOverrides->getCollisionFilterInfo();
   }
   else
   {
      params.collisionFilterInfo = mWorld->createCollisionFilterInfo();
   }

   params.userdata = userdata;
   params.breakable = false;
   params.fixed = fixed;

   setupObjectRepresentation(params);

   // If new hkShape was created here, then remove reference since the rigid body now holds a reference.
   if (newShapeCreated)
      pHavokShape->removeReference();

   mListHandle = mPhysicsObjects.addToTail(this);
}


//==============================================================================
// BPhysicsObject::BPhysicsObject
//==============================================================================
BPhysicsObject::BPhysicsObject(BPhysicsWorld *world, const BPhysicsObjectParams &params) : 
   mCenterOffset(0.0f, 0.0f, 0.0f),
   mCenterOfMassOffset(0.0f, 0.0f, 0.0f),
   mShapeHalfExtents(0.5f, 0.5f, 0.5f),
   mWorld(world),
   mphkRigidBody(NULL), 
   mbDeleteRigidBodyOnDestruction(true),
   mpUserData(NULL),
   mShapeID(-1),
   mInfoID(-1),
   mbRegisteredForCollisionCallback(false),
   mbFixed(false),
   mbBreakable(false),
   mbLoadedKeyframed(false)
{
   setupObjectRepresentation(params);

   mListHandle = mPhysicsObjects.addToTail(this);
}

//==============================================================================
// BPhysicsObject::BPhysicsObject
//==============================================================================
BPhysicsObject::BPhysicsObject(BPhysicsWorld *world) : 
   mCenterOffset(0.0f, 0.0f, 0.0f),
   mCenterOfMassOffset(0.0f, 0.0f, 0.0f),
   mShapeHalfExtents(0.5f, 0.5f, 0.5f),
   mWorld(world),
   mphkRigidBody(NULL), 
   mbDeleteRigidBodyOnDestruction(true),
   mpUserData(NULL),
   mShapeID(-1),
   mInfoID(-1),
   mbRegisteredForCollisionCallback(false),
   mbFixed(false),
   mbBreakable(false),
   mbLoadedKeyframed(false)
{
   mListHandle = mPhysicsObjects.addToTail(this);
}

//==============================================================================
// BPhysicsObject::setupObjectRepresentation
//==============================================================================
void BPhysicsObject::setupObjectRepresentation(const BPhysicsObjectParams &params, bool fromSave)
{
#ifdef SYNC_Unit
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation angularDamping", params.angularDamping);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation centerOffset", params.centerOffset);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation centerOfMassOffset", params.centerOfMassOffset);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation friction", params.friction);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation linearDamping", params.linearDamping);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation mass", params.mass);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation position", params.position);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation restitution", params.restitution);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation forward", params.rotation.getForward());
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation up", params.rotation.getUp());
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation right", params.rotation.getRight());
   //syncUnitActionData("BPhysicsObject::setupObjectRepresentation collisionFilterInfo", params.collisionFilterInfo);
   //syncUnitActionData("BPhysicsObject::setupObjectRepresentation userData", params.userdata);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation breakable", params.breakable);
   syncUnitActionData("BPhysicsObject::setupObjectRepresentation fixed", params.fixed);
#endif

   mCenterOffset = params.centerOffset;
   mCenterOfMassOffset = params.centerOfMassOffset;

   hkpRigidBodyCinfo info;

   info.m_shape = params.pHavokShape;

   //-- get the orientation and set it up
   //-- convert to Havok space along the way
   //-- convert to quaternion and normalize it
   //CLM[12.06.07] hkRot can become QNAN here..
   hkRotation hkRot;
   BPhysics::convertRotation(params.rotation, hkRot);
   if(!hkRot.isOk())
   {
      hkRot.setIdentity();
   }
   info.m_rotation.set(hkRot);
   info.m_rotation.normalize();
   
   

   //-- basic motion properties
   info.m_restitution = params.restitution;
   info.m_friction = params.friction;
   info.m_angularDamping = params.angularDamping;
   info.m_linearDamping = params.linearDamping;

   mbFixed = params.fixed;
   if (params.fixed)
      info.m_motionType = hkpMotion::MOTION_FIXED;
   else
      info.m_motionType = hkpMotion::MOTION_BOX_INERTIA;

   hkReal tempMass = Math::Max(params.mass, 1.0f);

   hkpMassProperties massProperties;
   hkpInertiaTensorComputer::computeShapeVolumeMassProperties(info.m_shape, tempMass, massProperties);
   
   BASSERT(massProperties.m_mass != 0.0f);

   info.m_inertiaTensor = massProperties.m_inertiaTensor;
   info.m_centerOfMass = massProperties.m_centerOfMass;
   info.m_mass = massProperties.m_mass;

   //-- adjust for our center of mass offset
   const BVector &massOffset = params.centerOfMassOffset;
   info.m_centerOfMass(0) += massOffset.x;
   info.m_centerOfMass(1) += massOffset.y;
   info.m_centerOfMass(2) += massOffset.z;

   mbBreakable = params.breakable;
   if(params.breakable)
   {
      //-- Need to only do this for breakable objects only, not all
      info.m_qualityType = HK_COLLIDABLE_QUALITY_DEBRIS;
	   info.m_numUserDatasInContactPointProperties = 1;
   }

   //-- create the bounding volume shape
#pragma push_macro("new")
#undef new
   hkpRigidBody *phkRigidBody = new hkpRigidBody(info);
#pragma pop_macro("new")

   //-- now we setup the physics object
   setRigidBody(phkRigidBody);
   setDeleteRigidBodyOnDestruction(true);
   if (!fromSave)
   {
      setPosition(params.position);
      setRotation(params.rotation);
   }
   else
   {
      BPhysicsObject::setPosition(params.position);
      BPhysicsObject::setRotation(params.rotation);
   }

   //-- now we do some common code to set properties and user data
   setProperty(BPhysicsWorld::cPropertyEntityReference, params.userdata);
   setUserData((void*)params.userdata);

   //-- finally we set the collision filter layer
   if (!fromSave)
      setCollisionFilterInfo(params.collisionFilterInfo);
   else
      BPhysicsObject::setCollisionFilterInfo(params.collisionFilterInfo);

   //-- just initialize these special ragdoll properties
   setProperty(BPhysicsWorld::cPropertyRagdollBoneIndex, DWORD(-1));
   setProperty(BPhysicsWorld::cPropertyTerrainNode, DWORD(-1));

   mShapeHalfExtents = params.shapeHalfExtents;
}






//==============================================================================
// BPhysicsObject::~BPhysicsObject
//==============================================================================
BPhysicsObject::~BPhysicsObject(void)
{
   clearAllCollisionListeners();

   if (mphkRigidBody)
   {
      if (mbDeleteRigidBodyOnDestruction)
      {
         removeFromWorld();
         mphkRigidBody->removeReference();
      }

      mphkRigidBody = NULL;
   }

   if (mListHandle != NULL)
      mPhysicsObjects.remove(mListHandle);
} 

//==============================================================================
// BPhysicsObject::BPhysicsObject
//==============================================================================
void BPhysicsObject::setRigidBody(hkpRigidBody *phkRigidBody)
{
   if (!phkRigidBody)
      return;

   if (mphkRigidBody)
   {
      removeFromWorld();

      if (mbDeleteRigidBodyOnDestruction)
      {
         mphkRigidBody->removeReference();
      }

      mphkRigidBody = NULL;

   }

   mphkRigidBody = phkRigidBody;  

  
  
}

//==============================================================================
// BPhysicsObject::addToWorld
//==============================================================================
bool BPhysicsObject::addToWorld( void )
{
   BASSERT(mphkRigidBody != NULL);
   mWorld->getHavokWorld()->addEntity(mphkRigidBody);

   if (mbDeleteRigidBodyOnDestruction)
   {
      if (mphkRigidBody->getEntityActivationListeners().indexOf(this) == -1)
         mphkRigidBody->addEntityActivationListener(this);
   }

   return (true);
}

//==============================================================================
// BPhysicsObject::removeFromWorld
//==============================================================================
bool BPhysicsObject::removeFromWorld( void )
{
   if (!isInWorld())
      return (false);

   BASSERT(mphkRigidBody != NULL);
   if (mbRegisteredForCollisionCallback)
   {
      removeHavokCollisionListener(this);
   }

   //-- remove our entity listener
   if (mphkRigidBody->getEntityActivationListeners().indexOf(this) != -1)
      mphkRigidBody->removeEntityActivationListener(this);

   mbRegisteredForCollisionCallback = false;

   mWorld->getHavokWorld()->removeEntity(mphkRigidBody);

   return true;
}

//==============================================================================
// BPhysicsObject::setDeleteRigidBodyOnDestruction
//==============================================================================
void BPhysicsObject::setDeleteRigidBodyOnDestruction(bool flag)
{
   mbDeleteRigidBodyOnDestruction = flag;

   if (!flag && mphkRigidBody)
   { 
      if (mphkRigidBody->getEntityActivationListeners().indexOf(this) != -1)
         mphkRigidBody->removeEntityActivationListener(this);
   }
}

//==============================================================================
// BPhysicsObject::getPosition
//==============================================================================
void BPhysicsObject::getPosition(BVector &pos) const
{
   BASSERT(mphkRigidBody != NULL);

   BPhysics::convertPoint(mphkRigidBody->getPosition(), pos); 
}

//==============================================================================
// BPhysicsObject::getRotation
//==============================================================================
void BPhysicsObject::getRotation(BPhysicsMatrix &rot) const
{
   BASSERT(mphkRigidBody != NULL);
   BPhysics::convertRotation(mphkRigidBody->getRotation(), rot);
}


//==============================================================================
// BPhysicsObject::setPosition
//==============================================================================
void BPhysicsObject::setPosition(const BVector &pos)
{
   BASSERT(mphkRigidBody != NULL);

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::setPosition pos", pos);
   #endif

   BPhysicsMatrix rot;
   getRotation(rot);
   BVector objectOffset;
   rot.transformPoint(mCenterOffset, objectOffset);

   hkVector4 hkPos;
   BPhysics::convertPoint(objectOffset + pos, hkPos);
   mphkRigidBody->setPosition(hkPos);
}

//==============================================================================
// BPhysicsObject::setRotation
//==============================================================================
void BPhysicsObject::setRotation(const BPhysicsMatrix &rot)
{
   BASSERT(mphkRigidBody != NULL);

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::setRotation forward", rot.getForward());
      syncUnitActionData("BPhysicsObject::setRotation up", rot.getUp());
      syncUnitActionData("BPhysicsObject::setRotation right", rot.getRight());
   #endif

   // Make sure we're using a matrix with rotation only
   BPhysicsMatrix rotOnly(rot);
   rotOnly.clearTranslation();

   // Calc center
   hkVector4 pos = mphkRigidBody->getPosition();
   BPhysicsMatrix oldRot;
   getRotation(oldRot);
   BVector objectOffset;
   oldRot.transformPoint(mCenterOffset, objectOffset);
   hkVector4 center = pos - objectOffset;

   //-- convert to quaternion and normalize it
   hkRotation hkRot;
   BPhysics::convertRotation(rotOnly, hkRot);
   hkQuaternion hkQuat;
   hkQuat.setAndNormalize(hkRot);

   mphkRigidBody->setRotation(hkQuat);

   // Set new position from center calculated above
   BPhysicsObject::setPosition(*reinterpret_cast<const BVector*>(&center));
}


//==============================================================================
// BPhysicsObject::setMass
//==============================================================================
void BPhysicsObject::setMass(float fMass)
{
    BASSERT(mphkRigidBody != NULL);

    mphkRigidBody->setMass(fMass);

    hkpMassProperties massProperties;
    const hkpCollidable *collidable = mphkRigidBody->getCollidable();
    hkpInertiaTensorComputer::computeShapeVolumeMassProperties(collidable->getShape(), mphkRigidBody->getMass(), massProperties);
    mphkRigidBody->setInertiaLocal(massProperties.m_inertiaTensor);

    
}


//==============================================================================
// BPhysicsObject::getMass
//==============================================================================
float BPhysicsObject::getMass( void ) const
{
    BASSERT(mphkRigidBody != NULL);

    return mphkRigidBody->getMass();
}



//==============================================================================
// BPhysicsObject::setAngularDamping
//==============================================================================
void BPhysicsObject::setAngularDamping(float damping)
{
   BASSERT(mphkRigidBody != NULL);
   mphkRigidBody->setAngularDamping(damping);
}


//==============================================================================
// BPhysicsObject::getAngularDamping
//==============================================================================
float BPhysicsObject::getAngularDamping() const
{
    BASSERT(mphkRigidBody != NULL);
    return mphkRigidBody->getAngularDamping();
}  

//==============================================================================
// BPhysicsObject::setLinearDamping
//==============================================================================
void BPhysicsObject::setLinearDamping(float damping)
{
   BASSERT(mphkRigidBody != NULL);
   mphkRigidBody->setLinearDamping(damping);
}


//==============================================================================
// BPhysicsObject::getLinearDamping
//==============================================================================
float BPhysicsObject::getLinearDamping() const
{
   BASSERT(mphkRigidBody != NULL);
   return mphkRigidBody->getLinearDamping();
}

//==============================================================================
// BPhysicsObject::getRestitution
//==============================================================================
float BPhysicsObject::getRestitution() const
{
   BASSERT(mphkRigidBody != NULL);
   return mphkRigidBody->getMaterial().getRestitution();
}

//==============================================================================
// BPhysicsObject::getFriction
//==============================================================================
float BPhysicsObject::getFriction() const
{
   BASSERT(mphkRigidBody != NULL);
   return mphkRigidBody->getMaterial().getFriction();
}

//==============================================================================
// BPhysicsObject::getLinearVelocity
//==============================================================================
void BPhysicsObject::getLinearVelocity(BVector &velocity) const
{
   BASSERT(mphkRigidBody != NULL);
   BPhysics::convertPoint(mphkRigidBody->getLinearVelocity(), velocity);
}


//==============================================================================
// BPhysicsObject::getAngularVelocity
//==============================================================================
void BPhysicsObject::getAngularVelocity(BVector &velocity) const
{
   BASSERT(mphkRigidBody != NULL);
   BPhysics::convertPoint(mphkRigidBody->getAngularVelocity(), velocity);
}


//==============================================================================
// BPhysicsObject::getAngularVelocity
//==============================================================================
void BPhysicsObject::setLinearVelocity(const BVector &velocity)
{
   BASSERT(mphkRigidBody != NULL);

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::setLinearVelocity velocity", velocity);
   #endif

   hkVector4 hkVelocity;
   BPhysics::convertPoint(velocity, hkVelocity);
   mphkRigidBody->setLinearVelocity(hkVelocity);
}


//==============================================================================
// BPhysicsObject::getAngularVelocity
//==============================================================================
void BPhysicsObject::setAngularVelocity(const BVector &velocity)
{
   BASSERT(mphkRigidBody != NULL);

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::setAngularVelocity velocity", velocity);
   #endif

   hkVector4 hkVelocity;
   BPhysics::convertPoint(velocity, hkVelocity);
   mphkRigidBody->setAngularVelocity(hkVelocity);
}


//==============================================================================
// BPhysicsObject::getCenterOfMassLocation
//==============================================================================
void BPhysicsObject::getCenterOfMassLocation(BVector &pos) const
{
   BASSERT(mphkRigidBody != NULL);
   BPhysics::convertPoint(mphkRigidBody->getCenterOfMassInWorld(), pos); 
}

//==============================================================================
// BPhysicsObject::setProperty
//==============================================================================
void BPhysicsObject::setProperty(long property, DWORD value) 
{
   BASSERT(mphkRigidBody != NULL);
   hkpPropertyValue hkValue;

   hkValue.setInt((long)value);

   mphkRigidBody->addProperty(property, hkValue);
}

//==============================================================================
// BPhysicsObject::setKeyframed
//==============================================================================
void BPhysicsObject::setKeyframed(bool flag)
{
   BASSERT(mphkRigidBody != NULL);
   if (flag)
      mphkRigidBody->setMotionType(hkpMotion::MOTION_KEYFRAMED);
   else
      mphkRigidBody->setMotionType(hkpMotion::MOTION_BOX_INERTIA);
}

//==============================================================================
// BPhysicsObject::isKeyframed
//==============================================================================
bool BPhysicsObject::isKeyframed( void ) const
{

   BASSERT(mphkRigidBody != NULL);

   if (mphkRigidBody->getMotionType() == hkpMotion::MOTION_KEYFRAMED)
      return (true);

   return (false);

}

//==============================================================================
// BPhysicsObject::setCollisionFilterInfo
//==============================================================================
void BPhysicsObject::setCollisionFilterInfo(long info)
{
   BASSERT(mphkRigidBody != NULL);
   mphkRigidBody->setCollisionFilterInfo((hkUint32) info);
}

//==============================================================================
// BPhysicsObject::getCollisionFilterInfo
//==============================================================================
long BPhysicsObject::getCollisionFilterInfo(void) const
{
   BASSERT(mphkRigidBody != NULL);
   return (long) mphkRigidBody->getCollisionFilterInfo();
}

//==============================================================================
// BPhysicsObject::updateCollisionFilter
//==============================================================================
void BPhysicsObject::updateCollisionFilter( void )
{
  BASSERT(mphkRigidBody != NULL);
  mWorld->getHavokWorld()->updateCollisionFilterOnEntity(mphkRigidBody, HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK , HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS );
}

//==============================================================================
// BPhysicsObject::applyPointImpulse
//==============================================================================
void BPhysicsObject::applyPointImpulse(const BVector &impulse, const BVector &point)
{
   BASSERT(mphkRigidBody != NULL);

   if (isKeyframed())
      return;

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::applyPointImpulse impulse", impulse);
      syncUnitActionData("BPhysicsObject::applyPointImpulse point", point);
   #endif

   hkVector4 hkImpulse, hkPoint;
   BPhysics::convertPoint(impulse, hkImpulse);
   BPhysics::convertPoint(point, hkPoint);

   mphkRigidBody->applyPointImpulse(hkImpulse, hkPoint);
}

//==============================================================================
// BPhysicsObject::applyImpulse
//==============================================================================
void BPhysicsObject::applyImpulse(const BVector &impulse)
{
   BASSERT(mphkRigidBody != NULL);

   if (isKeyframed())
      return;

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::applyImpulse impulse", impulse);
   #endif

   hkVector4 hkImpulse;
   BPhysics::convertPoint(impulse, hkImpulse);

   mphkRigidBody->applyLinearImpulse(hkImpulse);
}

//==============================================================================
//==============================================================================
void BPhysicsObject::applyAngularImpulse(const BVector& impulse)
{
   BASSERT(mphkRigidBody != NULL);

   if (isKeyframed())
      return;

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::applyAngularImpulse impulse", impulse);
   #endif

   mphkRigidBody->applyAngularImpulse((hkVector4) impulse);
}

//==============================================================================
// BPhysicsObject::applyForce
//==============================================================================
void BPhysicsObject::applyForce(const BVector& force)
{
   BASSERT(mphkRigidBody != NULL);

   if (isKeyframed())
      return;

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::applyForce force", force);
   #endif

   hkVector4 hkForce;
   BPhysics::convertPoint(force, hkForce);

   // WMJ HACK:  Come back and look up the correct delta time here
   mphkRigidBody->applyForce(cFixedUpdate, hkForce);
}

//==============================================================================
// BPhysicsObject::applyForce
//==============================================================================
void BPhysicsObject::applyForce(const BVector& force, const BVector& point)
{

   BASSERT(mphkRigidBody != NULL);

   if (isKeyframed())
      return;

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::applyForce force", force);
      syncUnitActionData("BPhysicsObject::applyForce point", point);
   #endif

   hkVector4 hkForce;
   BPhysics::convertPoint(force, hkForce);

   hkVector4 hkPoint;
   BPhysics::convertPoint(point, hkPoint);

    // WMJ HACK:  Come back and look up the correct delta time here
   mphkRigidBody->applyForce(cFixedUpdate, hkForce, hkPoint);
}


//==============================================================================
// BPhysicsObject::applyTorque
//==============================================================================
void BPhysicsObject::applyTorque(const BVector& torque)
{
   BASSERT(mphkRigidBody != NULL);

   if (isKeyframed())
      return;

   #ifdef SYNC_Unit
      syncUnitActionData("BPhysicsObject::applyTorque torque", torque);
   #endif

   hkVector4 hkTorque;
   BPhysics::convertPoint(torque, hkTorque);

   mphkRigidBody->applyTorque(cFixedUpdate, hkTorque);
}


//==============================================================================
// BPhysicsObject::forceActivate
//==============================================================================
void BPhysicsObject::forceActivate(void) 
{
   mphkRigidBody->activate();
   return;
}

//==============================================================================
// BPhysicsObject::forceDeactivate
//==============================================================================
void BPhysicsObject::forceDeactivate(void) 
{
   mphkRigidBody->deactivate();
   return;
}


//==============================================================================
// BPhysicsObject::enableDeactivation
//==============================================================================
void BPhysicsObject::enableDeactivation( bool flag )
{
   if (flag)
      mphkRigidBody->setDeactivator(hkpRigidBodyDeactivator::DEACTIVATOR_SPATIAL);
   else
      mphkRigidBody->setDeactivator(hkpRigidBodyDeactivator::DEACTIVATOR_NEVER);
}

//==============================================================================
//==============================================================================
bool BPhysicsObject::isDeactivationEnabled() const
{
   if (mphkRigidBody->getDeactivatorType() == hkpRigidBodyDeactivator::DEACTIVATOR_NEVER)
      return false;
   else
      return true;
}

//==============================================================================
// BPhysicsObject::isActive
//==============================================================================
bool BPhysicsObject::isActive( void ) const
{
    return mphkRigidBody->isActive();
}

//==============================================================================
//==============================================================================
void BPhysicsObject::addHavokCollisionListener(hkpCollisionListener* pListener)
{
   mphkRigidBody->addCollisionListener(pListener);
}

//==============================================================================
//==============================================================================
void BPhysicsObject::removeHavokCollisionListener(hkpCollisionListener* pListener)
{
   // Remove the collision listener
   mphkRigidBody->removeCollisionListener(pListener);

   // Clean up any null listeners in the array as the cleanup that happens inside
   // of the physics step is not thread safe.  This is copied from hkpEntityCallbackUtil.cpp,
   // cleanupNullPointers
   hkSmallArray<hkpCollisionListener*>& listeners = const_cast<hkSmallArray<hkpCollisionListener*>&>(mphkRigidBody->getCollisionListeners());
   for (int i = listeners.getSize() - 1; i >= 0; i-- )
   {
      if ( listeners[i] == HK_NULL )
      {
         listeners.removeAtAndCopy(i);
      }
   }
}

//==============================================================================
// BPhysicsObject::addGameCollisionListener
//==============================================================================
bool BPhysicsObject::addGameCollisionListener(BPhysicsCollisionListener *pListener)
{

   if (!mbRegisteredForCollisionCallback)
   {
      addHavokCollisionListener(this);
      mbRegisteredForCollisionCallback = true;
   }

   if (mCollisionListeners.addToTail(pListener))
      return (true);

   return (false);
}

//==============================================================================
// BPhysicsObject::removeGameCollisionListener
//==============================================================================
bool BPhysicsObject::removeGameCollisionListener(BPhysicsCollisionListener *pListener)
{
   BHandle hItem = NULL;
   BPhysicsCollisionListener *pTargetListener = mCollisionListeners.getHead(hItem);
   while (pTargetListener)
   {
      if (pTargetListener == pListener)
      {
         mCollisionListeners.removeAndGetNext(hItem);

         if (mbRegisteredForCollisionCallback)
         {
            if (mCollisionListeners.getSize() == 0)
            {
               removeHavokCollisionListener(this);
               mbRegisteredForCollisionCallback = false;
            }
         }

         return (true);
      }

      pTargetListener = mCollisionListeners.getNext(hItem);

   }
  
   return (false);
}

//==============================================================================
// BPhysicsObject::clearAllCollisionListeners
//==============================================================================
void BPhysicsObject::clearAllCollisionListeners(void)
{
   if (mbRegisteredForCollisionCallback)
   {
      removeHavokCollisionListener(this);
      mbRegisteredForCollisionCallback = false;
   }

   mCollisionListeners.empty();
}

//==============================================================================
// BPhysicsObject::contactPointConfirmedCallback
//==============================================================================
void BPhysicsObject::contactPointConfirmedCallback( hkpContactPointConfirmedEvent& event)
{
   event;
   return;
}

//==============================================================================
// BPhysicsObject::contactPointAddedCallback
//==============================================================================
void BPhysicsObject::contactPointAddedCallback(	hkpContactPointAddedEvent& event)
{
   if (mWorld->isSettling())
      return;

   dispatchContactPointAddedEvent(event);
}

//==============================================================================
// BPhysicsObject::contactPointRemovedCallback
//==============================================================================
void BPhysicsObject::contactPointRemovedCallback( hkpContactPointRemovedEvent& event )
{
   if (mWorld->isSettling())
      return;

   dispatchContactPointRemovedEvent(event);
}

//==============================================================================
// BPhysicsObject::contactProcessCallback
//==============================================================================
void BPhysicsObject::contactProcessCallback( hkpContactProcessEvent& event )
{
   if (mWorld->isSettling())
      return;

   dispatchContactProcessCallbackEvent(event);
}

//==============================================================================
// BPhysicsObject::dispatchContactPointAddedEvent
//==============================================================================
void BPhysicsObject::dispatchContactPointAddedEvent(	hkpContactPointAddedEvent& event)
{

   if (mCollisionListeners.getSize() <= 0)
      return;

   BPhysicsCollisionData cd;

   hkpRigidBody *phkCollidableA = (hkpRigidBody*) event.m_bodyA->getRootCollidable()->getOwner();
   hkpRigidBody *phkCollidableB = (hkpRigidBody*) event.m_bodyB->getRootCollidable()->getOwner();

   //-- switch this up so that we guarantee the listener
   //-- a certain order when the collision callbacks take place
   if (phkCollidableA != mphkRigidBody)
   {
      hkpRigidBody *hkTemp;
      hkTemp = phkCollidableA;
      phkCollidableA = phkCollidableB;
      phkCollidableB = hkTemp;
   }

   hkpPropertyValue &value = phkCollidableA->getProperty(BPhysicsWorld::cPropertyEntityReference);

   cd.mpObjectA = value.getPtr();


   value = phkCollidableB->getProperty(BPhysicsWorld::cPropertyEntityReference);
   cd.mpObjectB = value.getPtr(); 

   BPhysics::convertPoint(event.m_contactPoint->getNormal(), cd.mNormal);
   BPhysics::convertPoint(event.m_contactPoint->getPosition(), cd.mLocation);
  
   cd.mfVelocity = event.m_projectedVelocity;

   //-- find the contact surface
   cd.mSurfaceID = mWorld->findContactSurface((hkpRigidBody*) event.m_bodyB->getRootCollidable()->getOwner(), event.m_bodyB->getShapeKey());
   
   BHandle hItem = NULL;
   BPhysicsCollisionListener *pListener = mCollisionListeners.getHead(hItem);
   while (pListener)
   {
      //-- did we handle this collision?
      if (pListener->collision(cd))
      {
         event.m_status = HK_CONTACT_POINT_REJECT;
         return;
      }
      pListener = mCollisionListeners.getNext(hItem);
   }
}

//==============================================================================
// BPhysicsObject::dispatchContactPointRemovedEvent
//==============================================================================
void BPhysicsObject::dispatchContactPointRemovedEvent( hkpContactPointRemovedEvent& event )
{
   // To Be Implemented
}

//==============================================================================
// BPhysicsObject::dispatchContactProcessCallbackEvent
//==============================================================================
void BPhysicsObject::dispatchContactProcessCallbackEvent( hkpContactProcessEvent& event )
{
   /*BPhysicsCollisionData cd;

   hkpRigidBody *phkCollidableA = (hkpRigidBody*) event.m_collidableA.getOwner();
   hkpRigidBody *phkCollidableB = (hkpRigidBody*) event.m_collidableB.getOwner();

   hkpPropertyValue &value = phkCollidableA->getProperty(BPhysics::cPropertyEntityReference);
   cd.mpObjectA = (void*) value.asPtr; 

   value = phkCollidableB->getProperty(BPhysics::cPropertyEntityReference);
   cd.mpObjectB = (void*) value.asPtr; 


   
   hkpProcessCollisionOutput& result = event.m_collisionResult;
   if (result.m_contactPoints.getSize() <= 0)
      return;


   hkVector4 pos = result.m_contactPoints[0].m_position;

   hkVector4 normal = result.m_contactPoints[0].m_normal;

   BPhysics::convertPoint(pos, cd.mLocation); 
   BPhysics::convertPoint(normal, cd.mNormal);
   
   BHandle hItem = NULL;
   BPhysicsCollisionListener *pListener = mCollisionListeners.getHead(hItem);
   while (pListener)
   {
      if (pListener->preCollision(cd))
      {
         return;
      }

      pListener = mCollisionListeners.getNext(hItem);
   }*/
}

//==============================================================================
// BPhysicsObject::entityDeactivatedCallback
//==============================================================================
void BPhysicsObject::entityDeactivatedCallback(hkpEntity* entity)
{
   
}

//==============================================================================
// BPhysicsObject::entityActivatedCallback
//==============================================================================
void BPhysicsObject::entityActivatedCallback(hkpEntity* entity)
{
   
}

//==============================================================================
// BPhysicsObject::setShape
//==============================================================================
void BPhysicsObject::setShape(BShape &shape, long newID /*=-1*/)
{
   //hkpShape *phkShape = shape.getHavokShape();
  
   hkpRigidBodyCinfo cinfo;
   mphkRigidBody->getCinfo(cinfo);

   bool inWorld = isInWorld();
   if (inWorld)
      removeFromWorld();

   cinfo.m_shape = shape.getHavokShape();
   hkVector4 pos = mphkRigidBody->getPosition();
   hkTransform transform = mphkRigidBody->getTransform();
   hkVector4 velocity = mphkRigidBody->getLinearVelocity();
   hkVector4 angularVelocity = mphkRigidBody->getAngularVelocity();
   mphkRigidBody->removeReference();

#pragma push_macro("new")
#undef new
   mphkRigidBody = new hkpRigidBody(cinfo);
#pragma pop_macro("new")

   mphkRigidBody->setPosition(pos);
   mphkRigidBody->setTransform(transform);
   mphkRigidBody->setLinearVelocity(velocity);
   mphkRigidBody->setAngularVelocity(angularVelocity);

   //mphkRigidBody->setShape(phkShape);
  
   if (inWorld)
      addToWorld();

   if (newID != -1)
      mShapeID = newID;
}

//==============================================================================
// BPhysicsObject:isInWorld
//==============================================================================
 bool BPhysicsObject::isInWorld( void ) const
 {
    if (!mphkRigidBody)
       return (false);

    if (mphkRigidBody->getWorld() == HK_NULL)
       return (false);

    return (true);
 }


//==============================================================================
// BPhysicsObject::renderShape
//==============================================================================
void BPhysicsObject::renderShape(void)
{
   if (mphkRigidBody == NULL) 
      return;

   const hkpCollidable* pCollidable=mphkRigidBody->getCollidable();
   if(!pCollidable)
      return;

   const hkpShape *pShape = pCollidable->getShape();
   if (pShape == NULL)
      return;

   BVector drawCOM;
   BPhysicsMatrix drawMatrix;
   //BVector localCOM;
   //drawMatrix.makeIdentity();
   gPhysics->convertPoint(mphkRigidBody->getPosition(), drawCOM);
   gPhysics->convertRotation(mphkRigidBody->getRotation(), drawMatrix);
   drawMatrix.setTranslation(drawCOM);
   
   DWORD color = isActive() ? cDWORDGreen : cDWORDRed;
   BShape::renderWireframe(drawMatrix, pShape, mphkRigidBody->getCenterOfMassLocal(), false, color);
}


//=============================================================================
// BPhysicsObject::raySegmentIntersects
//=============================================================================
bool BPhysicsObject::raySegmentIntersects(const BVector &origin, const BVector &vector, bool segment, float &intersectDistanceSqr, BVector* pNormal/* = NULL*/) const
{
   if (mphkRigidBody == NULL) 
      return false;

   const hkpCollidable* pCollidable=mphkRigidBody->getCollidable();
   if(!pCollidable)
      return false;

   const hkpShape *pShape = pCollidable->getShape();
   if (pShape == NULL)
      return false;


	hkTransform worldTransform(mphkRigidBody->getRotation(), mphkRigidBody->getPosition());


   hkpShapeRayCastInput rayCastInput;

   BVector destination = origin + vector;
	rayCastInput.m_from.setTransformedInversePos( worldTransform, origin);
	rayCastInput.m_to.  setTransformedInversePos( worldTransform, destination);

   hkpShapeRayCastOutput rayCastOutput;

   bool collided = pShape->castRay(rayCastInput, rayCastOutput);

   if(collided)
   {
      // Compute intersection distance square
      intersectDistanceSqr = vector.lengthSquared() * (rayCastOutput.m_hitFraction * rayCastOutput.m_hitFraction);
      if (pNormal)
      {
         pNormal->x = rayCastOutput.m_normal(0);
         pNormal->y = rayCastOutput.m_normal(1);
         pNormal->z = rayCastOutput.m_normal(2);
         pNormal->w = rayCastOutput.m_normal(3);
      }
      return true;
   }

   return false;
}



//==============================================================================
//==============================================================================
// BClamshellPhysicsObject
//==============================================================================
//==============================================================================

//==============================================================================
//==============================================================================
BClamshellPhysicsObject::BClamshellPhysicsObject(BPhysicsWorld *world, const BPhysicsObjectBlueprint &upperBlueprint,
                                                 const BPhysicsObjectBlueprint &lowerBlueprint, const BPhysicsObjectBlueprint &pelvisBlueprint,
                                                 const BVector &position, const BVector& centerOffset, const BPhysicsMatrix &rotation, 
                                                 float upperHeightOffset, float lowerHeightOffset, DWORD userdata, bool fixed, const BPhysicsObjectBlueprintOverrides *pOverrides) :
   BPhysicsObject(world, pelvisBlueprint, position, centerOffset, rotation, userdata, fixed, pOverrides),
   mpUpperBody(NULL),
   mpLowerBody(NULL),
   mpUpperHinge(NULL),
   mpLowerHinge(NULL)
{
   // Get pelvis's collistion filter
   long collisionFilterID = getCollisionFilterInfo();

   BVector pelvisPos;
   getPosition(pelvisPos);

   // upper (torso, arms, head) rigid body
   BVector upperPos = pelvisPos + BVector(0.0, upperHeightOffset, 0.0);
   mpUpperBody = new BPhysicsObject(world, upperBlueprint, upperPos, cOriginVector, rotation, userdata, false, NULL);
   BASSERT(mpUpperBody);
   //if(!upperPO)
   //{
   //   pelvisPO->removeFromWorld();
   //   delete pelvisPO;
   //   return false;
   //}
   //upperPO->addCollisionListener(&clamshellCollisionListener);
   mpUpperBody->setCollisionFilterInfo(collisionFilterID);

   // lower (legs, feet) rigid body
   BVector lowerPos = pelvisPos + BVector(0.0, lowerHeightOffset, 0.0);
   mpLowerBody = new BPhysicsObject(world, lowerBlueprint, lowerPos, cOriginVector, rotation, userdata, false, NULL);
   BASSERT(mpLowerBody);
   /*
   if(!lowerPO)
   {
      upperPO->removeFromWorld();
      delete upperPO;
      pelvisPO->removeFromWorld();
      delete pelvisPO;
      return false;
   }
   lowerPO->addCollisionListener(&clamshellCollisionListener);
   */
   mpLowerBody->setCollisionFilterInfo(collisionFilterID);

   // Hinge Constraints
   setupHingeConstraints();
}

//==============================================================================
//==============================================================================
BClamshellPhysicsObject::BClamshellPhysicsObject(BPhysicsWorld *world) :
   BPhysicsObject(world),
   mpUpperBody(NULL),
   mpLowerBody(NULL),
   mpUpperHinge(NULL),
   mpLowerHinge(NULL)
{
}

//==============================================================================
//==============================================================================
BClamshellPhysicsObject::~BClamshellPhysicsObject()
{
   // Remove and delete objects and constraints
   if (mpUpperHinge)
   {
      delete mpUpperHinge;
      mpUpperHinge = NULL;
   }

   if (mpLowerHinge)
   {
      delete mpLowerHinge;
      mpLowerHinge = NULL;
   }

   if (mpUpperBody)
   {
      delete mpUpperBody;
      mpUpperBody = NULL;
   }

   if (mpLowerBody)
   {
      delete mpLowerBody;
      mpLowerBody = NULL;
   }
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setupHingeConstraints()
{
   BPhysicsMatrix rotation;
   getRotation(rotation);

   BVector pelvisPos;
   getPosition(pelvisPos);

   float upperHingeMinAngle = -HK_REAL_PI/3.0f;
   float upperHingeMaxAngle = HK_REAL_PI/8.0f;
   float lowerHingeMinAngle = -HK_REAL_PI/8.0f;
   float lowerHingeMaxAngle = HK_REAL_PI/4.0f;

   BLimitedHingeBlueprint lhbp;
   lhbp.mpObjectA = this;
   lhbp.mAxis = rotation.getRight();
   lhbp.mPivot = pelvisPos;
   lhbp.mFriction = 3.0f;

   lhbp.mpObjectB = mpUpperBody;
   lhbp.mMaxAngle = upperHingeMaxAngle;
   lhbp.mMinAngle = upperHingeMinAngle;
   mpUpperHinge = new BConstraint(mWorld);
   if (mpUpperHinge)
      mpUpperHinge->allocateLimitedHingeConstraint(lhbp);

   lhbp.mpObjectB = mpLowerBody;
   lhbp.mMaxAngle = lowerHingeMaxAngle;
   lhbp.mMinAngle = lowerHingeMinAngle;
   mpLowerHinge = new BConstraint(mWorld);
   if (mpLowerHinge)
      mpLowerHinge->allocateLimitedHingeConstraint(lhbp);
}

//==============================================================================
//==============================================================================
bool BClamshellPhysicsObject::addToWorld(void)
{
   // Add all physics objects and constraints
   BPhysicsObject::addToWorld();
   if (mpUpperBody)
      mpUpperBody->addToWorld();
   if (mpLowerBody)
      mpLowerBody->addToWorld();
   if (mpUpperHinge)
      mpUpperHinge->addToWorld();
   if (mpLowerHinge)
      mpLowerHinge->addToWorld();

   return true;
}

//==============================================================================
//==============================================================================
bool BClamshellPhysicsObject::removeFromWorld(void)
{
   // Remove all physics objects and constraints
   BPhysicsObject::removeFromWorld();
   if (mpUpperBody)
      mpUpperBody->removeFromWorld();
   if (mpLowerBody)
      mpLowerBody->removeFromWorld();
   if (mpUpperHinge)
      mpUpperHinge->removeFromWorld();
   if (mpLowerHinge)
      mpLowerHinge->removeFromWorld();

   return true;
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setPosition(const BVector &pos)
{
   // Set pelvis position and calc diff
   BVector oldPos, newPos, posDiff;
   BPhysicsObject::getPosition(oldPos);
   BPhysicsObject::setPosition(pos);
   BPhysicsObject::getPosition(newPos);
   posDiff = newPos - oldPos;

   // Upper body
   hkVector4 hkPos;
   mpUpperBody->getPosition(oldPos);
   mpUpperBody->setPosition(oldPos + posDiff);

   // Lower body
   mpLowerBody->getPosition(oldPos);
   mpLowerBody->setPosition(oldPos + posDiff);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setRotation(const BPhysicsMatrix &rot)
{
   BASSERT(mphkRigidBody);
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);

   // Make sure we're using a matrix with rotation only
   BPhysicsMatrix rotOnly(rot);
   rotOnly.clearTranslation();

   // Calc rotation transform to new rotation
   BVector c;
   BPhysicsMatrix oldRot, oldRotInvert, rotTrans;
   BPhysicsObject::getRotation(oldRot);
   oldRotInvert = oldRot;
   oldRotInvert.invert();
   rotTrans = oldRotInvert * rotOnly;

   // Calculate unit center
   BVector pelvisPos;
   BPhysicsObject::getPosition(pelvisPos);
   BVector objectOffset;
   oldRot.transformPoint(mCenterOffset, objectOffset);
   c = pelvisPos - objectOffset;

   // Pelvis
   BPhysicsObject::setRotation(rotOnly);

   // Upper body - transform about the unit center
   BVector pos;
   BPhysicsMatrix tempRot, newRot;
   mpUpperBody->getPosition(pos);
   mpUpperBody->getRotation(tempRot);
   tempRot.setTranslation(pos - c);
   newRot = tempRot * rotTrans;
   mpUpperBody->setRotation(newRot);
   mpUpperBody->setPosition(newRot.getTranslation() + c);

   // Lower body - transform about the unit center
   mpLowerBody->getPosition(pos);
   mpLowerBody->getRotation(tempRot);
   tempRot.setTranslation(pos - c);
   newRot = tempRot * rotTrans;
   mpLowerBody->setRotation(newRot);
   mpLowerBody->setPosition(newRot.getTranslation() + c);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setLinearVelocity(const BVector &velocity)
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::setLinearVelocity(velocity);
   mpUpperBody->setLinearVelocity(velocity);
   mpLowerBody->setLinearVelocity(velocity);
}


//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setAngularVelocity(const BVector &velocity)
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::setAngularVelocity(velocity);
   mpUpperBody->setAngularVelocity(velocity);
   mpLowerBody->setAngularVelocity(velocity);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::applyImpulse(const BVector &impulse)
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::applyImpulse(impulse);
   mpUpperBody->applyImpulse(impulse);
   mpLowerBody->applyImpulse(impulse);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::applyAngularImpulse(const BVector &impulse)
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::applyAngularImpulse(impulse);
   mpUpperBody->applyAngularImpulse(impulse);
   mpLowerBody->applyAngularImpulse(impulse);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::applyPointImpulses(const BVector &upperImpulse, const BVector &lowerImpulse, const BVector &pelvisImpulse, const BVector &pelvisOffset)
{
   BVector pos;
   getPosition(pos);
   BPhysicsObject::applyPointImpulse(pelvisImpulse, pos + pelvisOffset);

   if (mpUpperBody)
   {
      mpUpperBody->getPosition(pos);
      mpUpperBody->applyPointImpulse(upperImpulse, pos);
   }
   if (mpLowerBody)
   {
      mpLowerBody->getPosition(pos);
      mpLowerBody->applyPointImpulse(lowerImpulse, pos);
   }
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setKeyframed(bool flag)
{
   // Set all physics objects to keyframed
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::setKeyframed(flag);
   mpUpperBody->setKeyframed(flag);
   mpLowerBody->setKeyframed(flag);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setCollisionFilterInfo(long info)
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::setCollisionFilterInfo(info);
   mpUpperBody->setCollisionFilterInfo(info);
   mpLowerBody->setCollisionFilterInfo(info);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::updateCollisionFilter( void )
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::updateCollisionFilter();
   mpUpperBody->updateCollisionFilter();
   mpLowerBody->updateCollisionFilter();
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::addHavokCollisionListener(hkpCollisionListener* pListener)
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::addHavokCollisionListener(pListener);
   mpUpperBody->addHavokCollisionListener(pListener);
   mpLowerBody->addHavokCollisionListener(pListener);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::removeHavokCollisionListener(hkpCollisionListener* pListener)
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);
   BPhysicsObject::removeHavokCollisionListener(pListener);
   mpUpperBody->removeHavokCollisionListener(pListener);
   mpLowerBody->removeHavokCollisionListener(pListener);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::getClamshellData(BVector& pos, BVector& fwd, BVector& up, BVector& right, float& angle) const
{
   BASSERT(mpUpperBody);
   BASSERT(mpLowerBody);

   BVector upperFwd, lowerFwd;
   BPhysicsMatrix tempRot;

   mpUpperBody->getRotation(tempRot);
   tempRot.getForward(upperFwd);

   mpLowerBody->getRotation(tempRot);
   tempRot.getForward(lowerFwd);

   // Forward is average of upper / lower body forwards
   fwd = upperFwd + lowerFwd;
   fwd.normalize();
   
   // Get pelvis right vector
   BPhysicsObject::getRotation(tempRot);
   tempRot.getRight(right);

   // Up
   up = fwd.cross(right);
   up.normalize();

   // Position - clamshell offset
   BVector centerOffset = getCenterOffset();
   BVector worldOffset = right * centerOffset.x + up * centerOffset.y + fwd * centerOffset.z;
   getPosition(pos);
   pos -= worldOffset;

   // Figure out the clamshell angle
   BVector pelvisPos, upperPos, lowerPos, vec1, vec2;
   mpUpperBody->getPosition(upperPos);
   BPhysicsObject::getPosition(pelvisPos);
   mpLowerBody->getPosition(lowerPos);

   vec1 = upperPos - pelvisPos;
   vec2 = lowerPos - pelvisPos;
   vec1.normalize();
   vec2.normalize();

   angle = vec1.dot(vec2);

   BVector tempVec = vec1.cross(vec2);
   if (tempVec.dot(right) > 0.0f)
      angle = (angle + 1.0f) * 0.5f; // map -1..1 -> 0..1
   else
      angle = (angle + 1.0f) * -0.5f; // map -1..1 -> 0..-1
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::renderShape(void)
{
   BPhysicsObject::renderShape();
   if (mpUpperBody)
      mpUpperBody->renderShape();
   if (mpLowerBody)
      mpLowerBody->renderShape();

   BPhysicsMatrix rot;
   BVector pos;
   BPhysicsObject::getPosition(pos);
   BPhysicsObject::getRotation(rot);
   gpDebugPrimitives->addDebugAxis(pos, rot.getRight(), rot.getUp(), rot.getForward(), 1.0f);

   if(mpUpperBody)
   {
      mpUpperBody->getPosition(pos);
      mpUpperBody->getRotation(rot);
   }
   gpDebugPrimitives->addDebugAxis(pos, rot.getRight(), rot.getUp(), rot.getForward(), 1.0f);

   if(mpLowerBody)
   {
      mpLowerBody->getPosition(pos);
      mpLowerBody->getRotation(rot);
   }
   gpDebugPrimitives->addDebugAxis(pos, rot.getRight(), rot.getUp(), rot.getForward(), 1.0f);
}

//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setMass(float fMass)
{
    BPhysicsObject::setMass(fMass);
   if (mpUpperBody)
      mpUpperBody->setMass(fMass);
   if (mpLowerBody)
      mpLowerBody->setMass(fMass);
}
//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setAngularDamping(float damping)
{
   BPhysicsObject::setAngularDamping(damping);
   if (mpUpperBody)
      mpUpperBody->setAngularDamping(damping);
   if (mpLowerBody)
      mpLowerBody->setAngularDamping(damping);
}
//==============================================================================
//==============================================================================
void BClamshellPhysicsObject::setLinearDamping(float damping)
{
   BPhysicsObject::setLinearDamping(damping);
   if (mpUpperBody)
      mpUpperBody->setLinearDamping(damping);
   if (mpLowerBody)
      mpLowerBody->setLinearDamping(damping);
}

//==============================================================================
//==============================================================================
// BPhantom
//==============================================================================
//==============================================================================

//==============================================================================
//==============================================================================
BPhantom::BPhantom(BPhysicsWorld *world, BVector min, BVector max,
                   hkpPhantomOverlapListener* pListener, long collisionFilterInfo, int userData)
{
   hkAabb aabb;
   aabb.m_min = min;
   aabb.m_max = max;

   // Create aabb phantom
   mpPhantom = new hkpAabbPhantom(aabb);

   // Setup listener, filter info, entity property
   if (pListener)
      mpPhantom->addPhantomOverlapListener(pListener);
   mpPhantom->getCollidableRw()->setCollisionFilterInfo(collisionFilterInfo);

   hkpPropertyValue hkValue;
   hkValue.setInt(userData);
   mpPhantom->addProperty(BPhysicsWorld::cPropertyEntityReference, hkValue);

   // Add to world
   if (world)
   {
      world->getHavokWorld()->addPhantom(mpPhantom);
      mpPhantom->removeReference();
   }
}

//==============================================================================
//==============================================================================
BPhantom::~BPhantom()
{
   if (mpPhantom)
   {
      hkpWorld* pWorld = mpPhantom->getWorld();
      if (pWorld)
         pWorld->removePhantom(mpPhantom);
      
      mpPhantom->removeReference();
   }
}

//==============================================================================
//==============================================================================
void BPhantom::getAabbMinMax(BVector& min, BVector& max) const
{
   if (mpPhantom)
   {
      min = mpPhantom->getAabb().m_min;
      max = mpPhantom->getAabb().m_max;
   }
}

//==============================================================================
//==============================================================================
bool BPhysicsObject::save(BStream* pStream, int saveType) const
{
   //BPhysicsWorld* mWorld;
   //hkpRigidBody* mphkRigidBody;
   //BPointerList<BPhysicsCollisionListener> mCollisionListeners;
   //BHandle mListHandle;

   // Get mass to save - If this has storedDynamicMotion that means it is a dynamic
   // object that is currently set to keyframed.  In this case we want the original
   // dynamic mass, as the keyframed mass is always 0.0.
   float mass = getMass();
   const hkpMotion* pStoredMotion = mphkRigidBody->getStoredDynamicMotion();
   if (pStoredMotion)
   {
      mass = pStoredMotion->getMass();
   }

   GFWRITEVAR(pStream, long, mShapeID);
   GFWRITEVAR(pStream, long, mInfoID);
   //void* mpUserData;
   //bool mbDeleteRigidBodyOnDestruction;
   GFWRITEVAR(pStream, bool, mbFixed);
   GFWRITEVAR(pStream, bool, mbBreakable);
   //static BPointerList<BPhysicsObject> mPhysicsObjects;

   GFWRITEVAL(pStream, float, mass);
   GFWRITEVAL(pStream, float, getRestitution());
   GFWRITEVAL(pStream, float, getFriction());
   GFWRITEVECTOR(pStream, mCenterOffset);
   GFWRITEVECTOR(pStream, mCenterOfMassOffset);
   GFWRITEVECTOR(pStream, mShapeHalfExtents);
   GFWRITEVAL(pStream, float, getAngularDamping());
   GFWRITEVAL(pStream, float, getLinearDamping());
   GFWRITEVAL(pStream, long, getCollisionFilterInfo());

   BPhysicsMatrix rot;
   getRotation(rot);
   //GFWRITEVAR(pStream, BPhysicsMatrix, rot);
   BVector vec;
   rot.getForward(vec);
   GFWRITEVECTOR(pStream, vec);
   rot.getRight(vec);
   GFWRITEVECTOR(pStream, vec);
   rot.getUp(vec);
   GFWRITEVECTOR(pStream, vec);
   rot.getTranslation(vec);
   GFWRITEVECTOR(pStream, vec);

   getPosition(vec);
   BVector objectOffset;
   rot.transformPoint(mCenterOffset, objectOffset);
   vec = vec - objectOffset;

   GFWRITEVECTOR(pStream, vec);

   GFWRITEVAL(pStream, bool, isInWorld());

   getLinearVelocity(vec);
   GFWRITEVECTOR(pStream, vec);

   getAngularVelocity(vec);
   GFWRITEVECTOR(pStream, vec);

   GFWRITEVAR(pStream, bool, mbRegisteredForCollisionCallback);
   GFWRITEVAL(pStream, bool, isKeyframed());
   GFWRITEVAL(pStream, bool, isActive());
   GFWRITEVAL(pStream, bool, isDeactivationEnabled());

   return true;
}

//==============================================================================
//==============================================================================
bool BPhysicsObject::load(BStream* pStream, int saveType)
{
   //BPhysicsWorld* mWorld;
   //hkpRigidBody* mphkRigidBody;
   //BPointerList<BPhysicsCollisionListener> mCollisionListeners;
   //BHandle mListHandle;
   GFREADVAR(pStream, long, mShapeID);
   gPhysics->remapShapeID(mShapeID);
   GFREADVAR(pStream, long, mInfoID);
   gPhysics->remapPhysicsInfoID(mInfoID);
   //void* mpUserData;
   //bool mbDeleteRigidBodyOnDestruction;
   //static BPointerList<BPhysicsObject> mPhysicsObjects;

   BPhysicsObjectParams physicsparams;
   GFREADVAR(pStream, bool, physicsparams.fixed);
   GFREADVAR(pStream, bool, physicsparams.breakable);
   GFREADVAR(pStream, float, physicsparams.mass);
   GFREADVAR(pStream, float, physicsparams.restitution);
   GFREADVAR(pStream, float, physicsparams.friction);
   GFREADVECTOR(pStream, physicsparams.centerOffset);
   GFREADVECTOR(pStream, physicsparams.centerOfMassOffset);
   GFREADVECTOR(pStream, physicsparams.shapeHalfExtents);
   GFREADVAR(pStream, float, physicsparams.angularDamping);
   GFREADVAR(pStream, float, physicsparams.linearDamping);
   GFREADVAR(pStream, long, physicsparams.collisionFilterInfo);

   //GFREADVAR(pStream, BPhysicsMatrix, physicsparams.rotation);
   BVector forward, up, right, pos;
   GFREADVECTOR(pStream, forward);
   GFREADVECTOR(pStream, right);
   GFREADVECTOR(pStream, up);
   GFREADVECTOR(pStream, pos);
   physicsparams.rotation.set(right, up, forward, pos);

   GFREADVECTOR(pStream, physicsparams.position);

   bool newShape = false;
   BShape* pShape = gPhysics->getShapeManager().get(mShapeID);
   if (pShape)
      physicsparams.pHavokShape = pShape->getHavokShape();
   else
   {
      physicsparams.pHavokShape = (hkpShape*) new hkpBoxShape(physicsparams.shapeHalfExtents);
      newShape = true;
   }

   setupObjectRepresentation(physicsparams, true);
   if (newShape)
      physicsparams.pHavokShape->removeReference();  // remove reference from new shape as the rigid body now references it.

   bool inWorld;
   GFREADVAR(pStream, bool, inWorld);
   if (inWorld)
      BPhysicsObject::addToWorld();

   bool isFixed = mphkRigidBody->isFixed();

   BVector vec;
   GFREADVECTOR(pStream, vec);
   if (!isFixed)
      BPhysicsObject::setLinearVelocity(vec);

   GFREADVECTOR(pStream, vec);
   if (!isFixed)
      BPhysicsObject::setAngularVelocity(cOriginVector);

   bool bVal;
   GFREADVAR(pStream, bool, bVal);
   if (bVal && mphkRigidBody)
   {
      addHavokCollisionListener(this);
      mbRegisteredForCollisionCallback = true;
   }

   GFREADVAR(pStream, bool, mbLoadedKeyframed);
   //BPhysicsObject::setKeyframed(bVal);

   GFREADVAR(pStream, bool, bVal);
   if (!isFixed)
   {
      if (bVal)
         forceActivate();
      else
         forceDeactivate();
   }

   GFREADVAR(pStream, bool, bVal);
   enableDeactivation(bVal);

   BPhysicsObject::updateCollisionFilter();

   return true;
}

//==============================================================================
//==============================================================================
bool BClamshellPhysicsObject::save(BStream* pStream, int saveType) const
{
   GFWRITEVAL(pStream, bool, (mpUpperBody != NULL));
   if (mpUpperBody)
      GFWRITECLASSPTR(pStream, saveType, mpUpperBody);

   GFWRITEVAL(pStream, bool, (mpLowerBody != NULL));
   if (mpLowerBody)
      GFWRITECLASSPTR(pStream, saveType, mpLowerBody);

   if (!BPhysicsObject::save(pStream, saveType))
      return false;

   //BConstraint* mpUpperHinge;
   //BConstraint* mpLowerHinge;

   return true;
}

//==============================================================================
//==============================================================================
bool BClamshellPhysicsObject::load(BStream* pStream, int saveType)
{
   bool bVal;
   GFREADVAR(pStream, bool, bVal);
   if (bVal)
   {
      mpUpperBody = new BPhysicsObject(mWorld);
      if (!mpUpperBody)
         return false;
      GFREADCLASSPTR(pStream, saveType, mpUpperBody);
   }

   GFREADVAR(pStream, bool, bVal);
   if (bVal)
   {
      mpLowerBody = new BPhysicsObject(mWorld);
      if (!mpLowerBody)
         return false;
      GFREADCLASSPTR(pStream, saveType, mpLowerBody);
   }

   if (!BPhysicsObject::load(pStream, saveType))
      return false;

   setupHingeConstraints();

   if (mpUpperHinge)
      mpUpperHinge->addToWorld();
   if (mpLowerHinge)
      mpLowerHinge->addToWorld();

   return true;
}

//==============================================================================
// eof: physicobject.cpp
//==============================================================================
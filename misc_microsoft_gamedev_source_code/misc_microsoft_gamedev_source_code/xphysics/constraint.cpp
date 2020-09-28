//==============================================================================
// constraint.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "physics.h"
//#include "debugprimitives.h"
#include "constraint.h"
#include "physicsworld.h"
#include <Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h>
#include <Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h>

#pragma push_macro("new")
#undef new
//==============================================================================
// Defines


//==============================================================================
// BConstraint::BConstraint
//==============================================================================
BConstraint::BConstraint(BPhysicsWorld *world):
   mphkConstraint(NULL),
   mType(-1),
   mpListener(NULL),
   mWorld(world)
{

}


//==============================================================================
// BConstraint::BConstraint
//==============================================================================
BConstraint::BConstraint(BPhysicsWorld *world, hkpConstraintInstance* pConstraint) :
   mpListener(NULL),
   mWorld(world)
{
   setHavokConstraint(pConstraint);
}



//==============================================================================
// BConstraint::~BConstraint
//==============================================================================
BConstraint::~BConstraint( void )
{
   if (mphkConstraint)
   {
      removeFromWorld();
      mphkConstraint->removeReference();
      mphkConstraint = NULL;
   }
} 

//==============================================================================
// BConstraint::addReference
//==============================================================================
void BConstraint::addReference( void )
{
   if (mphkConstraint)
      mphkConstraint->addReference();
}



//==============================================================================
// BConstraint::BConstraint
//==============================================================================
void BConstraint::releaseHavokConstraint(void)
{
   if (mphkConstraint)
   {
      mphkConstraint->removeReference();
      mphkConstraint = NULL;
   }
}


//==============================================================================
// BConstraint::setHavokConstraint
//==============================================================================
void BConstraint::setHavokConstraint(hkpConstraintInstance* pConstraint)
{
   mphkConstraint = pConstraint;
   if (pConstraint)
   {
      if (pConstraint->getData())
      {
         switch (pConstraint->getData()->getType())
         {
         case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
            {
               mType = cConstraintTypeHinge;
               break;
            }
         default :
            {
               mType = cConstraintTypeUnknown;
               break;
            }
         }
      }
     
   }

}


//==============================================================================
// BConstraint::addToWorld
//==============================================================================
bool BConstraint::addToWorld()
{
   if (!mphkConstraint)
      return (false);

   mWorld->getHavokWorld()->addConstraint(mphkConstraint);
   return (true);
}

//==============================================================================
// BConstraint::removeFromWorld
//==============================================================================
bool BConstraint::removeFromWorld()
{
   if (!mphkConstraint)
      return (false);

   mWorld->getHavokWorld()->removeConstraint(mphkConstraint);
   return (true);
}

//==============================================================================
// BConstraint::allocateWorldConstraint
//==============================================================================
bool BConstraint::allocateWorldConstraint(const BPhysicsObject &objectA)
{

   return (false);
/*
   if (objectA.getRigidBody() == NULL)
      return(false);

   hkPrismaticConstraintCinfo cinfo;
   hkVector4 axis(0.0f, 1.0f, 0.0f);

   BVector worldPos = cOriginVector;
   objectA.getPosition(worldPos);
   hkVector4 hkPivot;
   BPhysics::convertPoint(worldPos, hkPivot);

   // Create constraint
   cinfo.setInWorldSpace(objectA.getRigidBody(), mWorld->getHavokWorld()->getFixedRigidBody(), hkPivot, axis);
   cinfo.setMaxLinearLimit(0.0f);
   cinfo.setMinLinearLimit(0.0f);

   hkPrismaticConstraint *pris = new hkPrismaticConstraint(cinfo); 
   mphkConstraint = pris;
   mType = cConstraintTypeWorld;
   return(true);
*/
}


//==============================================================================
// BConstraint::allocateBallAndSocket
//==============================================================================
bool BConstraint::allocateBallAndSocket(const BPhysicsObject &objectA, const BVector &point)
{

   return (false);

/*   if (objectA.getRigidBody() == NULL)
      return(false);

   hkBallAndSocketConstraintCinfo cinfo;
   hkVector4 axis(0.0f, 1.0f, 0.0f);

   hkVector4 hkPivot;
   BPhysics::convertPoint(point, hkPivot);

   // Create constraint
   cinfo.setInWorldSpace(objectA.getRigidBody(), mWorld->getHavokWorld()->getFixedRigidBody(), hkPivot);

   hkBallAndSocketConstraint *constraint = new hkBallAndSocketConstraint(cinfo); 
   mphkConstraint = constraint;
   mType = cConstraintTypeBallAndSocket;
   return(true);
*/
}



//==============================================================================
// BConstraint::allocateRigidConstraint
//==============================================================================
bool BConstraint::allocateRigidConstraint(const BPhysicsObject &objectA, const BPhysicsObject &objectB)
{

   return (false);

/*
   hkPrismaticConstraintCinfo cinfo;
   hkVector4 axis(0.0f, 1.0f, 0.0f);

   BVector worldPos = cOriginVector;
   objectB.getPosition(worldPos);
   hkVector4 hkPivot;
   BPhysics::convertPoint(worldPos, hkPivot);

   // Create constraint
   cinfo.setInWorldSpace(objectA.getRigidBody(), objectB.getRigidBody(), hkPivot, axis);
   cinfo.setMaxLinearLimit(0.0f);
   cinfo.setMinLinearLimit(0.0f);

   hkPrismaticConstraint *pris = new hkPrismaticConstraint(cinfo); 
   mphkConstraint = pris;
   mType = cConstraintTypeRigid;
   return(true);

*/

}

//==============================================================================
// BConstraint::allocateHingeConstraint
//==============================================================================
bool BConstraint::allocateHingeConstraint(const BPhysicsObject &objectA, const BPhysicsObject &objectB, const BVector &axis, const BVector& pivot)
{

   return (false);
/*
   hkVector4 hkPivot;
   hkVector4 hkAxis;
   BPhysics::convertPoint(axis, hkAxis);
   BPhysics::convertPoint(pivot, hkPivot);

   hkHingeConstraintCinfo cinfo;
   cinfo.setInWorldSpace(objectA.getRigidBody(), objectB.getRigidBody(), hkPivot, hkAxis);
   hkHingeConstraint *pConstraint = new hkHingeConstraint(cinfo);
   if (!pConstraint)
      return (false);

   mphkConstraint = pConstraint;
   mType = cConstraintTypeHinge;
   return (true);
*/
}

//==============================================================================
// BConstraint::allocateLimitedHingeConstraint
//==============================================================================
bool BConstraint::allocateLimitedHingeConstraint(const BLimitedHingeBlueprint &bp)
{
   if (!bp.mpObjectA || !bp.mpObjectB)
      return (false);

   //gpDebugPrimitives->addDebugSphere(bp.mPivot, .25f, RGB_RED);
   hkVector4 hkPivot;
   hkVector4 hkAxis;
   BPhysics::convertPoint(bp.mAxis, hkAxis);
   BPhysics::convertPoint(bp.mPivot, hkPivot);

   hkpLimitedHingeConstraintData* cinfo = new hkpLimitedHingeConstraintData();
   cinfo->setInWorldSpace(bp.mpObjectA->getRigidBody()->getTransform(), bp.mpObjectB->getRigidBody()->getTransform(), hkPivot, hkAxis);

   cinfo->setMinAngularLimit(bp.mMinAngle);
   cinfo->setMaxAngularLimit(bp.mMaxAngle);
   cinfo->setMaxFrictionTorque(bp.mFriction);

   mphkConstraint = new hkpConstraintInstance(bp.mpObjectA->getRigidBody(), bp.mpObjectB->getRigidBody(), cinfo);
   if (!mphkConstraint)
      return false;

   cinfo->removeReference();
   mType = cConstraintTypeLimitedHinge;
   return (true);
}


//==============================================================================
// BConstraint::allocatePrismaticConstraint
//==============================================================================
bool BConstraint::allocatePrismaticConstraint(const BPhysicsObject *objectA, const BPhysicsObject *objectB, const BVector &axis, const BVector &pivot, float max, float min)
{
   if (!objectA->getRigidBody() || !objectB->getRigidBody())
      return(false);

   hkVector4 hkPivot;
   hkVector4 hkAxis;
   BPhysics::convertPoint(axis, hkAxis);
   BPhysics::convertPoint(pivot, hkPivot);


   hkpPrismaticConstraintData* cinfo = new hkpPrismaticConstraintData();

   // Create constraint
   cinfo->setInWorldSpace(objectA->getRigidBody()->getTransform(), objectB->getRigidBody()->getTransform(), hkPivot, hkAxis);
   cinfo->setMaxLinearLimit(max);
   cinfo->setMinLinearLimit(min);

	mphkConstraint = new hkpConstraintInstance(objectA->getRigidBody(), objectB->getRigidBody(), cinfo);
   if (!mphkConstraint)
      return false;

   cinfo->removeReference();
   mType = cConstraintTypePrismatic;
   return(true);

}


//==============================================================================
// BConstraint::constraintBrokenCallback 
//==============================================================================
 void BConstraint::constraintBrokenCallback (hkpBreakableConstraintEvent &event)
 {
    BPhysicsEvent myevent;
    myevent.mType = cPhysicsEventConstraintBroken;
    myevent.mData1 = mType;

    if (mpListener != NULL)
      mpListener->notification(myevent);
 }

 //==============================================================================
 // BConstraint::makeBreakable
 //==============================================================================
 bool BConstraint::makeBreakable (float breakForce, bool bRemoveWhenBroken)
 {

    return (false);
/*
   //-- WMJ [6/1/2004] This is not a good idea :)
   //-- we need to know the basic type of constraint in the even system and this was masking it
   //mType = cConstraintTypeBreakable;

   // Create breakable constraint wrapper
   hkBreakableConstraint* breaker = new hkBreakableConstraint( mphkConstraint , mWorld->getHavokWorld() );
   breaker->setThreshold( breakForce );
   breaker->setRemoveWhenBroken(bRemoveWhenBroken);
  
   //-- the old one is now owned by the new breakable wrapper
   mphkConstraint->removeReference();


   mphkConstraint = breaker;
   breaker->setBreakableListener(this);

   return (true);
*/
 }

#pragma pop_macro("new")


//==============================================================================
// eof: constraint.cpp
//==============================================================================
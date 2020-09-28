//============================================================================
// File: physicsgravityballpullaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicsgravityballpullaction.h"
#include "world.h"
#include "physicsworld.h"
#include "Physics/Dynamics/World/hkpworld.h"
#include "object.h"
#include "physics.h"

//============================================================================
BPhysicsGravityBallPullAction::BPhysicsGravityBallPullAction(hkpEntity* body, BObject* pObject, const hkArray<hkpEntity*>& extraEntities) :
   hkpUnaryAction( body ),
   mRestLength(0.0f),
   mStrength(1000.0f),
   mDamping(0.5f),
   mOnCompression(false),
   mOnExtension(true)
{
   BASSERT(pObject);
   mpGravityBallObject = pObject;

   mExtraEntities.clear();
   for(long i = 0; i < extraEntities.getSize(); i++)
      mExtraEntities.pushBack(extraEntities[i]);
}

//============================================================================
void BPhysicsGravityBallPullAction::applyAction( const hkStepInfo& stepInfo )
{
   hkpRigidBody* pRb = static_cast<hkpRigidBody*>( m_entity );
   BASSERT(pRb);

   // add anti grav force
   hkVector4 antiGravForce;
   antiGravForce.setMul4( -1 * pRb->getMass(), gWorld->getPhysicsWorld()->getHavokWorld()->getGravity());
   pRb->applyForce( stepInfo.m_deltaTime, antiGravForce );     
   for(long i = 0; i < mExtraEntities.getSize(); i++)
   {
      hkpRigidBody* pExtraRB = static_cast<hkpRigidBody*>(mExtraEntities[i]);
      if(pExtraRB)
         pExtraRB->applyForce(stepInfo.m_deltaTime, antiGravForce);
   }

   // apply the spring force
   hkVector4 posA;
   BPhysics::convertPoint(mpGravityBallObject->getPosition(), posA);
   hkVector4 posB;
   posB = pRb->getCenterOfMassInWorld();

   hkVector4 dirAB; dirAB.setSub4( posB, posA );

   // what if rest length is not zero?
   hkReal length = dirAB.length3();
   if( length < 0.001f )	// can't normalize the zero vector!
      return;

   // normalise
   dirAB.mul4( 1.0f / length );

   if( !mOnCompression && (length < mRestLength) )
      return;
   if( !mOnExtension && (length > mRestLength) )
      return;

   hkVector4 velA;
   BPhysics::convertPoint(mpGravityBallObject->getVelocity(), velA);
   hkVector4 velB;
   pRb->getPointVelocity( posB, velB );
   hkVector4 velAB;
   velAB.setSub4( velB, velA );

   hkReal relVel = velAB.dot3( dirAB );

   hkReal force = (relVel * mDamping) + ((length - mRestLength) * mStrength);

   mLastForce.setMul4( -force, dirAB );
   pRb->applyForce( stepInfo.m_deltaTime, mLastForce, posB );
   for(long i = 0; i < mExtraEntities.getSize(); i++)
   {
      hkpRigidBody* pExtraRB = static_cast<hkpRigidBody*>(mExtraEntities[i]);
      if(pExtraRB)
         pExtraRB->applyForce(stepInfo.m_deltaTime, mLastForce, pExtraRB->getCenterOfMassInWorld());
   }

   // apply an impulse if the lateral velocity is too slow
   BVector objectVel, objectLateralVel, dir;
   BPhysics::convertPoint(velB, objectVel);
   BPhysics::convertPoint(dirAB, dir);
   dir.normalize();
   objectVel.projectOntoPlaneAsVector(dir, objectLateralVel);

   if (objectLateralVel.length() < mMinLateralSpeed)
   {
      hkVector4 lateralImp;
      BPhysics::convertPoint(objectLateralVel, lateralImp);
      lateralImp.mul4(pRb->getMass());

      pRb->applyPointImpulse(lateralImp, posB);
      for(long i = 0; i < mExtraEntities.getSize(); i++)
      {
         hkpRigidBody* pExtraRB = static_cast<hkpRigidBody*>(mExtraEntities[i]);
         if(pExtraRB)
            pExtraRB->applyPointImpulse(lateralImp, pExtraRB->getCenterOfMassInWorld());
      }
   }
}
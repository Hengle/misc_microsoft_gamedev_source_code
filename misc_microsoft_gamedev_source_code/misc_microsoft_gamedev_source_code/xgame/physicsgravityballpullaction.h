//============================================================================
// File: physicsgravityballpullaction.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Physics/Dynamics/Entity/hkpRigidBody.h"

class BObject;

//============================================================================
// BPhysicsGroundVehicleAction
// Simulates physics for the groundVehicle
//============================================================================
class BPhysicsGravityBallPullAction: public hkpUnaryAction
{
public:
   BPhysicsGravityBallPullAction(hkpEntity* body, BObject* pObject, const hkArray<hkpEntity*>& extraEntities);
   void applyAction( const hkStepInfo& stepInfo );
   hkpAction* clone( const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms ) const { return HK_NULL; }

   inline void setStrength( hkReal str ) { mStrength = str; }
   inline hkReal getStrength() const { return mStrength; }

   inline void setDamping( hkReal damp ) { mDamping = damp; }
   inline hkReal getDamping() const { return mDamping; }

   inline void setRestLength( hkReal restLength ) { mRestLength = restLength; }
   inline hkReal getRestLength() const { return mRestLength; }

   inline void setMinLateralSpeed( hkReal MinLateralSpeed ) { mMinLateralSpeed = MinLateralSpeed; }
   inline hkReal getMinLateralSpeed() const { return mMinLateralSpeed; }

   inline void setOnCompression( hkBool onCompression ) { mOnCompression = onCompression; }
   inline hkBool getOnCompression() const { return mOnCompression; }

   inline void setOnExtension( hkBool onExtension ) { mOnExtension = onExtension; }
   inline hkBool getOnExtension() const { return mOnExtension; }

   inline const hkVector4& getLastForce() const { return mLastForce; }

private:
   BObject* mpGravityBallObject;
   
   hkArray<hkpEntity*> mExtraEntities;

   hkVector4 mLastForce;
   hkReal	mRestLength;
   hkReal	mStrength;
   hkReal	mDamping;
   hkReal	mMinLateralSpeed;
   hkBool	mOnCompression;
   hkBool	mOnExtension;
};

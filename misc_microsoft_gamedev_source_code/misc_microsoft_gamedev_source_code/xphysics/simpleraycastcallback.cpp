//==============================================================================
// simpleraycastcallback.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "simpleraycastcallback.h"
#include "physics.h"

//==============================================================================
// Defines


//==============================================================================
// BSimpleRaycastCallback::BSimpleRaycastCallback
//==============================================================================
BSimpleRaycastCallback::BSimpleRaycastCallback(const hkpWorldRayCastInput& input, hkpWorldRayCastOutput* output, hkUint32 ignoreFilter)
: mInput(input), mOutput(output), mhkCollisionFilterInfo(ignoreFilter)
{
   
} // BSimpleRaycastCallback::BSimpleRaycastCallback

//==============================================================================
// BSimpleRaycastCallback::~BSimpleRaycastCallback
//==============================================================================
BSimpleRaycastCallback::~BSimpleRaycastCallback(void)
{

} // BSimpleRaycastCallback::~BSimpleRaycastCallback

//==============================================================================
// BSimpleRaycastCallback::addBroadPhaseHandle
//==============================================================================
hkReal BSimpleRaycastCallback::addBroadPhaseHandle(const hkpBroadPhaseHandle* broadphaseHandle)
{
   hkpCollidable* col = (hkpCollidable*) broadphaseHandle;
   const hkpShape* shape = col->getShape();

   hkReal hitFraction = mOutput->m_hitFraction;
   //-- ignore stuff that we have specified to ignore
   if (mhkCollisionFilterInfo!=0 && col->getCollisionFilterInfo() == mhkCollisionFilterInfo)
      return hitFraction;

   
   if (shape)
   {
      hkpShapeRayCastInput sinput;
      const hkTransform& trans = col->getTransform();

      // transform the ray into local space
      sinput.m_from.setTransformedInversePos(trans, mInput.m_from);
      sinput.m_to.setTransformedInversePos(  trans, mInput.m_to);

      // subshape filtering turned off
      sinput.m_rayShapeCollectionFilter = HK_NULL;


      if (shape->castRay(sinput, *mOutput))
      {	 
         mOutput->m_rootCollidable = col;
         // transform the normal back into worldspace
         mOutput->m_normal.setRotatedDir( trans.getRotation(), mOutput->m_normal );
      }
     
   }

   // return the current hitFraction, this will allow the broadphase to do early outs for
   // objects more distant than the current hit
   return mOutput->m_hitFraction;
}

//==============================================================================
// eof: simpleraycastcallback.cpp
//==============================================================================
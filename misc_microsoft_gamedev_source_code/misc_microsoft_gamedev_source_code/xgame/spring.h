//==============================================================================
// spring.h
//
// Copyright (c) 2004-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes


//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


//==============================================================================
// Class BSpring represents a spring with a single degree of freedom.  It will
// oscillate about 0.0 according to the paramaters specified (mass, spring constant
// K, damping ratio, current position, and applied forces).
//
// K and mass will determine how quickly the spring oscillates - the natural harmonic
// frequency of the oscillation will be sqrt(k / m).
//
// The damping ratio determines how quickly the spring's oscillation will decrease
// to nothing due to friction.  Below is a list of damping types:
// Damping ratio = 0.0   - No damping
// 0 < Damping ratio < 1 - Under-damped
// Damping ratio = 1.0   - Crtically damped
// Damping ratio > 1     - Over-damped
//
// Added forces are instantaneous and therefore should take time into consideration

class BSpring
{
   public :
      BSpring()
      {
         mfMassRecip = 1.0f;
         mfK = 1.0f;
         mfCurrentPosition = 0.0f;
         mfCurrentVelocity = 0.0f;
         mfMaxOffset = 1.0f;
      };

      ~BSpring()
      {
      };

      inline void init(float currentPos, float k, float mass, float dampingRatio, float maxOffset);
      inline void update(float deltaTime);
      inline void addForce(float force);
      

      // Spring data
      float                mfMassRecip;
      float                mfK;
      float                mfCurrentPosition;
      float                mfCurrentVelocity;
      float                mfDampingConstant;
      float                mfMaxOffset;
      BDynamicSimArray<float>  mForces;

      static const DWORD      msSaveVersion;
};

//==============================================================================
// Inline functions
//==============================================================================

//==============================================================================
// BSpring::init
//==============================================================================
inline void BSpring::init(float currentPos, float k, float mass, float dampingRatio, float maxOffset)
{
   mfCurrentVelocity = 0.0f;
   mfCurrentPosition = currentPos;
   mfK = k;
   mfMassRecip = 1.0f / mass;
   mfMaxOffset = maxOffset;

   // Damping ratio = damping constant / (2*m*harmonic frequence=sqrt(k/m)
   mfDampingConstant = dampingRatio * 2.0f * mass * sqrt(k / mass);
}

//==============================================================================
// BSpring::update
//==============================================================================
inline void BSpring::update(float deltaTime)
{
   // Spring force + damping force
   float forceSum = -mfK * mfCurrentPosition - mfDampingConstant * mfCurrentVelocity;

   // External forces
   long i;
   for (i = 0; i < mForces.getNumber(); i++)
   {
      forceSum += mForces[i];
   }
   mForces.clear();

   // Velocity += a (= F / m) * dt
   mfCurrentVelocity += forceSum * mfMassRecip * deltaTime;
   if (fabs(mfCurrentVelocity) < cFloatCompareEpsilon)
   {
      mfCurrentVelocity = 0.0f;
   }

   // Position += v * dt
   mfCurrentPosition += mfCurrentVelocity * deltaTime;
   if (mfCurrentPosition > mfMaxOffset)
   {
      mfCurrentPosition = mfMaxOffset;
      mfCurrentVelocity = 0.0f;
   }
   else if (mfCurrentPosition < -mfMaxOffset)
   {
      mfCurrentPosition = -mfMaxOffset;
      mfCurrentVelocity = 0.0f;
   }
   if (fabs(mfCurrentPosition) < cFloatCompareEpsilon)
   {
      mfCurrentPosition = 0.0f;
   }
}

//==============================================================================
// BSpring::addForce
//==============================================================================
inline void BSpring::addForce(float force)
{
   mForces.add(force);
}
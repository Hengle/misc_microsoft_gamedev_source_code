//==============================================================================
// physicsinternal.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================



#ifndef _PHYSICS_INTERNAL_H_
#define _PHYSICS_INTERNAL_H_


//==============================================================================
// Error Handling 3.0
//==============================================================================
class BPhysicsError : public hkError
{
   /// displayMessage is used for asserts
   virtual int message(Message m, int id, const char* description, const char* file, int line);

   /// Enables/disables diagnostic by id.
   virtual void setEnabled( int id, hkBool enabled );

   /// Enables/disables diagnostic by id.
   virtual hkBool isEnabled( int id );

   /// Force all diagnostics to be enabled.
   virtual void enableAll();
};

#if 0
//==============================================================================
// Havok Utility Functions
//==============================================================================
class BPhysicsUtil
{
public:
   static hkpRigidBody* createBox(const hkVector4 &halfExtents, const hkReal mass, const hkVector4 &position);
   static hkpRigidBody* createSphere(const hkReal radius, const hkReal mass, const hkVector4 &position);
};

#endif

//==============================================================================
#endif // _PHYSICS_INTERNAL_H_

//==============================================================================
// eof: physicsinternal.h
//==============================================================================
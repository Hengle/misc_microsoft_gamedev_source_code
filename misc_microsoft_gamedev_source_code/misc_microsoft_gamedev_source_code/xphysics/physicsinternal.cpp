//============================================================================
// physicsinternal.cpp
//
// Copyright (c) 2003, Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "physicsinternal.h"
#include "consoleOutput.h"



#pragma push_macro("new")
#undef new



//==============================================================================
// BPhysicsError::message
//==============================================================================
int BPhysicsError::message(Message m, int id, const char* description, const char* file, int line)
{
   switch (m)
   {
      case MESSAGE_REPORT :
         gConsoleOutput.debug("Havok Report ID:0x%x, Description:%.128s, file:%s(%d)", id, description, file, line);
         break;
      case MESSAGE_WARNING :
         gConsoleOutput.warning("Havok Warning ID:0x%x, Description:%.128s, file:%s(%d)", id, description, file, line);
         break;
      case MESSAGE_ASSERT :
         gConsoleOutput.warning("Havok Assert ID:0x%x, Description:%.128s, file:%s(%d)", id, description, file, line);
         break;
      case MESSAGE_ERROR :
         gConsoleOutput.error("Havok Error ID:0x%x, Description:%.128s, file:%s(%d)", id, description, file, line);
         break;
   }
   trace("what:%u id:0x%x description:%s file:%s(%d)", m, id, description, file, line);
   return 0;
}

//==============================================================================
// BPhysicsError::setEnabled
//==============================================================================
void BPhysicsError::setEnabled( int id, hkBool enabled )
{
   return;
}

//==============================================================================
// BPhysicsError::isEnabled
//==============================================================================
hkBool BPhysicsError::isEnabled( int id )
{
   return (true);
}

//==============================================================================
// BPhysicsError::enableAll
//==============================================================================
void BPhysicsError::enableAll()
{
   return;
}


#if 0

//==============================================================================
// BPhysicsUtil::createBox
//==============================================================================
hkpRigidBody* BPhysicsUtil::createBox(const hkVector4 &halfExtents, const hkReal mass, const hkVector4 &position)
{
  
   hkpBoxShape* cube = new hkpBoxShape(halfExtents);	

   //
   // Create a rigid body construction template
   //
   hkpRigidBodyCinfo boxInfo;

   if(mass != 0.0f)
   {
      boxInfo.m_mass = mass;
      hkpMassProperties massProperties;
      hkpInertiaTensorComputer::computeBoxVolumeMassProperties(halfExtents, mass, massProperties);
      boxInfo.m_inertiaTensor = massProperties.m_inertiaTensor;
      boxInfo.m_motionType = hkpMotion::MOTION_BOX_INERTIA;

   }
   else
   {	
      boxInfo.m_motionType = hkpMotion::MOTION_FIXED;

   }
   boxInfo.m_rotation.setIdentity();
   boxInfo.m_shape = cube;
   boxInfo.m_position = position;

   //
   // create a rigid body (using the template above) 
   //

   hkpRigidBody* boxRigidBody = new hkpRigidBody(boxInfo);

   cube->removeReference();

   return boxRigidBody;
}

//==============================================================================
// BPhysicsUtil::createSphere
//==============================================================================
hkpRigidBody* BPhysicsUtil::createSphere(const hkReal radius, const hkReal mass, const hkVector4 &position)
{
   hkpSphereShape* sphere = new hkpSphereShape(radius);

   hkpRigidBodyCinfo sphereInfo;
   
   if(mass != 0.0f)
   {
      sphereInfo.m_mass = mass;
      hkpMassProperties massProperties;
      hkpInertiaTensorComputer::computeSphereVolumeMassProperties(radius, mass, massProperties);
      sphereInfo.m_inertiaTensor = massProperties.m_inertiaTensor;

      sphereInfo.m_motionType = hkpMotion::MOTION_SPHERE_INERTIA;

   }
   else
   {		
      sphereInfo.m_motionType = hkpMotion::MOTION_FIXED;

   }

   sphereInfo.m_rotation.setIdentity();
   sphereInfo.m_shape = sphere;
   sphereInfo.m_position = position;

   hkpRigidBody* sphereRigidBody = new hkpRigidBody(sphereInfo);
   
   sphere->removeReference();
   
   return sphereRigidBody;
}
#endif

#pragma pop_macro("new")


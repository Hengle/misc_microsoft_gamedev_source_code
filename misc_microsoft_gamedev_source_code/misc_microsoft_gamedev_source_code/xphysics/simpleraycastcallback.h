//==============================================================================
// simpleraycastcallback.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================



#ifndef _SIMPLE_RAYCAST_CALLBACK_H_
#define _SIMPLE_RAYCAST_CALLBACK_H_
#include "physicsworld.h"

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


//==============================================================================
class BSimpleRaycastCallback : public hkpBroadPhaseCastCollector
{
public:

	BEGIN_FLAG_MAP
		ADD_FLAG_ENTRY(cFlagIgnoreBackfaces,       1)
	END_FLAG_MAP

   // Constructors
   BSimpleRaycastCallback(const hkpWorldRayCastInput& input, hkpWorldRayCastOutput* output, hkUint32 ignoreFilter);

   // Destructors
   virtual ~BSimpleRaycastCallback( void );

   void           setCollisionFilterInfo(hkUint32 hkFilter)       { mhkCollisionFilterInfo = hkFilter; }
   hkUint32       getCollisionFilterInfo( void ) const            { return mhkCollisionFilterInfo;     }


protected:

   /// the function which is called every time the broadphase raycaster hits the aabb of an
   /// object. This implementation checks the type of object and calls object->raycast if
   /// necessary

   virtual	hkReal addBroadPhaseHandle( const hkpBroadPhaseHandle* broadPhaseHandle );

   /// The information about the ray start and end point
   hkpWorldRayCastInput           mInput;

   /// A pointer to the result data structure
   hkpWorldRayCastOutput*         mOutput;

   hkUint32                      mhkCollisionFilterInfo;
   
   DECLARE_FLAGS;

}; 


//==============================================================================
#endif // _SIMPLE_RAYCAST_CALLBACK_H_

//==============================================================================
// eof: simpleraycastcallback.h
//==============================================================================
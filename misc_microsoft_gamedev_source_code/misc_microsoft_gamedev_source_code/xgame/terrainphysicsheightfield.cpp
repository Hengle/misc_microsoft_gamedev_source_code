//==============================================================================
// terrainphysicsheightfield.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#include "common.h"


#include "terrainphysicsheightfield.h"
#include "game.h"
#include "world.h"
#include "physics.h"
#include "physicsworld.h"

//==============================================================================
// BTerrainPhysicsHeightfield::~BTerrainPhysicsHeightfield
//==============================================================================
BTerrainPhysicsHeightfield::~BTerrainPhysicsHeightfield()
{
   BScopedPhysicsWrite maker(gWorld->getPhysicsWorld());

	removeEntityFromWorld();
}


//==============================================================================
// BTerrainPhysicsHeightfield::removeEntityFromWorld
//==============================================================================
void BTerrainPhysicsHeightfield::removeEntityFromWorld(void)
{
   // No need to do anything if we have no entity.
   if(!mHavokEntity)
      return;
      
   // Remove from havok world.
   mHavokEntity->getWorld()->removeEntity(mHavokEntity);
      
   // Remove our reference to it.
   mHavokEntity->removeReference();
      
   // Null out pointer.
   mHavokEntity = NULL;
}


//==============================================================================
// BTerrainPhysicsHeightfield::init
//==============================================================================
bool BTerrainPhysicsHeightfield::init( void )
{

   BScopedPhysicsWrite marker(gWorld->getPhysicsWorld());

   // Nuke any existing entity.
   removeEntityFromWorld();
   
   // Assemble construction info for havok.
	hkpSampledHeightFieldBaseCinfo ci;
	
	// Size in each direction (plus a skirt)
	ci.m_xRes = gTerrainSimRep.getNumXHeightVerts() + (cNumSkirtVerts * 2);
	ci.m_zRes = gTerrainSimRep.getNumXHeightVerts() + (cNumSkirtVerts * 2);

   // Let havok know about the scaling of the heightfield.  The Z scale is negative because we're converting
   // to havok's coordinate system here.	
   #ifdef HANDEDNESS_FLIP
      ci.m_scale.set(gTerrainSimRep.getHeightTileScale(), 1.0f, -gTerrainSimRep.getHeightTileScale());
   #else
	   ci.m_scale.set(gTerrainSimRep.getHeightTileScale(), 1.0f, gTerrainSimRep.getHeightTileScale());
   #endif

   // Create the shape.
	BTerrainPhysicsHeightfieldShape *heightFieldShape = new BTerrainPhysicsHeightfieldShape(ci);


   // Set up rigid body info 
   hkpRigidBodyCinfo bodyCI;
   
   // Terrain can't move.
   bodyCI.m_motionType = hkpMotion::MOTION_FIXED;
	
	// Located at the origin (minus an offset for the skirt)
	bodyCI.m_position.set(-gTerrainSimRep.getHeightTileScale() * cNumSkirtVerts, 0.0f, -gTerrainSimRep.getHeightTileScale() * cNumSkirtVerts);
	
	// Hand it the shape object.
	bodyCI.m_shape = heightFieldShape;
	
	// Hack: hardcoded properties for now.
	bodyCI.m_friction = 1.9f;
	bodyCI.m_restitution = 1.0f;
   bodyCI.m_collisionFilterInfo = gWorld->getPhysicsWorld()->createCollisionFilterInfo(BPhysicsWorld::cLayerTerrain);


   // Create the rigid body.
   hkpRigidBody* rigidBody = new hkpRigidBody(bodyCI);
   if(!rigidBody)
      return(false);

   // Add it to the world.
   mHavokEntity = gWorld->getPhysicsWorld()->getHavokWorld()->addEntity(rigidBody);


   // Remove our reference to the shape since we won't be holding a pointer to it.
	heightFieldShape->removeReference();
	
	// Success.
	return(true);
}

//==============================================================================
// BTerrainPhysicsHeightfield::collideSpheres
//==============================================================================
void BTerrainPhysicsHeightfieldShape::collideSpheres( const CollideSpheresInput& input, SphereCollisionOutput* outputArray) const
{
      return hkSampledHeightFieldShape_collideSpheres<BTerrainPhysicsHeightfieldShape>(*this, input, outputArray);
}




//==============================================================================
// eof: terrainphysicsheightfield.cpp
//==============================================================================

//==============================================================================
// terrainphysicsheightfield.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once


//==============================================================================
// Includes
#include "xphysics.h"
#include "terrainsimrep.h"

#include <Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h>
#include <Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldBaseCinfo.h>

//==============================================================================
// Forward declarations
class BTerrainBase;

const int cNumSkirtVerts = 10;

//==============================================================================
// class BTerrainPhysicsHeightfieldShape
// 
// Shape for terrain derived from havok class -- we must implement height function.
//==============================================================================
class BTerrainPhysicsHeightfieldShape : public hkpSampledHeightFieldShape
{
	public:
		BTerrainPhysicsHeightfieldShape( const hkpSampledHeightFieldBaseCinfo& ci) : hkpSampledHeightFieldShape(ci)
		  
		{
		}

      /// hkpHeightFieldShape interface implementation
      virtual void collideSpheres( const CollideSpheresInput& input, SphereCollisionOutput* outputArray) const;

		HK_FORCE_INLINE hkReal  getHeightAt(int x, int z) const 
      {
         float ret=0.0f;
         gTerrainSimRep.getHeight(x - cNumSkirtVerts, z - cNumSkirtVerts, ret, true);
         return ret;
      }
		
		// Our triangles are always "flipped" in havok's terms.
		HK_FORCE_INLINE hkBool getTriangleFlip() const {return true;}
		
 
};


//==============================================================================
// class BTerrainPhysicsHeightfield
//==============================================================================
class BTerrainPhysicsHeightfield
{
   public:
                              BTerrainPhysicsHeightfield(void) : mHavokEntity(NULL) {}
                              ~BTerrainPhysicsHeightfield();
                              
      bool                    init( void );

      const hkpEntity*         getHKEntity() const { return mHavokEntity; }
      
   
   protected:
      void                    removeEntityFromWorld(void);
   
      hkpEntity                *mHavokEntity;
};



//==============================================================================
// eof: terrainphysicsheightfield.h
//==============================================================================

//==============================================================================
// pathenginemanager.h
//
// pathenginemanager manages the PathEngine interface
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
//==============================================================================

#if 0
#include <vector>

#include "i_pathengine.h"

//==============================================================================
// Defines
//==============================================================================
#define MESH_SCALE      100
#define MAX_NUM_SHAPES  30
#define MAX_NUM_STATIONARY_ENTITIES   500

//==============================================================================
//Forward Declarations
//==============================================================================
class iShape;
class iMesh;
class iAgent;
class iPath;
class iPathEngine;
class iCollisionContext;
class BEntity;

typedef struct 
{
   iShape*        shape;
   int            width;
}BUnitShape;

typedef struct 
{
   iShape*        shape;
   int            width;
   int            length;
}BBuildingShape;


class BPathEngineManager
{
public:

   BPathEngineManager();
   virtual ~BPathEngineManager();
   
   bool                    init();   
   void                    reset();
   bool                    loadMesh( const char* filename );
   iMesh*                  getPathMeshGround() { return mpPathMeshGround; }
   iMesh*                  getPathMeshAir() { return mpPathMeshAir; }
   iCollisionContext*      getCollisionContextGround() { return mpCollisionContextGround; }
   iCollisionContext*      getCollisionContextAir() { return mpCollisionContextAir; }
   iCollisionContext*      getCollisionContextStationary() { return mpCollisionContextStationary; }

   iObstacleSet*           getObstacleSetStationary() { return mpObstacleSetStationary; }

   cPosition               getMeshPosition( BVector& vec, iMesh* mesh );
   BVector                 getVectorFromMesh( cPosition& pos, iMesh* mesh );

   iPathEngine*            getPathEnginePtr() { return pPathEngine; }
   iShape*                 getPathEngineBuildingShape( int width, int length );
   iShape*                 getPathEngineGroundUnitShape( int width );
   iShape*                 getPathEngineFlyingUnitShape( int width );

   iShape*                 getTestBox1000() { return mpTestBox1000; }
   BEntity*                getStationaryEntity( int i ) { return mStationaryEntity[i]; }
   int                     getNumStationaryEntities() { return mNumStationaryEntities; }
   void                    addStationaryEntity( BEntity* entity );

protected:

   iPathEngine*            pPathEngine;
   iMesh*                  mpPathMeshGround;
   iMesh*                  mpPathMeshAir;
   iCollisionContext*      mpCollisionContextGround;
   iCollisionContext*      mpCollisionContextAir;
   iCollisionContext*      mpCollisionContextStationary;

   iObstacleSet*           mpObstacleSetStationary;

   int                     mNumGroundUnitShapes;
   int                     mNumFlyingUnitShapes;
   int                     mNumBuildingShapes;

   BUnitShape              mGroundUnitShape[MAX_NUM_SHAPES];
   BUnitShape              mFlyingUnitShape[MAX_NUM_SHAPES];
   BBuildingShape          mBuildingShape[MAX_NUM_SHAPES];
   BEntity*                mStationaryEntity[MAX_NUM_STATIONARY_ENTITIES];
   int                     mNumStationaryEntities;
   iShape*                 mpTestBox1000;

   bool                    loadECFPTH(int dirID,const char *filenamePTH);
};
#endif
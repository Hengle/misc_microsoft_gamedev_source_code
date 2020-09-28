//==============================================================================
// pathenginemanager.cpp
//
// pathenginemanager
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
//xgame
#include "common.h"
#include "pathenginemanager.h"
#include "gamedirectories.h"
#include "Singletons.h"
#include "configsgame.h"
#include "database.h"
#include "protoobject.h"
//terrain
#include "TerrainVisual.h"
//#include "world.h"

// xcore
#include "consoleOutput.h"
#include "resource\ecfUtils.h"

// xsystem
#include "bfileStream.h"

//general
#include <stdio.h>

#if 0

void
PathEngine_DebugBreak()
{
   DebugBreak();
}
void
PathEngine_Abort()
{
   _exit(1);
}

bool MemoryTrackingIsEnabled() {return false;}
unsigned long GetTotalMemoryAllocated() {return 0;}
unsigned long GetMaximumMemoryAllocated() {return 0;}
void ResetMaximumMemoryAllocated() {}
unsigned long GetTotalNumberOfAllocations() {return 0;}

#pragma comment(lib, "Content_Processing_Xenon_Release.lib")
#pragma comment(lib, "Expat_Xenon_Release.lib")
#pragma comment(lib, "Geometry_Xenon_Release.lib")
#pragma comment(lib, "Mesh2D_Xenon_Release.lib")
#pragma comment(lib, "Mesh3D_Xenon_Release.lib")
#pragma comment(lib, "MeshPair_Xenon_Release.lib")
#pragma comment(lib, "ParseXML_Xenon_Release.lib")
#pragma comment(lib, "PathEngine_Core_Xenon_Release.lib")
#pragma comment(lib, "PathEngine_Interface_Xenon_Release.lib")

class cMinimalErrorHandler : public iErrorHandler
{
public:
   eAction
      handle(const char* type, const char* description, const char *const* attributes)
   {
      DebugBreak();
      return CONTINUE;
   }
};


//==============================================================================
// BPathEngineManager::BPathEngineManager
//==============================================================================
BPathEngineManager::BPathEngineManager()
{
   pPathEngine = NULL;
   mpPathMeshGround  = NULL;
   mpPathMeshAir     = NULL;
   mpCollisionContextGround      = NULL;
   mpCollisionContextAir         = NULL;
   mpCollisionContextStationary  = NULL;

   mNumStationaryEntities =  0;
   mpObstacleSetStationary = NULL;
}

//==============================================================================
// BPathEngineManager::~BPathEngineManager
//==============================================================================
BPathEngineManager::~BPathEngineManager()
{
}

//==============================================================================
// BPathEngineManager::init
//==============================================================================
bool BPathEngineManager::init()
{
   // Init PathEngine
   nSingletons::init_StandAlone();
   pPathEngine = &nSingletons::pathEngineI();

   //******* make sure you setup a suitable error handler - this one just debug breaks
   cMinimalErrorHandler errorHandler;
   pPathEngine->setErrorHandler(&errorHandler);

   mNumGroundUnitShapes = 0;
   mNumFlyingUnitShapes = 0;
   mNumBuildingShapes = 0;

   return true;
}

//==============================================================================
// BPathEngineManager::reset
//==============================================================================
void BPathEngineManager::reset()
{
   nSingletons::shutDown_StandAlone();
}
//==============================================================================
// BPathEngineManager::loadECFPTH Enums
//==============================================================================
enum ePTHChunkID
{
   cPTHHeaderID =  0x34F00817,
   cPTHVersion  = 0x0001,
   cPTHGroundMeshID = 0x0001,
   cPTHPrecomputedShapeID = 0x0002,
   cPTHAirMeshID = 0x0003,
};

enum ePathEngineShapeID
{
   cPTHBuildingShapeID = 0xAAAA,
   cPTHUnitShapeID = 0xBBBB,
   cPTHAirShapeID = 0xCCCC,
};
//==============================================================================
// BPathEngineManager::loadECFPTH
//==============================================================================
bool BPathEngineManager::loadECFPTH(int dirID,const char *filenamePTH)
{
   mNumGroundUnitShapes = 0;
   mNumFlyingUnitShapes = 0;
   mNumBuildingShapes = 0;

   //char buf[256];
   //dirID = cDirProduction;
   gConsoleOutput.resource("BPathEngineManager::loadECFPTH: %s", filenamePTH);

   BFileSystemStream tfile;
   if (!tfile.open(dirID, filenamePTH, cSFReadable | cSFSeekable))
   {
      gConsoleOutput.error("BPathEngineManager::loadECFPTH : Unable to open file %s\n", filenamePTH);
      return false;
   }

    

   BECFFileStream ecfReader;
   if(!ecfReader.open(&tfile)) //file checking and version ID done during constructor
   {
      gConsoleOutput.error("BPathEngineManager::loadECFPTH : ECFHeader or Checksum invalid  %s\n", filenamePTH);
      tfile.close();
      return false;
   }

   BECFHeader ecfHeader = ecfReader.getHeader();
   int numECFChunks = ecfHeader.getNumChunks();


   //find our PTHChunk header
   int headerIndex = ecfReader.findChunkByID(cPTHHeaderID);
   if(headerIndex==-1)
   { 
      gConsoleOutput.error("BPathEngineManager:loadECFPTH : Could not find PTH Chunk Header\n");
      tfile.close();
      return false;
   }
   ecfReader.seekToChunk(headerIndex);


   //check our header data
   int version =0;
   ecfReader.getStream()->readObjLittleEndian(version);
   if(version !=cPTHVersion)
   {
      gConsoleOutput.error("BPathEngineManager:loadECFPTH : Could not find PTH Chunk Header\n");
      tfile.close();
      return false;
   }

   //we're good. start walking through chunks and doing stuff...
   for(int i=0;i<numECFChunks;i++)
   {
      const BECFChunkHeader& cHeader = ecfReader.getChunkHeader(i);
      const uint64 id = cHeader.getID();
      switch(id)
      {
      case cPTHGroundMeshID:
         {
            ecfReader.seekToChunk(i);

            int dataSize = cHeader.getSize();
            char* pFileData = new char[dataSize];
            ecfReader.getStream()->readBytes(pFileData,dataSize);

            mpPathMeshGround = pPathEngine->loadMeshFromBuffer("tok", pFileData, dataSize, 0);
            if( mpPathMeshGround )
            {
               char* attributes[3];
               attributes[0] = "markForPreprocessing";
               attributes[1] = "true";
               attributes[2] = 0;               
               mpObstacleSetStationary = mpPathMeshGround->newObstacleSet_WithAttributes(attributes);

               mpCollisionContextGround = mpPathMeshGround->newContext();
               mpCollisionContextStationary = mpPathMeshGround->newContext();
//               mpCollisionContextStationary->addObstacleSet(mpObstacleSetStationary);
            }
            else
            {
               gConsoleOutput.error("BPathEngineManager:loadECFPTH : Could not Load Ground Mesh\n");
               delete []pFileData;
               tfile.close();
               //DebugBreak();
               return false;
            }
            delete []pFileData;

            break;
         }
      case cPTHAirMeshID:
         {
            ecfReader.seekToChunk(i);

            int dataSize = cHeader.getSize();
            char* pFileData = new char[dataSize];
            ecfReader.getStream()->readBytes(pFileData,dataSize);

            mpPathMeshAir = pPathEngine->loadMeshFromBuffer("tok", pFileData, dataSize, 0);
            if( mpPathMeshAir )
            {
               mpCollisionContextAir = mpPathMeshAir->newContext();
            }
            else
            {
               gConsoleOutput.error("BPathEngineManager:loadECFPTH : Could not Load Air Mesh\n");
               delete []pFileData;
               tfile.close();
               //DebugBreak();
               return false;
            }
            delete []pFileData;

            break;
         }
      case cPTHPrecomputedShapeID:
         {
            ecfReader.seekToChunk(i);

            //general shape info
            int shapeType=0;
            int shapeTypeIndex=0;
            int shapeSize=0;
            ecfReader.getStream()->readObjLittleEndian(shapeType);
            ecfReader.getStream()->readObjLittleEndian(shapeTypeIndex);
            ecfReader.getStream()->readObjLittleEndian(shapeSize);

            //read the shape definition data
            int numshapeVerts=0;
            
            ecfReader.getStream()->readObjLittleEndian(numshapeVerts);
            int shapeVertMemSize = numshapeVerts*2*sizeof(long);

            long *shapeArrayDat =new long[numshapeVerts*2];
            ecfReader.getStream()->readBytes(shapeArrayDat,shapeVertMemSize);


            //cater to specific shape indexes
            iShape *shape =pPathEngine->newShape(numshapeVerts, shapeArrayDat);
            if(shapeType==cPTHBuildingShapeID)
            {
               mBuildingShape[shapeTypeIndex].shape=shape;
               mBuildingShape[shapeTypeIndex].length =shapeSize;
               mBuildingShape[shapeTypeIndex].width =shapeSize;
               mNumBuildingShapes++;
            }
            else if(shapeType==cPTHUnitShapeID)
            {
               mGroundUnitShape[shapeTypeIndex].shape=shape;
               mGroundUnitShape[shapeTypeIndex].width = shapeSize;
               mNumGroundUnitShapes++;
            }
            else if(shapeType==cPTHAirShapeID)
            {
               mFlyingUnitShape[shapeTypeIndex].shape=shape;
               mFlyingUnitShape[shapeTypeIndex].width = shapeSize;
               mNumFlyingUnitShapes++;
            }
            
           

            //read the pathing preprocessed data
            long shapePathingDataSize=0;
            long shapeCollisionDataSize=0;
            ecfReader.getStream()->readObjLittleEndian(shapePathingDataSize);
            ecfReader.getStream()->readObjLittleEndian(shapeCollisionDataSize);

            char *pPathingData =new char[shapePathingDataSize];
            ecfReader.getStream()->readBytes(pPathingData, shapePathingDataSize);

            char *pCollisionData =new char[shapeCollisionDataSize];
            ecfReader.getStream()->readBytes(pCollisionData, shapeCollisionDataSize);

            if(shapeType==cPTHBuildingShapeID || shapeType==cPTHUnitShapeID)
            {
               mpPathMeshGround->loadCollisionPreprocessFor(shape,pCollisionData,shapeCollisionDataSize);
               if(shapePathingDataSize)
                  mpPathMeshGround->loadPathfindPreprocessFor(shape,pPathingData,shapePathingDataSize);
            }
            else if(shapeType==cPTHAirShapeID) 
            {
               mpPathMeshAir->loadCollisionPreprocessFor(shape,pCollisionData,shapeCollisionDataSize);
               mpPathMeshAir->loadPathfindPreprocessFor(shape,pPathingData,shapePathingDataSize);
            }

            

            shape=NULL;
            delete [] shapeArrayDat;
            delete []pPathingData;
            delete []pCollisionData;

            break;
         }
     
      }
   }

   tfile.close();
   return true;
}


//==============================================================================
// BPathEngineManager::loadMesh
//==============================================================================
bool BPathEngineManager::loadMesh( const char* fileName )
{
   if (!gConfig.isDefined(cConfigUsePathEngine))
      return( false);

   BString meshPath = fileName;
   meshPath += ".PTH";

   const char * path = meshPath.asNative();

   bool success = loadECFPTH(cDirProduction,path);
   
   // Set up test shape(s)
   int radius = 10 * MESH_SCALE;
   long array[]=
   {
      -radius, radius,
      radius, radius,
      radius, -radius,
      -radius, -radius
   };
   mpTestBox1000 = pPathEngine->newShape(sizeof(array)/sizeof(*array)/2, array);
   mpPathMeshGround->generateCollisionPreprocessFor(mpTestBox1000, 0);

   return( success );
}

//==============================================================================
// BPathEngineManager::getMeshPosition
//==============================================================================
cPosition BPathEngineManager::getMeshPosition( BVector& vec, iMesh* mesh )
{
   cPosition position;

   long point[3];
   BVector modVec = vec * MESH_SCALE;
   point[0] = (long) modVec.x;
   point[1] = (long) modVec.z;
   point[2] = 0;

   position = mesh->positionFor3DPoint( point );

   int tries = 0;
   long horizontalRange = 100 * MESH_SCALE;
   long verticalRange = 100 * MESH_SCALE;
   while( position.cell == -1 ) // Invalid position
   {
      if( tries > 10 )
         BASSERT(0);

      position = mesh->positionNear3DPoint( point, horizontalRange, verticalRange );
      horizontalRange += 100 * MESH_SCALE;
      verticalRange += 100 * MESH_SCALE;
      tries++;
   }

   return( position );
}

//==============================================================================
// BPathEngineManager::getVectorFromMesh
//==============================================================================
BVector BPathEngineManager::getVectorFromMesh( cPosition& pos, iMesh* mesh )
{
   BVector vec;
   vec.x = (float) pos.x / MESH_SCALE;
   vec.y = 0.0f;
   vec.z = (float) pos.y / MESH_SCALE;

   return( vec );
}


//==============================================================================
// BPathEngineManager::getPathEngineBuildingShape
//==============================================================================
iShape* BPathEngineManager::getPathEngineBuildingShape( int width, int length )
{
   for(int  i=0; i<mNumBuildingShapes; i++ )
   {
      if( width <= mBuildingShape[i].width && length <= mBuildingShape[i].length )
         return( mBuildingShape[i].shape );
   }
   return mBuildingShape[mNumBuildingShapes-1].shape;
}


//==============================================================================
// BPathEngineManager::getPathEngineGroundUnitShape
//==============================================================================
iShape* BPathEngineManager::getPathEngineGroundUnitShape( int width )
{
   for(int i=0; i<mNumGroundUnitShapes; i++ )
   {
      if( width <= mGroundUnitShape[i].width )
         return( mGroundUnitShape[i].shape );
   }

   return mGroundUnitShape[mNumGroundUnitShapes-1].shape;
}

//==============================================================================
// BPathEngineManager::getPathEngineFlyingUnitShape
//==============================================================================
iShape* BPathEngineManager::getPathEngineFlyingUnitShape( int width )
{
  
   for(int i=0; i<mNumFlyingUnitShapes; i++ )
   {
      if( width == mFlyingUnitShape[i].width )
         return( mFlyingUnitShape[i].shape );
   }

   return( mFlyingUnitShape[mNumFlyingUnitShapes-1].shape );

}

//==============================================================================
// BPathEngineManager::addStationaryEntity
//==============================================================================
void BPathEngineManager::addStationaryEntity( BEntity* entity )
{
   mStationaryEntity[mNumStationaryEntities] = entity;
   mNumStationaryEntities++;
}
#endif
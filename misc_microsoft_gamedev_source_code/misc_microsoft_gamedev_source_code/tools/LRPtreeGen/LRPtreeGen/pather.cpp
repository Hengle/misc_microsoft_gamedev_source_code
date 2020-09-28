//==============================================================================
// pather.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

// Includes
//#include "common.h"
#include "pather.h"
//#include "protoobject.h"
//#include "game.h"
//#include "world.h"
//#include "player.h"
#include "obstructionmanager.h"
//#include "path.h"
//#include "segintersect.h"
//#include "pathquad.h"
//#include "unit.h"
//#include "pathqueues.h"
//#include "database.h"
//#include "syncmacros.h"
#include "lrptree.h"
//#include "bitquadtree.h"
//#include "squad.h"
#include "terrainsimrep.h"
//#include "configsgame.h"
//#include "debugprimitives.h"

// xsystem
#include "bfileStream.h"
#include "file\win32FileStream.h"


BPather gPather;
//==============================================================================
// Constants
const float cfMinQuadFactor         = 1.0f;           // Size of minimum quad in relation to a tile size.  (ie., 1.0 means min quad == 1 tile )

//==============================================================================
// Defines



//==============================================================================
// BPather::BPather

//==============================================================================
BPather::BPather(void) : 
//   mbInitialized(false),
//   mfLimitDist(0.0f),
   mbQuadUpdate(true),
//   mQuadTreeLand(NULL),
//   mQuadTreeWater(NULL),
//   mlBadPathThresholdLow(clBadPathThresholdLow),
//   mlBadPathThresholdHigh(clBadPathThresholdHigh),
//   mNchPool(NULL),
   mLrpTreeLand(NULL),
   mLrpTreeFlood(NULL),
   mLrpTreeScarab(NULL), 
   mLrpTreeHover(NULL),
   mLrpTreeAir(NULL),
//   mbSkipObBegin(false),
//   mlPathType(cLongRange),
//   mvLimitPos(cOriginVector),
   mfTileSize(0.0f),
   mfTileSizeSqr(0.0f)
//   mbLRPTreeLoaded(false)
{
   // The Pathing Queues
//   QueryPerformanceFrequency(&mQPFreq);
//   mQueues = new BPathQueues;
//   if (!mQueues)
//   {
//      BASSERT(0);
//      return;
//   }

//   BNonconvexHull::startup();

   // Give ourselves some initial space in this array... It can always grow later.
//   mObArray.setNumber(256);
//   mNonconvexArray.setNumber(0);

} // BPather::BPather

//==============================================================================
// BPather::~BPather
//==============================================================================
BPather::~BPather(void)
{
   delete mLrpTreeLand;
   delete mLrpTreeFlood;
   delete mLrpTreeScarab;

   if(mLrpTreeHover)
      delete mLrpTreeHover;

   if(mLrpTreeAir)
      delete mLrpTreeAir;
//   BNonconvexHull::shutdown();
} // BPather::~BPather





//==============================================================================
// BPather::buildPathingQuads
//==============================================================================
void BPather::buildPathingQuads(BObstructionManager *pObManager)
{

   // Why are you building pathing quads without an obstruction manager??
   if (!pObManager)
   {
      BASSERT(0);
      return;
   }

   long lLandOptions = BObstructionManager::cIsNewTypeBlockLandUnits | 
      BObstructionManager::cIsNewTypeSolidNonMoveableUnits;

   long lFloodOptions = BObstructionManager::cIsNewTypeBlockFloodUnits | 
      BObstructionManager::cIsNewTypeSolidNonMoveableUnits;

   long lScarabOptions = BObstructionManager::cIsNewTypeBlockScarabUnits | 
      BObstructionManager::cIsNewTypeSolidNonMoveableUnits;
   
   long lTerrainXTiles = gTerrainSimRep.getNumXTiles();
   long lTerrainZTiles = gTerrainSimRep.getNumXTiles();

   // We keep Terrain Tile Size around for some limit checks in the quad pather..
   mfTileSize = gTerrainSimRep.getTileScale();
   mfTileSizeSqr = mfTileSize * mfTileSize;

   float fMaxX = lTerrainXTiles * mfTileSize;
   float fMaxZ = lTerrainZTiles * mfTileSize;

   // Three quarter size quad's for now..
//   float fMinSize = mfTileSize * 0.75f;
   float fMinSize = mfTileSize * cfMinQuadFactor;

   // Get the obmgr going in "land" mode.  
   // dlm 5/8/02 -- Use half the min quad size as the expansion radius for obMgr checks.
   #ifdef DEBUG_UPDATEBASEQUAD3
   pObManager->begin(BObstructionManager::cBeginNone, fMinSize * 0.5f, lLandOptions, BObstructionManager::cObsNodeTypeAll, 0, 
      0.0f, NULL, 0);
   #else
   pObManager->begin(BObstructionManager::cBeginNone, 0.0f, lLandOptions, BObstructionManager::cObsNodeTypeAll, 0, 
      0.0f, NULL, 0);
   #endif

   // Try out new LRP Quad Trees Here! 
   if (mLrpTreeLand)
      delete mLrpTreeLand;
   if (mLrpTreeFlood)
      delete mLrpTreeFlood;
   if (mLrpTreeScarab)
      delete mLrpTreeScarab;
   
   mLrpTreeLand = new BLrpTree;
   if (!mLrpTreeLand)
   {
      BASSERT(0);
      return;
   }
   if (mLrpTreeLand->init(pObManager, fMaxX, fMaxZ, fMinSize) != true)
   {
      BASSERT(0);
      return;
   }

   mLrpTreeLand->setObMgrOptions(lLandOptions);

   mLrpTreeFlood = new BLrpTree;
   if (!mLrpTreeFlood)
   {
      BASSERT(0);
      return;
   }
   if (mLrpTreeFlood->init(pObManager, fMaxX, fMaxZ, fMinSize) != true)
   {
      BASSERT(0);
      return;
   }

   mLrpTreeFlood->setObMgrOptions(lFloodOptions);

   mLrpTreeScarab = new BLrpTree;
   if (!mLrpTreeScarab)
   {
      BASSERT(0);
      return;
   }
   if (mLrpTreeScarab->init(pObManager, fMaxX, fMaxZ, fMinSize) != true)
   {
      BASSERT(0);
      return;
   }

   mLrpTreeScarab->setObMgrOptions(lScarabOptions);

   pObManager->end();   
}


//==============================================================================
// BPather::updatePathingQuad
//==============================================================================
void BPather::updatePathingQuad(BObstructionManager *pObManager, BOPObstructionNode* theNode, bool AddNode)
{

   // Is updating currently enabled?
   if (!mbQuadUpdate /*|| gWorld->getPatherUpdating() == false*/)
      return;

   if (!(mLrpTreeLand && mLrpTreeFlood && mLrpTreeScarab))
      return;

   #ifndef DEBUG_UPDATEBASEQUAD3
   static BConvexHull tempHull;
	static BVector tempPoints[4];
   
   // Hull Points  (make sure these are in clockwise order
   tempPoints[0].set(theNode->mX1, 0.0f, theNode->mZ1);
	tempPoints[1].set(theNode->mX2, 0.0f, theNode->mZ2);
	tempPoints[2].set(theNode->mX3, 0.0f, theNode->mZ3);
	tempPoints[3].set(theNode->mX4, 0.0f, theNode->mZ4);

	tempHull.initialize(tempPoints, 4, true);

	// Get player owned obstruction value if needed

	long playerId = theNode->mPlayerID;
/* Don't convert playerID, as it will be turned into a mask inside altobjomamma. 
	if (playerId != 0)
	{
		playerId = ObsSystemPlayerMasks[playerId];		// lookup correct mask values
	}
*/
   #endif

	// -------------------------------------------------------------------------------------
	// updatePathingQuad(pObManager, tempHull, flags, AddNode, playerId);
	// -------------------------------------------------------------------------------------

   long lPathOptions[5] = 
   {
      BObstructionManager::cIsNewTypeBlockLandUnits,
      BObstructionManager::cIsNewTypeBlockFloodUnits,
      BObstructionManager::cIsNewTypeBlockScarabUnits,
      0,
      0,
   };

   BLrpTree* pPathTrees[5] =
   {
      mLrpTreeLand,
      mLrpTreeFlood,
      mLrpTreeScarab,
      NULL,
      NULL
   };

   long count = 5;

   bool bInUse = pObManager->inUse();
	BSaveObsManagerState saveState, newState;

   // if in use, save the current state
   pObManager->getSessionState(saveState);
   
   for (long idx=0; idx<count; idx++)
   {
      if(pPathTrees[idx]==NULL)
         continue;

      long pathOptions = lPathOptions[idx];

      if(pPathTrees[idx]!=mLrpTreeAir)
         pathOptions|=(BObstructionManager::cIsNewTypeSolidNonMoveableUnits| BObstructionManager::cIsNewTypeBlockAllMovement);

      #ifdef DEBUG_UPDATEBASEQUAD3
      if (bInUse)
      {
		   newState = saveState;
		   newState.mSQuadTreestoScan = pathOptions;
		   newState.mSValidNodeTypes = BObstructionManager::cObsNodeTypeAllSolid;
         newState.mSRadius = pPathTrees[idx]->getCellSize() * 0.5f;
		   pObManager->setSessionState(newState);
	   }
	   else
		   pObManager->begin(BObstructionManager::cBeginNone, pPathTrees[idx]->getCellSize() * 0.5f, pathOptions, BObstructionManager::cObsNodeTypeAllSolid, theNode->mPlayerID, 0.0f, NULL, 0);
      #else
      if (bInUse)
	   {
		   newState = saveState;
		   newState.mSQuadTreestoScan = pathOptions;
		   newState.mSValidNodeTypes = BObstructionManager::cObsNodeTypeAllSolid;
		   pObManager->setSessionState(newState);
	   }
	   else
		   pObManager->begin(BObstructionManager::cBeginNone, 0.0f, pathOptions, BObstructionManager::cObsNodeTypeAllSolid, 0, 0.0f, NULL, 0);
      #endif		   

      // Pass The filter?
      bool bValid = true;

      if ((theNode->mProperties & BObstructionManager::cObsPropertyInIgnoreList) != 0)
      {
         bValid = false;
      }

	   // Compare theis node's quadtree against the list of selected quadtrees, and see if 
      // it's valid.  
      // NOTE: Wrap this into a fuction in Ob Mgr.. Ug.  
      if (bValid && (((0x001 << theNode->mThisNodeQuadTree) & pathOptions) == 0))
      {
         bValid = false;
      }

      #ifdef DEBUG_UPDATEBASEQUAD3
      // Add (or remove) the hull..
      if (bValid)
            pPathTrees[idx]->updateQuadTree(theNode, AddNode);
      #else
      if (bValid)
            pPathTrees[idx]->updateQuadTree(tempHull, playerId, AddNode);
      #endif

      if (!bInUse)
         pObManager->end();
   }

   // if in use, restore the previous state
	if (bInUse)
		pObManager->setSessionState(saveState);
}

//==============================================================================
// BPather::updatePathingQuad
//==============================================================================
void BPather::updatePathingQuad(BObstructionManager *pObManager, const BVector &vMin, const BVector &vMax)
{

   // Is updating currently enabled?
   if (!mbQuadUpdate /*|| gWorld->getPatherUpdating() == false*/)
      return;

   if (!(mLrpTreeLand && mLrpTreeFlood && mLrpTreeScarab))
      return;

   #ifndef DEBUG_UPDATEBASEQUAD3
	static BConvexHull tempHull;
	static BVector tempPoints[4];

	// Hull Points  (make sure these are in clockwise order
   tempPoints[0].set(vMin.x, 0.0f, vMin.z);
	tempPoints[1].set(vMin.x, 0.0f, vMax.z);
	tempPoints[2].set(vMax.x, 0.0f, vMax.z);
	tempPoints[3].set(vMax.x, 0.0f, vMin.z);

	tempHull.initialize(tempPoints, 4, true);
   #endif

   long lPathOptions[5] = 
   {
      BObstructionManager::cIsNewTypeBlockLandUnits,
      BObstructionManager::cIsNewTypeBlockFloodUnits,
      BObstructionManager::cIsNewTypeBlockScarabUnits,
      0,
      0,
   };

   BLrpTree* pPathTrees[5] =
   {
      mLrpTreeLand,
      mLrpTreeFlood,
      mLrpTreeScarab,
      NULL,
      NULL
   };

   long count = 5;

   bool bInUse = pObManager->inUse();
	BSaveObsManagerState saveState, newState;

   // if in use, save the current state
	pObManager->getSessionState(saveState);
   
   for (long idx=0; idx<count; idx++)
   {
      if(pPathTrees[idx]==NULL)
         continue;

      long pathOptions = lPathOptions[idx];

      if(pPathTrees[idx]!=mLrpTreeAir)
         pathOptions|=(BObstructionManager::cIsNewTypeSolidNonMoveableUnits| BObstructionManager::cIsNewTypeBlockAllMovement);

      if (bInUse)
	   {
		   newState = saveState;
		   newState.mSQuadTreestoScan = pathOptions;
		   newState.mSValidNodeTypes = BObstructionManager::cObsNodeTypeAllSolid;
         #ifdef DEBUG_UPDATEBASEQUAD3
         newState.mSRadius = pPathTrees[idx]->getCellSize() * 0.5f;
         #endif
		   pObManager->setSessionState(newState);
	   }
	   else
      {
         #ifdef DEBUG_UPDATEBASEQUAD3
		   pObManager->begin(BObstructionManager::cBeginNone, pPathTrees[idx]->getCellSize() * 0.5f, pathOptions, BObstructionManager::cObsNodeTypeAllSolid, 0, 0.0f, NULL, 0);
         #else
		   pObManager->begin(BObstructionManager::cBeginNone, 0.0f, pathOptions, BObstructionManager::cObsNodeTypeAllSolid, 0, 0.0f, NULL, 0);
         #endif
      }

      #ifdef DEBUG_UPDATEBASEQUAD3
      pPathTrees[idx]->updateQuadTree(vMin, vMax);
      #else
      pPathTrees[idx]->updateQuadTree(tempHull, 0x0000, false);
      #endif

      if (!bInUse)
         pObManager->end();
   }

   // if in use, restore the previous state
	if (bInUse)
		pObManager->setSessionState(saveState);
}

//==============================================================================
// clearECFBuilder
//==============================================================================
void clearECFBuilder(BECFFileBuilder &ecfBuilder)
{
   for(uint i=0;i<ecfBuilder.getNumChunks();i++)
   {
      ecfBuilder.removeChunkByIndex(i);
      i--;
   }
}

//==============================================================================
// BPather::saveLRPTree
//==============================================================================
bool BPather::saveLRPTree(const char* terrainName)
{
   BECFFileBuilder ecfBuilder;
   ecfBuilder.setID(cLRP_ECFFileID);

   int version =cLRPVersion;
   ecfBuilder.addChunk(cLRPHeaderID,(BYTE*)&version,sizeof(int));

   BLrpNode ***landTree    = mLrpTreeLand->getTree();
   BLrpNode ***floodTree   = mLrpTreeFlood->getTree();
   BLrpNode ***scarabTree  = mLrpTreeScarab->getTree();

   if (mLrpTreeLand->isInitialized() && landTree)
   {
      for (uint8 i=0; i<cMaxDepth; i++ )
         ecfBuilder.addChunk(cLandPassECFChunkID | i+1, (BYTE*) *(landTree[i]), sizeof(BLrpNode) * mLrpTreeLand->getSizeX(i) * mLrpTreeLand->getSizeZ(i));
   }
   else
      return false;

   if (mLrpTreeFlood->isInitialized() && floodTree)
   {
      for (uint8 i=0; i<cMaxDepth; i++ )
         ecfBuilder.addChunk(cFloodPassECFChunkID | i+1, (BYTE*) *(floodTree[i]), sizeof(BLrpNode) * mLrpTreeFlood->getSizeX(i) * mLrpTreeFlood->getSizeZ(i));
   }
   if (mLrpTreeScarab->isInitialized() && scarabTree)
   {
      for (uint8 i=0; i<cMaxDepth; i++ )
         ecfBuilder.addChunk(cScarabPassECFChunkID | i+1, (BYTE*) *(scarabTree[i]), sizeof(BLrpNode) * mLrpTreeScarab->getSizeX(i) * mLrpTreeScarab->getSizeZ(i));
   }

   BWin32FileStream fileStream;

   if (!fileStream.open(terrainName, cSFWritable ))
      return false;

   ecfBuilder.writeToStream(fileStream);

   return true;
}


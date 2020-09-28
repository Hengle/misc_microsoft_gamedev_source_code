//==============================================================================
// lrptree.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "convexhull.h"
//#include "aconfigenums.h"
#include "entity.h"
#include "game.h"
#include "lrptree.h"
#include "obstructionmanager.h"
#include "path.h"
#include "pather.h"
#include "syncmacros.h"
//#include "terrainbase.h"
#include "world.h"
#include "unit.h"
//#include "terrainbase.h"
#include "terrainsimrep.h"
#include "terrain.h"
#include "scenario.h"

// debug stuff
//#include "debugprimrender.h"
#include "boundingbox.h"
#include "usermanager.h"
#include "user.h"
#include "render.h"
#include "gamedirectories.h"

// xcore
#include "consoleOutput.h"
#include "resource\ecfUtils.h"

// xsystem
#include "bfileStream.h"


#define CANJUMP_CHANGE

//==============================================================================
// Defines

// Debug aids only
#ifndef BUILD_FINAL
  #define DEBUG_LRPTREE_GRAPHPATHS
#endif

//#define DEBUG_LRPPATHSYNC
//#define DEBUG_FIXUP_WAYPOINTS
//#define DEBUG_ESTIMATE_WAYPOINTS
//#define DEBUG_FIND_WAYPOINT
//#define DEBUG_ADDPATHNODE
//#define DEBUG_LRPFINDPATH
//#define DEBUG_ADDOBSTRUCTION
//#define DEBUG_UPDATEQUAD
//#define DEBUG_SETCONNECTION
//#define DEBUG_UPDATEBASEQUAD
//#define DEBUG_INIT
//#define DEBUG_SETTARGETQUAD
//#define DEBUG_ADDTOOPENLIST
//#define DEBUG_REMOVEOBSTRUCTION
//#define DEBUG_UPDATEQUADTREE
//#define DEBUG_LRPTIMING_STATS
//#define DEBUG_LRPTIMING_OL
//#define DEBUG_FINDCLOSESTPASSABLETILE
//#define DEBUG_RENDER_TARGET_QUADS
//#define DEBUG_NOFIXUP            // This now only disables fixup.
//#define DEBUG_NOBACKPATH         // This disables backpathing only
//#define DEBUG_BACKPATH


#define DEBUG_LISTEXCLUSIONS


// Things that change behavior
//#define DEBUG_NOSUBQUADANALYSIS
//#define DEBUG_ITERATIONCAP
//#define DEBUG_ALTERNATEUPDATE
//#define DEBUG_UPDATEBASEQUAD2
//#define DEBUG_RECURSIVEEXCLUDE
//#define DEBUG_NOBUILD
//#define DEBUG_LRPPATHSYNC
//#define DEBUG_CLOSETHREESIDEDQUADS

// DLM - 7/31/08 - This is causing all kinds of things to be ostructed that should
// not be obstructed.  Turning it off.
//#define ENABLE_EXPANDED_IMPASSABILITY

const long cLrpRenderingMode = 0;
const long cLrpPathArrayInitSize = 4096L;
const long cLrpMaxTargetExpansion = 3;
const long clIterationCap = 2048;
const long clNodeStackInitSize = 512;
const long clMaxFindExpansions = 4;
const int cMaxPassableTileArraySize = 5000;



// Adjustment array for indexing around a quad or a cell.
static long lAdj[8][2] =
{
   -1, +1,
    0, +1,
   +1, +1,
   -1,  0,
   +1,  0,
   -1, -1,
    0, -1,
   +1, -1
};

//==============================================================================
// Statics
BLrpPathNodeFreeList BLrpTree::mNodeFreeList(&gSimHeap);
BLrpPathPtrArray BLrpTree::mOpenList;
BLrpPathPtrArray BLrpTree::mClosedList;
BLrpPathPtrArray BLrpTree::mFixupList;
BLrpNodeAddyArray BLrpTree::mNodeStack;
#ifndef BUILD_FINAL
BPath BLrpTree::mDebugPath;
#endif

static BObstructionNodePtrArray sTempObs;
static BSegIntersectResultArray sSegResults;


BConvexHull BLrpTree::mStaticHull;
BVector BLrpTree::mStaticPoints[5];

//==============================================================================
// BLrpTree::BLrpTree
//==============================================================================
BLrpTree::BLrpTree(void)
 : 
mpTree(NULL),
mbInitialized(false),
mpObMgr(false),
mfCellSize(0.0f),
mlTreeSizeX(0L),
mlTreeSizeZ(0L),
mlMaxQuadSize(0L),
mlTargetCellX(0L),
mlTargetCellZ(0L),
mvStart(cOriginVector),
mvGoal(cOriginVector),
mlEntityID(-1L),
mlPlayerID(-1L),
mfRadius(0.0f),
mlUnitWidth(0L),
mfTargetRadius(0.0f),
mlBackPathOption(0),
mGn(0),
mGx(0),
mGz(0),
mlIterations(0L),
mlMaxQuadTileSize(0L),
mfRange(0.0f),
mlObMgrOptions(0L),
mCanJump(false),
mlBucket(0L),
mlPCCalls(0),
mfPCSum(0.0f),
mfPCMax(0.0f),
mlFPCalls(0),
mfFPSum(0.0f),
mfFPMax(0.0f),
mlOLCalls(0),
mfOLSum(0.0f),
mfOLMax(0.0f),
mlAPCalls(0),
mfAPSum(0.0f),
mfAPMax(0.0f),
mbEnableSync(true)
{
   mOpenList.setNumber(0);
   mClosedList.setNumber(0);
   mFixupList.setNumber(0);
   mNodeStack.setNumber(clNodeStackInitSize);
   mNodeStack.setNumber(0);
   for (long n = 0; n < cMaxDepth; n++)
   {
      mlSizeX[n] = 0;
      mlSizeZ[n] = 0;
      mlTerrainSizeX[n] = 0;
      mlTerrainSizeZ[n] = 0;
      mlSizeGrid[n] = 0;
   }
   mQPFreq.QuadPart = 0;
} // BLrpTree::BLrpTree

//==============================================================================
// BLrpTree::~BLrpTree
//==============================================================================
BLrpTree::~BLrpTree(void)
{
   #ifdef DEBUG_LRPTIMING_STATS
   dumpStats();
   #endif

   reset();

} // BLrpTree::~BLrpTree

//==============================================================================
// BLrpTree::init
//==============================================================================
bool BLrpTree::init(BObstructionManager *pObMgr, float fMaxX, float fMaxZ,
                    float fMinQuadSize)
{
   SCOPEDSAMPLE(BLrpTree_init)
   if (mbInitialized)
      reset();

   if (!pObMgr)
   {
      BASSERT(0);
      return false;
   }

   #ifdef DEBUG_INIT
   blog("---> BLrpTree::init...");
   #endif

   #ifdef DEBUG_LRPTIMING_STATS
   initStats();
   #endif

   mpObMgr = pObMgr;
   mfCellSize = fMinQuadSize;
   mfRecipCellSize = 1.0f/mfCellSize;
   mlMaxQuadTileSize = 1 << cMaxLevel;
   mlMaxQuadSize = (long)(mlMaxQuadTileSize * mfCellSize);

   #ifdef DEBUG_LRPPATHSYNC
   if (mbEnableSync)
   {
      syncPathingCode("init");
      syncPathingData("mfCellSize", mfCellSize);
      syncPathingData("mlMaxQuadsize", mlMaxQuadSize);
   }
   #endif

   // Allocate space for the arrays.. 
   long lNumMinQuadsX = (long)(fMaxX * mfRecipCellSize);
   long lNumMinQuadsZ = (long)(fMaxZ * mfRecipCellSize); 

   // Fill the terrain size array
   mlTerrainSizeX[0] = lNumMinQuadsX;
   mlTerrainSizeZ[0] = lNumMinQuadsZ;
   if ((mlTerrainSizeX[0] & 1) == 1)
      mlTerrainSizeX[0]++;
   if ((mlTerrainSizeZ[0] & 1) == 1)
      mlTerrainSizeZ[0]++;
   for (long n = 1; n < cMaxDepth; n++)
   {
      mlTerrainSizeX[n] = mlTerrainSizeX[n-1] >> 1;
      if ((mlTerrainSizeX[n] & 1) == 1)
         mlTerrainSizeX[n]++;
      mlTerrainSizeZ[n] = mlTerrainSizeZ[n-1] >> 1;
      if ((mlTerrainSizeZ[n] & 1) == 1)
         mlTerrainSizeZ[n]++;
   }

   long lNumSugQuadsX = 1;
   long lNumSugQuadsZ = 1;
   while (lNumMinQuadsX > lNumSugQuadsX)
      lNumSugQuadsX <<= 1;
   while (lNumMinQuadsZ > lNumSugQuadsZ)
      lNumSugQuadsZ <<= 1;

   mlTreeSizeX = (long)(lNumSugQuadsX * mfCellSize);
   mlTreeSizeZ = (long)(lNumSugQuadsZ * mfCellSize);

   long lXCurrent = lNumSugQuadsX;
   long lZCurrent = lNumSugQuadsZ;

   // Allocate the Levels.. 
   mpTree = HEAP_NEW_ARRAY(BLrpNode**, cMaxDepth, gSimHeap);
   
   // Allocate the rows & columns
   long lNodeCount = 0L;
   for (long n = 0; n < cMaxDepth; n++)
   {
      mlSizeX[n] = lXCurrent;
      mlSizeZ[n] = lZCurrent;
      mlSizeGrid[n] = mlSizeX[n] * mlSizeZ[n];
      
      mpTree[n] = HEAP_NEW_ARRAY(BLrpNode*, lXCurrent, gSimHeap);
            
      BYTE* tempNodeGrid = HEAP_NEW_ARRAY(BYTE, sizeof(BLrpNode) * lXCurrent * lZCurrent, gSimHeap);
      Utils::FastMemSet(tempNodeGrid, 0, sizeof(BLrpNode) * lXCurrent * lZCurrent);
      
      for (long x = 0; x < lXCurrent; x++)
      {
         mpTree[n][x] = (BLrpNode*) tempNodeGrid;
         tempNodeGrid += lZCurrent * sizeof(BLrpNode);
         
         lNodeCount += lZCurrent;
      }
      
      lXCurrent >>= 1;
      lZCurrent >>= 1;
   }

   // Init the bit array of quad exclusions..
   #ifdef DEBUG_LISTEXCLUSIONS
   mbaExclusions.setNumber(lNodeCount);
   mbaExclusions.clear();
   #endif

   // Initialize the Connectivity Info..
   for (long x = 0; x < mlSizeX[cMaxLevel]; x++)
      for (long z = 0; z < mlSizeZ[cMaxLevel]; z++)
         initConnectivity(cMaxLevel, x, z);

   #ifdef DEBUG_NOBUILD
   mbInitialized = true;
   return mbInitialized;
   #endif

   // Go through and add the obstructions..
   #ifdef DEBUG_INIT
   blog("adding Obstructions");
   #endif

	long quadTreesToScan = pObMgr->getTreesToScan();
	long treeNum = 0;

	long	quadNodesPerTree = pObMgr->getNumQuadTreeNodes();

	while (quadTreesToScan != 0)
	{
		if ((quadTreesToScan & 0x01) != 0)
		{
			BOPQuadTreeNode*	QuadTreeNodes = pObMgr->getQuadTreeNodeList(treeNum);

			for (long n = 0; n < quadNodesPerTree; n++,QuadTreeNodes++)
			{
				if (QuadTreeNodes->mNumObstructions > 0)
				{
					for (long i = 0; i < QuadTreeNodes->mNumObstructions; i++)
					{
						BOPObstructionNode* theNode = QuadTreeNodes->mObstructionList[i];
                  #ifdef DEBUG_UPDATEBASEQUAD3
						if ((theNode->mProperties & BObstructionManager::cObsPropertyInIgnoreList) == 0)
						{
                     if (!addObstruction(theNode))
                     {
                        BASSERT(0);
                        return false;
                     }
                  }
                  #else
						if ((theNode->mProperties & BObstructionManager::cObsPropertyInIgnoreList) == 0)
						{
                     long lPlayerMask = 0x0000;
                     if (theNode->mProperties & BObstructionManager::cObsPropertyMultiPlayerOwned)
                     {
                        // BUnit *pUnit = theNode->mUnit;
                        // newer safer version -- dlm 7/24/02
                        BUnit *pUnit = mpObMgr->getUnit(theNode);
                        if (pUnit)
                           lPlayerMask = ObsSystemPlayerMasks[pUnit->getPlayerID()];
                     }

							mStaticPoints[0].set(theNode->mX1, 0.0f, theNode->mZ1);
							mStaticPoints[1].set(theNode->mX2, 0.0f, theNode->mZ2);
							mStaticPoints[2].set(theNode->mX3, 0.0f, theNode->mZ3);
							mStaticPoints[3].set(theNode->mX4, 0.0f, theNode->mZ4);

							mStaticHull.initialize(mStaticPoints, 4, true);

							if (!addObstruction(mStaticHull, lPlayerMask))
							{
								BASSERT(0);
								return false;
							}
						}
                  #endif
					} // end i loop
				}
			} // end n loop


		}
		quadTreesToScan >>=1;
		treeNum++;
	}
	



   #ifdef DEBUG_INIT
   blog("setting Pathing Connections...");
   #endif

   // addObstruction has already set the pathing connection for bucket zero.
   // But we still need to call the routine to do the reverse quad check.  
   // at level zero.  
   for (long k = 0; k < clNumBuckets; k++)
      for (long x = 0; x < mlSizeX[0]; x++)
         for (long z = 0; z < mlSizeZ[0]; z++)
         {
            setPathingConnections(k, 0, x, z);
         }

   #ifdef DEBUG_INIT
   blog("updating Quads");
   #endif
   // Now update the entire tree..
   // Update each bucket..
   for (long k = 0; k < clNumBuckets; k++)
      for (long x = 0; x < mlSizeX[cMaxLevel]; x++)
         for (long z = 0; z < mlSizeZ[cMaxLevel]; z++)
         {
            updateQuad(k, cMaxLevel, x, z);
         }


   #ifdef DEBUG_INIT
   blog("<--- BLrpTree::init");
   #endif

   mbInitialized = true;
   return mbInitialized;

} // BLrpTree::init


//==============================================================================
// BLrpTree::reset
//==============================================================================
void BLrpTree::reset()
{
   if (!mbInitialized)
      return;

   if (mpTree)
   {
      for (long n = 0; n < cMaxDepth; n++)
      {
         if (mpTree[n])
         {
            HEAP_DELETE_ARRAY(mpTree[n][0], gSimHeap);
                  
            HEAP_DELETE_ARRAY(mpTree[n], gSimHeap);
         }
      }
      
      HEAP_DELETE_ARRAY(mpTree, gSimHeap);
      mpTree = NULL;
   }

   mlTreeSizeX = mlTreeSizeZ = 0;
   mfCellSize = 0.0f;
   mfRecipCellSize = 0.0f;
   mbInitialized = false;

} // BLrpTree::reset


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
// BLrpTree::saveECFLRP
//==============================================================================
void BLrpTree::saveECFLRP(const char* fileNameLRP)
{
   if (!mbInitialized)
      return;

   BECFFileBuilder ecfBuilder;
   ecfBuilder.setID(cLRP_ECFFileID);

   int version =cLRPVersion;
   ecfBuilder.addChunk(cLRPHeaderID,(BYTE*)&version,sizeof(int));


   if (mpTree)
   {
      for (uint8 i=0; i<cMaxDepth; i++ )
         ecfBuilder.addChunk(i+1, (BYTE*) *(mpTree[i]), sizeof(BLrpNode) * mlSizeX[i] * mlSizeZ[i]);
   }

   BFileSystemStream fileStream;

   if (!fileStream.open(cDirScenario, fileNameLRP, cSFWritable | cSFSeekable | cSFEnableBuffering))
   {
      clearECFBuilder(ecfBuilder);
      return;
   }

   ecfBuilder.writeToStream(fileStream);
   clearECFBuilder(ecfBuilder);
   fileStream.close();

} // BLrpTree::saveECFLRP


//==============================================================================
// BLrpTree::loadECFLRP
//==============================================================================
bool BLrpTree::loadECFLRP(const uint64 treeID, const char* fileNameLRP)
{
   gConsoleOutput.resource("BLrpTree::loadECFLRP : %s", fileNameLRP);

   BFileSystemStream tfile;
   if (!tfile.open(cDirScenario, fileNameLRP, cSFReadable | cSFSeekable))// | cSFDiscardOnClose))
   {
      gConsoleOutput.error("BLrpTree::loadECFLRP : Unable to open file %s\n", fileNameLRP);
      return false;
   }

   BECFFileStream ecfReader;
   if(!ecfReader.open(&tfile)) //file checking and version ID done during constructor
   {
      gConsoleOutput.error("BLrpTree::loadECFLRP : ECFHeader or Checksum invalid  %s\n", fileNameLRP);
      tfile.close();
      return false;
   }

   BECFHeader ecfHeader = ecfReader.getHeader();
   int numECFChunks = ecfHeader.getNumChunks();

   //find our LRPChunk header
   int headerIndex = ecfReader.findChunkByID(cLRPHeaderID);
   if(headerIndex==-1)
   { 
      gConsoleOutput.error("BLrpTree::loadECFLRP : Could not find LRP Chunk Header\n");
      tfile.close();
      return false;
   }
   ecfReader.seekToChunk(headerIndex);

   //check our header data
   int version =0;
   ecfReader.getStream()->readObjLittleEndian(version);
   //   ecfReader.getStream()->readObj(version);
   if(version !=cLRPVersion)
   {
      gConsoleOutput.error("BLrpTree::loadECFLRP : Could not find LRP Chunk Header\n");
      tfile.close();
      return false;
   }

   bool bLoadedSomething = false;

   //we're good. start walking through chunks and doing stuff...
   for(int i=0; i<numECFChunks; i++)
   {
      const BECFChunkHeader& cHeader = ecfReader.getChunkHeader(i);
      const uint64 id = cHeader.getID();

      uint64 treeContext = id & cLandPassECFChunkID;
      if (treeID == cFloodPassECFChunkID)
         treeContext = id & cFloodPassECFChunkID;
      else if (treeID == cScarabPassECFChunkID)
         treeContext = id & cScarabPassECFChunkID;

      if (!treeContext)
         continue;

      uint64 nodeIndex = id & (~treeContext);
      switch(nodeIndex)
      {
      case 1:
      case 2:
      case 3:
      case 4:
         ecfReader.seekToChunk(i);

         uint dataSize = cHeader.getSize();
         BASSERT( (dataSize % sizeof(BUnpackedLrpNode)) == 0 );
         
#ifdef USE_PACKED_LRP_NODES
         BDynamicSimBYTEArray buf(dataSize);
                  
         if (ecfReader.getStream()->readBytes(buf.getPtr(), dataSize) < dataSize)
         {
            gConsoleOutput.error("BLrpTree::loadECFLRP : Read failed\n");
            return false;
         }
         
         uint numLrpNodes = dataSize / sizeof(BUnpackedLrpNode);
         
         const BUnpackedLrpNode* pSrcNodes = reinterpret_cast<const BUnpackedLrpNode*>(buf.getPtr());
         BLrpNode* pDstNodes = *(mpTree[nodeIndex-1]);
         
         for (uint i = 0; i < numLrpNodes; i++)
         {
            for (uint j = 0; j < clNumBuckets; j++)
            {
               pDstNodes[i].btPathingConns[j] = pSrcNodes[i].btPathingConns[j];
               pDstNodes[i].btPassability[j] = pSrcNodes[i].btPassability[j];
               
               // Pretty sure the status is for temporary use only.
               pDstNodes[i].btStatus = 0;
            }
         }
#else
         ecfReader.getStream()->readBytes(*(mpTree[nodeIndex-1]),dataSize);
#endif
         
         bLoadedSomething = true;
         break;
      }
   }
      
   tfile.close();
   
   //analyzeNodes();
         
   return (bLoadedSomething);

} // BLrpTree::loadECFLRP

//==============================================================================
// BLrpTree::addObstruction
//==============================================================================
void BLrpTree::analyzeNodes() const
{
   //BYTE                    btPathingConns[clNumBuckets]; // 3 sets of pathing connection info.. 1 for each bucket.  
   //BYTE                    btPassability[clNumBuckets];
   //BYTE                    btStatus;               // Status
   
   uint pathingConnsHist[clNumBuckets][256]; 
   Utils::ClearObj(pathingConnsHist);
   
   uint passabilityHist[clNumBuckets][256];
   Utils::ClearObj(passabilityHist);
   
   uint statusHist[256];
   Utils::ClearObj(statusHist);
   
   for (long n = 0; n < cMaxDepth; n++)
   {
      for (long x = 0; x < mlSizeX[n]; x++)
      {
         for (long z = 0; z < mlSizeZ[n]; z++)
         {
            const BLrpNode *pNode = &mpTree[n][x][z];

            for (long i = 0; i < clNumBuckets; i++)
            {
               pathingConnsHist[i][(uint)pNode->btPathingConns[i]]++;
               passabilityHist[i][pNode->btPassability[i]]++;
            }
            
            statusHist[pNode->btStatus]++;
         }
      }
   }
         
   for (uint i = 0; i < clNumBuckets; i++)
   {
      trace("--- pathing conns %u hist: ", i);
      for (uint j = 0; j < 256; j++)
         trace("0x%02X: %u", j, pathingConnsHist[i][j]);
   }
   
   for (uint i = 0; i < clNumBuckets; i++)
   {
      trace("--- passability %u hist: ", i);
      for (uint j = 0; j < 256; j++)
         trace("0x%02X: %u", j, passabilityHist[i][j]);
   }
   
   trace("--- status hist: ", i);
   for (uint j = 0; j < 256; j++)
      trace("0x%02X: %u", j, statusHist[j]);
}   

//==============================================================================
// BLrpTree::addObstruction
//==============================================================================
#ifdef DEBUG_UPDATEBASEQUAD3
bool BLrpTree::addObstruction(BOPObstructionNode *pObNode)
{
   // Get the Player Mask from THE node.
   long lPlayerMask = 0x0000;
   if (pObNode->mProperties & BObstructionManager::cObsPropertyMultiPlayerOwned)
   {
      //BUnit *pUnit = pObNode->mUnit;
      // newer safer mode -- dlm 7/24/02
      BEntity *pUnit = mpObMgr->getObject(pObNode);
      if (pUnit)
         lPlayerMask = ObsSystemPlayerMasks[pUnit->getPlayerID()];
   }

   // Get the expanded hull for this obstruction.
   const BOPQuadHull *pQuadHull;
   pQuadHull = mpObMgr->getExpandedHull(pObNode, 0.0f);

   if (!pQuadHull)
   {
      BASSERT(0);
      return false;
   }
   // Get the Cell Range for the hull we're adding.
   float fMinX = pQuadHull->mDirectVal[pQuadHull->mIdxMinX];
   float fMinZ = pQuadHull->mDirectVal[pQuadHull->mIdxMinZ];
   float fMaxX = pQuadHull->mDirectVal[pQuadHull->mIdxMaxX];
   float fMaxZ = pQuadHull->mDirectVal[pQuadHull->mIdxMaxZ];

   long lMinX = (long)(fMinX * mfRecipCellSize);
   if (lMinX < 0)
      lMinX = 0;

   long lMinZ = (long)(fMinZ * mfRecipCellSize);
   if (lMinZ < 0)
      lMinZ = 0;

   long lMaxX = (long)(fMaxX * mfRecipCellSize);
   if (lMaxX > mlSizeX[0] - 1)
      lMaxX = mlSizeX[0] - 1;

   long lMaxZ = (long)(fMaxZ * mfRecipCellSize);
   if (lMaxZ > mlSizeZ[0] - 1)
      lMaxZ = mlSizeZ[0] - 1;

   for (long x = lMinX; x <= lMaxX; x++)
      for (long z = lMinZ; z <= lMaxZ; z++)
         updateBaseQuad3(x, z, pObNode, pQuadHull, lPlayerMask);

   return true;
}
#else
bool BLrpTree::addObstruction(const BConvexHull &hull, long lPlayerMask)
{

   const BVector &vMin = hull.getBoundingMin();
   const BVector &vMax = hull.getBoundingMax(); 

   #ifdef DEBUG_ADDOBSTRUCTION
   blog("----> addObstruction");
   blog("hull.vMin: (%f, %f)", vMin.x, vMin.z);
   blog("hull.vMax: (%f, %f)", vMax.x, vMax.z);
   blog("lPlayerMask: %d", lPlayerMask);
   #endif
/*
   #ifdef DEBUG_LRPPATHSYNC
   syncPathingCode("addObstruction..");
   syncPathingData("vMin.x", vMin.x);
   syncPathingData("vMin.z", vMin.z);
   syncPathingData("vMax.x", vMax.x);
   syncPathingData("vMax.z", vMax.z);
   syncPathingData("lPlayerMask", lPlayerMask);
   #endif
*/
   // Add Obstruction *only* updates the valid/invalid
   // information for the base level quads.  
   long lX1 = (long)(vMin.x * mfRecipCellSize) - 1;
   if (lX1 < 0)
      lX1 = 0;
   if (lX1 >= mlSizeX[0])
      lX1 = mlSizeX[0] - 1;

   long lZ1 = (long)(vMin.z * mfRecipCellSize) - 1;
   if (lZ1 < 0)
      lZ1 = 0;
   if (lZ1 >= mlSizeZ[0])
      lZ1 = mlSizeZ[0] - 1;

   long lX2 = (long)(vMax.x * mfRecipCellSize) + 1;
   if (lX2 < 0)
      lX2 = 0;
   if (lX2 >= mlSizeX[0])
      lX2 = mlSizeX[0] - 1;

   long lZ2 = (long)(vMax.z * mfRecipCellSize) + 1;
   if (lZ2 < 0)
      lZ2 = 0;
   if (lZ2 >= mlSizeZ[0])
      lZ2 = mlSizeZ[0] - 1;

   #ifdef DEBUG_ADDOBSTRUCTION
   blog("Calling updateBaseQuad: (lX1, Z1) (lX2, lZ2): (%d, %d) (%d, %d)",
      lX1, lX2, lZ1, lZ2);
   #endif

/*
   #ifdef DEBUG_LRPPATHSYNC
   syncPathingCode("calling updateBaseQuad..");
   syncPathingData("lX1", lX1);
   syncPathingData("lX2", lX2);
   syncPathingData("lZ1", lZ1);
   syncPathingData("lZ2", lZ2);
   #endif
*/

   for (long x = lX1; x <= lX2; x++)
      for (long z = lZ1; z <= lZ2; z++)
         #ifdef DEBUG_UPDATEBASEQUAD2
         updateBaseQuad2(x, z, hull, lPlayerMask);
         #else
         updateBaseQuad(x, z, hull, lPlayerMask);
         #endif


   #ifdef DEBUG_ADDOBSTRUCTION
   blog("<--- addObstruction");
   #endif
   
   return true;

}
#endif

//==============================================================================
// BLrpTree::removeObstruction
//==============================================================================
bool BLrpTree::removeObstruction(long lX1, long lZ1, long lX2, long lZ2)
{

   #ifdef DEBUG_REMOVEOBSTRUCTION
   blog("---> RemoveObstruction");
   #endif

   // Convert the boundary quad coords into real-world x,z values for the hull..
   mStaticPoints[0].y = mStaticPoints[1].y = mStaticPoints[2].y = mStaticPoints[3].y = 0.0f;

   mStaticPoints[0].x = (float)(lX1 * mlMaxQuadSize);
   mStaticPoints[0].z = (float)(lZ1 * mlMaxQuadSize);
   mStaticPoints[1].x = mStaticPoints[0].x;
   mStaticPoints[1].z = (float)(lZ2 * mlMaxQuadSize);
   mStaticPoints[2].x = (float)(lX2 * mlMaxQuadSize);
   mStaticPoints[2].z = mStaticPoints[1].z;
   mStaticPoints[3].x = mStaticPoints[2].x;
   mStaticPoints[3].z = mStaticPoints[0].z;
   mStaticHull.initialize(mStaticPoints, 4, true);

   #ifdef DEBUG_REMOVEOBSTRUCTION
   blog("Hull Dims: (%f, %f) (%f, %f) (%f, %f) (%f, %f)",
      mStaticPoints[0].x, mStaticPoints[0].z, mStaticPoints[1].x, mStaticPoints[1].z, mStaticPoints[2].x, mStaticPoints[2].z, mStaticPoints[3].x, mStaticPoints[3].z);
   #endif

   BObstructionNodePtrArray &obstructions = mpObMgr->getFoundObstructionResults();
   // NOTE: These parameters have already been passed to begin.. it'd be nice to not have to pass them again.
   if (mlObMgrOptions == 0L)
   {
      BASSERT(0);
      return false;
   }
   mpObMgr->findObstructions(mStaticHull, 0.0f, mlObMgrOptions, BObstructionManager::cObsNodeTypeAllSolid & ~BObstructionManager::cObsNodeTypeEdgeofMap,
      BObstructionManager::cNoPlayerOwnedIgnores, false, obstructions);

   #ifdef DEBUG_REMOVEOBSTRUCTION
   blog("findObstructions returned %d Obs.", obstructions.getNumber());
   #endif

   // Reset the connectivity for the affected quads..
   for (long x = lX1; x < lX2; x++)
      for (long z = lZ1; z < lZ2; z++)
         initConnectivity(cMaxLevel, x, z);

   long lNumObs = obstructions.getNumber();
   for (long m = 0; m < lNumObs; m++)
   {
	   BOPObstructionNode* pObNode = obstructions[m];
	   if ((pObNode->mProperties & BObstructionManager::cObsPropertyInIgnoreList) == 0)
	   {
         #ifdef DEBUG_UPDATEBASEQUAD3
         if (!addObstruction(pObNode))
            return false;
         #else
		   mStaticPoints[0].set(pObNode->mX1, 0.0f, pObNode->mZ1);
		   mStaticPoints[1].set(pObNode->mX2, 0.0f, pObNode->mZ2);
		   mStaticPoints[2].set(pObNode->mX3, 0.0f, pObNode->mZ3);
		   mStaticPoints[3].set(pObNode->mX4, 0.0f, pObNode->mZ4);
		   mStaticHull.initialize(mStaticPoints, 4, true);
         // Get the player owner and turn it into a player ID
         long lPlayerOwner = pObNode->mPlayerID;
         long lPlayerMask = ObsSystemPlayerMasks[lPlayerOwner];
         if (!addObstruction(mStaticHull, lPlayerMask))
            return false;
         #endif
      }
   }         


   return true;
   
}

//Halwes - 10/9/2006 - Remove recursion
#if !defined( PERF_REMOVE_RECURSION )
//==============================================================================
// BLrpTree::updateQuad
// k = bucket level
// n = quad level
// x = x loc at that quad level
// z = z loc at that quad level
// hull is hull being added
// playermask is bit mask of players this hull belongs to, if any
//==============================================================================
bool BLrpTree::updateQuad(long k, long n, long x, long z)
{
   BLrpNode *pnode = &mpTree[n][x][z];

   #ifdef DEBUG_UPDATEQUAD
   blog("----> updateQuad");
   blog("k: %d, n: %d, x: %d, z: %d", k, n, x, z);
   #endif
/*
   #ifdef DEBUG_LRPPATHSYNC
   syncPathingCode("updateQuad");
   syncPathingData("k", k);
   syncPathingData("n", n);
   syncPathingData("x", x);
   syncPathingData("z", z);
   syncPathingData("lPlayerMask", lPlayerMask);
   #endif
*/
   // Turn off inUse for me.. it will be set appropriately by my parent,
   // or at the bottom of this routine if I'm at the top level. 
   pnode->btPassability[k] &= ~cInUse;

   BLrpNode *pnodeChildren[4];
   long lNextLevel = n-1;

   // Update my children first, if necessary.
   long x1 = x << 1;
   long x2 = x1 + 1;
   long z1 = z << 1;
   long z2 = z1 + 1;

   // SW Child
   pnodeChildren[0] = &mpTree[lNextLevel][x1][z1];
   if (n > 1)
      updateQuad(k, lNextLevel, x1, z1);
   else
      pnodeChildren[0]->btPassability[k] &= ~cInUse;

   if ((pnodeChildren[0]->btPassability[k] & cAllInvalid) == cAllInvalid)
      pnode->btPassability[k] |= cSWInvalid;

   // NW Child
   pnodeChildren[1] = &mpTree[lNextLevel][x1][z2];
   if (n > 1)
      updateQuad(k, lNextLevel, x1, z2);
   else
      pnodeChildren[1]->btPassability[k] &= ~cInUse;

   if ((pnodeChildren[1]->btPassability[k] & cAllInvalid) == cAllInvalid)
      pnode->btPassability[k] |= cNWInvalid;

   // NE Child
   pnodeChildren[2] = &mpTree[lNextLevel][x2][z2];
   if (n > 1)
      updateQuad(k, lNextLevel, x2, z2);
   else
      pnodeChildren[2]->btPassability[k] &= ~cInUse;

   if ((pnodeChildren[2]->btPassability[k] & cAllInvalid) == cAllInvalid)
      pnode->btPassability[k] |= cNEInvalid;

   // SE Child
   pnodeChildren[3] = &mpTree[lNextLevel][x2][z1];
   if (n > 1)
      updateQuad(k, lNextLevel, x2, z1);
   else
      pnodeChildren[3]->btPassability[k] &= ~cInUse;

   if ((pnodeChildren[3]->btPassability[k] & cAllInvalid) == cAllInvalid)
      pnode->btPassability[k] |= cSEInvalid;
      
   // My children have been updated.. now update myself.

   // I'm not broken unless my children are broken, or I am.  Start me off unbroken.
   pnode->btPassability[k] &= ~cBroken;

   // Now adjust our pathing connections..
   setPathingConnections(k, n, x, z);

   // If I have *no* connections into or out of me, then I'm invalid.  What's more,
   // we should mark my children as invalid as well.  dlm 5/15/02
   if ((pnode->btPathingConns[k] & cConnAll) == cConnNone)
      invalidateQuad(k, n, x, z);

   // Now determine if I'm broken or valid.
   long lBrokenOrGate = cBroken | cPlayerQuad;

   if ((pnodeChildren[0]->btPassability[k] & lBrokenOrGate) || (pnodeChildren[1]->btPassability[k] & lBrokenOrGate) || 
       (pnodeChildren[2]->btPassability[k] & lBrokenOrGate) || (pnodeChildren[3]->btPassability[k] & lBrokenOrGate))
      pnode->btPassability[k] |= cBroken;

   if (((pnode->btPassability[k] & cAllInvalid) != cAllInvalid) && ((pnode->btPassability[k] & cBroken) == 0))
   {
      // Now determine if this quad is broken or not.. 
      long lConnected[4];
      
      for (long m = 1; m < 5; m++)
         lConnected[m-1] = ((pnodeChildren[m-1]->btPassability[k] & cAllInvalid) == cAllInvalid)?0:m;

      // Trace the connections from one child to the next, propagating the value in each cell of
      // lConnected.  
      long lNumConnections = 0;
      if (pnodeChildren[0]->btPathingConns[k] & cConnNorth)
      {
         lConnected[1] = lConnected[0];
         ++lNumConnections;
      }
      if (pnodeChildren[1]->btPathingConns[k] & cConnEast)
      {
         lConnected[2] = lConnected[1];
         ++lNumConnections;
      }
      if (pnodeChildren[2]->btPathingConns[k] & cConnSouth)
      {
         lConnected[3] = lConnected[2];
         ++lNumConnections;
      }
      if (pnodeChildren[3]->btPathingConns[k] & cConnWest)
      {
         lConnected[0] = lConnected[3];
         ++lNumConnections;
      }
      // Twice more, to propagate the last valid value, if need be, but don't re-count the connections.
      if (pnodeChildren[0]->btPathingConns[k] & cConnNorth)
         lConnected[1] = lConnected[0];
      if (pnodeChildren[1]->btPathingConns[k] & cConnEast)
         lConnected[2] = lConnected[1];

      bool bNotConnected = false;
      
/*
      if (lNumConnections != 0)
      {
*/
         long lTest = -1;
         for (long m = 0; m < 4; m++)
         {
            if (lConnected[m] != 0)
            {
               if (lTest == -1)
                  lTest = lConnected[m];
               else if (lTest != lConnected[m])
               {
                  bNotConnected = true;
                  break;
               }
            }
         }
//      }

      if (bNotConnected)
         pnode->btPassability[k] |= cBroken;
   }

   // If I'm broken, then mark those of my children that are either not broken or not already InUse
   // as InUse.. 
   if (pnode->btPassability[k] & cBroken)
   {
      long lBrokenOrInUse = cBroken | cInUse;
      for (long m = 0; m < 4; m++)
      {
         if ((pnodeChildren[m]->btPassability[k] & lBrokenOrInUse) == 0)
            pnodeChildren[m]->btPassability[k] |= cInUse;
      }
   }
   else
   {
      if (n == cMaxLevel)
         pnode->btPassability[k] |= cInUse;
   }
/*
   // We are done. 
   #ifdef DEBUG_LRPPATHSYNC
   syncPathingData("pnode->passability[k]", pnode->btPassability[k]);
   syncPathingData("pnode->pathingConns[k]", pnode->btPathingConns[k]);
   #endif
*/
   #ifdef DEBUG_UPDATEQUAD
   blog("<--- updateQuad");
   blog(".");  
   #endif
   return true;

   
}
#else // PERF_REMOVE_RECURSION
//==============================================================================
// BLrpTree::updateQuad
// k = bucket level
// n = quad level
// x = x loc at that quad level
// z = z loc at that quad level
// hull is hull being added
// playermask is bit mask of players this hull belongs to, if any
//==============================================================================
void BLrpTree::updateQuad(long k, long n, long x, long z)
{
   long       stackIndex = -1;
   nodeData   nodeStack[cMaxDepth];

   // Set and push root node
   nodeData tempNode;
   tempNode.n     = n;
   tempNode.x     = x;
   tempNode.z     = z;
   tempNode.visit = 0;
   stackIndex     = pushStack( nodeStack, stackIndex, tempNode );

   long lNextLevel = 0;
   long x1         = 0;
   long z1         = 0;  
   long x2         = 0;
   long z2         = 0;
   long tempIndex  = 0;

   // Iterate through children nodes
   while( stackIndex >= 0 )
   {             
      switch( nodeStack[stackIndex].visit )
      {
         // SW Child X1Z1
         case 0:
            lNextLevel = nodeStack[stackIndex].n - 1;
            x1         = nodeStack[stackIndex].x << 1;
            z1         = nodeStack[stackIndex].z << 1;
            x2         = x1 + 1;
            z2         = z1 + 1;

            nodeStack[stackIndex].pNode = &mpTree[nodeStack[stackIndex].n][nodeStack[stackIndex].x][nodeStack[stackIndex].z];      

            nodeStack[stackIndex].pNodeChildren[0] = &mpTree[lNextLevel][x1][z1];
            nodeStack[stackIndex].pNodeChildren[1] = &mpTree[lNextLevel][x1][z2];
            nodeStack[stackIndex].pNodeChildren[2] = &mpTree[lNextLevel][x2][z2];
            nodeStack[stackIndex].pNodeChildren[3] = &mpTree[lNextLevel][x2][z1];

#ifdef DEBUG_UPDATEQUAD
            blog("----> updateQuad");
            blog("k: %d, n: %d, x: %d, z: %d", k, nodeStack[stackIndex].n, nodeStack[stackIndex].x, nodeStack[stackIndex].z);
#endif
            /*
            #ifdef DEBUG_LRPPATHSYNC
            syncPathingCode("updateQuad");
            syncPathingData("k", k);
            syncPathingData("n", nodeStack[stackIndex].n);
            syncPathingData("x", nodeStack[stackIndex].x);
            syncPathingData("z", nodeStack[stackIndex].z);
            syncPathingData("lPlayerMask", lPlayerMask);
            #endif
            */
            // Turn off inUse for me.. it will be set appropriately by my parent,
            // or at the bottom of this routine if I'm at the top level. 
            nodeStack[stackIndex].pNode->btPassability[k] &= ~cInUse;

            nodeStack[stackIndex].visit++;
            if( nodeStack[stackIndex].n > 1 )
            {
               tempNode.n = lNextLevel;
               tempNode.x = x1;
               tempNode.z = z1;
               tempIndex  = pushStack( nodeStack, stackIndex, tempNode );
            }
            else
            {               
               nodeStack[stackIndex].pNodeChildren[0]->btPassability[k] &= ~cInUse;
               tempIndex = stackIndex;
            }

            if( ( nodeStack[stackIndex].pNodeChildren[0]->btPassability[k] & cAllInvalid ) == cAllInvalid )
            {
               nodeStack[stackIndex].pNode->btPassability[k] |= cSWInvalid;
            }

            stackIndex = tempIndex;
            break;

         // NW Child X1Z2
         case 1:  
            lNextLevel = nodeStack[stackIndex].n - 1;
            x1         = nodeStack[stackIndex].x << 1;
            z1         = nodeStack[stackIndex].z << 1;
            z2         = z1 + 1;

            nodeStack[stackIndex].visit++;
            if( nodeStack[stackIndex].n > 1 )
            {               
               tempNode.n = lNextLevel;
               tempNode.x = x1;
               tempNode.z = z2;
               tempIndex  = pushStack( nodeStack, stackIndex, tempNode );
            }
            else
            {
               nodeStack[stackIndex].pNodeChildren[1]->btPassability[k] &= ~cInUse;
               tempIndex = stackIndex;
            }

            if( ( nodeStack[stackIndex].pNodeChildren[1]->btPassability[k] & cAllInvalid ) == cAllInvalid )
            {
               nodeStack[stackIndex].pNode->btPassability[k] |= cNWInvalid;
            }

            stackIndex = tempIndex;
            break;

         // NE Child X2Z2
         case 2:
            lNextLevel = nodeStack[stackIndex].n - 1;
            x1         = nodeStack[stackIndex].x << 1;
            z1         = nodeStack[stackIndex].z << 1;
            x2         = x1 + 1;
            z2         = z1 + 1;

            nodeStack[stackIndex].visit++;
            if( nodeStack[stackIndex].n > 1 )
            {               
               tempNode.n = lNextLevel;
               tempNode.x = x2;
               tempNode.z = z2;
               tempIndex  = pushStack( nodeStack, stackIndex, tempNode );
            }
            else
            {               
               nodeStack[stackIndex].pNodeChildren[2]->btPassability[k] &= ~cInUse;
               tempIndex = stackIndex;
            }

            if( ( nodeStack[stackIndex].pNodeChildren[2]->btPassability[k] & cAllInvalid ) == cAllInvalid )
            {
               nodeStack[stackIndex].pNode->btPassability[k] |= cNEInvalid;
            }

            stackIndex = tempIndex;
            break;

         // SE Child X2Z1
         case 3:
            lNextLevel = nodeStack[stackIndex].n - 1;
            x1         = nodeStack[stackIndex].x << 1;
            z1         = nodeStack[stackIndex].z << 1;
            x2         = x1 + 1;

            nodeStack[stackIndex].visit++;
            if( nodeStack[stackIndex].n > 1 )
            {               
               tempNode.n = lNextLevel;
               tempNode.x = x2;
               tempNode.z = z1;               
               tempIndex  = pushStack( nodeStack, stackIndex, tempNode );
            }
            else
            {               
               nodeStack[stackIndex].pNodeChildren[3]->btPassability[k] &= ~cInUse;
               tempIndex = stackIndex;
            }

            if( ( nodeStack[stackIndex].pNodeChildren[3]->btPassability[k] & cAllInvalid ) == cAllInvalid )
            {
               nodeStack[stackIndex].pNode->btPassability[k] |= cSEInvalid;
            }

            stackIndex = tempIndex;
            break;

         case 4:
            // My children have been updated.. now update myself.

            // I'm not broken unless my children are broken, or I am.  Start me off unbroken.
            nodeStack[stackIndex].pNode->btPassability[k] &= ~cBroken;

            // Now adjust our pathing connections..
            setPathingConnections(k, nodeStack[stackIndex].n, nodeStack[stackIndex].x, nodeStack[stackIndex].z);

            // If I have *no* connections into or out of me, then I'm invalid.  What's more,
            // we should mark my children as invalid as well.  dlm 5/15/02
            if ((nodeStack[stackIndex].pNode->btPathingConns[k] & cConnAll) == cConnNone)
               invalidateQuad(k, nodeStack[stackIndex].n, nodeStack[stackIndex].x, nodeStack[stackIndex].z);

            // Now determine if I'm broken or valid.
            long lBrokenOrGate = cBroken | cPlayerQuad;

            if ((nodeStack[stackIndex].pNodeChildren[0]->btPassability[k] & lBrokenOrGate) || (nodeStack[stackIndex].pNodeChildren[1]->btPassability[k] & lBrokenOrGate) || 
               (nodeStack[stackIndex].pNodeChildren[2]->btPassability[k] & lBrokenOrGate) || (nodeStack[stackIndex].pNodeChildren[3]->btPassability[k] & lBrokenOrGate))
               nodeStack[stackIndex].pNode->btPassability[k] |= cBroken;

            if (((nodeStack[stackIndex].pNode->btPassability[k] & cAllInvalid) != cAllInvalid) && ((nodeStack[stackIndex].pNode->btPassability[k] & cBroken) == 0))
            {
               // Now determine if this quad is broken or not.. 
               long lConnected[4];

               for (long m = 1; m < 5; m++)
                  lConnected[m-1] = ((nodeStack[stackIndex].pNodeChildren[m-1]->btPassability[k] & cAllInvalid) == cAllInvalid)?0:m;

               // Trace the connections from one child to the next, propagating the value in each cell of
               // lConnected.  
               long lNumConnections = 0;
               if (nodeStack[stackIndex].pNodeChildren[0]->btPathingConns[k] & cConnNorth)
               {
                  lConnected[1] = lConnected[0];
                  ++lNumConnections;
               }
               if (nodeStack[stackIndex].pNodeChildren[1]->btPathingConns[k] & cConnEast)
               {
                  lConnected[2] = lConnected[1];
                  ++lNumConnections;
               }
               if (nodeStack[stackIndex].pNodeChildren[2]->btPathingConns[k] & cConnSouth)
               {
                  lConnected[3] = lConnected[2];
                  ++lNumConnections;
               }
               if (nodeStack[stackIndex].pNodeChildren[3]->btPathingConns[k] & cConnWest)
               {
                  lConnected[0] = lConnected[3];
                  ++lNumConnections;
               }
               // Twice more, to propagate the last valid value, if need be, but don't re-count the connections.
               if (nodeStack[stackIndex].pNodeChildren[0]->btPathingConns[k] & cConnNorth)
                  lConnected[1] = lConnected[0];
               if (nodeStack[stackIndex].pNodeChildren[1]->btPathingConns[k] & cConnEast)
                  lConnected[2] = lConnected[1];

               bool bNotConnected = false;

               /*
               if (lNumConnections != 0)
               {
               */
               long lTest = -1;
               for (long m = 0; m < 4; m++)
               {
                  if (lConnected[m] != 0)
                  {
                     if (lTest == -1)
                        lTest = lConnected[m];
                     else if (lTest != lConnected[m])
                     {
                        bNotConnected = true;
                        break;
                     }
                  }
               }
               //      }

               if (bNotConnected)
                  nodeStack[stackIndex].pNode->btPassability[k] |= cBroken;
            }

            // If I'm broken, then mark those of my children that are either not broken or not already InUse
            // as InUse.. 
            if (nodeStack[stackIndex].pNode->btPassability[k] & cBroken)
            {
               long lBrokenOrInUse = cBroken | cInUse;
               for (long m = 0; m < 4; m++)
               {
                  if ((nodeStack[stackIndex].pNodeChildren[m]->btPassability[k] & lBrokenOrInUse) == 0)
                     nodeStack[stackIndex].pNodeChildren[m]->btPassability[k] |= cInUse;
               }
            }
            else
            {
               if (nodeStack[stackIndex].n == cMaxLevel)
                  nodeStack[stackIndex].pNode->btPassability[k] |= cInUse;
            }
            /*
            // We are done. 
            #ifdef DEBUG_LRPPATHSYNC
            syncPathingData("pnode->passability[k]", nodeStack[stackIndex].pNode->btPassability[k]);
            syncPathingData("pnode->pathingConns[k]", nodeStack[stackIndex].pNode->btPathingConns[k]);
            #endif
            */
#ifdef DEBUG_UPDATEQUAD
            blog("<--- updateQuad");
            blog(".");  
#endif
            stackIndex = popStack( nodeStack, stackIndex );
            break;
      }
   }
}
#endif // PERF_REMOVE_RECURSION


//==============================================================================
// BLrpTree::updateBaseQuad
// updateBaseQuad builds the connection and validity information for the bucket 0 (size = 1)
// quads at level 0.  This is the "base" tile-to-tile connection information by which
// the other levels for this and the other buckets will be built off of. 
// Basically, 0 *assumes* a bucket size of 1.  DLMTODO: Modify the bucketSizes
// array to reflect this, by removing the size for bucket zero.
//==============================================================================
void BLrpTree::updateBaseQuad(long x, long z, const BConvexHull &hull, long lPlayerMask)
{

   #ifdef DEBUG_UPDATEBASEQUAD
   blog("----> updateBaseQuad");
   blog("x: %d, z: %d", x, z);
   #endif

   long n = 0;
   bool bHasPlayer = (lPlayerMask != 0x0000);

   // Turn off inuse..
   BLrpNode *pnode = &mpTree[n][x][z];

   static short sInvalidArray[4] = 
   {
      cSWInvalid,
      cNWInvalid,
      cNEInvalid,
      cSEInvalid
   };

/*
   #ifdef DEBUG_LRPPATHSYNC
   syncPathingData("N", n);
   syncPathingData("X", x);
   syncPathingData("Z", z);
   syncPathingData("lPlayerMask", lPlayerMask);
   #endif
*/
   BVector vMid, vMin, vMax, vQuadMid;
   mStaticPoints[0].y = mStaticPoints[1].y = mStaticPoints[2].y = mStaticPoints[3].y = 0.0f;
   vMin.y = vMax.y = 0.0f;
   vQuadMid.y = 0.0f;

   getQuadBoundaries(n, x, z, mStaticPoints[0], mStaticPoints[2]);
   if (!hull.overlapsBox(mStaticPoints[0].x, mStaticPoints[0].z, mStaticPoints[2].x, mStaticPoints[2].z))
      return;

   if (bHasPlayer)
   {
      // Update the cPlayerQuad bit for all the buckets at level 0..
      #ifdef DEBUG_LRPPATHSYNC
      if (mbEnableSync)
      {
         syncPathingCode("setting Player Data for node..");
         syncPathingData("lPlayerMask", lPlayerMask);
      }
      #endif

      for (long k = 0; k < clNumBuckets; k++)
         pnode->btPassability[k] |= (cPlayerQuad | cInUse); // jce [3/31/2005] -- changed this to also set cInUse since some checks only look at that flag 
      pnode->lPlayerMask = lPlayerMask;
      return;
   }

   if ((pnode->btPassability[0] & cAllInvalid) == cAllInvalid)
      return;

   // Check each of the four vertices.  
   mStaticPoints[1].x = mStaticPoints[0].x;
   mStaticPoints[1].z = mStaticPoints[2].z;
   mStaticPoints[3].x = mStaticPoints[2].x;
   mStaticPoints[3].z = mStaticPoints[0].z;
   mStaticPoints[4].x = mStaticPoints[0].x + ((mStaticPoints[2].x - mStaticPoints[0].x) / 2.0f);
   mStaticPoints[4].z = mStaticPoints[0].z + ((mStaticPoints[2].z - mStaticPoints[0].z) / 2.0f);
/*
   BYTE btInvalids[4] = 
   {
      cSWInvalid,
      cNWInvalid,
      cNEInvalid,
      cSEInvalid
   };
*/
   bool bInsides[4];
   bInsides[0] = hull.insideOrOnEdge(mStaticPoints[0]);
   bInsides[1] = hull.insideOrOnEdge(mStaticPoints[1]);
   bInsides[2] = hull.insideOrOnEdge(mStaticPoints[2]);
   bInsides[3] = hull.insideOrOnEdge(mStaticPoints[3]);

   // If all four corners are inside the hull, mark it as invalid and move on..
   if (bInsides[0] && bInsides[1] && bInsides[2] && bInsides[3])
   {
      pnode->btPassability[0] |= cAllInvalid;
      return;
   }

   // Check each of the four subquads.. 
   if (hull.overlapsBox(mStaticPoints[0].x, mStaticPoints[0].z, mStaticPoints[4].x, mStaticPoints[4].z))
      pnode->btPassability[0] |= cSWInvalid;
   if (hull.overlapsBox(mStaticPoints[0].x, mStaticPoints[4].z, mStaticPoints[4].x, mStaticPoints[2].z))
      pnode->btPassability[0] |= cNWInvalid;
   if (hull.overlapsBox(mStaticPoints[4].x, mStaticPoints[4].z, mStaticPoints[2].x, mStaticPoints[2].z))
      pnode->btPassability[0] |= cNEInvalid;
   if (hull.overlapsBox(mStaticPoints[4].x, mStaticPoints[0].z, mStaticPoints[2].x, mStaticPoints[4].z))
      pnode->btPassability[0] |= cSEInvalid;



/*

   // This is the code that does the segintersect checks of each side of the quad against
   // the hull, and then sets passablity based on the distance to the intersections.  
   // It has it's own set of problems.

   float fSegLen = v[2].x - v[0].x;
   float fSegChk = fSegLen * 0.25f;
   float fSegChk2 = fSegLen - fSegChk;

   #ifdef DEBUG_UPDATEBASEQUAD
   blog("\tfSegLen: %f fSegChk: %f fSegChk2: %f",
      fSegLen, fSegChk, fSegChk2);
   #endif

   long lNext = 0;
   for (long lCurr = 0; lCurr < 4; lCurr++)
   {

      lNext = (lCurr < 3)?lCurr+1:0;

      if ((pnode->btPassability[0] & btInvalids[lCurr]) && (pnode->btPassability[0] & btInvalids[lNext]))
         continue;

      if (bInsides[lCurr])
      {
         if (bInsides[lNext])
         {
            pnode->btPassability[0] |= btInvalids[lCurr];
            pnode->btPassability[0] |= btInvalids[lNext];
            continue;
         }
         else
         {
            BVector vI;
            long lSeg = -1L, lNumIntersects = 0;
            float fDistSqr = 0.0f;
            if (!hull.segmentIntersects(v[lCurr], v[lNext], -1L, vI, lSeg, fDistSqr, lNumIntersects, true, true))
               continue;

            float fDist = (float)sqrt((double)fDistSqr);
            #ifdef DEBUG_UPDATEBASEQUAD
            blog("\tInside going out: lCurr: %d fDist: %f", lCurr, fDist);
            #endif
            
            if (fDist > fSegChk)
            {
               pnode->btPassability[0] |= btInvalids[lCurr];
               pnode->btPassability[0] |= btInvalids[lNext];
            }

//            if (fDist > fSegChk2)
//               pnode->btPassability[0] |= btInvalids[lNext];

            
         }
      }
      else
      {

         if (bInsides[lNext])
         {
            BVector vI;
            long lSeg = -1L, lNumIntersects = 0;
            float fDistSqr = 0.0f;
            if (!hull.segmentIntersects(v[lCurr], v[lNext], -1L, vI, lSeg, fDistSqr, lNumIntersects, true, true))
               continue;

            float fDist = (float)sqrt((double)fDistSqr);
            #ifdef DEBUG_UPDATEBASEQUAD
            blog("\tOutside going in: lCurr: %d fDist: %f", lCurr, fDist);
            #endif
            if (fDist < fSegChk2)
            {
               pnode->btPassability[0] |= btInvalids[lNext];
               pnode->btPassability[0] |= btInvalids[lCurr];
            }
            
//            if (fDist < fSegChk)
//               pnode->btPassability[0] |= btInvalids[lCurr];
            

         }
      }
   }

*/

/*
   #ifdef DEBUG_LRPPATHSYNC
   syncPathingData("pnode->PathingConns[0]", pnode->btPathingConns[0]);
   for (long k = 0; k < clNumBuckets; k++)
   {
      syncPathingData("pnode->passability[k]", pnode->btPassability[k]);
   }
   #endif
*/
   return;

}

//==============================================================================
// BLrpTree::updateBaseQuad2
// updateBaseQuad builds the connection and validity information for the bucket 0 (size = 1)
// quads at level 0.  This is the "base" tile-to-tile connection information by which
// the other levels for this and the other buckets will be built off of. 
// Basically, 0 *assumes* a bucket size of 1.  DLMTODO: Modify the bucketSizes
// array to reflect this, by removing the size for bucket zero.
//==============================================================================
void BLrpTree::updateBaseQuad2(long x, long z, const BConvexHull &hull, long lPlayerMask)
{

   #ifdef DEBUG_UPDATEBASEQUAD
   blog("----> updateBaseQuad");
   blog("x: %d, z: %d", x, z);
   #endif

   long n = 0;
   bool bHasPlayer = (lPlayerMask != 0x0000);

   BLrpNode *pnode = &mpTree[n][x][z];

   BVector vMid, vMin, vMax, vQuadMid;
   mStaticPoints[0].y = mStaticPoints[1].y = mStaticPoints[2].y = mStaticPoints[3].y = 0.0f;
   vMin.y = vMax.y = 0.0f;
   vQuadMid.y = 0.0f;

   getQuadBoundaries(n, x, z, mStaticPoints[0], mStaticPoints[2]);

   if (bHasPlayer)
   {
      if (!hull.overlapsBox(mStaticPoints[0].x, mStaticPoints[0].z, mStaticPoints[2].x,
         mStaticPoints[2].z))
         return;

      // Update the cPlayerQuad bit for all the buckets at level 0..
      #ifdef DEBUG_LRPPATHSYNC
      if (mbEnableSync)
      {
         syncPathingCode("setting Player Data for node..");
         syncPathingData("lPlayerMask", lPlayerMask);
      }
      #endif

      for (long k = 0; k < clNumBuckets; k++)
         pnode->btPassability[k] |= (cPlayerQuad | cInUse); // jce [3/31/2005] -- changed this to also set cInUse since some checks only look at that flag
      pnode->lPlayerMask = lPlayerMask;
      return;
   }

   // If this quad has already been marked, then bail.
   if (pnode->btPassability[0] & cAllInvalid)
      return;

   // Check each of the four vertices.  
   mStaticPoints[1].x = mStaticPoints[0].x;
   mStaticPoints[1].z = mStaticPoints[2].z;
   mStaticPoints[3].x = mStaticPoints[2].x;
   mStaticPoints[3].z = mStaticPoints[0].z;
   mStaticPoints[4].x = mStaticPoints[0].x + ((mStaticPoints[2].x - mStaticPoints[0].x) / 2.0f);
   mStaticPoints[4].z = mStaticPoints[0].z + ((mStaticPoints[2].z - mStaticPoints[0].z) / 2.0f);

   // Okay.. Check for exact match of vertices..
   const BDynamicSimVectorArray &vHullPoints = hull.getPoints();
   if (vHullPoints[0].xzEqualTo(mStaticPoints[0]) &&
       vHullPoints[1].xzEqualTo(mStaticPoints[1]) &&
       vHullPoints[2].xzEqualTo(mStaticPoints[2]) &&
       vHullPoints[3].xzEqualTo(mStaticPoints[3]))
   {
       pnode->btPassability[0] |= cAllInvalid;
	   pnode->btPathingConns[0] = cConnNone;
       return;
   }
   // Determine if any and/or all of the vertices are inside the hull.
   bool bHullInsides[4];
   for (long m = 0; m < 4; m++)
	   bHullInsides[m] = hull.inside(mStaticPoints[m]);

   // If all four vertices are inside the hull, we are done.
   if (bHullInsides[0] && bHullInsides[1] && bHullInsides[2] && bHullInsides[3])
   {
	   pnode->btPassability[0] |= cAllInvalid;
	   pnode->btPathingConns[0] = cConnNone;
	   return;
   }

   // Check each edge of the quad, and do an obMgr segintersect on it.
   // If there is a single "open" gap of size >= 1, then leave the edge
   // as connected, otherwise disconnect it.
   long lP1 = 3;
   long lP2 = 0;
 
   BVector vIPoint(0.0f);
   static long lEdges[4] = 
   {
      cConnSouth,
      cConnWest,
      cConnNorth,
      cConnEast
   };
   for (long m = 0; m < 4; m++)
   {
      // If the edge is already disconnected, just move on..
      if (!(pnode->btPathingConns[0] & lEdges[m]))
      {
         lP1 = lP2;
         lP2++;
         continue;
      }

      // Before we do the edge detection tests, run the "corners to middle" test.
      BVector vInt1(0.0f);
      BVector vInt2(0.0f);
      long lSegIdx = 0;
      long lNumInt = 0;
      float fDistSqr1 = 0.0f;
      bool bInt1 = false;
      bool bInt2 = false;
      if (!bHullInsides[lP1])
      {
         bInt1 = hull.segmentIntersects(mStaticPoints[lP1], mStaticPoints[4], -1, vInt1, lSegIdx,
            fDistSqr1, lNumInt, false, false, 0.0f);
      }
      else
      {
         bInt1 = true;
         vInt1 = mStaticPoints[lP1];
      }
      if (!bHullInsides[lP2])
      {
         bInt2 = hull.segmentIntersects(mStaticPoints[lP2], mStaticPoints[4], -1, vInt2, lSegIdx,
            fDistSqr1, lNumInt, false, false, 0.0f);
      }
      else
      {
         bInt2 = true;
         vInt2 = mStaticPoints[lP2];
      }
      float fHalfDist = mfCellSize * 0.5f;
      if ((bInt1 && bInt2) && (vInt1.xzDistanceSqr(vInt2) >= fHalfDist))
      {
         pnode->btPathingConns[0] &= ~lEdges[m];
         lP1 = lP2;
         lP2++;
         continue;
      }

      BObstructionNodePtrArray &obstructions = mpObMgr->getFoundObstructionResults();
      float fDistSqr2 = 0.0f;
      float fDistSqr3 = 0.0f;

      // Get the Object Intersections for this segment.
      mpObMgr->getObjectsIntersections(BObstructionManager::cGetAllIntersections, mStaticPoints[lP1],
         mStaticPoints[lP2], false, vIPoint, obstructions);

      // If no Intersections, continue
      long lNumIntersections = obstructions.getNumber();
      if (lNumIntersections == 0)
      {
         lP1 = lP2;
         lP2++;
         continue;
      }

      // If more than 2 intersections, just mark the damn thing as blocked.
      if (lNumIntersections > 2)
      {
         pnode->btPathingConns[0] &= ~lEdges[m];
         lP1 = lP2;
         lP2++;
         continue;
      }
      switch(lNumIntersections)
      {
         case 1:
         {
            // If the segment is contained within the hull, the above routine will have returned
            // the first midpoint as the intersection point.  To verify this, if the intersection
            // point is the same as the first segment point, and if the second one is *inside* the
            // hull, call it disconnected.
            BOPObstructionNode *pOb = obstructions[0];
            const BOPQuadHull *pQuadHull = pOb->getHull();

            // Note, this obstruction is the same one as the passed in hull.. so use the
            // inside results we've already calculated.
            bool bInside2 = bHullInsides[lP2];
            if (mStaticPoints[lP1].xzEqualTo(vIPoint) && bInside2)
            {
               pnode->btPathingConns[0] &= ~lEdges[m];
               lP1 = lP2;
               lP2++;
               continue;
            }
            // Both points not inside.  Get distance from outside point to intersection.
            // if less than 1, mark edge as blocked.
            if (bInside2)
            {
               if (mStaticPoints[lP1].xzDistance(vIPoint) < 1.0f)
               {
                  pnode->btPathingConns[0] &= ~lEdges[m];
                  lP1 = lP2;
                  lP2++;
                  continue;
               }
            }
            else
            {
               bool bInside1 = bHullInsides[lP1];
               if (bInside1)
               {
                  if (mStaticPoints[lP2].xzDistance(vIPoint) < 1.0f)
                  {
                     pnode->btPathingConns[0] &= ~lEdges[m];
                     lP1 = lP2;
                     lP2++;
                     continue;
                  }
               }
               else
               {
                  // We intersected the hull, but neither point is inside.  
                  float fDist1 = mStaticPoints[lP1].xzDistanceSqr(vIPoint);
                  float fDist2 = 0.0f;
                  // Get the distance from point 2 to the hulls.
                  pQuadHull->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt1, lSegIdx,
                     fDist2, lNumInt, false, false, 0.0f);

                  // Otherwise, we have some obstruction blocking the side, but
                  // not covering the vertices.
                  if (fDist1 <= 1.0f && fDist2 <= 1.0f)
                  {
                     pnode->btPathingConns[0] &= ~lEdges[m];
                     lP1 = lP2;
                     lP2++;
                     continue;
                  }
               }

            }
            break;
         }
         case 2:
         {
            // Hull 1 & Hull 2
            BOPObstructionNode *pOb1 = obstructions[0];
            BOPObstructionNode *pOb2 = obstructions[1];
            const BOPQuadHull *pQuadHull1 = pOb1->getHull();
            const BOPQuadHull *pQuadHull2 = pOb2->getHull();
            long lState1 = 0;
            long lState2 = 0;
            // We have a lot of combinations to account for.
            // here's how we do it.  lState1 is the state
            // of P1.  0 = out of both.  1 = In hull 1 but not hull2
            // 2 = in hull2 but not hull 1, and 3 == in hull 1 & hull2.
            // Repeat for State2.
            if (pQuadHull1->inside(mStaticPoints[lP1]))
               lState1 += 1;
            if (pQuadHull2->inside(mStaticPoints[lP1]))
               lState1 += 2;
            if (pQuadHull1->inside(mStaticPoints[lP2]))
               lState2 += 1;
            if (pQuadHull2->inside(mStaticPoints[lP2]))
               lState2 += 2;

            // The action we take is defined by the table below.
            static int lCombinations[4][4] = 
            {
               0, 1, 2, 3,
               4, 9, 5, 9,
               6, 7, 9, 9,
               8, 9, 9, 9
            };
            long lAction = lCombinations[lState1][lState2];
            switch(lAction)
            {
               case 0:     // Both points outside.. but we intersect two hulls.  
               {
                  // Get the distance from point 1 to the hulls..
                  float fDist1 = mStaticPoints[lP1].xzDistanceSqr(vIPoint);
                  // Get the distance from point 2 to the hulls.
                  pQuadHull1->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  pQuadHull2->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt2, lSegIdx,
                     fDistSqr2, lNumInt, false, false, 0.0f);
                  // Get the min distance of point 2 to the hulls.
                  if (fDistSqr1 < fDistSqr2)
                     fDistSqr3 = fDistSqr1;
                  else
                     fDistSqr3 = fDistSqr2;

                  // If the distance to each of the closest intersections
                  // is exactly zero, but neither of the points is inside
                  // either of the hulls, we're *exactly* between two hulls,
                  // so leave this edge alone.
                  if (fDist1 < cFloatCompareEpsilon && fDistSqr3 < cFloatCompareEpsilon)
                     break;

                  // Otherwise, we have some obstruction blocking the side, but
                  // not covering the vertices.
                  if (fDist1 <= 1.0f && fDistSqr3 <= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               }
               case 1:     // p1 outside, p2 in hull 1
               {
                  pQuadHull1->segmentIntersects(mStaticPoints[lP1], mStaticPoints[lP2], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  if (fDistSqr1 <= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               }
               case 2:     // p1 outside, p2 in hull 2
                  pQuadHull2->segmentIntersects(mStaticPoints[lP1], mStaticPoints[lP2], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  if (fDistSqr1 <= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               case 3:     // p1 outside, p2 in both hulls
                  // Get the distance from point 1 to the hulls.
                  pQuadHull1->segmentIntersects(mStaticPoints[lP1], mStaticPoints[lP2], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  pQuadHull2->segmentIntersects(mStaticPoints[lP1], mStaticPoints[lP2], -1, vInt1, lSegIdx,
                     fDistSqr2, lNumInt, false, false, 0.0f);
                  // Get the min distance of point 1 to the hulls.
                  if (fDistSqr1 < fDistSqr2)
                     fDistSqr3 = fDistSqr1;
                  else
                     fDistSqr3 = fDistSqr2;
                  if (fDistSqr3 <= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               case 4:     // p1 in hull 1, p2 outside
                  pQuadHull1->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  if (fDistSqr1 <= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               case 5:     // p1 in hull 1, p2 in hull 2
                  pQuadHull1->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  fDistSqr1 = 2.0f - (float)sqrt((double)fDistSqr1);
                  pQuadHull2->segmentIntersects(mStaticPoints[lP1], mStaticPoints[lP2], -1, vInt1, lSegIdx,
                     fDistSqr2, lNumInt, false, false, 0.0f);
                  fDistSqr2 = 2.0f - (float)sqrt((double)fDistSqr2);
                  if ((fDistSqr1 + fDistSqr2) >= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               case 6:     // p1 in hull 2, p2 outside
                  pQuadHull2->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  if (fDistSqr1 <= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               case 7:     // p1 in hull 2, p2 in hull 1
                  pQuadHull1->segmentIntersects(mStaticPoints[lP1], mStaticPoints[lP2], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  fDistSqr1 = 2.0f - (float)sqrt((double)fDistSqr1);
                  pQuadHull2->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt1, lSegIdx,
                     fDistSqr2, lNumInt, false, false, 0.0f);
                  fDistSqr2 = 2.0f - (float)sqrt((double)fDistSqr2);
                  if ((fDistSqr1 + fDistSqr2) >= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               case 8:     // p1 in both hulls, p2 outside
                  // Get the distance from point 1 to the hulls.
                  pQuadHull1->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt1, lSegIdx,
                     fDistSqr1, lNumInt, false, false, 0.0f);
                  pQuadHull2->segmentIntersects(mStaticPoints[lP2], mStaticPoints[lP1], -1, vInt1, lSegIdx,
                     fDistSqr2, lNumInt, false, false, 0.0f);
                  // Get the min distance of point 1 to the hulls.
                  if (fDistSqr1 < fDistSqr2)
                     fDistSqr3 = fDistSqr1;
                  else
                     fDistSqr3 = fDistSqr2;
                  if (fDistSqr3 <= 1.0f)
                     pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
               case 9:     // p1 and p2 both in the same hull
                  pnode->btPathingConns[0] &= ~lEdges[m];
                  break;
            }
            break;
         }
      }

      lP1 = lP2;
      lP2++;
   }

   if ((pnode->btPathingConns[0] & cConnAll) == cConnNone)
      pnode->btPassability[0] |= cAllInvalid;


   return;

}

#ifdef DEBUG_UPDATEBASEQUAD3
//==============================================================================
// BLrpTree::updateBaseQuad3
//==============================================================================
void BLrpTree::updateBaseQuad3(long x, long z, BOPObstructionNode *pObNode, const BOPQuadHull *pQuadHull, 
                               long lPlayerMask)
{

   long n = 0;
   bool bHasPlayer = (lPlayerMask != 0x0000);

   BLrpNode *pnode = &mpTree[n][x][z];
   BLrpNode tempNode;
   BLrpNode *pOrigNode = NULL;

   BVector vExteriorPoints[4];
   mStaticPoints[0].y = mStaticPoints[1].y = mStaticPoints[2].y = mStaticPoints[3].y = 0.0f;

   // Get the boundaries of the cell.
   getQuadBoundaries(n, x, z, mStaticPoints[0], mStaticPoints[2]);
   bool bDone = false;


   if (bHasPlayer)
   {

      if (!pQuadHull->overlapsBox(mStaticPoints[0].x, mStaticPoints[0].z, mStaticPoints[2].x,
         mStaticPoints[2].z))
         return;
      else
      {
         // Make a copy of the node, to use to determine if we're really going to make this a
         // player quad or not.  If after analysis, we've blocked 2 or more sides of the quad,
         // then it becomes a player quad.  Otherwise, we want to leave it untouched.
         tempNode = *pnode;
         pOrigNode = pnode;
         pnode = &tempNode;
         // Clear the quad.
         pnode->btPathingConns[0] = cConnAll;
         pnode->btPassability[0] = cPassable;
      }
   }

/*
      // Update the cPlayerQuad bit for all the buckets at level 0..
      #ifdef DEBUG_LRPPATHSYNC
      if (mbEnableSync)
      {
         syncPathingCode("setting Player Data for node..");
         syncPathingData("lPlayerMask", lPlayerMask);
      }
      #endif

      for (long k = 0; k < clNumBuckets; k++)
      {
         pnode->btPassability[k] |= cPlayerQuad;
         // Clear the connectivity information in this quad.  This is the *only* time we
         // clear connectivity info when adding an obstruction.
         pnode->btPathingConns[k] = cConnAll;
      
         // But don't ever reset the edge of world connections..
         if (x == 0)
         {
            pnode->btPathingConns[k] &= ~cConnWest;
         }
         if (x == mlSizeX[n] - 1)
         {
            pnode->btPathingConns[k]&= ~cConnEast;
         }
         if (z == 0)
         {
            pnode->btPathingConns[k] &= ~cConnSouth;
         }
         if (z == mlSizeZ[n] - 1)
         {
            pnode->btPathingConns[k] &= ~cConnNorth;
         }

      }
   
      pnode->lPlayerMask = lPlayerMask;
      return;
   }

*/
   // If this quad has already been marked, then bail.
   if (pnode->btPassability[0] & cAllInvalid)
   {
      if (x > 0 && !(pnode->btPathingConns[0] & cConnWest))
         mpTree[0][x-1][z].btPathingConns[0] &= ~cConnEast;
      if (z < mlSizeZ[0] - 1 && !(pnode->btPathingConns[0] & cConnNorth))
         mpTree[0][x][z+1].btPathingConns[0] &= ~cConnSouth;
      if (x < mlSizeX[0] - 1 && !(pnode->btPathingConns[0] & cConnEast))
         mpTree[0][x+1][z].btPathingConns[0] &= ~cConnWest;
      if (z > 0 && !(pnode->btPathingConns[0] & cConnSouth))
         mpTree[0][x][z-1].btPathingConns[0] &= ~cConnNorth;
      return;
   }

   bool bHullInsides[4];

   // Check each of the four vertices.  
   mStaticPoints[1].x = mStaticPoints[0].x;
   mStaticPoints[1].z = mStaticPoints[2].z;
   mStaticPoints[3].x = mStaticPoints[2].x;
   mStaticPoints[3].z = mStaticPoints[0].z;
   mStaticPoints[4].x = mStaticPoints[0].x + ((mStaticPoints[2].x - mStaticPoints[0].x) * 0.5f);
   mStaticPoints[4].z = mStaticPoints[0].z + ((mStaticPoints[2].z - mStaticPoints[0].z) * 0.5f);

   // Determine if any and/or all of the vertices are inside the hull.
   for (long m = 0; m < 4; m++)
	   bHullInsides[m] = pQuadHull->inside(mStaticPoints[m]);

   // If all four vertices are inside the hull, we are done.
   if (bHullInsides[0] && bHullInsides[1] && bHullInsides[2] && bHullInsides[3])
   {
	   pnode->btPassability[0] |= cAllInvalid;
	   pnode->btPathingConns[0] = cConnNone;
      if (!bHasPlayer)
         return;
      else
         bDone = true;
   }

   long lP1 = 3;
   long lP2 = 0;
 
   static long lEdges[4] = 
   {
      cConnSouth,
      cConnWest,
      cConnNorth,
      cConnEast
   };


   if (!bDone)
   {
      for (long m = 0; m < 4; m++)
      {
         // If the edge is already disconnected, just move on..
         if (!(pnode->btPathingConns[0] & lEdges[m]))
         {
            lP1 = lP2;
            lP2++;
            continue;
         }

         BVector vInt1(0.0f);
         bool bInt1 = false;
         bInt1;
         BVector vDir(0.0f);
         float fScale = 0.0f;
         fScale;
         bool bBothOutside = false;
         bBothOutside;

         vExteriorPoints[m].x = -1;

         long lInIdx = -1;
         lInIdx;
         long lOutIdx = -1;
         lOutIdx;

         // TRB (1/24/08):  If the obstruction overlaps any of the edges, mark the edge impassable regardless
         // of how much it intersects.  This will increase the size of obstructions in the LRP tree.
         // For the large units in Halo Wars there were times when the long range pather would find a path the
         // short range pather couldn't get through.
         #ifdef ENABLE_EXPANDED_IMPASSABILITY

            if (bHullInsides[lP1] || bHullInsides[lP2])
               pnode->btPathingConns[0] &= ~lEdges[m];

         #else

            if (bHullInsides[lP1] && bHullInsides[lP2])
            {
               pnode->btPathingConns[0] &= ~lEdges[m];
               lP1 = lP2;
               lP2++;
               continue;
            }

            if (!bHullInsides[lP1])
            {
               if (bHullInsides[lP2])
               {
                  lInIdx = lP2;
                  lOutIdx = lP1;
               }
               else
                  bBothOutside = true;
            }
            else
            {
               if (!bHullInsides[lP2])
               {
                  lInIdx = lP1;
                  lOutIdx = lP2;
               }
            }
            if (!bBothOutside)
            {   
               // See if the outside point is insie of any other expanded obs..
               mpObMgr->findObstructions(mStaticPoints[lOutIdx], false, false, sTempObs);
               if (sTempObs.getNumber() == 0)
               {
                  bInt1 = mpObMgr->segmentIntersects(mStaticPoints[lOutIdx], mStaticPoints[lInIdx], false, vInt1);
                  if (bInt1)
                  {               
                     if (mStaticPoints[lOutIdx].xzDistanceSqr(vInt1) < .0025)
                     {
                        pnode->btPathingConns[0] &= ~lEdges[m];
                        lP1 = lP2;
                        lP2++;
                        continue;
                     }

                  }
                  else
                  {
                     // 1 point outside, and 1 point inside, but no seg intersect?  wtf?
                     //BASSERT(0);
                  }
               }
               else
               {
                  // Get the list of obs this seg intersects with..
                  mpObMgr->getObjectsIntersections(BObstructionManager::cGetAllIntersections,
                     mStaticPoints[lOutIdx], mStaticPoints[lInIdx], false, sSegResults);
               
                  // Find the closest Intersect hull that's not a hull the outside point is within.
                  long lNumInts = sSegResults.getNumber();
                  long lNumObs = sTempObs.getNumber();
                  float fBestDistSqr = cMaximumFloat;
                  // FoundCurrHull is a hack to handle the case where "inside" reports that
                  // the point at lInIdx is inside our current hull, but "segIntersect" between lOutIdx &
                  // lInIdx doesn't find our current hull.
                  bool bFoundCurrHull = false;
                  for (long i = 0; i < lNumInts; i++)
                  {
                     if (sSegResults[i].pObNode == pObNode)
                        bFoundCurrHull = true;
                     long j;
                     for (j = 0; j < lNumObs; j++)
                        if (sSegResults[i].pObNode == sTempObs[j])
                           break;
                     if (j != lNumObs)
                        continue;

                     if (sSegResults[i].fDistSqr < fBestDistSqr)
                     {
                        fBestDistSqr = sSegResults[i].fDistSqr;
                     }
                  }
                  if (fBestDistSqr == cMaximumFloat)
                  {
                     if (!bFoundCurrHull)
                        fBestDistSqr = mfCellSize * mfCellSize;
                  }
                  if (fBestDistSqr < cMaximumFloat)
                  {
                     sTempObs.setNumber(0);
                     sTempObs.add(pObNode);

                     fScale = (float)sqrt((double)fBestDistSqr) - 0.05f;
                     vDir = mStaticPoints[lInIdx] - mStaticPoints[lOutIdx];
                     vDir.normalize();
                     vDir *= fScale;
                     vExteriorPoints[m] = mStaticPoints[lOutIdx] + vDir;
                     // Get the intersection point, and see if it's inside any other hulls..
                     mpObMgr->findObstructions(vExteriorPoints[m], false, true, sTempObs);
                     if (sTempObs.getNumber() > 1)
                        pnode->btPathingConns[0] &= ~lEdges[m];
                  }
               
               }
            }
            else
            {
               // Do something with both outside..
            }

         #endif

         // Continue..
         lP1 = lP2;
         lP2++;
      }
   } // end of if !bdone)


   // If this is a player quad, don't mark it as obstructed.  Instead, see if the obstruction blocked 2 or more
   // sides.  If so, then mark it as a player owned quad.
   if (bHasPlayer)
   {
      // If everything is still connected, or there are connections
      // ONLY on one side of the quad, then bail, leaving the original
      // node untouched.
      if (pnode->btPathingConns[0] == 15 ||  // 1111
          pnode->btPathingConns[0] == 14 ||  // 1110
          pnode->btPathingConns[0] == 13 ||  // 1101
          pnode->btPathingConns[0] == 11 ||  // 1011
          pnode->btPathingConns[0] == 7)     // 0111
      {
         return;
      }

      // Update the cPlayerQuad bit for all the buckets at level 0..
      #ifdef DEBUG_LRPPATHSYNC
      if (mbEnableSync)
      {
         syncPathingCode("setting Player Data for node..");
         syncPathingData("lPlayerMask", lPlayerMask);
      }
      #endif

      // rest pnode, to do the real deal.
      pnode = pOrigNode;

      for (long k = 0; k < clNumBuckets; k++)
      {
         pnode->btPassability[k] = (cPlayerQuad | cInUse);  // jce [3/31/2005] -- changed this to also set cInUse since some checks only look at that flag
         // Clear the connectivity information in this quad.  This is the *only* time we
         // clear connectivity info when adding an obstruction.
         pnode->btPathingConns[k] = cConnAll;

         // We need to adjust the opposite connections at the base level.
         if (x > 0)
            mpTree[0][x-1][z].btPathingConns[0] |= cConnEast;
         if (z < mlSizeZ[0] - 1)
            mpTree[0][x][z+1].btPathingConns[0] |= cConnSouth;
         if (x < mlSizeX[0] - 1)
            mpTree[0][x+1][z].btPathingConns[0] |= cConnWest;
         if (z > 0)
            mpTree[0][x][z-1].btPathingConns[0] |= cConnNorth;
      
         // But don't ever reset the edge of world connections..
         if (x == 0)
         {
            pnode->btPathingConns[k] &= ~cConnWest;
         }
         if (x == mlSizeX[n] - 1)
         {
            pnode->btPathingConns[k]&= ~cConnEast;
         }
         if (z == 0)
         {
            pnode->btPathingConns[k] &= ~cConnSouth;
         }
         if (z == mlSizeZ[n] - 1)
         {
            pnode->btPathingConns[k] &= ~cConnNorth;
         }

      }
   
      pnode->lPlayerMask = lPlayerMask;
      return;
   }

   // For Level 0 connections.. mark dead-end quads (ie, three sides blocked) as invalid..      
   #ifdef DEBUG_CLOSETHREESIDEDQUADS
   if ((pnode->btPathingConns[0] & cConnAll) == cConnNorth)
   {
      pnode->btPathingConns[0] &= ~cConnNorth;
   }
   else if ((pnode->btPathingConns[0] & cConnAll) == cConnEast)
   {
      pnode->btPathingConns[0] &= ~cConnEast;
   }
   else if ((pnode->btPathingConns[0] & cConnAll) == cConnSouth)
   {
      pnode->btPathingConns[0] &= ~cConnSouth;
   }
   else if ((pnode->btPathingConns[0] & cConnAll) == cConnWest)
   {
      pnode->btPathingConns[0] &= ~cConnWest;
   }
   #endif

   // Do the opposite sides check.
   if (x > 0 && !(pnode->btPathingConns[0] & cConnWest))
      mpTree[0][x-1][z].btPathingConns[0] &= ~cConnEast;
   if (z < mlSizeZ[0] - 1 && !(pnode->btPathingConns[0] & cConnNorth))
      mpTree[0][x][z+1].btPathingConns[0] &= ~cConnSouth;
   if (x < mlSizeX[0] - 1 && !(pnode->btPathingConns[0] & cConnEast))
      mpTree[0][x+1][z].btPathingConns[0] &= ~cConnWest;
   if (z > 0 && !(pnode->btPathingConns[0] & cConnSouth))
      mpTree[0][x][z-1].btPathingConns[0] &= ~cConnNorth;

   if ((pnode->btPathingConns[0] & cConnAll) == cConnNone)
      pnode->btPassability[0] |= cAllInvalid;

   return;

}
#endif

//==============================================================================
// BLrpTree::setPathingConnections
// setPathingConnections updates the pathing connections for all levels of the 
// quad, for each bucket except bucket zero, which is the "special" bucket that
// was build with updateBaseQuad.  
//==============================================================================
void BLrpTree::setPathingConnections(long k, long n, long x, long z)
{

   BLrpNode *pnode = &mpTree[n][x][z];

   #ifdef DEBUG_LRPTIMING_STATS
   ++mlPCCalls;

   LARGE_INTEGER startTime;
   LARGE_INTEGER endTime;
   int64delta;
   float elapsed;
   QueryPerformanceCounter(&startTime);
   #endif

   #ifdef DEBUG_SETPATHINGCONNECTION
   blog("----> setPathingConnection");
   blog("k: %d, n: %d, x: %d, z: %d", k, n, x, z);
   #endif
/*
   #ifdef DEBUG_LRPPATHSYNC
   syncPathingCode("setPathingConnections");
   syncPathingData("k", k);
   syncPathingData("n", n);
   syncPathingData("x", x);
   syncPathingData("z", z);
   #endif
*/
   // First, get the bucket size of the appropriate bucket.
   long lSize = clBucketSizes[k];
   long lQuadSize = 1 << n;
   #ifdef DEBUG_SETCONNECTION
   blog("QuadSize: %d BucketSize: %d", lQuadSize, lSize);
   #endif
   
   if (n == 0)
   {
      // For bucket 0, we determine pathing connection based on the passability of the 
      // tile & it's surrounding tiles.
      if (k == 0)
      {
         // The connection info has already been set by updateBaseQuad..
         // just to the opposite checks here, and return.
         if (x > 0 && !(pnode->btPathingConns[0] & cConnWest))
            mpTree[0][x-1][z].btPathingConns[0] &= ~cConnEast;
         if (z < mlSizeZ[0] - 1 && !(pnode->btPathingConns[0] & cConnNorth))
            mpTree[0][x][z+1].btPathingConns[0] &= ~cConnSouth;
         if (x < mlSizeX[0] - 1 && !(pnode->btPathingConns[0] & cConnEast))
            mpTree[0][x+1][z].btPathingConns[0] &= ~cConnWest;
         if (z > 0 && !(pnode->btPathingConns[0] & cConnSouth))
            mpTree[0][x][z-1].btPathingConns[0] &= ~cConnNorth;

         #ifdef DEBUG_LRPTIMING_STATS
         QueryPerformanceCounter(&endTime);
         delta = (long)(endTime.QuadPart - startTime.QuadPart);
         elapsed = 1000.0f*(float)delta/(float)(mQPFreq.QuadPart);
         mfPCSum += elapsed;

         if (elapsed > mfPCMax)
            mfPCMax = elapsed;
         #endif

         return;
      }

      // For the remaining buckets, determine the "pathing" connection based
      // on the pathing connections for the appropriate amount of bucket 0 tiles.
      for (BYTE btConn = cConnNorth; btConn <= cConnWest; btConn <<= 1)
      {
         long lDiff = (lSize - lQuadSize) >> 1;
         long lStartX = x << n;
         long lStartZ = z << n;
         long x1 = 0, x2 = 0, xn = 0;
         long z1 = 0, z2 = 0, zn = 0;

         switch (btConn)
         {
            case cConnNorth:
               zn = lStartZ + lQuadSize - 1;
               x1 = lStartX - lDiff;
               x2 = x1 + lSize;
               break;
            case cConnEast:
               xn = lStartX + lQuadSize - 1;
               z1 = lStartZ - lDiff;
               z2 = z1 + lSize;
               break;
            case cConnSouth:
               zn = lStartZ;
               x1 = lStartX - lDiff;
               x2 = x1 + lSize;
               break;
            case cConnWest:
               xn = lStartX;
               z1 = lStartZ - lDiff;
               z2 = z1 + lSize;
               break;
         }

         #ifdef DEBUG_SETCONNECTION
         blog("x1: %d, x2: %d, xn: %d", x1, x2, xn);
         blog("z1: %d, z2: %d, zn: %d", z1, z2, zn);
         #endif

         if (btConn == cConnNorth || btConn == cConnSouth)
         {
            for (xn = x1; xn < x2; xn++)
            {
               if (xn < 0)
                  break;
               if (xn > mlSizeX[0] - 1)
                  break;
               if ((mpTree[0][xn][zn].btPathingConns[0] & btConn) == 0)
                  break;
            }
            if (xn == x2)
            {
               #ifdef DEBUG_SETCONNECTION
               blog("Gap found");
               blog("<---- setPathingConnection");
               #endif
               continue;
            }
         }
         else
         {
            for (zn = z1; zn < z2; zn++)
            {
               if (zn < 0)
                  break;
               if (zn > mlSizeZ[0] - 1)
                  break;
               if ((mpTree[0][xn][zn].btPathingConns[0] & btConn) == 0)
                  break;
            }
            if (zn == z2)
            {
               #ifdef DEBUG_SETCONNECTION
               blog("Gap found");
               blog("<---- setPathingConnection");
               #endif
               continue;
            }
         }

         // If we're here, we didn't find a gap big enough.
         #ifdef DEBUG_SETCONNECTION
         blog("No Gap found, turning off PATHING connection");
         blog("<---- setConnection");
         #endif
         pnode->btPathingConns[k] &= ~btConn;

      } // end of for btConn...

      // For Level 0 connections.. mark dead-end quads (ie, three sides blocked) as invalid..      
      #ifdef DEBUG_CLOSETHREESIDEDQUADS
      if ((pnode->btPathingConns[k] & cConnAll) == cConnNorth)
      {
         pnode->btPathingConns[k] &= ~cConnNorth;
      }
      else if ((pnode->btPathingConns[k] & cConnAll) == cConnEast)
      {
         pnode->btPathingConns[k] &= ~cConnEast;
      }
      else if ((pnode->btPathingConns[k] & cConnAll) == cConnSouth)
      {
         pnode->btPathingConns[k] &= ~cConnSouth;
      }
      else if ((pnode->btPathingConns[k] & cConnAll) == cConnWest)
      {
         pnode->btPathingConns[k] &= ~cConnWest;
      }
      #endif
      
      // turn off the connections of the surrounding quads.
      if (x > 0 && !(pnode->btPathingConns[k] & cConnWest))
         mpTree[0][x-1][z].btPathingConns[k] &= ~cConnEast;
      if ((x < mlSizeX[0] - 1) && !(pnode->btPathingConns[k] & cConnEast))
         mpTree[0][x+1][z].btPathingConns[k] &= ~cConnWest;
      if (z > 0 && !(pnode->btPathingConns[k] & cConnSouth))
         mpTree[0][x][z-1].btPathingConns[k] &= ~cConnNorth;
      if ((z < mlSizeZ[0] - 1) & !(pnode->btPathingConns[k] & cConnNorth))
         mpTree[0][x][z+1].btPathingConns[k] &= ~cConnSouth;

   }
   else
   {
      // If we're equal to or greater than the bucket size, then check the children.  If either of
      // them is connected, then we're connected.
      BLrpNode *pnodeChildren[4];
      long lNextLevel = n - 1;
      long x1 = x << 1;
      long x2 = x1 + 1;
      long z1 = z << 1;
      long z2 = z1 + 1;

      // SW Child
      pnodeChildren[0] = &mpTree[lNextLevel][x1][z1];
      pnodeChildren[1] = &mpTree[lNextLevel][x1][z2];
      pnodeChildren[2] = &mpTree[lNextLevel][x2][z2];
      pnodeChildren[3] = &mpTree[lNextLevel][x2][z1];
      
      // We have a valid connection if and only if both children on that side are also connected... 
      if (!(pnodeChildren[0]->btPathingConns[k] & cConnWest) && !(pnodeChildren[1]->btPathingConns[k] & cConnWest))
         pnode->btPathingConns[k] &= ~cConnWest;
      if (!(pnodeChildren[1]->btPathingConns[k] & cConnNorth) && !(pnodeChildren[2]->btPathingConns[k] & cConnNorth))
         pnode->btPathingConns[k] &= ~cConnNorth;
      if (!(pnodeChildren[2]->btPathingConns[k] & cConnEast) && !(pnodeChildren[3]->btPathingConns[k] & cConnEast))
         pnode->btPathingConns[k] &= ~cConnEast;
      if (!(pnodeChildren[3]->btPathingConns[k] & cConnSouth) && !(pnodeChildren[0]->btPathingConns[k] & cConnSouth))
         pnode->btPathingConns[k] &= ~cConnSouth;

      // turn off the connections of the surrounding quads.
      if (x > 0 && !(pnode->btPathingConns[k] & cConnWest))
         mpTree[n][x-1][z].btPathingConns[k] &= ~cConnEast;
      if ((x < mlSizeX[n] - 1) && !(pnode->btPathingConns[k] & cConnEast))
         mpTree[n][x+1][z].btPathingConns[k] &= ~cConnWest;
      if (z > 0 && !(pnode->btPathingConns[k] & cConnSouth))
         mpTree[n][x][z-1].btPathingConns[k] &= ~cConnNorth;
      if ((z < mlSizeZ[n] - 1) & !(pnode->btPathingConns[k] & cConnNorth))
         mpTree[n][x][z+1].btPathingConns[k] &= ~cConnSouth;
   }

   // If I have no connections in our out, mark me as invalid.
   if ((pnode->btPathingConns[k] & cConnAll) == cConnNone)
      pnode->btPassability[k] |= cAllInvalid;

   #ifdef DEBUG_LRPTIMING_STATS
   QueryPerformanceCounter(&endTime);
   delta = (long)(endTime.QuadPart - startTime.QuadPart);
   elapsed = 1000.0f*(float)delta/(float)(mQPFreq.QuadPart);
   mfPCSum += elapsed;

   if (elapsed > mfPCMax)
      mfPCMax = elapsed;

   #endif

   return;

}


//Halwes - 10/9/2006 - Remove recursion
#if !defined( PERF_REMOVE_RECURSION )
//==============================================================================
// BLrpTree::initConnectivity
// This sets all quads connected at all levels.. and passable.
//==============================================================================
bool BLrpTree::initConnectivity(long n, long x, long z)
{
   if (n < 0 || n > cMaxLevel)
      return false;
   if (x < 0 || x > mlSizeX[n] - 1)
      return false;
   if (z < 0 || z > mlSizeZ[n] - 1)
      return false;

   // Update my children..
   if (n > 0)
   {
      long x1 = x << 1;
      long z1 = z << 1;
      long x2 = x1 + 1;
      long z2 = z1 + 1;
      long lNextLevel = n - 1;
      initConnectivity(lNextLevel, x1, z1);
      initConnectivity(lNextLevel, x1, z2);
      initConnectivity(lNextLevel, x2, z2);
      initConnectivity(lNextLevel, x2, z1);
   }

   // Update my self..
   BLrpNode *pNode = &mpTree[n][x][z];
   pNode->lPlayerMask = 0x0000;
   pNode->btStatus = cUnknown;

   for (long k = 0; k < clNumBuckets; k++)
   {
      pNode->btPathingConns[k] = cConnAll;
      pNode->btPassability[k] = cPassable;

      if (x == 0)
      {
         pNode->btPathingConns[k] &= ~cConnWest;
      }
      if (x == mlSizeX[n] - 1)
      {
         pNode->btPathingConns[k]&= ~cConnEast;
      }
      if (z == 0)
      {
         pNode->btPathingConns[k] &= ~cConnSouth;
      }
      if (z == mlSizeZ[n] - 1)
      {
         pNode->btPathingConns[k] &= ~cConnNorth;
      }

      // Turn on the InUse flags..
      if (n == cMaxLevel)
         pNode->btPassability[k] |= cInUse;

   }


   return true;
}
#else // PERF_REMOVE_RECURSION
//==============================================================================
// BLrpTree::initConnectivity
// This sets all quads connected at all levels.. and passable.
//==============================================================================
void BLrpTree::initConnectivity(long n, long x, long z)
{
   if( ( n < 0 ) || ( n > cMaxLevel ) )
   {
      return;
   }
   if( ( x < 0 ) || ( x > ( mlSizeX[n] - 1 ) ) )
   {
      return;
   }
   if( ( z < 0 ) || ( z > ( mlSizeZ[n] - 1 ) ) )
   {
      return;
   }

   long       stackIndex = -1;
   nodeData   nodeStack[cMaxDepth];

   // Set and push root node
   nodeData tempNode;
   tempNode.n     = n;
   tempNode.x     = x;
   tempNode.z     = z;
   tempNode.visit = 0;
   stackIndex     = pushStack( nodeStack, stackIndex, tempNode );

   // Iterate through children nodes
   while( stackIndex >= 0 )
   {      
      if( ( nodeStack[stackIndex].n > 0 ) && ( nodeStack[stackIndex].visit < 4 ) )
      {
         long x1 = nodeStack[stackIndex].x << 1;
         long z1 = nodeStack[stackIndex].z << 1;

         switch( nodeStack[stackIndex].visit )
         {
            // X1Z1
            case 0:                                             
               tempNode.x = x1;
               tempNode.z = z1;
               break;

            // X1Z2
            case 1:
               tempNode.x = x1;
               tempNode.z = z1 + 1;
               break;

            // X2Z1
            case 2:
               tempNode.x = x1 + 1;
               tempNode.z = z1;
               break;

            // X2Z2
            case 3:
               tempNode.x = x1 + 1;
               tempNode.z = z1 + 1;
               break;         
         }                  

         tempNode.n = nodeStack[stackIndex].n - 1;
         nodeStack[stackIndex].visit++;
         stackIndex = pushStack( nodeStack, stackIndex, tempNode );
      }
      else
      {         
         // Update this node and pop
         BLrpNode *pNode    = &mpTree[nodeStack[stackIndex].n][nodeStack[stackIndex].x][nodeStack[stackIndex].z];
         pNode->lPlayerMask = 0x0000;
         pNode->btStatus    = cUnknown;

         for( long k = 0; k < clNumBuckets; k++ )
         {
            pNode->btPathingConns[k] = cConnAll;
            pNode->btPassability[k]  = cPassable;

            if( nodeStack[stackIndex].x == 0 )
            {
               pNode->btPathingConns[k] &= ~cConnWest;
            }
            if( nodeStack[stackIndex].x == mlSizeX[nodeStack[stackIndex].n] - 1 )
            {
               pNode->btPathingConns[k]&= ~cConnEast;
            }
            if( nodeStack[stackIndex].z == 0 )
            {
               pNode->btPathingConns[k] &= ~cConnSouth;
            }
            if( nodeStack[stackIndex].z == mlSizeZ[nodeStack[stackIndex].n] - 1 )
            {
               pNode->btPathingConns[k] &= ~cConnNorth;
            }

            // Turn on the InUse flags..
            if( nodeStack[stackIndex].n == cMaxLevel )
            {
               pNode->btPassability[k] |= cInUse;
            }
         }
         stackIndex = popStack( nodeStack, stackIndex );
      }
   }
}
#endif // PERF_REMOVE_RECURSION


//==============================================================================
// BLrpTree::render
//==============================================================================
void BLrpTree::render()
{
   if (!mbInitialized)
      return;

   for (long x = 0; x < mlSizeX[cMaxLevel]; x++)
      for (long z = 0; z < mlSizeZ[cMaxLevel]; z++)
          renderQuad(cMaxLevel, x, z, 0);//BClipHints::cClipFlagsMask);
   return;
}

//==============================================================================
// BLrpTree::renderQuad
//==============================================================================
void BLrpTree::renderQuad(long n, long x, long z, long clipHint, bool bInvalid)
{
   BLrpNode *pNode = &mpTree[n][x][z];
   if (!pNode)
   {
      BASSERT(0);
      return;
   }

   // Check a bounding box.
   // Make box.
   BVector minCorner;
   BVector maxCorner;
   getQuadBoundaries(n, x, z, minCorner, maxCorner);      
   minCorner.y=0;
   maxCorner.y=0;

   if(!gRender.getViewParams().isPointOnScreen(minCorner) && !gRender.getViewParams().isPointOnScreen(maxCorner))
      return;

   // Render Mode
   // 1     - Quad & Conn Info for bucket 0
   // 2     - Quad & Conn Info for bucket 1
   // 3     - Quad & Conn Info for bucket 2
   // 4     - Conn Info for base quad.
   // 5     - Valid/Invalid for bucket 0
   long lRenderMode = gGame.getRenderPathingQuad();
   long lDisplayMode = 0;
   long lBucket = 0;
   long lMinDisplayLevel = 0;
   /*
   switch(lRenderMode)
   {
      case 1:
      case 2:
      case 3:
         lBucket = lRenderMode - 1;
         break;
      case 4:
         lDisplayMode = 1;
         break;
      case 5:
         lDisplayMode = 2;
         break;
   }
   */
   switch(lRenderMode)
   {
      case 1:
      case 2:
      case 3:
         lBucket = lRenderMode - 1;
         break;
      case 4:
      case 5:
      case 6:
      case 7:
//         lBucket = lRenderMode - 4;
         lBucket = 0;
         lMinDisplayLevel = lRenderMode - 4;
         lDisplayMode = 1;
//         lDisplayMode = 2;
         break;
   }
   
   if (lBucket > clNumBuckets - 1)
      lBucket = clNumBuckets - 1;

   switch (lDisplayMode)
   {
      case 0: // InUse breakdown w/ Conn Info
      case 1: // Conn Info at level 0
      {
         if ((lDisplayMode == 0 && (pNode->btPassability[lBucket] & cInUse)) ||
             (lDisplayMode == 1 && n == lMinDisplayLevel))
         {
            // Render this quad
            mStaticPoints[0].y = mStaticPoints[1].y = mStaticPoints[2].y = mStaticPoints[3].y = 0.1f;
            getQuadBoundaries(n, x, z, mStaticPoints[0], mStaticPoints[2]);
            mStaticPoints[1].x = mStaticPoints[0].x;
            mStaticPoints[1].z = mStaticPoints[2].z;
            mStaticPoints[3].x = mStaticPoints[2].x;
            mStaticPoints[3].z = mStaticPoints[0].z;
            mStaticHull.initialize(mStaticPoints, 4, true);
            mStaticHull.expand(-0.1f);
            const BDynamicSimVectorArray &v2 = mStaticHull.getPoints();

            BVector vCenter;
            getQuadEstimate(n, x, z, vCenter);

            if (bInvalid)
            {
               gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDCyan, cDWORDCyan, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDCyan, cDWORDCyan, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDCyan, cDWORDCyan, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDCyan, cDWORDCyan, 0.75f);
            }
            else
            {

               if (pNode->btPassability[lBucket] & cPlayerQuad)
               {
                  long lPlayerID = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID();
                  long lPlayerMask = (1 << (lPlayerID - 1));
                  if ((pNode->lPlayerMask & lPlayerMask) == 0)
                  {
                     if (pNode->btPathingConns[lBucket] & cConnWest)
                        gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDPurple, cDWORDPurple, 0.75f);
                     else
                        gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDRed, cDWORDRed, 0.75f);

                     if (pNode->btPathingConns[lBucket] & cConnNorth)
                        gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDPurple, cDWORDPurple, 0.75f);
                     else
                        gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDRed, cDWORDRed, 0.75f);

                     if (pNode->btPathingConns[lBucket] & cConnEast)
                        gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDPurple, cDWORDPurple, 0.75f);
                     else
                        gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDRed, cDWORDRed, 0.75f);

                     if (pNode->btPathingConns[lBucket] & cConnSouth)
                        gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDPurple, cDWORDPurple, 0.75f);
                     else
                        gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDRed, cDWORDRed, 0.75f);
/*
                     gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDPurple, cDWORDPurple, 0.75f);
                     gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDPurple, cDWORDPurple, 0.75f);
                     gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDPurple, cDWORDPurple, 0.75f);
                     gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDPurple, cDWORDPurple, 0.75f);
*/
                  }
                  else
                  {
                     if (pNode->btPathingConns[lBucket] & cConnWest)
                        gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDGreen, cDWORDGreen, 0.75f);
                     else
                        gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDRed, cDWORDRed, 0.75f);

                     if (pNode->btPathingConns[lBucket] & cConnNorth)
                        gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDGreen, cDWORDGreen, 0.75f);
                     else
                        gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDRed, cDWORDRed, 0.75f);

                     if (pNode->btPathingConns[lBucket] & cConnEast)
                        gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDGreen, cDWORDGreen, 0.75f);
                     else
                        gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDRed, cDWORDRed, 0.75f);

                     if (pNode->btPathingConns[lBucket] & cConnSouth)
                        gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDGreen, cDWORDGreen, 0.75f);
                     else
                        gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDRed, cDWORDRed, 0.75f);
/*
                     gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDGreen, cDWORDGreen, 0.75f);
                     gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDGreen, cDWORDGreen, 0.75f);
                     gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDGreen, cDWORDGreen, 0.75f);
                     gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDGreen, cDWORDGreen, 0.75f);
*/
                  }
               }
               else
               {

                  if (pNode->btPathingConns[lBucket] & cConnWest)
                     gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDGreen, cDWORDGreen, 0.75f);
                  else
                     gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDRed, cDWORDRed, 0.75f);

                  if (pNode->btPathingConns[lBucket] & cConnNorth)
                     gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDGreen, cDWORDGreen, 0.75f);
                  else
                     gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDRed, cDWORDRed, 0.75f);

                  if (pNode->btPathingConns[lBucket] & cConnEast)
                     gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDGreen, cDWORDGreen, 0.75f);
                  else
                     gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDRed, cDWORDRed, 0.75f);

                  if (pNode->btPathingConns[lBucket] & cConnSouth)
                     gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDGreen, cDWORDGreen, 0.75f);
                  else
                     gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDRed, cDWORDRed, 0.75f);
               }
            }
            
            if (pNode->btStatus & cInTarget)
            {
               gTerrainSimRep.addDebugPointOverTerrain(vCenter, 2.0f, cDWORDBlack, 2.0f);
            }
            
            if (pNode->btStatus & cInPath)
            {
               gTerrainSimRep.addDebugPointOverTerrain(vCenter, 2.0f, cDWORDYellow, 2.0f);
            }

            long lChildLevel = n - 1;
            if (lChildLevel >= 0)
            {
               long x1 = x << 1;
               long z1 = z << 1;
               long x2 = x1 + 1;
               long z2 = z1 + 1;
               renderQuad(lChildLevel, x1, z1, clipHint, true);
               renderQuad(lChildLevel, x1, z2, clipHint, true);
               renderQuad(lChildLevel, x2, z2, clipHint, true);
               renderQuad(lChildLevel, x2, z1, clipHint, true);
            }

         }
         else
         {
            // Render my children then..
            long lChildLevel = n - 1;
            if (lChildLevel >= 0)
            {
               long x1 = x << 1;
               long z1 = z << 1;
               long x2 = x1 + 1;
               long z2 = z1 + 1;
               renderQuad(lChildLevel, x1, z1, clipHint, bInvalid);
               renderQuad(lChildLevel, x1, z2, clipHint, bInvalid);
               renderQuad(lChildLevel, x2, z2, clipHint, bInvalid);
               renderQuad(lChildLevel, x2, z1, clipHint, bInvalid);
            }
         }
         break;
      }
      case 2: // Invalid/Valid Info
      {
         if (((pNode->btPassability[lBucket] & cAllInvalid) == cAllInvalid) || (n == lMinDisplayLevel))
         {
            mStaticPoints[0].y = mStaticPoints[1].y = mStaticPoints[2].y = mStaticPoints[3].y = 0.1f;
            getQuadBoundaries(n, x, z, mStaticPoints[0], mStaticPoints[2]);
            mStaticPoints[1].x = mStaticPoints[0].x;
            mStaticPoints[1].z = mStaticPoints[2].z;
            mStaticPoints[3].x = mStaticPoints[2].x;
            mStaticPoints[3].z = mStaticPoints[0].z;
            mStaticHull.initialize(mStaticPoints, 4, true);
            mStaticHull.expand(-0.1f);
            const BDynamicSimVectorArray &v2 = mStaticHull.getPoints();
            //BTerrainBase *pterrain = game->getWorld()->getTerrain();

            if ((pNode->btPassability[lBucket] & cAllInvalid) == cAllInvalid)
            {
               gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDWhite, cDWORDWhite, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDWhite, cDWORDWhite, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDWhite, cDWORDWhite, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDWhite, cDWORDWhite, 0.75f);
            }
            else
            {
               gTerrainSimRep.addDebugLineOverTerrain(v2[0], v2[1], cDWORDGreen, cDWORDGreen, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[1], v2[2], cDWORDGreen, cDWORDGreen, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[2], v2[3], cDWORDGreen, cDWORDGreen, 0.75f);
               gTerrainSimRep.addDebugLineOverTerrain(v2[3], v2[0], cDWORDGreen, cDWORDGreen, 0.75f);
            }
         }
         else
         {
            // Render my children then..
            long lChildLevel = n - 1;
            long x1 = x << 1;
            long z1 = z << 1;
            long x2 = x1 + 1;
            long z2 = z1 + 1;
            renderQuad(lChildLevel, x1, z1, clipHint);
            renderQuad(lChildLevel, x1, z2, clipHint);
            renderQuad(lChildLevel, x2, z2, clipHint);
            renderQuad(lChildLevel, x2, z1, clipHint);
         }
         break;
      }
   }
   return;
}

//==============================================================================
// BLrpTree::renderDebugPath
//==============================================================================
#ifndef BUILD_FINAL
void BLrpTree::renderDebugPath()
{
   if (!mbInitialized)
      return;

   gTerrainSimRep.addDebugPointOverTerrain(mDebugPath.getWaypoint(0), 2.0f, cDWORDGreen, 2.0f, BDebugPrimitives::cCategoryPathing);
   for (long l = 0; l < mDebugPath.getNumberWaypoints() - 1; l++)
   {
      gTerrainSimRep.addDebugLineOverTerrain(mDebugPath.getWaypoint(l), mDebugPath.getWaypoint(l+1), cDWORDYellow, cDWORDYellow, 1.0f, BDebugPrimitives::cCategoryPathing);
      gTerrainSimRep.addDebugPointOverTerrain(mDebugPath.getWaypoint(l+1), 2.0f, cDWORDYellow, 2.0f, BDebugPrimitives::cCategoryPathing);
   }

   return;
}
#endif

//==============================================================================
// BLrpTree::getQuadBoundaries
//==============================================================================
void BLrpTree::getQuadBoundaries(long n, long x, long z, BVector &vMin, BVector &vMax)
{
   long lCellsPerQuad = 1 << n;
   long lQuadSize = (long)(lCellsPerQuad * mfCellSize);
   vMin.y = vMax.y = 0.0f;
   vMin.x  = (float)(x * lQuadSize);
   vMax.x = vMin.x + (float)lQuadSize;
   vMin.z = (float)(z * lQuadSize);
   vMax.z = vMin.z + (float)lQuadSize;
   return;
}


//==============================================================================
// BLrpTree::getNewNode
//==============================================================================
BLrpPathNode *BLrpTree::getNewNode(BLrpPathNode *lrpParent)
{

   BLrpPathNode *pnodeReturn = mNodeFreeList.acquire();

   // Initialize the node..
   pnodeReturn->fCost = 0.0f;
   pnodeReturn->fEstimate = 0.0f;
   pnodeReturn->lrpParent = lrpParent;
   pnodeReturn->sN = pnodeReturn->sDir = 0;
   pnodeReturn->sX = pnodeReturn->sZ = 0;
   pnodeReturn->fX = pnodeReturn->fZ = 0.0f;

   return pnodeReturn;
}


//==============================================================================
// BLrpTree::addToOpenList
//==============================================================================
BLrpPathNode *BLrpTree::addToOpenList(BLrpPathNode *lrpParent, long n, long x, long z, long lDir,
                                      float fX, float fZ, float fAdditionalCost, bool skipBoundsCheck)
{

   // Timing Stats Init
   #ifdef DEBUG_LRPTIMING_OL
   ++mlOLCalls;

   LARGE_INTEGER startTime;
   LARGE_INTEGER endTime;
   int64delta;
   float elapsed;
   QueryPerformanceCounter(&startTime);
   #endif
   
   // jce [10/3/2008] -- To support playable bounds without long pather quad updates, we're adding a check here for out of bounds nodes
   // as they are considered.  skipBoundsCheck is used for the start node since the playable bounds don't split quads, which means a valid
   // start point, once shifted to the node center, can be out of bounds even though it's really not.
   if(!skipBoundsCheck)
   {
      if(fX < gWorld->getSimBoundsMinX())
         return(NULL);
      if(fX > gWorld->getSimBoundsMaxX())
         return(NULL);
      if(fZ < gWorld->getSimBoundsMinZ())
         return(NULL);
      if(fZ > gWorld->getSimBoundsMaxZ())
         return(NULL);
   }   

   float fNewCost = 0.0f;
   long lOldIndex = -1L;

   BLrpPathNode *pnodeReturn = NULL;

   BLrpNode *pquad = &mpTree[n][x][z];


   BVector vEstPnt(fX, 0.0f, fZ);

   if (lrpParent)
   {

      BVector vParentEstPnt(lrpParent->fX, 0.0f, lrpParent->fZ);

      float fDist = vParentEstPnt.xzDistance(vEstPnt);
      fNewCost = lrpParent->fCost + fDist - ((1 << n) / 2.0f);
      // Optional Additional Cost used to weight certain nodes.. 
      fNewCost += fAdditionalCost;

      // Scan the open list.        
      if (pquad->btStatus & cInQueue)
      {
         long lNum = mOpenList.getNumber();
         for (long m = 0; m < lNum; m++)
         {
            BLrpPathNode *pnode = mOpenList[m];
            if (pnode->sN == n && pnode->sX == x && pnode->sZ == z)
            {
               if (pnode->fCost <= fNewCost)
               {
                  #ifdef DEBUG_LRPTIMING_OL
                  QueryPerformanceCounter(&endTime);
                  delta = (long)(endTime.QuadPart - startTime.QuadPart);
                  elapsed = 1000.0f*(float)delta/(float)(mQPFreq.QuadPart);
                  mfOLSum += elapsed;

                  if (elapsed > mfOLMax)
                     mfOLMax = elapsed;
                  #endif
                  return NULL;
               }
               else
               {
                  pnodeReturn = pnode;
                  pnodeReturn->lrpParent = lrpParent;
                  lOldIndex = m;
                  break;
               }
            }
         }
         if (pnodeReturn == NULL)
         {
            // Scan the closed list.  
            lNum = mClosedList.getNumber();
            for (long m = 0; m < lNum; m++)
            {
               BLrpPathNode *pnode = mClosedList[m];
               if (pnode->sN == n && pnode->sX == x && pnode->sZ == z)
               {
                  if (pnode->fCost <= fNewCost)
                  {
                     #ifdef DEBUG_LRPTIMING_OL
                     QueryPerformanceCounter(&endTime);
                     delta = (long)(endTime.QuadPart - startTime.QuadPart);
                     elapsed = 1000.0f*(float)delta/(float)(mQPFreq.QuadPart);
                     mfOLSum += elapsed;

                     if (elapsed > mfOLMax)
                        mfOLMax = elapsed;
                     #endif
                     return NULL;
                  }
                  else
                  {
                     pnodeReturn = pnode;
                     pnodeReturn->lrpParent = lrpParent;
                     mClosedList.removeIndex(m);
                     break;
                  }
               }
            }
         }
      }
      
      if (!pnodeReturn)
      {
         pnodeReturn = getNewNode(lrpParent);
      }
   }
   else
      pnodeReturn = getNewNode(lrpParent);

   // Set the new parameters.. 
   pnodeReturn->fCost = fNewCost;
   pnodeReturn->fEstimate = vEstPnt.xzDistance(mvGoal);
   pnodeReturn->sN = (BYTE)n;
   pnodeReturn->sX = (short)x;
   pnodeReturn->sZ = (short)z;
   pnodeReturn->sDir = (BYTE)lDir;
   pnodeReturn->fX = fX;
   pnodeReturn->fZ = fZ;

   #ifdef DEBUG_ADDTOOPENLIST
   debug("Node (%d %d %d) with dir %d added to open list with Cost %f, Est: %f, Total: %f",
      pnodeReturn->sN, pnodeReturn->sX, pnodeReturn->sZ, pnodeReturn->sDir, pnodeReturn->fCost,
      pnodeReturn->fEstimate, pnodeReturn->fCost + pnodeReturn->fEstimate);
   #endif

   // Put in the open list appropriately.. 
   if (lOldIndex > -1L)
   {
      // Change in Heap..
      long m = lOldIndex;
      BLrpPathNode *pnodeCurrent = mOpenList[m];

      // Up Heap..
      while ((mOpenList[m >> 1]->fCost + mOpenList[m >> 1]->fEstimate) >= (pnodeCurrent->fCost + pnodeCurrent->fEstimate))
      {
         mOpenList[m] = mOpenList[m >> 1];
         m = m >> 1;
      }
      mOpenList[m] = pnodeCurrent;
   }
   else
   {
      // Add To Heap
      long m = mOpenList.getNumber();
      BLrpPathNode *pnodeCurrent;
      if (m < 1)
      {
         pnodeCurrent = getNewNode(NULL);
         pnodeCurrent->fCost = -cMaximumFloat;
         mOpenList.add(pnodeCurrent);
         m = 1;
      }

      mOpenList.setNumber(m+1);
      mOpenList[m] = pnodeReturn;

      // Up heap..
      pnodeCurrent = mOpenList[m];
      while ((mOpenList[m >> 1]->fCost + mOpenList[m >> 1]->fEstimate) >= (pnodeCurrent->fCost + pnodeCurrent->fEstimate))
      {
         mOpenList[m] = mOpenList[m >> 1];
         m = m >> 1;
      }
      mOpenList[m] = pnodeCurrent;

   }

   #ifdef DEBUG_LRPTIMING_OL
   QueryPerformanceCounter(&endTime);
   delta = (long)(endTime.QuadPart - startTime.QuadPart);
   elapsed = 1000.0f*(float)delta/(float)(mQPFreq.QuadPart);
   mfOLSum += elapsed;

   if (elapsed > mfOLMax)
      mfOLMax = elapsed;
   #endif

   return pnodeReturn;

}


//==============================================================================
// BLrpTree::getOpenNode
//==============================================================================
BLrpPathNode *BLrpTree::getOpenNode()
{
   long lNum = mOpenList.getNumber();
   if (lNum <= 1)
      return NULL;

   BLrpPathNode *pnodeRet = mOpenList[1];

   mOpenList[1] = mOpenList[--lNum];
   BLrpPathNode *pnodeCurrent;

   // DownHeap
   long k = 1;
   long j = 0;
   long limit = lNum >> 1;
   pnodeCurrent = mOpenList[k];
   while (k <= limit)
   {
      j = k + k;     // j is the first child of k, j+1 is the second child of k.
      // We want to compare the smaller of the two children
      if (j < lNum && ((mOpenList[j]->fCost + mOpenList[j]->fEstimate) > (mOpenList[j+1]->fCost + mOpenList[j+1]->fEstimate)))
         ++j;

      // If we're already smaller than the smallest of our two children, we're done.
      if ((pnodeCurrent->fCost+pnodeCurrent->fEstimate) <= (mOpenList[j]->fCost+mOpenList[j]->fEstimate))
         break;

      // Otherise, Exchange me with the smallest of my two children, and continue.
      mOpenList[k] = mOpenList[j];
      k = j;
   }
   mOpenList[k] = pnodeCurrent;
   mOpenList.setNumber(lNum);

   return pnodeRet;

}

//==============================================================================
// BLrpTree::findPath
//==============================================================================
long BLrpTree::findPath(long lEntityID, BVector &vStart, BVector &vGoal, float fRange, BPath *path,
                        long lPlayerID, float fRadius, float fTargetRadius, long lBackPathOption, bool canJump)
{   

   SCOPEDSAMPLE(BLrpTree_findPath);
   long lResult = BPath::cFailed;
   mlIterations = 0;
   mlEntityID = lEntityID;
   mCanJump = canJump;

   if (!mbInitialized)
      return BPath::cError;

   // Timing Stats Init
   #ifdef DEBUG_LRPTIMING_STATS
   ++mlFPCalls;

   LARGE_INTEGER startTime;
   LARGE_INTEGER endTime;
   int64delta;
   float elapsed;
   QueryPerformanceCounter(&startTime);
   // Set addToOpenList and addPathNode Sums to zero.. 
   mfOLSum = 0.0f;
   mfAPSum = 0.0f;
   mlAPCalls = 0;
   #endif

   #ifdef DEBUG_LRPFINDPATH
   debug("-----> findPath");
   debug("Start: (%f, %f)", vStart.x, vStart.z);
   debug("Goal: (%f, %f)", vGoal.x, vGoal.z);
   //debug("NoFixup: %s", (bNoFixup)?"true":"false");
   debug("Range: %f", fRange);
   debug("Radius: %f", fRadius);
   debug("Target Radius: %f", fTargetRadius);
   //long bStartZone = game->getWorld()->getTerrain()->getTerrainZone(vStart);
   //long bEndZone = game->getWorld()->getTerrain()->getTerrainZone(vGoal);
   //debug("Start Zone: %d", bStartZone);
   //debug("End Zone: %d", bEndZone);
   #endif

   // Set current Entity, start & goal & player
   mvStart = vStart;
   mvStart.y = 0.0f;
   mvGoal = vGoal;
   mvGoal.y = 0.0f;
   mlPlayerID = lPlayerID;
   mfRadius = fRadius;
   mfTargetRadius = fTargetRadius;
   mlBackPathOption = lBackPathOption;
   // Always make range at least non zero..
   if (fRange < cFloatCompareEpsilon)
      mfRange = 0.1f;
   else
      mfRange = fRange;

   #ifdef DEBUG_LRPPATHSYNC
   if (mbEnableSync)
   {
      syncPathingCode("findPath");
      syncPathingData("lEntityID", lEntityID);
      syncPathingData("vStart", vStart);
      syncPathingData("vGoal", vGoal);
      syncPathingData("lBackPathOption", lBackPathOption);
      syncPathingData("lPlayerID", lPlayerID);
      syncPathingData("fRadius", fRadius);
      syncPathingData("fTargetRadius", fTargetRadius);
   }
   #endif

   // Convert unit width into number of cells
   float fDiam = (fRadius * 2.0f) * mfRecipCellSize;
   if ((fDiam - (float)((long)fDiam)) > cFloatCompareEpsilon)
      mlUnitWidth = (long)fDiam + 1;
   else
      mlUnitWidth = (long)fDiam;

   // Pick the bucket.. 
   long lBucket = 0;
   while (lBucket < clNumBuckets && mlUnitWidth > clBucketSizes[lBucket])
      lBucket++;

   // If we're pathing a unit bigger than our biggest bucket, make do
   // with our biggest bucket. 
   if (lBucket == clNumBuckets)
      lBucket = clNumBuckets - 1;

   mlBucket = lBucket;

   #ifdef DEBUG_LRPPATHSYNC
   if (mbEnableSync)
   {
      syncPathingData("fDiam", fDiam);
      syncPathingData("mfCellSize", mfCellSize);
      syncPathingData("mlUnitWidth", mlUnitWidth);
      syncPathingData("lBucket", lBucket);
   }
   #endif

   // Clamp Start & Goal
   if ((long)mvStart.x < 0)
      mvStart.x = 0;
   if ((long)mvStart.z < 0)
      mvStart.z = 0;
   if ((long)mvGoal.x < 0)
      mvGoal.x = 0;
   if ((long)mvGoal.z < 0)
      mvGoal.z = 0;

   if ((long)mvStart.x > mlTreeSizeX)
      mvStart.x = (float)mlTreeSizeX;
   if ((long)mvStart.z > mlTreeSizeZ)
      mvStart.z = (float)mlTreeSizeZ;
   if ((long)mvGoal.x > mlTreeSizeX)
      mvGoal.x = (float)mlTreeSizeX;
   if ((long)mvGoal.z > mlTreeSizeZ)
      mvGoal.z = (float)mlTreeSizeZ;

   // Set Target Quad(s)..
   // Expand range by radius and target Radius
   fRange += (mfRadius + mfTargetRadius);


   bool bValidTargets = setTargetQuad(lBucket, mvGoal, fRange, true);

   // If we don't care about the waypoints and we're just interested in the
   // result, (ie., can path), then return failed if we didn't find any valid target quads
   if (mlBackPathOption == BPather::cBackPathOff && !bValidTargets)
   {

      #ifdef DEBUG_LRPTIMING_STATS
      QueryPerformanceCounter(&endTime);
      delta = (long)(endTime.QuadPart - startTime.QuadPart);
      elapsed = 1000.0f*((float)delta/(float)(mQPFreq.QuadPart));
      mfFPSum += elapsed;
      if (elapsed > mfFPMax)
         mfFPMax = elapsed;
      #endif

      #ifdef DEBUG_LRPFINDPATH
      debug("Unable to find ValidTarget quad(s)");
      debug("returning BPath::cFailed");
      #endif
      pathCleanup(false);
      return BPath::cFailed;
   }

   // Clear path.
   path->zeroWaypoints();

   // Init some vars..
   BLrpPathNode *pnodeBest = NULL;
   BLrpPathNode *pnodeCurr = NULL;
   bool bGotToGoal = false;
   float fBestDistSqr = cMaximumFloat;

   // Save the goal node
   long n = 0, x = 0, z = 0;

   // Init the start node..
   BLrpNode *pquadCurr = NULL;
   pquadCurr = findValidStart(mvStart, lBucket, n, x, z);
   if (!pquadCurr)
   {
      #ifdef DEBUG_LRPTIMING_STATS
      QueryPerformanceCounter(&endTime);
      delta = (long)(endTime.QuadPart - startTime.QuadPart);
      elapsed = 1000.0f*((float)delta/(float)(mQPFreq.QuadPart));
      mfFPSum += elapsed;

      if (elapsed > mfFPMax)
         mfFPMax = elapsed;
      #endif

      #ifdef DEBUG_LRPFINDPATH
      debug("Unable to find Start quad");
      debug("returning lResult of : %d", lResult);
      #endif
      pathCleanup(false);
      return lResult;
   }

   // Use the centerpoint of the start quad as the first node, not the
   // start point itself, so that the pather is internally consistent.
   BVector vEstimate(0.0f);
   getQuadEstimate(n, x, z, vEstimate);
   pnodeCurr = addToOpenList(NULL, n, x, z, cDirNone, vEstimate.x, vEstimate.z, 0.0f, true);
   if (!pnodeCurr)
   {
      // jce [10/6/2008] -- now that we reject out of bound points, if the path starts out of bounds,
      // this code path will trigger, so I'm taking out the assert since it's now an expected result.
      //BASSERT(0);

      #ifdef DEBUG_LRPTIMING_STATS
      QueryPerformanceCounter(&endTime);
      delta = (long)(endTime.QuadPart - startTime.QuadPart);
      elapsed = 1000.0f*((float)delta/(float)(mQPFreq.QuadPart));
      mfFPSum += elapsed;

      if (elapsed > mfFPMax)
         mfFPMax = elapsed;
      #endif

      #ifdef DEBUG_LRPFINDPATH
      debug("addToOpenList failed while attempting to put start node on list");
      debug("returning lResult of: %d", lResult);
      #endif

      pathCleanup(false);
      return lResult;
   }

   // get the inuse quad that actually has the goal.
   getInUseQuadWithPoint(mvGoal, lBucket, mGn, mGx, mGz);

   #ifdef DEBUG_LRPFINDPATH
   debug("InUse Quad with Goal is lBucket (n, x, z): %d %d, %d, %d", lBucket, mGn, mGx, mGz);
   #endif

   // Special Case
   // If Start & Goal are in same "in-use quad", then just add them to the path and
   // return full.
   // dlm 8/26/02 - This is insane.  We don't do any "get closest point in quad" if we just return immediately.
   // I have no idea how long this has been doing this, but it ain't right.  
/*   
   if (mGn == n && mGx == x && mGz == z)
   {
      #ifdef DEBUG_LRPFINDPATH
      debug("Start & Goal are in same Quad.. putting them in path and returning BPath::cFull, as they're nothing more to do.");
      debug("<----- findPath");
      #endif
      
      path->addWaypointAtStart(mvGoal);
      path->addWaypointAtStart(mvStart);

      #ifdef DEBUG_LRPTIMING_STATS
      QueryPerformanceCounter(&endTime);
      delta = (long)(endTime.QuadPart - startTime.QuadPart);
      elapsed = 1000.0f*((float)delta/(float)(mQPFreq.QuadPart));
      mfFPSum += elapsed;

      if (elapsed > mfFPMax)
         mfFPMax = elapsed;
      #endif

      pathCleanup(false);
      return BPath::cFull;
      
   }
*/
  /* #ifdef DEBUG_LRPTREE_GRAPHPATHS
   BDebugPrimRender::removeDebugLines("estimates");
   BDebugPrimRender::removeDebugLines("quadpather");
   #endif
*/
   #ifdef DEBUG_LRPFINDPATH
   debug("Beginning iteration loop...");
   #endif

   {
   SCOPEDSAMPLE(BLrpTree_findPath_iterationloop);

   bool bDone = false;
   while (!bDone)
   {
      // Get the next new node.. 
      pnodeCurr = getOpenNode();
      if (!pnodeCurr)
      {
         // We are done.
         bDone = true;
         continue;
      }

      #ifdef DEBUG_LRPFINDPATH
      debug("Current Node: (N X Z Dir) %d %d %d %d  Cost: %f Estimate: %f Total %f", pnodeCurr->sN, pnodeCurr->sX,
         pnodeCurr->sZ, pnodeCurr->sDir, pnodeCurr->fCost, pnodeCurr->fEstimate, pnodeCurr->fCost + pnodeCurr->fEstimate);
      #endif

      #ifdef DEBUG_LRPPATHSYNC
      if (mbEnableSync)
      {
         syncPathingCode("Current Node");
         syncPathingData("sN", pnodeCurr->sN);
         syncPathingData("sX", pnodeCurr->sX);
         syncPathingData("sZ", pnodeCurr->sZ);
         syncPathingData("sDir", pnodeCurr->sDir);
         syncPathingData("fCost", pnodeCurr->fCost);
         syncPathingData("fEstimate", pnodeCurr->fEstimate);
         syncPathingData("fX", pnodeCurr->fX);
         syncPathingData("fZ", pnodeCurr->fZ);
      }
      #endif

      // Put it on the closed list..
      addToClosedList(pnodeCurr);      

      ++mlIterations;
      #ifdef DEBUG_ITERATIONCAP
      // If we're out of iterations, then bail.
      if (mlIterations > clIterationCap)
      {
         bDone = true;
         continue;
      }
      #endif

      // See if we can get to the goal from where we are..
      /*#ifdef DEBUG_LRPTREE_GRAPHPATHS
      if (game->getShowPaths() == 3) 
      {
         if(pnodeCurr->lrpParent)
         {
            BVector v1, v2;
            getQuadEstimate(pnodeCurr->sN, pnodeCurr->sX, pnodeCurr->sZ, v1);
            getQuadEstimate(pnodeCurr->lrpParent->sN, pnodeCurr->lrpParent->sX, pnodeCurr->lrpParent->sZ, v2);
            //game->getWorld()->getTerrain()->addDebugLineOverTerrain("quadpather", 0, );
            gTerrainSimRep.addDebugLineOverTerrain(v2, v1, cDWORDYellow, cDWORDYellow, 0.75f);
         }
      }
      #endif*/

      // Are we at the target?  
      pquadCurr = &mpTree[pnodeCurr->sN][pnodeCurr->sX][pnodeCurr->sZ];

      // Mark us as being in path.
      #ifdef DEBUG_LISTEXCLUSIONS
      markQuadInPath(pnodeCurr->sN, pnodeCurr->sX, pnodeCurr->sZ);
      #else
      pquadCurr->btStatus |= cInPath;
      #endif

      if (pquadCurr->btStatus & cInTarget)
      {
         #ifdef DEBUG_LRPFINDPATH
         debug("This node was in target node.");
         #endif
         // If we have no valid targets, then take the first quad we find in the target
         if (!bValidTargets)
         {
            #ifdef DEBUG_LRPFINDPATH
            debug("They're are no valid targets, so we're using this quad.");
            #endif
            pnodeBest = pnodeCurr;
            bGotToGoal = true;
            bDone = true;
            continue;
         }
         else
         {
            // Only break if this is an unobstructed target quad.
            if (pquadCurr->btStatus & cTargetClear)
            {
               #ifdef DEBUG_LRPFINDPATH
               debug("This is an unobstructed target quad.");
               #endif
               pnodeBest = pnodeCurr;
               bGotToGoal = true;
               bDone = true;
               continue;
            }
         }
      }

      // Is this the bast path so far?
      vEstimate.x = pnodeCurr->fX;
      vEstimate.z = pnodeCurr->fZ;
      float fDistSqr = vEstimate.xzDistanceSqr(mvGoal);
      if (fDistSqr < fBestDistSqr || pnodeBest == NULL)
      {
         #ifdef DEBUG_LRPFINDPATH
         debug("This was best node.  Adding it to list.");
         #endif
         pnodeBest = pnodeCurr;
         fBestDistSqr = fDistSqr;
      }

      // Add it to the exlude list..
/*
      BLrpNodeAddy nodeaddy;
      nodeaddy.n = pnodeCurr->sN;
      nodeaddy.x = pnodeCurr->sX;
      nodeaddy.z = pnodeCurr->sZ;
      mNodeStack.add(nodeaddy);
*/
      #ifdef DEBUG_LRPTIMING_STATS
      LARGE_INTEGER startTimeAP;
      LARGE_INTEGER endTimeAP;
      int64deltaAP;
      float elapsedAP;
      QueryPerformanceCounter(&startTimeAP);
      #endif

      // Check connection in each of four directions..
      if (pquadCurr->btPathingConns[lBucket] & cConnNorth) 
         addPathNode(lBucket, pnodeCurr->sN, pnodeCurr->sX, pnodeCurr->sZ+1, cDirNorth, pnodeCurr);

      if (pquadCurr->btPathingConns[lBucket] & cConnEast)
         addPathNode(lBucket, pnodeCurr->sN, pnodeCurr->sX+1, pnodeCurr->sZ, cDirEast, pnodeCurr);

      if (pquadCurr->btPathingConns[lBucket] & cConnSouth)
         addPathNode(lBucket, pnodeCurr->sN, pnodeCurr->sX, pnodeCurr->sZ-1, cDirSouth, pnodeCurr);

      if (pquadCurr->btPathingConns[lBucket] & cConnWest)
         addPathNode(lBucket, pnodeCurr->sN, pnodeCurr->sX-1, pnodeCurr->sZ, cDirWest, pnodeCurr);

      #ifdef DEBUG_LRPTIMING_STATS
      QueryPerformanceCounter(&endTimeAP);
      deltaAP = (long)(endTimeAP.QuadPart - startTimeAP.QuadPart);
      elapsedAP = 1000.0f*(float)deltaAP/(float)(mQPFreq.QuadPart);
      mfAPSum += elapsedAP;

      if (elapsedAP > mfAPMax)
         mfAPMax = elapsedAP;
      #endif

   } // end of while !bDone
   } // SCOPED SAMPLE THINGY -- don't let the lack of indentation disturb you... much..

   // Fix up Waypoints.. 

   #ifdef DEBUG_LRPFINDPATH
   debug(".");
   debug("Exiting iteration Loop.  Iterations: %d", mlIterations);
   #endif

   #ifdef DEBUG_LRPTIMING_STATS
   QueryPerformanceCounter(&endTime);
   delta = (long)(endTime.QuadPart - startTime.QuadPart);
   elapsed = 1000.0f*((float)delta/(float)(mQPFreq.QuadPart));
   mfFPSum += elapsed;

   if (elapsed > mfFPMax)
      mfFPMax = elapsed;
//   if (mfFPMax > 10.0f)
//      blog("What up?  Path > 10 ms");

   #endif

   #ifdef DEBUG_LRPPATHSYNC
   if (mbEnableSync)
   {
      syncPathingCode("Unfixed up Path...");
      pnodeCurr = pnodeBest;
      while (pnodeCurr)
      {
         syncPathingData("   pnodeCurr->fX", pnodeCurr->fX);
         syncPathingData("   pnodeCurr->fZ", pnodeCurr->fZ);
         pnodeCurr = pnodeCurr->lrpParent;
      }
   }
   #endif

   // Fixup the path, provided we have at least two waypoints, 
   // and we're not just doing a canpath.
   // Determine if we need to fixup the path.
   bool bFixup = false;
   
   if (pnodeBest->lrpParent && (mlBackPathOption != BPather::cBackPathOff))
   {
      if (mlBackPathOption == BPather::cBackPathOn)
         bFixup = true;
      else // Backpath only on full
      {
         if (bValidTargets && bGotToGoal)
            bFixup = true;
      }
   }
#ifdef DEBUG_NOFIXUP
   bFixup = false;
#endif
   if (bFixup)
   {
      fixupWaypoints(lBucket, pnodeBest);
   }

   #ifdef DEBUG_LRPPATHSYNC
   if (mbEnableSync)
   {
      syncPathingCode("Fixed up Path...");
      pnodeCurr = pnodeBest;
      while (pnodeCurr)
      {
         syncPathingData("   pnodeCurr::fX", pnodeCurr->fX);
         syncPathingData("   pnodeCurr::fZ", pnodeCurr->fZ);
         pnodeCurr = pnodeCurr->lrpParent;
      }
   }
   #endif

   // Take what we got, and copy it back to front.
   pnodeCurr = pnodeBest;
   while (pnodeCurr)
   {
      BVector vEstimatePoint(pnodeCurr->fX, 0.0f, pnodeCurr->fZ);
      if (pnodeCurr->lrpParent == NULL)
         path->addWaypointAtStart(mvStart);
      else
         path->addWaypointAtStart(vEstimatePoint);
      pnodeCurr = pnodeCurr->lrpParent;
   }

   // DLMTODO: Merge these code paths from bValidTargets into a single code path.
   if (bValidTargets)
   {
      #ifdef DEBUG_LRPFINDPATH
      debug("Had Valid Targets..");
      #endif
      // Find closest point in quad to goal..
      BVector vClosest(0.0f);
      if (findClosestPointInQuad(lBucket, pnodeBest->sN, pnodeBest->sX, pnodeBest->sZ, mvGoal, mvStart, vClosest) >= 0)
      {
         #ifdef DEBUG_LRPFINDPATH
         debug("Adding Closest Point in Quad (%f, %f)", vClosest.x, vClosest.z);
         #endif
         path->addWaypointAtEnd(vClosest);
      }
      else
      {
         // Special Case: If we only have one waypoint, (the start) and we couldn't find
         // the closest point in the quad, it means we started in a bad quad, and this *is* a target quad.  
         // Add the closest point in this quad.
         if (pnodeBest->lrpParent == NULL)
         {
            long lSegment;
            float fDistSqr;
            BVector vAdjClosest(0.0f);

            getQuadBoundaries(pnodeBest->sN, pnodeBest->sX, pnodeBest->sZ, mStaticPoints[0], mStaticPoints[2]);
            mStaticPoints[1].x = mStaticPoints[0].x;
            mStaticPoints[1].z = mStaticPoints[2].z;
            mStaticPoints[3].x = mStaticPoints[2].x;
            mStaticPoints[3].z = mStaticPoints[0].z;
            mStaticHull.initialize(mStaticPoints, 4, true);
            vClosest = mStaticHull.findClosestPointOnHull(vGoal, &lSegment, &fDistSqr);
            // DLM 2/15/08 - AdjustForRadius never really considered there would be
            // 16 meter radius units.  It's causing push offs that *really* screw up the
            // path.  Turning it off.
            //adjustForRadius(vClosest, vGoal, vAdjClosest);            
            path->addWaypointAtEnd(vClosest);
         }
      }
      if (bGotToGoal)            
      {
         #ifdef DEBUG_LRPFINDPATH
         debug("Got to Goal.. returning full path.");
         #endif
         lResult = BPath::cFull;
      }
      else
      {
         // See if the closest point is at least one tile away from the starting point.  If so, partial.  If not, failed.
         if ((long)(vClosest.x * mfRecipCellSize) != (long)(mvStart.x * mfRecipCellSize) || 
             (long)(vClosest.z * mfRecipCellSize) != (long)(mvStart.z * mfRecipCellSize))
            lResult = BPath::cPartial;
         else
            lResult = BPath::cFailed;
      }

      // Do the lrp backpathing thing..
      #ifndef DEBUG_NOBACKPATH
      if (bFixup && lResult != BPath::cFailed)
         backPath(lBucket, path);
      #endif

   }
   else
   {
      #ifdef DEBUG_LRPFINDPATH
      debug("Had No Valid Targets..");
      #endif

      // add the closest point to the goal from our quad.
      BVector vClosest(0.0f);
      if (findClosestPointInQuad(lBucket, pnodeBest->sN, pnodeBest->sX, pnodeBest->sZ, mvGoal, mvStart, vClosest) >= 0)
      {
         #ifdef DEBUG_LRPFINDPATH
         debug("Adding Closest Point in Quad: (%f, %f)", vClosest.x, vClosest.z);
         #endif
         // If we got to goal, replace the last waypoint with this one.  Otherwise, just add this one. 
         // DLM - If we did not have valid targets, then in all cases (regardless of whether we got to goal or not
         // I think we want to replace the estimate location in the last quad with the "closest point in quad to goal" location.
         //if (bGotToGoal && pnodeBest->lrpParent)
         if (pnodeBest->lrpParent)
         {
            #ifdef DEBUG_LRPFINDPATH
            debug("Removing last waypoint and replacing it with closest one..");
            #endif
            path->removeWaypoint(path->getNumberWaypoints() - 1);
         }
         path->addWaypointAtEnd(vClosest);
      }
      else
      {
         // Special Case: If we only have one waypoint, (the start) and we couldn't find
         // the closest point in the quad, it means we started in a bad quad, and this *is* a target quad.  
         // Add the closest point in this quad.
         if (pnodeBest->lrpParent == NULL)
         {
            long lSegment;
            float fDistSqr;
            BVector vAdjClosest(0.0f);

            getQuadBoundaries(pnodeBest->sN, pnodeBest->sX, pnodeBest->sZ, mStaticPoints[0], mStaticPoints[2]);
            mStaticPoints[1].x = mStaticPoints[0].x;
            mStaticPoints[1].z = mStaticPoints[2].z;
            mStaticPoints[3].x = mStaticPoints[2].x;
            mStaticPoints[3].z = mStaticPoints[0].z;
            mStaticHull.initialize(mStaticPoints, 4, true);
            vClosest = mStaticHull.findClosestPointOnHull(vGoal, &lSegment, &fDistSqr);
            // DLM 2/15/08 - AdjustForRadius never really considered there would be
            // 16 meter radius units.  It's causing push offs that *really* screw up the
            // path.  Turning it off.
            //adjustForRadius(vClosest, vGoal, vAdjClosest);            
            path->addWaypointAtEnd(vClosest);
         }
      }
      
      // See if the closest point is at least one tile away from the starting point.  If so, partial.  If not, failed.
      if ((long)(vClosest.x * mfRecipCellSize) != (long)(mvStart.x * mfRecipCellSize) || 
          (long)(vClosest.z * mfRecipCellSize) != (long)(mvStart.z * mfRecipCellSize))
         lResult = BPath::cPartial;
      else
         lResult = BPath::cFailed;

      // Do the lrp backpathing thing..
      #ifndef DEBUG_NOBACKPATH
      if (bFixup && lResult != BPath::cFailed)
         backPath(lBucket, path);
      #endif

   }

   // Little Syncotron.
   #ifdef DEBUG_LRPPATHSYNC
   if (mbEnableSync)
   {
      syncPathingCode("Returning from lrpTree::findPath");
      syncPathingData("Result:", lResult);
      syncPathingData("Number of Waypoints in Path", path->getNumberWaypoints());
      BVector vWaypoint(0.0f);
      for (long m = 0; m < path->getNumberWaypoints(); m++)
      {
         vWaypoint = path->getWaypoint(m);
         syncPathingData("   Waypoint", vWaypoint);
      }
   }
   #endif
/*
   #ifdef DEBUG_LRPFINDPATH
   debug("Returning from lrpTree::findPath");
   debug("Result: %d", lResult);
   debug("Number of Waypoints in Path %d", path->getNumberWaypoints());
   BVector vWaypoint(0.0f);
   for (long m = 0; m < path->getNumberWaypoints(); m++)
   {
      vWaypoint = path->getWaypoint(m);
      debug("   Waypoint: (%f, %f)", vWaypoint.x, vWaypoint.z);
   }
   #endif
*/
   #ifdef DEBUG_LRPTREE_GRAPHPATHS   
   // If we're debugging paths, then stuff our path into the debugpath for rendering
   mDebugPath.zeroWaypoints();
   for (long l = 0; l < path->getNumberWaypoints() - 1; l++)
   {
      //game->getWorld()->getTerrain()->addDebugLineOverTerrain("quadpather", 0, path->getWaypoint(l), path->getWaypoint(l+1), cDWORDWhite, cDWORDWhite, 1.0f);
      //gTerrainSimRep.addDebugLineOverTerrain(path->getWaypoint(l), path->getWaypoint(l+1), cDWORDWhite, cDWORDWhite, 1.0f);
      mDebugPath.addWaypointAtEnd(path->getWaypoint(l));
   }
   mDebugPath.addWaypointAtEnd(path->getWaypoint(l));
   #endif



   // Cleanup the path..
   pathCleanup(true);



   #ifdef DEBUG_LRPFINDPATH
   debug("Returning: %d", lResult);
   debug("<----- findPath");
   #endif
   
   return lResult;

}


//==============================================================================
// BLrpTree::pathCleanup
// Housecleaning chores to do before you leave pathing.. 
//==============================================================================
void BLrpTree::pathCleanup(bool bDoExclusions)
{
   // Clear Target Quads
   
   // jce [8/24/2004] -- old way, redo the math to find what nodes we need to un-tag. 
   //float fRange = mfRange + (mfRadius + mfTargetRadius);
   //setTargetQuad(mlBucket, mvGoal, fRange, false);

   // jce [8/24/2004] -- New version caches nodes and clears them without doing math. 
   for(long i=0; i<mTargetQuadList.getNumber(); i++)
      mTargetQuadList[i]->btStatus &= ~(cInTarget | cTargetClear);
   mTargetQuadList.setNumber(0);
  
   if (bDoExclusions)
   {
    #ifndef DEBUG_LISTEXCLUSIONS
      for (long x = 0; x < mlTerrainSizeX[cMaxLevel]; x++)
         for (long z = 0; z < mlTerrainSizeZ[cMaxLevel]; z++)
            excludeQuad(mlBucket, cMaxLevel, x, z, false);
   #else
   mbaExclusions.clear();
   #endif
   }

   // Reset the lists..
   BLrpNode *pquadCurr;
   for (long l = 0; l < mOpenList.getNumber(); l++)
   {
      pquadCurr = &mpTree[mOpenList[l]->sN][mOpenList[l]->sX][mOpenList[l]->sZ];
      pquadCurr->btStatus &= ~cInQueue;
      mNodeFreeList.release(mOpenList[l]);
   }
   for (long l = 0; l < mClosedList.getNumber(); l++)
   {
      pquadCurr = &mpTree[mClosedList[l]->sN][mClosedList[l]->sX][mClosedList[l]->sZ];
      pquadCurr->btStatus &= ~cInQueue;
      mNodeFreeList.release(mClosedList[l]);
   }

   mOpenList.setNumber(0);
   mClosedList.setNumber(0);

   return;
}


//==============================================================================
// BLrpTree::addPathNode
//==============================================================================
void BLrpTree::addPathNode(long lBucket, long n, long x, long z, long lDir, BLrpPathNode *lrpParent)
{

   SCOPEDSAMPLE(BLrpTree_addPathNode);
   #ifdef DEBUG_LRPTIMING_STATS
   ++mlAPCalls;
   #endif

   static BVector vEstimate;

   // If I'm invalid.. bail.
   BLrpNode *pquad = &mpTree[n][x][z];

   #ifdef DEBUG_LRPPATHSYNC
   if (mbEnableSync)
   {
      syncPathingCode("addPathNode");
      syncPathingData("lBucket", lBucket);
      syncPathingData("n", n);
      syncPathingData("x", x);
      syncPathingData("z", z);
      syncPathingData("lDir", lDir);
      syncPathingData("pquad->btPassability[lBucket]", pquad->btPassability[lBucket]);
      syncPathingData("pquad->btPathingConns[lBucket]", pquad->btPathingConns[lBucket]);
      syncPathingData("pquad->btStatus", pquad->btStatus);
      syncPathingData("pquad->lPlayerMask", pquad->lPlayerMask);
   }
   #endif

   if ((pquad->btPassability[lBucket] & cAllInvalid) == cAllInvalid)
      return;

   #ifdef DEBUG_LISTEXCLUSIONS
   
   {
      SCOPEDSAMPLE(BLrpTree_addPathNode_checkQuadInPath);
      if (checkQuadInPath(n, x, z))
         return;
   }
   #else
   if (pquad->btStatus & cInPath)
      return;
   #endif

   if (pquad->btPassability[lBucket] & cInUse)
   {
      // If it's a target quad, allow it to go in, so it can be popped off and 
      // complete the path.
      if (pquad->btPassability[lBucket] & cPlayerQuad) 
         /*&& ((pquad->btStatus & cInTarget) == 0)) */
      {
#ifdef CANJUMP_CHANGE
         if( !mCanJump )
#else
         long lPlayerMask = (1 << (mlPlayerID - 1));
         if ((pquad->lPlayerMask & lPlayerMask) != lPlayerMask)
#endif
         {

            #ifdef DEBUG_LRPPATHSYNC
            if (mbEnableSync)
               syncPathingCode("failed to add node because it belongs to another player.");
            #endif

            #ifdef DEBUG_ADDPATHNODE
            debug("Failed to add node (%d, %d, %d, %d) because it belongs to another player.",
               n, x, z, lDir);
            #endif

            return;
         }
      }
      // If our parent was the start node.. ie., it's cost was zero.. then
      // calc distance from this node to the start *point* and add it as an additional
      // cost to the node.  That will weight quads that are closer to the start point
      // a little heavier.  
      float fAddCost = 0.0f;
      if (lrpParent->fCost < cFloatCompareEpsilon)
         fAddCost = this->getDistanceToQuad(n, x, z, mvStart, vEstimate);

      BLrpPathNode *pnode;  
      if (n == mGn && x == mGx && z == mGz)
      {
         SCOPEDSAMPLE(BLrpTree_addPathNode_addtoopenlist);
         pnode = addToOpenList(lrpParent, n, x, z, lDir, mvGoal.x, mvGoal.z, fAddCost);
      }
      else
      {
         SCOPEDSAMPLE(BLrpTree_addPathNode_getQEst);
         getQuadEstimate(n, x, z, vEstimate);
         pnode = addToOpenList(lrpParent, n, x, z, lDir, vEstimate.x, vEstimate.z, fAddCost);
      }

      #ifdef DEBUG_LRPPATHSYNC
      if (mbEnableSync)
      {
         if (pnode)
         {
            syncPathingCode("Just added pnode");
            syncPathingData("sN", pnode->sN);
            syncPathingData("sX", pnode->sX);
            syncPathingData("sZ", pnode->sZ);
            syncPathingData("sDir", pnode->sDir);
            syncPathingData("fCost", pnode->fCost);
            syncPathingData("fEstimate", pnode->fEstimate);
            syncPathingData("fX", pnode->fX);
            syncPathingData("fZ", pnode->fZ);
         }
         else
            syncPathingCode("Attempted to add node but failed...");
      }
      #endif

      if (pnode)
         pquad->btStatus |= cInQueue;
      #ifdef DEBUG_ADDPATHNODE
      if (pnode)
         debug("Node Added (n, x, z, dir): %d %d %d %d", pnode->sN, pnode->sX, pnode->sZ, pnode->sDir);
      else
         debug("Failed to add node (n, x, z, dir): %d %d %d %d", n, x, z, lDir);
      #endif
      return;
   }
   else if (pquad->btPassability[lBucket] & cBroken)
   {
      // Add appropriate children, based on direction..
      long m = n-1;
      long x1 = x << 1;
      long z1 = z << 1;
      long x2 = x1 + 1;
      long z2 = z1 + 1;
      switch(lDir)
      {
         case cDirNorth:
            if (mpTree[m][x1][z1].btPathingConns[lBucket] & cConnSouth)
               addPathNode(lBucket, m, x1, z1, lDir, lrpParent);
            if (mpTree[m][x2][z1].btPathingConns[lBucket] & cConnSouth)
               addPathNode(lBucket, m, x2, z1, lDir, lrpParent);
            break;
         case cDirEast:
            if (mpTree[m][x1][z1].btPathingConns[lBucket] & cConnWest)
               addPathNode(lBucket, m, x1, z1, lDir, lrpParent);
            if (mpTree[m][x1][z2].btPathingConns[lBucket] & cConnWest)
               addPathNode(lBucket, m, x1, z2, lDir, lrpParent);
            break;
         case cDirSouth:
            if (mpTree[m][x1][z2].btPathingConns[lBucket] & cConnNorth)
               addPathNode(lBucket, m, x1, z2, lDir, lrpParent);
            if (mpTree[m][x2][z2].btPathingConns[lBucket] & cConnNorth)
               addPathNode(lBucket, m, x2, z2, lDir, lrpParent);
            break;
         case cDirWest:
            if (mpTree[m][x2][z2].btPathingConns[lBucket] & cConnEast)
               addPathNode(lBucket, m, x2, z2, lDir, lrpParent);
            if (mpTree[m][x2][z1].btPathingConns[lBucket] & cConnEast)
               addPathNode(lBucket, m, x2, z1, lDir, lrpParent);
            break;
      }
   }
   else
   {
      // Add the parent..
      long m = n+1;
      long x1 = x >> 1;
      long z1 = z >> 1;
      addPathNode(lBucket, m, x1, z1, lDir, lrpParent);
   }

   return;
 
}

//==============================================================================
// BLrpTree::setTargetQuad
// Returns true if at least one target cell was found that had connections,
// false if not.
//==============================================================================
bool BLrpTree::setTargetQuad(long lBucket, BVector &vTarget, float fRange, bool bSet)
{


   #ifdef DEBUG_SETTARGETQUAD
   debug("-----> setTargetQuad");
   debug("vTarget: (%f, %f)", vTarget.x, vTarget.z);
   debug("fRange: %f", fRange);
   debug("bSet: %s", (bSet)?"true":"false");
   #endif
   
/*
   #ifdef DEBUG_LRPTREE_GRAPHPATHS
   if (bSet)
      BDebugPrimRender::removeDebugLines("targets"); //DBJFIXME
   #endif
*/
   long x = (long)(vTarget.x * mfRecipCellSize);
   if (x >= mlSizeX[0])
      x = mlSizeX[0] - 1;
   long z = (long)(vTarget.z * mfRecipCellSize);
   if (z >= mlSizeZ[0])
      z = mlSizeZ[0] - 1;


   // Set TargetCell Parms..
   mlTargetCellX = x;
   mlTargetCellZ = z;

   /*
   // Convert the range to use the hypotenuse of the side, versus the side.  This errs on sometimes
   // getting us closer than we have to be, but it's better than leaving us too far away.
   float fCellRadius = fRange * mfRecipCellSize + 1.0f;
   float fCellRadiusSqr = fCellRadius * fCellRadius;
   float fCellRangeSqr = fCellRadiusSqr / 2.0f;
   float fCellRange = (float)sqrt(fCellRangeSqr);
   long lCellRange = (long)(fCellRange + 0.5f);
   */

   // jce [8/26/2004] -- Since the above code sometimes fails to get full paths and the 
   // real in range check is performed on every tile anyway, go back to using the complete
   // target area.
   long lCellRange = (long)((fRange * mfRecipCellSize) + 1.0f);


   if (lCellRange < 0)
      lCellRange = 0;

   #ifdef DEBUG_SETTARGETQUAD
   debug("Cell Range: %d", lCellRange);
   #endif

   // Get the pathing entity..
   BPather *pPather = &gPather;
   if (!pPather)
   {
      BASSERT(0);
      return false;
   }
   const BEntity *pEntity = pPather->getPathingEntity();
   const BUnit *pTarget = pPather->getPathingTarget();
   BVector vPos;

   bool bValidTargetFound = false;


   // Set ranges, and clamp.
   long lMinX = x - lCellRange;
   if (lMinX < 0)
      lMinX = 0;
   if (lMinX > mlSizeX[0] - 1)
      lMinX = mlSizeX[0] - 1;
   long lMinZ = z - lCellRange;
   if (lMinZ < 0)
      lMinZ = 0;
   if (lMinZ > mlSizeZ[0] - 1)
      lMinZ = mlSizeZ[0] - 1;
   long lMaxX = x + lCellRange;
   if (lMaxX < 0)
      lMaxX = 0;
   if (lMaxX > mlSizeX[0] - 1)
      lMaxX = mlSizeX[0] - 1;
   long lMaxZ = z + lCellRange;
   if (lMaxZ < 0)
      lMaxX = 0;
   if (lMaxZ > mlSizeZ[0] - 1)
      lMaxZ = mlSizeZ[0] - 1;

   #ifdef DEBUG_SETTARGETQUAD
   debug("lMinX, lMinZ: (%d, %d)", lMinX, lMinZ);
   debug("lMaxX, lMaxZ: (%d, %d)", lMaxX, lMaxZ);
   #endif

#ifdef DEBUG_RENDER_TARGET_QUADS
   bool bRender = true;
#endif

   for (z = lMinZ; z <= lMaxZ; z++)
      for (x = lMinX; x <= lMaxX; x++)
      {
         // Find the first inUse quad up the tree.. 
         long n = 0;
         long r = z;
         long c = x;
         BLrpNode *pNode = &mpTree[n][c][r];

         bool bObstructed = ((pNode->btPassability[lBucket] & cAllInvalid) == cAllInvalid);
#ifdef DEBUG_SETTARGETQUAD
         debug("Cell %d %d %d is %s", n, c, r, (bObstructed)?"obstructed":"clear");
#endif
         //-- early out
         if (bObstructed && bSet)
         {
            continue;
         }

         getClosestPointInCell(c, r, mvGoal, vPos);

         if (pTarget)
         {
            if (pEntity)
            {
               if (pEntity->calculateXZDistance(vPos, pTarget) > mfRange)
                  continue;
            }
            else
            {
               // fRange has already been adjusted by the pathing unit's
               // radius and the target radius.
               float dist = vPos.xzDistance(mvGoal);
               if (dist > fRange)
                  continue;
            }

         }
         else
         {
            if (pEntity)
            {
               if (pEntity->calculateXZDistance(vPos, mvGoal) > mfRange)
                  continue;
            }
            else
            {
               // fRange has already been adjusted by the pathing unit's
               // radius and the target radius.
               if (vPos.xzDistance(mvGoal) > fRange)
                  continue;
            }
         }
         // Mark those quads that are in the target range as InTarget.  Additionally,
         // mark those that actually have a valid connection into them as "TargetClear".
        

         // Before you walk up the tree.. mark the low level quad you're looking at.
         // Use this in render info..
         if (bSet)
         {
            pNode->btStatus |= cInTarget;
            mTargetQuadList.add(pNode);
#ifdef DEBUG_RENDER_TARGET_QUADS    
            if (bRender)
            {
               gTerrainSimRep.addDebugPointOverTerrain(vPos, 2.0f, cDWORDBlack, 2.0f, BDebugPrimitives::cCategoryPathing);
            }
#endif
         }
         else
            pNode->btStatus &= ~cInTarget;

         while ((pNode->btPassability[lBucket] & cInUse) == 0 && n < cMaxDepth - 1)
         {
            n++;
            r >>= 1;
            c >>= 1;
            pNode = &mpTree[n][c][r];
         }
         #ifdef DEBUG_SETTARGETQUAD
         debug("Setting: %d %d %d", n, c, r);
         #endif
         if (bSet)
         {               
            pNode->btStatus |= cInTarget;
            mTargetQuadList.add(pNode);

            if (!bObstructed)
            {
               pNode->btStatus |= cTargetClear;
               #ifdef DEBUG_SETTARGETQUAD
               debug("Set this quad to TargetClear.");
               #endif
               bValidTargetFound = true;
            }
/*
            #ifdef DEBUG_LRPTREE_GRAPHPATHS
            if (game->getShowPaths() == 3)
            {
               BVector vMid;
               getQuadEstimate(n, c, r, vMid);
               debugAddPoint("targets", vMid, cColorBlack);
            }
            #endif
*/
         }
         else
         {
            pNode->btStatus &= ~(cInTarget | cTargetClear);
         }
      }

   #ifdef DEBUG_SETTARGETQUAD
   debug("returning %s",(bValidTargetFound)?"true":"false");
   debug("<---- setTargetQuad");
   #endif
   

   return bValidTargetFound;
   
}

//==============================================================================
// BLrpTree::excludeArea
// Goes through the specified area, and marks the specified quads as "un-useable"
// by setting the "inPath" bit on them.  
// false if not.
//==============================================================================
void BLrpTree::excludeArea(long lBucket, const BVector &vMin, const BVector &vMax, bool bSet)
{

   float fQuadSize = (1 << cMaxLevel) * mfCellSize;
   
   long x1 = (long)(vMin.x / fQuadSize);
   if (x1 >= mlSizeX[0])
      x1 = mlSizeX[0] - 1;
   long z1 = (long)(vMin.z / fQuadSize);
   if (z1 >= mlSizeZ[0])
      z1 = mlSizeZ[0] - 1;

   long x2 = (long)(vMax.x / fQuadSize);
   if (x2 >= mlSizeX[0])
      x2 = mlSizeX[0] - 1;
   long z2 = (long)(vMax.z / fQuadSize);
   if (z2 >= mlSizeZ[0])
      z2 = mlSizeZ[0] - 1;

   for (long x = x1; x <= x2; x1++)
      for (long z = z1; z <= z2; z1)
         excludeQuad(lBucket, cMaxLevel, x, z, bSet);

   return;

}
      
//==============================================================================
// BLrpTree::excludeQuad
//==============================================================================
#ifdef DEBUG_RECURSIVEEXCLUDE
void BLrpTree::excludeQuad(long lBucket, long n, long x, long z, bool bSet)
{

   BLrpNode *pnode = &mpTree[n][x][z];
   if (pnode->btPassability[lBucket] & cInUse)
   {
      if (bSet)
         if ((pnode->btStatus & cInPath) == 0)
         {
            pnode->btStatus |= cInPath;
            return;
         }
         else
            return;
      else 
         if (pnode->btStatus & cInPath)
         {                  
            pnode->btStatus &= ~cInPath;
            return;
         }
         else
            return;
   }
   else if (n > 0)
   {
      long m = n - 1;
      long x1 = x << 1;
      long z1 = z << 1;
      long x2 = x1+1;
      long z2 = z1+1;
      excludeQuad(lBucket, m, x1, z1, bSet);
      excludeQuad(lBucket, m, x1, z2, bSet);
      excludeQuad(lBucket, m, x2, z2, bSet);
      excludeQuad(lBucket, m, x2, z1, bSet);
   }
   return;

}
#else
void BLrpTree::excludeQuad(long lBucket, long n, long x, long z, bool bSet)
{

   BLrpNode *pnode = &mpTree[n][x][z];
   BLrpNodeAddy addy;
   addy.n = n;
   addy.x = x;
   addy.z = z;

   long lStackIdx = 0;
   mNodeStack[lStackIdx++] = addy;
   bool bDone = false;
   long x1 = x;
   long z1 = z;
   long x2 = 0;
   long z2 = 0;
   while (!bDone)
   {
      addy = mNodeStack[--lStackIdx];
      pnode = &mpTree[addy.n][addy.x][addy.z];
      if (pnode->btPassability[lBucket] & cInUse)
      {
         if (bSet && (pnode->btStatus & cInPath) == 0)
            pnode->btStatus |= cInPath;
         else if (!bSet && (pnode->btStatus & cInPath))
            pnode->btStatus &= ~cInPath;
      }
      else if (addy.n > 0)
      {
         x1 = addy.x << 1;
         z1 = addy.z << 1;
         x2 = x1+1;
         z2 = z1+1;
         --addy.n;
         addy.x = x1;
         addy.z = z1;
         if (lStackIdx < clNodeStackInitSize)
            mNodeStack[lStackIdx++] = addy;
         addy.z = z2;
         if (lStackIdx < clNodeStackInitSize)
            mNodeStack[lStackIdx++] = addy;
         addy.x = x2;
         if (lStackIdx < clNodeStackInitSize)
            mNodeStack[lStackIdx++] = addy;
         addy.z = z1;
         if (lStackIdx < clNodeStackInitSize)
            mNodeStack[lStackIdx++] = addy;
         if (lStackIdx >= clNodeStackInitSize)
         {
            BFAIL("BLrpTree::excludeQuad -- Overflowed the stack while adding nodes to exclude from the path.");            
         }
      }
      if (lStackIdx == 0)
         bDone = true;
   }
   return;

  
}

#endif

//==============================================================================
// BLrpTree::findClosestValidQuad
//==============================================================================
bool BLrpTree::findClosestValidQuad(long lBucket, long n, long x, long z, long &n1, long &x1, long &z1)
{

   /*#ifdef DEBUG_LRPTREE_GRAPHPATHS
   BDebugPrimRender::removeDebugLines("targets");
   #endif*/

   BLrpNode *pnode = &mpTree[n][x][z];
   if ((pnode->btPathingConns[lBucket] & cConnAll) == cConnNone)
   {
      for (long lIdx = 0; lIdx < 8; lIdx ++)
      {

         long x2 = x + lAdj[lIdx][0];
         long z2 = z + lAdj[lIdx][1];
         if (x2 < 0 || x2 > mlSizeX[n] - 1)
            continue;
         if (z2 < 0 || z2 > mlSizeZ[n] - 1)
            continue;
         if (checkValidQuad(lBucket, n, x2, z2, n1, x1, z1))
            return true;
      }
      return false;
   }

   n1 = n;
   x1 = x;
   z1 = z;

   return true;

}

//==============================================================================
// BLrpTree::checkValidQuad
//==============================================================================
bool BLrpTree::checkValidQuad(long lBucket, long n, long x, long z, long &lNewN, long &lNewX, long &lNewZ)
{
   BLrpNode *pnode = &mpTree[n][x][z];
   if (pnode->btPassability[lBucket] & cInUse)
   {
      if ((pnode->btPathingConns[lBucket] & cConnAll) == cConnNone)
         return false;
      else
      {
         lNewN = n;
         lNewX = x;
         lNewZ = z;
         /*#ifdef DEBUG_LRPTREE_GRAPHPATHS
         if (game->getShowPaths() == 3) 
         {
            BVector vMid;
            getQuadEstimate(n, x, z, vMid);
            debugAddPoint("targets", vMid, cDWORDBlue);
         }
         #endif*/
         return true;
      }
   }
   else if (pnode->btPassability[lBucket] & cBroken)
   {
      long m = n-1;
      long x1 = x << 1;
      long z1 = z << 1;
      long x2 = x1 + 1;
      long z2 = z1 + 1;
      if (checkValidQuad(lBucket, m, x1, z1, lNewN, lNewX, lNewZ))
         return true;
      if (checkValidQuad(lBucket, m, x1, z2, lNewN, lNewX, lNewZ))
         return true;
      if (checkValidQuad(lBucket, m, x2, z2, lNewN, lNewX, lNewZ))
         return true;
      if (checkValidQuad(lBucket, m, x2, z1, lNewN, lNewX, lNewZ))
         return true;
   }
   else
   {
      // Check the parent
      long m = n+1;
      long x1 = x >> 1;
      long z1 = z >> 1;
      if (checkValidQuad(lBucket, m, x1, z1, lNewN, lNewX, lNewZ))
         return true;
   }
   return false;
   
}



//==============================================================================
// BLrpTree::getInUseQuadWithPoint
//==============================================================================
BLrpNode *BLrpTree::getInUseQuadWithPoint(const BVector &vPoint, long lBucket, long &n, long &x,
                                     long &z)
{

   x = (long)(vPoint.x * mfRecipCellSize);
   if (x >= mlSizeX[0])
      x = mlSizeX[0] - 1;
   z = (long)(vPoint.z * mfRecipCellSize);
   if (z >= mlSizeZ[0])
      z = mlSizeZ[0] - 1;
   
   // Find the first inUse quad up the tree.. 
   n = 0;
   BLrpNode *pNode = &mpTree[n][x][z];
   while ((pNode->btPassability[lBucket] & cInUse) == 0 && n < cMaxDepth - 1)
   {
      n++;
      z >>= 1;
      x >>= 1;
      pNode = &mpTree[n][x][z];
   }

   return pNode;

}

//==============================================================================
// BLrpTree::getQuadEstimate
//==============================================================================
void BLrpTree::getQuadEstimate(long n, long x, long z, BVector &vEstimate)
{
   // Basically returns the midpoint of the quad.
   BVector vMin, vMax;
   getQuadBoundaries(n, x, z, vMin, vMax);
   vEstimate.x = vMin.x + ((vMax.x - vMin.x) / 2.0f);
   vEstimate.y = 0.0f;
   vEstimate.z = vMin.z + ((vMax.z - vMin.z) / 2.0f);

   return;
}

//==============================================================================
// BLrpTree::fixupWaypoints
//==============================================================================
void BLrpTree::fixupWaypoints(long lBucket, BLrpPathNode *pnodePath)
{

   #ifdef DEBUG_FIXUP_WAYPOINTS
   debug("----> fixupWaypoints");
   #endif

   BLrpPathNode *pnodeCurr = pnodePath;
   if (!pnodeCurr || !pnodeCurr->lrpParent)
   {
      return;
   }

   BVector vWaypoint(0.0f), vLastWaypoint(0.0f), vNextWaypoint(0.0f);
   pnodeCurr = pnodePath;
   vLastWaypoint = mvGoal;
   while (pnodeCurr->lrpParent)
   {  
      if ((pnodeCurr->lrpParent)->lrpParent)
         estimateWaypoint(pnodeCurr->lrpParent, vNextWaypoint);
      else
         vNextWaypoint = mvStart;
      //vNextWaypoint.x = pnodeCurr->lrpParent->fX;
      //vNextWaypoint.z = pnodeCurr->lrpParent->fZ;

      if (!findWaypoint(lBucket, pnodeCurr, vLastWaypoint, vWaypoint, vNextWaypoint))
      {
         #ifdef DEBUG_FIXUP_WAYPOINTS
         debug("Unable to findWaypoint, using Estimate instead.");
         #endif
         estimateWaypoint(pnodeCurr, vWaypoint);

         #ifdef DEBUG_LRPPATHSYNC
         if (mbEnableSync)
         {
            syncPathingCode("Unable to findWaypoint, using Estimate instead.");
            syncPathingData("estimateWaypoint", vWaypoint);
         }
         #endif

      }
      #ifdef DEBUG_FIXUP_WAYPOINTS
      debug("Waypoint returned: (%f, %f)", vWaypoint.x, vWaypoint.z);
      #endif

      #ifdef DEBUG_LRPPATHSYNC
      if (mbEnableSync)
         syncPathingData("findWaypoint", vWaypoint);
      #endif

      pnodeCurr->fX = vWaypoint.x;
      pnodeCurr->fZ = vWaypoint.z;

      vLastWaypoint = vWaypoint;
      pnodeCurr = pnodeCurr->lrpParent;
   }

   // Put the start point in the last node..
   pnodeCurr->fX = mvStart.x;
   pnodeCurr->fZ = mvStart.z;

   #ifdef DEBUG_FIXUP_WAYPOINTS
   debug("<---- fixupWaypoints");      
   #endif

   return;
}


//==============================================================================
// BLrpTree::estimateWaypoint
//==============================================================================
void BLrpTree::estimateWaypoint(BLrpPathNode *pnode, BVector &vWaypoint)
{

   #ifdef DEBUG_ESTIMATE_WAYPOINT
   debug("----> estimateWaypoint");
   #endif

   if (!pnode)
      return;

   if (!pnode->lrpParent)
      return;

   // Determine Limiting Segment..
   BLrpPathNode *pnodeLimit = NULL;
   BLrpPathNode *pnodeParent = pnode->lrpParent;
   if (pnode->sN < pnodeParent->sN)
      pnodeLimit = pnode;
   else
      pnodeLimit = pnodeParent;

   long lLimitQuadSize = 1 << pnodeLimit->sN;
   long lQuadSize = 1 << pnode->sN; 
   long lXStart = -1, lXEnd = -1, lXCurr = -1;
   long lZStart = -1, lZEnd = -1, lZCurr = -1;

   if (pnode->sDir == cDirNorth || pnode->sDir == cDirSouth)
   {
      lXStart = pnodeLimit->sX << pnodeLimit->sN;
      lXEnd = lXStart + lLimitQuadSize; 
      lXCurr = lXStart + (lLimitQuadSize >> 1);
      if (pnode->sDir == cDirNorth)
      {
         lZCurr = pnode->sZ << pnode->sN;
      }
      else
      {
         lZCurr = (pnode->sZ << pnode->sN) + lQuadSize;
      }
   }
   else
   {
      lZStart = pnodeLimit->sZ << pnodeLimit->sN;
      lZEnd = lZStart + lLimitQuadSize;
      lZCurr = lZStart + (lLimitQuadSize >> 1);
      if (pnode->sDir == cDirEast)
      {
         lXCurr = pnode->sX << pnode->sN;
      }
      else
      {
         lXCurr = (pnode->sX << pnode->sN) + lQuadSize;
      }
   }

   vWaypoint.x = lXCurr * mfCellSize;
   vWaypoint.z = lZCurr * mfCellSize;

   /*#ifdef DEBUG_LRPTREE_GRAPHPATHS
   if (game->getShowPaths() == 3) 
   {
      debugAddPoint("estimates", vWaypoint, cDWORDCyan);
   }
   #endif*/

   #ifdef DEBUG_ESTIMATE_WAYPOINT
   debug("----> estimateWaypoint");
   #endif

   return;

}



//==============================================================================
// BLrpTree::findWaypoint
// Given three estimatedwaypoints.. the last waypoint, the current one, and the next 
// one, this routine will attempt to slide the current waypoint in the appropriate
// direction as far as it can to bring it inline with the previous and the next
// waypoints.
// NOTE: The estimate/actual waypoint for a particular quad is the point at which
// the path enters that quad.
//==============================================================================
bool BLrpTree::findWaypoint(long lBucket, BLrpPathNode *pnode, BVector &vLastWaypoint, BVector &vWaypoint, BVector &vNextWaypoint)
{
   #ifdef DEBUG_FIND_WAYPOINT
   debug("----> findWaypoint");
   #endif

   if (!pnode)
      return false;

   if (!pnode->lrpParent)
      return false;

   // Determine Limiting Segment..
   BLrpPathNode *pnodeLimit = NULL;
   BLrpPathNode *pnodeParent = pnode->lrpParent;
   if (pnode->sN < pnodeParent->sN)
      pnodeLimit = pnode;
   else
      pnodeLimit = pnodeParent;

   #ifdef DEBUG_FIND_WAYPOINT
   debug("Finding Waypoint for node (N, X, Z, Dir): %d, %d, %d, %d, Bucket %d", pnode->sN, pnode->sX, pnode->sZ, pnode->sDir, lBucket);
   #endif

   long lLimitQuadSize = 1 << pnodeLimit->sN;
   long lQuadSize = 1 << pnode->sN; 
   long lXStart = -1, lXEnd = -1, lXCurr = -1;
   long lZStart = -1, lZEnd = -1, lZCurr = -1;
   float fDeltaX = vNextWaypoint.x - vLastWaypoint.x;
   float fDeltaZ = vNextWaypoint.z - vLastWaypoint.z;
   float fInterX = 0.0f;
   float fInterZ = 0.0f;
   float fXCurr = 0.0f;
   float fZCurr = 0.0f;
   long lInterX = 0L;
   long lInterZ = 0L;
   long lDirTest = -1;
   long lConnTest = 0L;
   bool bMoved = false;
   bool bOffset = false;
   bool bFound = false;
   bool bTranslate = true;
   long lOffsetDir = -1; // 0 = offset to the right of the path, 1 = offset to the left of the path
   long lAdjAxis = -1;   // 0 = adjust x axis.. 1 = adjust z axis (this is the half-tile adjustment to avoid overlapping obstructions)
   if (pnode->sDir == cDirNorth || pnode->sDir == cDirSouth)
   {
      lXStart = pnodeLimit->sX << pnodeLimit->sN;
      lXEnd = lXStart + lLimitQuadSize; 
      lXCurr = lXStart + (lLimitQuadSize >> 1);
      lAdjAxis = 0;
      if (pnode->sDir == cDirNorth)
      {
         lZCurr = pnode->sZ << pnode->sN;
         lDirTest = (pnode->sDir + 2) % 4;
      }
      else
      {
         lZCurr = (pnode->sZ << pnode->sN) + lQuadSize;
         lDirTest = pnode->sDir;
      }
      lConnTest = 1 << lDirTest;
      fInterZ = (lZCurr * mfCellSize) - vLastWaypoint.z;
      if (fabs((double)fDeltaZ) > cFloatCompareEpsilon)
      {
         fInterX = fInterZ * fDeltaX / fDeltaZ;
         fInterX += vLastWaypoint.x;
         lInterX = (long)(fInterX * mfRecipCellSize);
      }
      else
      {
         lInterX = lXCurr;
         fInterX = (float)lInterX * mfCellSize;
      }

      fInterZ += vLastWaypoint.z;

      #ifdef DEBUG_FIND_WAYPOINT
      debug("lXStart: %d lZStart: %d", lXStart, lZStart);
      debug("lXEnd:   %d lZEnd:   %d", lXEnd, lZEnd);
      debug("lZCurr (fixed): %d", lZCurr);
      debug("lInterX: %d", lInterX);
      #endif
      // Is the intersection to the left of the quad?
      if (lInterX < lXStart)
      {
         // Find the closest unobstructed spot to the left side of the quad.
         for (lXCurr = lXStart; lXCurr < lXEnd; lXCurr++)
         {
            if (mpTree[0][lXCurr][lZCurr].btPathingConns[lBucket] & lConnTest)
            {
               bFound = true;
               #ifdef DEBUG_FIND_WAYPOINT
               debug("Found XCurr searching from Left: %d", lXCurr);
               #endif
               break;
            }
            bMoved = true;
         }
         // If we didn't find any unobstructed spots, something's wrong.
         if (!bFound)
         {
            //BASSERT(0);
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Couldn't find a valid XCurr searching from left.");
            #endif
            return false;
         }
      }
      // Is the intersection to the right of the quad?
      if (lInterX > lXEnd - 1)
      {
         // Find the closest unobstucted spot to the right of the quad. 
         for (lXCurr = lXEnd - 1; lXCurr >= lXStart; lXCurr--)
         {
            if (mpTree[0][lXCurr][lZCurr].btPathingConns[lBucket] & lConnTest)
            {
               bFound = true;
               #ifdef DEBUG_FIND_WAYPOINT
               debug("Found XCurr searching from Right: %d", lXCurr);
               #endif
               break;
            }
            bMoved = true;
         }
         // If we didn't find any unobstructed spots, something's wrong.
         if (!bFound)
         {
            //BASSERT(0);
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Couldn't find a valid XCurr searching from right.");
            #endif
            return false;
         }
      }
      if (!bFound)
      {
         // The intersection must be between the endpoints.  Find the closest unobstructed spot to that square.  
         long lDistFromInter = -1;
         long lXBest = 0;
         // First try from the intersection point to the right.. 
         for (lXCurr = lInterX; lXCurr < lXEnd; lXCurr++)
         {
            if (mpTree[0][lXCurr][lZCurr].btPathingConns[lBucket] & lConnTest)
            {
               bFound = true;
               lDistFromInter = lXCurr - lInterX;
               lXBest = lXCurr;
               break;
            }
            bMoved = true;
         }
         // Now look from 1 before the intersection to the left.. 
         if (lDistFromInter != 0)
         {
            for (lXCurr = lInterX - 1; lXCurr >= lXStart; lXCurr--)
            {
               if (mpTree[0][lXCurr][lZCurr].btPathingConns[lBucket] & lConnTest)
               {
                  bFound = true;
                  if (lDistFromInter == -1 || lInterX - lXCurr < lDistFromInter)
                     lXBest = lXCurr;
                  else if (lInterX - lXCurr == lDistFromInter)
                  { 
                     // Do a distsqr check between the two contenders to the next estimated waypoint,
                     // and take whichever one is closer. 
                     float fXTest1 = (lXBest*mfCellSize) - vNextWaypoint.x;
                     float fXTest2 = (lXCurr+1)*mfCellSize - vNextWaypoint.x;
                     float fZTest = (lZCurr*mfCellSize) - vNextWaypoint.z;
                     float fZTestSqr = fZTest * fZTest;
                     if ((fXTest2*fXTest2 + fZTestSqr) < (fXTest1*fXTest1 + fZTestSqr))
                        lXBest = lXCurr;
                  }
                  break;
               }
            }
         }
         if (!bFound)
         {
            //BASSERT(0);
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Couldn't find a valid XCurr searching between endpoints.");
            #endif
            return false;
         }
         if (lDistFromInter == 0)
         {
            fXCurr = fInterX;
            fZCurr = fInterZ;
            bTranslate = false;
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Using Actual Intersection point of (%f) for XCurr", fXCurr);
            #endif
         }
         else
         {
            lXCurr = lXBest;
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Found valid XCurr between endpoints: %d", lXCurr);
            #endif
         }
      }

      // If we had to move, then we *know* we need to offset by the radius of the pathing unit.  
      // Otherwise, even if we didn't move, see if the spot to the left or to the right of us is
      // blocked, and if so, then we also need to offset.  
      if (!bMoved)
      {
         if (lXCurr > 0 && ((mpTree[0][lXCurr-1][lZCurr].btPathingConns[lBucket] & lConnTest) == 0))
         {
            if (pnode->sDir == cDirSouth)
               lOffsetDir = 0;
            else
               lOffsetDir = 1;
            bOffset = true;
         }
         if (lXCurr < mlSizeX[0] - 1 && ((mpTree[0][lXCurr+1][lZCurr].btPathingConns[lBucket] & lConnTest) == 0))
         {
            if (bOffset)
               bOffset = false;
            else
            {
               if (pnode->sDir == cDirSouth)
                  lOffsetDir = 1;
               else
                  lOffsetDir = 0;
               bOffset = true;
            }
         }
      }
      else
         bOffset = true;
   }
   else
   {
      lZStart = pnodeLimit->sZ << pnodeLimit->sN;
      lZEnd = lZStart + lLimitQuadSize;
      lZCurr = lZStart + (lLimitQuadSize >> 1);
      lAdjAxis = 1;
      if (pnode->sDir == cDirEast)
      {
         lXCurr = pnode->sX << pnode->sN;
         lDirTest = (pnode->sDir + 2) % 4;
      }
      else
      {
         lXCurr = (pnode->sX << pnode->sN) + lQuadSize;
         lDirTest = pnode->sDir;
      }
      lConnTest = 1 << lDirTest;
      fInterX = (lXCurr * mfCellSize) - vLastWaypoint.x;
      if (fabs((double)fDeltaX) > cFloatCompareEpsilon)
      {
         fInterZ = fInterX * fDeltaZ / fDeltaX;
         fInterZ += vLastWaypoint.z;
         lInterZ = (long)(fInterZ * mfRecipCellSize);
      }
      else
      {
         lInterZ = lZCurr;
         fInterZ = (float)lInterZ * mfCellSize;
      }

      #ifdef DEBUG_FIND_WAYPOINT
      debug("lXStart: %d lZStart: %d", lXStart, lZStart);
      debug("lXEnd:   %d lZEnd:   %d", lXEnd, lZEnd);
      debug("lXCurr (fixed): %d", lXCurr);
      debug("lInterZ: %d", lInterZ);
      #endif

      fInterX += vLastWaypoint.x;

      // Is the intersection below the quad?
      if (lInterZ < lZStart)
      {
         // Find the closest unobstructed spot to the bottom of the quad...
         for (lZCurr = lZStart; lZCurr < lZEnd; lZCurr++)
         {
            if (mpTree[0][lXCurr][lZCurr].btPathingConns[lBucket] & lConnTest)
            {
               bFound = true;
               #ifdef DEBUG_FIND_WAYPOINT
               debug("Found ZCurr searching from Bottom: %d", lZCurr);
               #endif
               break;
            }
            bMoved = true;
         }
         // If we didn't find any unobstructed spots, something's wrong.
         if (!bFound)
         {
            //BASSERT(0);
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Unable to find lZCurr searching from Bottom");
            #endif
            return false;
         }
      }
      // Is the intersection above the quad?
      if (lInterZ > lZEnd - 1)
      {
         // Find the closest unobstucted spot to the top of the quad..
         for (lZCurr = lZEnd - 1; lZCurr >= lZStart; lZCurr--)
         {
            if (mpTree[0][lXCurr][lZCurr].btPathingConns[lBucket] & lConnTest)
            {
               bFound = true;
               #ifdef DEBUG_FIND_WAYPOINT
               debug("Found ZCurr searching from Top: %d", lZCurr);
               #endif
               break;
            }
            bMoved = true;
         }
         // If we didn't find any unobstructed spots, something's wrong.
         if (!bFound)
         {
            //BASSERT(0);
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Unable to find lZCurr searching from Top");
            #endif
            return false;
         }
      }
      if (!bFound)
      {
         // The intersection must be between the endpoints.  Find the closest unobstructed spot to that square.  
         long lDistFromInter = -1;
         long lZBest = 0;
         // First try from the intersection point up..
         for (lZCurr = lInterZ; lZCurr < lZEnd; lZCurr++)
         {
            if (mpTree[0][lXCurr][lZCurr].btPathingConns[lBucket] & lConnTest)
            {
               bFound = true;
               lDistFromInter = lZCurr - lInterZ;
               lZBest = lZCurr;
               break;
            }
            bMoved = true;
         }
         // Now look from 1 before the intersection down..
         if (lDistFromInter != 0)
         {
            for (lZCurr = lInterZ - 1; lZCurr >= lZStart; lZCurr--)
            {
               if (mpTree[0][lXCurr][lZCurr].btPathingConns[lBucket] & lConnTest)
               {
                  bFound = true;
                  if (lDistFromInter == -1 || lInterZ - lZCurr < lDistFromInter)
                     lZBest = lZCurr;
                  else if (lInterZ - lZCurr == lDistFromInter)
                  { 
                     // Do a distsqr check between the two contenders to the next estimated waypoint,
                     // and take whichever one is closer. 
                     float fZTest1 = (lZBest*mfCellSize) - vNextWaypoint.z;
                     float fZTest2 = (lZCurr)*mfCellSize - vNextWaypoint.z;
                     float fXTest = (lXCurr*mfCellSize) - vNextWaypoint.x;
                     float fXTestSqr = fXTest * fXTest;
                     // why do we work out fXTestSqr and then just add it to both sides of the
                     // compare down here.  Isn't it the same just to not add it and thus not
                     // work it out?
                     if ((fZTest2*fZTest2 + fXTestSqr) < (fZTest1*fZTest1 + fXTestSqr))
                        lZBest = lZCurr;
                  }
                  break;
               }
            }
         }
         if (!bFound)
         {
            BASSERT(0);
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Unable to find lZCurr between the endpoints");
            #endif
            return false;
         }
         if (lDistFromInter == 0)
         {
            fXCurr = fInterX;
            fZCurr = fInterZ;
            bTranslate = false;
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Using actual intersection Z between endpoints: %f", fZCurr);
            #endif
         }
         else
         {
            lZCurr = lZBest;
            #ifdef DEBUG_FIND_WAYPOINT
            debug("Found lZCurr between endpoints: %d", lZCurr);
            #endif
         }
      }

      // If we had to move, then we *know* we need to offset by the radius of the pathing unit.  
      // Otherwise, even if we didn't move, see if the spot above or below the current spot to see if we should move away..
      if (!bMoved)
      {
         if (lZCurr > 0 && ((mpTree[0][lXCurr][lZCurr-1].btPathingConns[lBucket] & lConnTest) == 0))
         {
            if (pnode->sDir == cDirEast)
               lOffsetDir = 0;
            else
               lOffsetDir = 1;
            bOffset = true;
         }
         if (lZCurr < mlSizeZ[0] - 1 && ((mpTree[0][lXCurr][lZCurr+1].btPathingConns[lBucket] & lConnTest) == 0))
         {
            if (bOffset)
               bOffset = false;
            else
            {
               if (pnode->sDir == cDirEast)
                  lOffsetDir = 1;
               else
                  lOffsetDir = 0;
               bOffset = true;
            }
         }
      }
      else
         bOffset = true;
   }


   if (bTranslate)
   {
      vWaypoint.x = lXCurr * mfCellSize;
      if (bMoved && lAdjAxis == 0)
         vWaypoint.x += (mfCellSize * 0.5f);

      vWaypoint.z = lZCurr * mfCellSize;
      if (bMoved && lAdjAxis == 1)
         vWaypoint.z += (mfCellSize * 0.5f);
   }
   else
   {
      vWaypoint.x = fInterX;
      vWaypoint.z = fInterZ;
   }

   #ifdef DEBUG_FIND_WAYPOINT
   debug("Intersect Point:     %f, %f", fInterX, fInterZ);
   debug("Unadjusted Waypoint: %f, %f", vWaypoint.x, vWaypoint.z);
   #endif

   /*#ifdef DEBUG_LRPTREE_GRAPHPATHS
   // Show unadjusted final waypoint in Red..
   if (game->getShowPaths() == 3) 
   {
      debugAddPoint("estimates", vWaypoint, cDWORDRed);
   }
   #endif*/
   // If we need to offset, do so here.
   if (bOffset)
   {
      BVector v1 = vWaypoint - vLastWaypoint;
      BVector v2 = vNextWaypoint - vLastWaypoint;
      v1.normalize();
      v2.normalize();
      // Use the dot to get the cos. of the angle between the vector that's perpindular to
      // v1 and v2.  If this is positive, the vector turns to the left, if it's negative, it turns to the right. 
      float fDot = -v1.z*v2.x + v1.x*v2.z;
      // Positive dotproduct, line turns left, push off right..
      // The direction determined by the angle of the line turn overides the direction
      // set above.  We only use that if we are not turning, but moving in a straight light.
      if (fDot > cFloatCompareEpsilon)
      {
         lOffsetDir = 0;
      }
      else if (fDot < -cFloatCompareEpsilon)
      {
         // Negative dotproduct, line turns right, push off left..
         lOffsetDir = 1;
      }
      // If by now we haven't set direction, then we don't know
      // what direction to move, so just punt.
      if (lOffsetDir != -1)
      {
         if (lOffsetDir == 0)
         {
            BVector vDir(v2.z, 0.0f, -v2.x);
            vWaypoint += vDir * mfRadius;
         }
         else
         {
            BVector vDir(-v2.z, 0.0f, v2.x);
            vWaypoint += vDir * mfRadius;
         }
      }

      /*
      else
      {
         vWaypoint += v2 * mfRadius;
      }
      */
      #ifdef DEBUG_FIND_WAYPOINT
      debug("Final      Waypoint: %f, %f", vWaypoint.x, vWaypoint.z);
      #endif

      /*#ifdef DEBUG_LRPTREE_GRAPHPATHS
      // Show adjusted final waypoint in Blue..
      if (game->getShowPaths() == 3)
      {
         debugAddPoint("estimates", vWaypoint, cDWORDBlue);
      }
      #endif*/
      // Test to make sure the projected point is still in our quad.. if not, then reject it. 
//      BVector vMin, vMax;
//      getQuadBoundaries(pnode->sN, pnode->sX, pnode->sZ, vMin, vMax);
//      if (vTest.x >= vMin.x && vTest.z >= vMin.z && vTest.x <= vMax.x && vTest.z <= vMax.z)
//         vWaypoint = vTest;

   }

         
/*   


   // Find the closest point from the center to the distance between the last waypoint
   // and the next one. 
   BVector vCenter(pnode->fX, 0.0f, pnode->fZ);
   vCenter.xzDistanceToLineSegmentSqr(vLastWaypoint, vNextWaypoint, &vWaypoint);

   // Is point inside quad?  If so, that's our guy.
   BVector vMin, vMax;
   getQuadBoundaries(pnode->sN, pnode->sX, pnode->sZ, vMin, vMax);
   if (vWaypoint.x >= vMin.x && vWaypoint.z >= vMin.z && vWaypoint.x <= vMax.x && vWaypoint.z <= vMax.z)
      return;

   // No?  Then do our own "find closest point on square" check here by doing the distance check 
   // to all four segments of the quad.
   BVector v1, v2, vClosest, vCheck;
   float fBestDistance;
   float fDistance = 0.0f;
   v1 = vMin;
   v2.x = vMin.x;
   v2.z = vMax.z;
   fBestDistance = vWaypoint.xzDistanceToLineSegment(v1, v2, &vClosest);

   // Segment 2
   v1 = vMax;
   fDistance = vWaypoint.xzDistanceToLineSegmentSqr(v1, v2, &vCheck);
   if (fDistance < fBestDistance)
   {
      fBestDistance = fDistance;
      vClosest = vCheck;
   }

   // Segment 3
   v2.x = vMax.x;
   v2.z = vMin.z;
   fDistance = vWaypoint.xzDistanceToLineSegmentSqr(v1, v2, &vCheck);
   if (fDistance < fBestDistance)
   {
      fBestDistance = fDistance;
      vClosest = vCheck;
   }

   // Segment 4
   v1 = vMin;
   fDistance = vWaypoint.xzDistanceToLineSegmentSqr(v1, v2, &vCheck);
   if (fDistance < fBestDistance)
   {
      fBestDistance = fDistance;
      vClosest = vCheck;
   }

   // Note.. might do additional check here to make sure closest point is unobstructed 
   // to reduce stress on the low level pather.
   vWaypoint = vClosest;

   #ifdef DEBUG_LRPTREE_GRAPHPATHS
   if (game->getShowPaths() == 3)
   {
      debugAddPoint("estimates", vWaypoint, cColorRed);
   }
   #endif
*/
   #ifdef DEBUG_FIND_WAYPOINT
   debug("<--- findWaypoint");
   #endif

   return true;
}


//==============================================================================
// BLrpTree::pointInQuad
//==============================================================================
bool BLrpTree::pointInQuad(BLrpPathNode *pnode, const BVector &v)
{

   BVector vMin, vMax;
   getQuadBoundaries(pnode->sN, pnode->sX, pnode->sZ, vMin, vMax);
   if(v.x>=vMin.x && v.x<=vMax.x && 
      v.z>=vMin.z && v.z<=vMax.z)
   {
      return true;
   }

   return false;
}


//==============================================================================
// BLrpTree::findClosestPointInQuad
//==============================================================================
long BLrpTree::findClosestPointInQuad(long k, long n, long x, long z, const BVector &vGoal, const BVector &vStart,
                                      BVector &vClosest)
{

   long m = 0;
   vClosest.x = 0.0f;
   vClosest.y = 0.0f;
   vClosest.z = 0.0f;

   BLrpNode *node = &mpTree[n][x][z];

   // If I'm invalid, then just return -1 now..
   if ((node->btPathingConns[k] & cConnAll) == cConnNone)
      return -1L;

   // If we're at the root level, then the closest cell in this
   // quad *is* this quad.  
   // Return the cell distance from this cell to the goal cell.
   if (n == 0)
   {
      long lDestX = (long)(vGoal.x * mfRecipCellSize);
      if (lDestX >= mlSizeX[0])
         lDestX = mlSizeX[0] - 1;
      long lDestZ = (long)(vGoal.z * mfRecipCellSize);
      if (lDestZ >= mlSizeZ[0])
         lDestZ = mlSizeZ[0] - 1;

      getClosestPointInCell(x, z, vGoal, vClosest);
      return calcCellDistance(k, n, x, z, lDestX, lDestZ);
   }

   
   // Examine each of the four children.  Return the distance
   // of the closest valid child.  
   long lDistances[4];
   BVector vPoints[4];

   x = x << 1;
   z = z << 1;
   m = n - 1;
   long x1 = x + 1;
   long z1 = z + 1;
   lDistances[0] = findClosestPointInQuad(k, m, x,   z, vGoal, vStart, vPoints[0]);
   lDistances[1] = findClosestPointInQuad(k, m, x,  z1, vGoal, vStart, vPoints[1]);
   lDistances[2] = findClosestPointInQuad(k, m, x1, z1, vGoal, vStart, vPoints[2]);
   lDistances[3] = findClosestPointInQuad(k, m, x1,  z, vGoal, vStart, vPoints[3]);

   long lChildBest = cMaximumLong;
   float fStartDistSqr = cMaximumFloat;

   for (long p = 0; p < 4; p++)
   {
      if (lDistances[p] != -1)
      {
         if (lDistances[p] < lChildBest)
         {
            vClosest = vPoints[p];
            lChildBest = lDistances[p];
            fStartDistSqr = vPoints[p].xzDistanceSqr(mvStart);
         }
         else if (lDistances[p] == lChildBest)
         {
            if (vPoints[p].xzDistanceSqr(mvStart) < fStartDistSqr)
            {
               vClosest = vPoints[p];
               lChildBest = lDistances[p];
            }
         }
      }

   }
   if (lChildBest == cMaximumLong)
   {
      // None of the distances were set.. and we're going to return an uninitialized value.
      BASSERT(0);
   }

   return lChildBest;
   
}


//==============================================================================
// BLrpTree::calcCellDistance
//==============================================================================
long BLrpTree::calcCellDistance(long k, long n, long lSrcX, long lSrcZ, long lDestX, long lDestZ)
{

   if ((mpTree[n][lSrcX][lSrcZ].btPathingConns[k] & cConnAll) == cConnNone)
      return -1L;

   long dx = lDestX - lSrcX;
   long dz = lDestZ - lSrcZ;

   return dx*dx + dz*dz;

}

//==============================================================================
// BLrpTree::getClosestPointInCell
//==============================================================================
float BLrpTree::getClosestPointInCell(long x, long z, const BVector &vGoal,
                                      BVector &vClosest)
{

   getQuadBoundaries(0, x, z, mStaticPoints[0], mStaticPoints[2]);
   if (vGoal.x >= mStaticPoints[0].x && vGoal.x <= mStaticPoints[2].x &&
       vGoal.z >= mStaticPoints[0].z && vGoal.z <= mStaticPoints[2].z)
   {
      vClosest = vGoal;
      return 0.0f;
   }
   float fDist = getDistanceToQuad(0, x, z, vGoal, vClosest);
   BVector vAdjClosest(0.0f);
   // DLM 2/15/08 - AdjustForRadius never really considered there would be
   // 16 meter radius units.  It's causing push offs that *really* screw up the
   // path.  Turning it off.
   //adjustForRadius(vClosest, vGoal, vAdjClosest);
   //vClosest = vAdjClosest;
   fDist += mfRadius;
   return fDist;
}

//==============================================================================
// BLrpTree::getClosestPointInNode
//==============================================================================
float BLrpTree::getDistanceToQuad(long n, long x, long z, const BVector &vPoint,
                                      BVector &vClosest)
{
   
   long lSegment;
   float fDistSqr;
   BVector vMid, vMin, vMax, vQuadMid;
   mStaticPoints[0].y = mStaticPoints[1].y = mStaticPoints[2].y = mStaticPoints[3].y = 0.0f;
   vMin.y = vMax.y = 0.0f;
   vQuadMid.y = 0.0f;

   getQuadBoundaries(n, x, z, mStaticPoints[0], mStaticPoints[2]);
   mStaticPoints[1].x = mStaticPoints[0].x;
   mStaticPoints[1].z = mStaticPoints[2].z;
   mStaticPoints[3].x = mStaticPoints[2].x;
   mStaticPoints[3].z = mStaticPoints[0].z;
   mStaticHull.initialize(mStaticPoints, 4, true);
   vClosest = mStaticHull.findClosestPointOnHull(vPoint, &lSegment, &fDistSqr);
   return (float)sqrt(fDistSqr);
}

//==============================================================================
// BLrpTree::debug
//==============================================================================
void BLrpTree::debug(char* v, ... ) const
{
#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigDebugPather) == false)
      return;
#endif

   long lSpecificSquad = -1;

#ifndef BUILD_FINAL
   gConfig.get(cConfigDebugSpecificSquad, &lSpecificSquad);
#endif

   bool bMatch = false;

   if (lSpecificSquad == -1 || ((lSpecificSquad != -1) && (lSpecificSquad == mlEntityID)))
      bMatch = true;

   if (!bMatch)
      return;

   static char out[BLogManager::cMaxLogTextLength];
   va_list va;
   va_start(va, v);
   bvsnprintf(out, sizeof(out), v, va);
   static char out2[BLogManager::cMaxLogTextLength*2];
   bsnprintf(out2, sizeof(out2), "ENTITY #%5d: %s", mlEntityID, out);
   gConsole.output(cChannelSim, out2);

}
   
//==============================================================================
// BLrpTree::debugAddPoint
//==============================================================================
void BLrpTree::debugAddPoint(const char *szName, const BVector &start, DWORD color)
{
   static float fDepth = 0.2f;
   fDepth += 0.2f;
   if (fDepth > 1.4f)
      fDepth = 0.2f;
   BVector p1 = start;
   BVector p2 = start;
   p1.x -= 0.25f;
   p1.z -= 0.25f;
   p2.x += 0.25f;
   p2.z += 0.25f;
   gTerrainSimRep.addDebugLineOverTerrain(/*szName, 0L,*/ p1, p2, color, color, fDepth);
   p1.z = start.z + 0.25f;
   p2.z = start.z - 0.25f;
   gTerrainSimRep.addDebugLineOverTerrain(/*szName, 0L,*/ p1, p2, color, color, fDepth);
}


//==============================================================================
// BLrpTree::segmentIntersect
//==============================================================================
bool BLrpTree::segmentIntersect(long lBucket, const BVector &v1, const BVector &v2)
{   
   // Determine the goodies from the line..
   /*#ifdef DEBUG_LRPTREE_GRAPHPATHS
   BDebugPrimRender::removeDebugLines("segintersect");
   #endif*/

   long lXStart = (long)(v1.x * mfRecipCellSize);
   long lZStart = (long)(v1.z * mfRecipCellSize);
   long lXEnd = (long)(v2.x * mfRecipCellSize);
   long lZEnd = (long)(v2.z * mfRecipCellSize);
   long lXDelta = lXEnd - lXStart;
   long lZDelta = lZEnd - lZStart;

   long lXCurr = 0;
   long lZCurr = 0;
   long lNr = 0;
   long lDiff = 0;

   /*#ifdef DEBUG_LRPTREE_GRAPHPATHS
   if (game->getShowPaths() == 3) 
   {
      BVector p1(lXStart*mfCellSize, 0.0f, lZStart*mfCellSize);
      BVector p2(lXEnd*mfCellSize, 0.0f, lZEnd*mfCellSize);
      //game->getWorld()->getTerrain()->addDebugLineOverTerrain("segintersect", 0L, p1, p2, cDWORDDarkGrey, cDWORDDarkGrey, 0.75f);
      gTerrainSimRep.addDebugLineOverTerrain(p1, p2, cDWORDDarkGrey, cDWORDDarkGrey, 0.75f);
   }
   #endif*/

   // Check the four axis first
   if (lXDelta > 0 && lZDelta == 0)
   {
      lZCurr = lZStart;
      for (lXCurr = lXStart + 1; lXCurr < lXEnd; lXCurr++)
      {
         if (testCell(cConnWest, lBucket, lXCurr, lZCurr))
            return true;
      }
      return false;
   }
   if (lZDelta > 0 && lXDelta == 0)
   {
      lXCurr = lXStart;
      for (lZCurr = lZStart + 1; lZCurr < lZEnd; lZCurr++)
      {
         if (testCell(cConnWest, lBucket, lXCurr, lZCurr))
            return true;
      }
      return false;
   }
   if (lXDelta < 0 && lZDelta == 0)
   {
      lZCurr = lZStart;
      for (lXCurr = lXStart - 1; lXCurr > lXEnd; lXCurr--)
      {
         if (testCell(cConnEast, lBucket, lXCurr, lZCurr))
            return true;
      }
      return false;
   }
   if (lZDelta < 0 && lXDelta == 0)
   {
      lXCurr = lXStart;
      for (lZCurr = lZStart - 1; lZCurr > lZEnd; lZCurr--)
      {
         if (testCell(cConnNorth, lBucket, lXCurr, lZCurr))
            return true;
      }
      return false;
   }

   // 0 <= theta < 45 deg.
   if (lXDelta >= 0 && lZDelta >= 0 && lZDelta < lXDelta)
   {
      // This is code for Octant 1
      lXCurr = lXStart + 1;
      lZCurr = lZStart;
      lNr = 0;
      lDiff = lXDelta - lZDelta;

      for (lXCurr; lXCurr < lXEnd; lXCurr++)
      {
         if (lNr < lDiff)
         {
            lNr += lZDelta;
            if (lNr != 0)
            {
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr)) 
                  return true;
            }
            else
            {
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
            }
         }
         else
         {
            lZCurr += 1;
            lNr -= lDiff;
            if (lNr != 0)
            {
               if (testCell(cConnSouth, lBucket, lXCurr-1, lZCurr))
                  return true;
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr))
                  return true;
            }
            else
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
         }
      }
   }
   // 45 <= theta < 90 deg.
   if (lXDelta > 0 && lZDelta >= 0 && lZDelta >= lXDelta)
   {
      // This is code for Octant 2
      lZCurr = lZStart + 1;                                    // f1
      lXCurr = lXStart;                                        // f2
      lNr = 0;
      lDiff = lZDelta - lXDelta;                               // f3

      for (lZCurr; lZCurr < lZEnd /* f4 */; lZCurr++ /* f5 */)
      {
         if (lNr < lDiff)
         {
            lNr += lXDelta; /* i1 */
            if (lNr)
            {
               if (testCell(cConnSouth, lBucket, lXCurr, lZCurr))
                  return true;
            }
            else
            {
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
            }
         }
         else
         {
            lXCurr++;                                           // s1
            lNr -= lDiff;
            if (lNr)
            {
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr-1))
                  return true;
               if (testCell(cConnSouth,lBucket, lXCurr, lZCurr))
                  return true;
            }
            else
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
         }
      }
   }
   // 90 <= theta < 135
   if (lXDelta <= 0 && lZDelta >= 0 && lZDelta > -lXDelta)
   {
      // This is code for Octant 3
      lZCurr = lZStart + 1;                                    // f1
      lXCurr = lXStart;                                        // f2
      lNr = 0;
      lDiff = lZDelta + lXDelta;                               // f3

      for (lZCurr; lZCurr < lZEnd /* f4 */; lZCurr++ /* f5 */)
      {
         if (lNr < lDiff)
         {
            lNr -= lXDelta; /* i1 */
            if (lNr)
            {
               if (testCell(cConnSouth, lBucket, lXCurr-1, lZCurr))
                  return true;
            }
            else
            {
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
            }
         }
         else
         {
            lXCurr--;                                          // s1
            lNr -= lDiff;
            if (lNr)
            {
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr-1))
                  return true;
               if (testCell(cConnSouth, lBucket, lXCurr-1, lZCurr))
                  return true;
            }
            else
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
         }
      }

   }
   // 135 <= theta < 180
   if (lXDelta <= 0 && lZDelta > 0 && lZDelta <= -lXDelta)
   {
      // This is code for Octant 4
      lXCurr = lXStart - 1;                                    // f1
      lZCurr = lZStart;                                        // f2
      lNr = 0;
      lDiff = -lXDelta - lZDelta;                              // f3

      for (lXCurr; lXCurr > lXEnd /* f4 */; lXCurr-- /* f5 */)
      {
         if (lNr < lDiff)
         {
            lNr += lZDelta; /* i1 */
            if (lNr)
            {
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr))
                  return true;
            }
            else
            {
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
            }
         }
         else
         {
            lZCurr++;                                          // s1
            lNr -= lDiff;
            if (lNr)
            {
               if (testCell(cConnSouth,lBucket, lXCurr, lZCurr))
                  return true;
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr))
                  return true;
            }
            else
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
         }
      }

   }
   // 180 <= theta < 225
   if (lXDelta <= 0 && lZDelta <= 0 && lZDelta > lXDelta)
   {
      // This is code for Octant 5
      lXCurr = lXStart - 1;                                    // f1
      lZCurr = lZStart;                                        // f2
      lNr = 0;
      lDiff = -lXDelta + lZDelta;                              // f3

      for (lXCurr; lXCurr > lXEnd /* f4 */; lXCurr-- /* f5 */)
      {
         if (lNr < lDiff)
         {
            lNr -= lZDelta; /* i1 */
            if (lNr)
            {
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr-1))
                  return true;
            }
            else
            {
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
            }
         }
         else
         {
            lZCurr--;                                          // s1
            lNr -= lDiff;
            if (lNr)
            {
               if (testCell(cConnSouth,lBucket, lXCurr, lZCurr))
                  return true;
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr-1))
                  return true;
            }
            else
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
         }
      }

   }
   // 225 <= theta < 270
   if (lXDelta < 0 && lZDelta <= 0 && lZDelta <= lXDelta)
   {
      // This is code for Octant 6
      lZCurr = lZStart - 1;                                    // f1
      lXCurr = lXStart;                                        // f2
      lNr = 0;
      lDiff = -lZDelta + lXDelta;                              // f3

      for (lZCurr; lZCurr > lZEnd /* f4 */; lZCurr-- /* f5 */)
      {
         if (lNr < lDiff)
         {
            lNr -= lXDelta; /* i1 */
            if (lNr)
            {
               if (testCell(cConnSouth, lBucket, lXCurr-1, lZCurr))
                  return true;
            }
            else
            {
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
            }
         }
         else
         {
            lXCurr--;                                          // s1
            lNr -= lDiff;
            if (lNr)
            {
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr))
                  return true;
               if (testCell(cConnSouth, lBucket, lXCurr-1, lZCurr))
                  return true;
            }
            else
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
         }
      }

   }
   // 270 <= theta < 315
   if (lXDelta >= 0 && lZDelta <= 0 && -lZDelta > lXDelta)
   {
      // This is code for Octant 7
      lZCurr = lZStart - 1;                                    // f1
      lXCurr = lXStart;                                        // f2
      lNr = 0;
      lDiff = -lZDelta - lXDelta;                              // f3

      for (lZCurr; lZCurr > lZEnd /* f4 */; lZCurr-- /* f5 */)
      {
         if (lNr < lDiff)
         {
            lNr += lXDelta; /* i1 */
            if (lNr)
            {
               if (testCell(cConnSouth,lBucket, lXCurr, lZCurr))
                  return true;
            }
            else
            {
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
            }
         }
         else
         {
            lXCurr++;                                          // s1
            lNr -= lDiff;
            if (lNr)
            {
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr))
                  return true;
               if (testCell(cConnSouth,lBucket, lXCurr, lZCurr))
                  return true;
            }
            else
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
         }
      }

   }
   // 315 <= theta < 360
   if (lXDelta >= 0 && lZDelta < 0 && -lZDelta <= lXDelta)
   {
      // This is code for Octant 8
      lXCurr = lXStart + 1;                                    // f1
      lZCurr = lZStart;                                        // f2
      lNr = 0;
      lDiff = lXDelta + lZDelta;                               // f3

      for (lXCurr; lXCurr < lXEnd /* f4 */; lXCurr++ /* f5 */)
      {
         if (lNr < lDiff)
         {
            lNr -= lZDelta; /* i1 */
            if (lNr)
            {
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr-1))
                  return true;
            }
            else
            {
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
            }
         }
         else
         {
            lZCurr--;                                          // s1
            lNr -= lDiff;
            if (lNr)
            {
               if (testCell(cConnSouth, lBucket, lXCurr-1, lZCurr))
                  return true;
               if (testCell(cConnWest, lBucket, lXCurr, lZCurr-1))
                  return true;
            }
            else
               if (testCell(cConnNone, lBucket, lXCurr, lZCurr))
                  return true;
         }
      }

   }
   
   return false;

}

//==============================================================================
// BLrpTree::testCell
//==============================================================================
bool BLrpTree::testCell(long lConnDir, long lBucket, long lX, long lZ)
{
   float fX = lX * mfCellSize;
   float fZ = lZ * mfCellSize;
   BVector v1(fX, 0.0f, fZ);
/*   if (game->getShowPaths() == 3)
   {
      switch(lConnDir)
      {
         case cConnNone:
            debugAddPoint("segintersect", v1, cDWORDWhite);
            break;

         case cConnSouth:
         {
            BVector v2(fX+mfCellSize, 0.0f, fZ);
            game->getWorld()->getTerrain()->addDebugLineOverTerrain("segintersect", 0, v1, v2, cDWORDWhite, cDWORDWhite);
            break;
         }
         case cConnWest:
         {
            BVector v2(fX, 0.0f, fZ+mfCellSize);
            game->getWorld()->getTerrain()->addDebugLineOverTerrain("segintersect", 0, v1, v2, cDWORDWhite, cDWORDWhite);
            break;
         }
      }
   }*/

   if (lConnDir == cConnNone)
   {
      if ((mpTree[0][lX][lZ].btPathingConns[lBucket] & cConnAll) != cConnAll)
          return true;
      return false;
   }

   if ((mpTree[0][lX][lZ].btPathingConns[lBucket] & lConnDir) == 0)
      return true;
      
    
   return false;

}

//==============================================================================
// BLrpTree::backPath
//==============================================================================
void BLrpTree::backPath(long lBucket, BPath *path)
{
   BVector vAnchor;
   BVector vTesting;
   BVector vRemoval;
   BVector vMarker(-1.0, -1.0, -1.0);
   long lAnchor = 0;
   long lNextAnchor = 1;
   long lTesting = 2;
   long lNumberWaypoints = path->getNumberWaypoints();

   #ifdef DEBUG_BACKPATH
   debug("backpath-->");
   #endif
   
   bool bDone = false;
   while (!bDone)
   {
      vAnchor = path->getWaypoint(lAnchor);
      #ifdef DEBUG_BACKPATH
      debug("New Iteration -- Anchor is %d: (%5.2f, %5.2f)", lAnchor, vAnchor.x, vAnchor.z);
      #endif
      for (lTesting = lAnchor + 2; lTesting < lNumberWaypoints; lTesting++)
      {
         vTesting = path->getWaypoint(lTesting);
         //AJL FIXME 10/12/06 - Why would the waypoint ever be negative?
         if (vTesting.x < 0.0f || vTesting.z < 0.0f)
         {
            #ifdef DEBUG_BACKPATH
            debug("Skipping waypoint %d: It is a marker.", lTesting);
            #endif
            continue;
         }

         #ifdef DEBUG_BACKPATH
         debug("Testing waypoint %d: doing seg check from (%5.2f, %5.2f) to (%5.2f, %5.2f)", lTesting, vAnchor.x, vAnchor.z, vTesting.x, vTesting.z);
         #endif

         if (!segmentIntersect(lBucket, vAnchor, vTesting))
         {
            #ifdef DEBUG_BACKPATH
            debug("No Intersection found, marking waypoint %d with a marker.", lTesting-1);
            #endif
            // We don't want to just set this with a marker.. if at any point we don't intersect anything between
            // ourselves and the marker, then remove everything between ourselves and the marker.
            for (long n = lAnchor + 1; n < lTesting; n++)
               path->setWaypoint(n, vMarker);
            lNextAnchor = lTesting;
         }
         #ifdef DEBUG_BACKPATH
         else
         {
            debug("Intersection found, moving on to next waypoint..");
         }
         #endif
      }
      lAnchor = lNextAnchor;
      #ifdef DEBUG_BACKPATH
      debug("Anchor advanced to %d.", lAnchor);
      #endif
      lNextAnchor++;
      if (lNextAnchor >= lNumberWaypoints)
         bDone = true;
   }
   #ifdef DEBUG_BACKPATH
   debug("Now removing markers...");
   #endif
   for (long n = 0; n < path->getNumberWaypoints(); n++)
   {
      vTesting = path->getWaypoint(n);
      if (vTesting.x < 0.0f || vTesting.z < 0.0f)
      {
         #ifdef DEBUG_BACKPATH
         debug("Removed wayoint at %d", n);
         #endif
         path->removeWaypoint(n);
         n--;
      }

   }

   // Icky code to make sure that the distance between waypoints is small enough
   // to make everything copasetic with the low-level pather..
   BVector vCurrent = path->getWaypoint(0);
   for (long l = 1; l < path->getNumberWaypoints(); l++)
   {
      BVector vNext = path->getWaypoint(l);
      if (vCurrent.xzDistanceSqr(vNext) > cMaximumLowLevelDistSqr - cLowLevelRelaxFactorSqr)
      {
         // Project a point along this vector and insert a waypoint here..
         BVector vDir = vNext - vCurrent;
         vDir.y = 0.0f;
         vDir.normalize();
         // Use a fudge factor just to make sure we stay under the limit.. dlm 2/21/01
         BVector vNew = vCurrent + (vDir * (cMaximumLowLevelDist - (cLowLevelRelaxFactor + 1.0f)));
         path->addWaypointBefore(l, vNew);
         vCurrent = vNew;            
      }
      else
         vCurrent = vNext;
   }

   #ifdef DEBUG_BACKPATH
   debug("<-- backpath");
   #endif
   
   return;
}


//==============================================================================
// BLrpTree::findValidStart
//==============================================================================
BLrpNode *BLrpTree::findValidStart(const BVector &vStart, long lBucket, long &n, long &x, long &z)
{
   long x1 = (long) (vStart.x * mfRecipCellSize);
   long z1 = (long) (vStart.z * mfRecipCellSize);

   // Safety check
   if (x1 <= (gWorld->getSimBoundsMinX() * mfRecipCellSize))
      x1 = (long) (gWorld->getSimBoundsMinX() * mfRecipCellSize);
   if (x1 >= (gWorld->getSimBoundsMaxX() * mfRecipCellSize))
      x1 = (long) ((gWorld->getSimBoundsMaxX() * mfRecipCellSize)) - 1;

   if (z1 <= (gWorld->getSimBoundsMinZ() * mfRecipCellSize))
      z1 = (long) (gWorld->getSimBoundsMinZ() * mfRecipCellSize);
   if (z1 >= (gWorld->getSimBoundsMaxZ() * mfRecipCellSize))
      z1 = (long) ((gWorld->getSimBoundsMaxZ() * mfRecipCellSize)) - 1;

   BLrpNode *pnode = &mpTree[0][x1][z1];
   if ((pnode->btPathingConns[lBucket] & cConnAll) == cConnNone)
   {
      for (long lIdx = 0; lIdx < 8; lIdx ++)
      {

         long x2 = x1 + lAdj[lIdx][0];
         long z2 = z1 + lAdj[lIdx][1];
         if (x2 < 0 || x2 > mlSizeX[n] - 1)
            continue;
         if (z2 < 0 || z2 > mlSizeZ[n] - 1)
            continue;
         n = 0;
         x = x2;
         z = z2;
         pnode = &mpTree[0][x2][z2];
         if ((pnode->btPathingConns[lBucket] & cConnAll) != cConnNone)
         {
            // Find the first inUse class
            while ((pnode->btPassability[lBucket] & cInUse) == 0 &&  n < cMaxDepth - 1)
            {
               n++;
               x >>= 1;
               z >>= 1;
               pnode = &mpTree[n][x][z];
            }
            return pnode;
         }
      }
      // If we got to here, then we searched every surrounding tile and didn't find a single 
      // valid one.
      return NULL;
   }

   // The start tile was valid..
   pnode = getInUseQuadWithPoint(vStart, lBucket, n, x, z);

   return pnode;

}

#ifdef DEBUG_UPDATEBASEQUAD3
//==============================================================================
// BLrpTree::updateQuadTree
//==============================================================================
void BLrpTree::updateQuadTree(BOPObstructionNode *pObNode, bool bAdd)
{
   // We have to go to this trouble whether we're adding or 
   // removing, because we have to determine the actual rang over which
   // we're resetting quads.


   // Get the expanded hull of the obstruction, and reset the connections
   // for that set of tiles.  
   const BOPQuadHull *pQuadHull;
   pQuadHull = mpObMgr->getExpandedHull(pObNode, 0.0f);

   if (!pQuadHull)
   {
      BASSERT(0);
      return;
   }

   float fMinX = pQuadHull->mDirectVal[pQuadHull->mIdxMinX];
   float fMinZ = pQuadHull->mDirectVal[pQuadHull->mIdxMinZ];
   float fMaxX = pQuadHull->mDirectVal[pQuadHull->mIdxMaxX];
   float fMaxZ = pQuadHull->mDirectVal[pQuadHull->mIdxMaxZ];

   long lLargest = (long)(((float)(clBucketSizes[clNumBuckets-1]) * 0.5f) + 0.5);

   long lMinX = (long)(fMinX * mfRecipCellSize) - lLargest;
   if (lMinX < 0)
      lMinX = 0;

   long lMinZ = (long)(fMinZ * mfRecipCellSize) - lLargest;
   if (lMinZ < 0)
      lMinZ = 0;

   long lMaxX = (long)(fMaxX * mfRecipCellSize) + lLargest + 1;
   if (lMaxX > mlSizeX[0] - 1)
      lMaxX = mlSizeX[0] - 1;

   long lMaxZ = (long)(fMaxZ * mfRecipCellSize) + lLargest + 1;
   if (lMaxZ > mlSizeZ[0] - 1)
      lMaxZ = mlSizeZ[0] - 1;


   // Convert the boundaries to high-level quad coords..
   long lQuadX1 = lMinX >> cMaxLevel;
   long lQuadZ1 = lMinZ >> cMaxLevel;

   long lQuadX2 = lMaxX >> cMaxLevel;
   long lQuadZ2 = lMaxZ >> cMaxLevel;

   // If MaxX falls on an even quad boundary, then don't increment it..
   // otherwise, do so.
   // QuadX2 & QuadZ2 should be 1 GREATER than the actual high level quads you're going
   // to update.  So the range should go from x = QuadX1 to x < QuadX2.. same for Z.
   // dlm 8/05/02
   if (lMaxX % mlMaxQuadTileSize != 0)
      ++lQuadX2;

   if (lMaxZ % mlMaxQuadTileSize != 0)
      ++lQuadZ2;

   #ifdef DEBUG_UPDATEQUADTREE
   blog("    Updating High-Level Quad: (%ld, %ld) to (%ld, %ld)",
      lQuadX1, lQuadZ1, lQuadX2, lQuadZ2);
   #endif

   if (bAdd)
      addObstruction(pObNode);
   else
      removeObstruction(lQuadX1, lQuadZ1, lQuadX2, lQuadZ2);

   // Now Convert the quad boundares back to cell coords..
   long lTileX1 = (lQuadX1 << cMaxLevel);
   long lTileZ1 = (lQuadZ1 << cMaxLevel);
   long lTileX2 = (lQuadX2 << cMaxLevel);
   long lTileZ2 = (lQuadZ2 << cMaxLevel);

   if (lTileX2 > mlSizeX[0])
      lTileX2 = mlSizeX[0];
   if (lTileZ2 > mlSizeZ[0])
      lTileZ2 = mlSizeZ[0];

   // Independant of quad updating.. set the pathing connections for
   // the level 0 buckets..
   // Update each bucket..
   for (long k = 0; k < clNumBuckets; k++)
      for (long x = lTileX1; x < lTileX2; x++)
         for (long z = lTileZ1; z < lTileZ2; z++)
         {
            setPathingConnections(k, 0, x, z);
         }

   // Now update the entire tree..
   // Update each bucket, and update them from x - 1 to x + 1, and z - 1 to z + 1
   // at the high level side.  dlm 8/05/02
   for (long k = 0; k < clNumBuckets; k++)
      for (long x = lQuadX1 - 1; x <= lQuadX2; x++)
         for (long z = lQuadZ1 - 1; z <= lQuadZ2; z++)
         {
            if (x > -1 && x < mlSizeX[cMaxLevel] &&
                z > -1 && z < mlSizeZ[cMaxLevel])
               updateQuad(k, cMaxLevel, x, z);
         }

   #ifdef DEBUG_UPDATEQUADTREE
   debug("<---- updateQuadTree");
   #endif

}

//==============================================================================
// BLrpTree::updateQuadTree(const BConvexHull &hull, long lPlayerMask)
//==============================================================================
void BLrpTree::updateQuadTree(const BVector &vMin, const BVector &vMax)
{
   // This is the refresh only version.. 
   long lLargest = ((clBucketSizes[clNumBuckets-1]) >> 1);

   long lMinX = (long)(vMin.x * mfRecipCellSize) - lLargest;
   if (lMinX < 0)
      lMinX = 0;

   long lMinZ = (long)(vMin.z * mfRecipCellSize) - lLargest;
   if (lMinZ < 0)
      lMinZ = 0;

   long lMaxX = (long)(vMax.x * mfRecipCellSize) + lLargest + 1;
   if (lMaxX > mlSizeX[0] - 1)
      lMaxX = mlSizeX[0] - 1;

   long lMaxZ = (long)(vMax.z * mfRecipCellSize) + lLargest + 1;
   if (lMaxZ > mlSizeZ[0] - 1)
      lMaxZ = mlSizeZ[0] - 1;


   // Convert the boundaries to high-level quad coords..
   long lQuadX1 = lMinX >> cMaxLevel;
   long lQuadZ1 = lMinZ >> cMaxLevel;

   long lQuadX2 = lMaxX >> cMaxLevel;
   long lQuadZ2 = lMaxZ >> cMaxLevel;

   // If MaxX falls on an even quad boundary, then don't increment it..
   // otherwise, do so.
   if (lMaxX % mlMaxQuadTileSize != 0)
      ++lQuadX2;

   if (lMaxZ % mlMaxQuadTileSize != 0)
      ++lQuadZ2;

   #ifdef DEBUG_UPDATEQUADTREE
   blog("    Updating High-Level Quad: (%ld, %ld) to (%ld, %ld)",
      lQuadX1, lQuadZ1, lQuadX2, lQuadZ2);
   #endif

   // Force the refresh and re-add of the affected quads..
   removeObstruction(lQuadX1, lQuadZ1, lQuadX2, lQuadZ2);

   // Now Convert the quad boundares back to cell coords..
   long lTileX1 = (lQuadX1 << cMaxLevel);
   long lTileZ1 = (lQuadZ1 << cMaxLevel);
   long lTileX2 = (lQuadX2 << cMaxLevel);
   long lTileZ2 = (lQuadZ2 << cMaxLevel);

   // Independant of quad updating.. set the pathing connections for
   // the level 0 buckets..
   // Update each bucket..
   for (long k = 0; k < clNumBuckets; k++)
      for (long x = lTileX1; x < lTileX2; x++)
         for (long z = lTileZ1; z < lTileZ2; z++)
         {
            setPathingConnections(k, 0, x, z);
         }

   // Now update the entire tree..
   // Update each bucket..
   for (long k = 0; k < clNumBuckets; k++)
      for (long x = lQuadX1; x < lQuadX2; x++)
         for (long z = lQuadZ1; z < lQuadZ2; z++)
         {
            updateQuad(k, cMaxLevel, x, z);
         }


}
#else
//==============================================================================
// BLrpTree::updateQuadTree
//==============================================================================
void BLrpTree::updateQuadTree(const BConvexHull &hull, long lPlayerMask, bool bAdd)
{

   #ifdef DEBUG_UPDATEQUADTREE
   debug("----> updateQuadTree");
   #endif

   BVector vMin = hull.getBoundingMin();
   BVector vMax = hull.getBoundingMax(); 

   long lMaxX = mlSizeX[0];
   long lMaxZ = mlSizeZ[0];

   #ifdef DEBUG_NOBUILD
   return;
   #endif

   // Expand the range of tiles to be updated by the size of the largest bucket / 2.
   long lLargest = ((clBucketSizes[clNumBuckets-1]) >> 1);

   long lX1 = (long)(vMin.x * mfRecipCellSize) - lLargest;
   if (lX1 < 0)
      lX1 = 0;
   long lZ1 = (long)(vMin.z * mfRecipCellSize) - lLargest;
   if (lZ1 < 0)
      lZ1 = 0;
   float fX2 = vMax.x * mfRecipCellSize;
   long lX2 = 0;
   if ((fX2 - (float)(long)fX2) > cFloatCompareEpsilon)
      lX2 = (long)fX2 + lLargest + 1;
   else
      lX2 = (long)fX2 + lLargest;
   if (lX2 > lMaxX)
      lX2 = lMaxX;

   float fZ2 = vMax.z * mfRecipCellSize;
   long lZ2 = 0;
   if ((fZ2 - (float)(long)fZ2) > cFloatCompareEpsilon)
      lZ2 = (long)fZ2 + lLargest + 1;
   else
      lZ2 = (long)fZ2 + lLargest;
   if (lZ2 > lMaxZ)
      lZ2 = lMaxZ;

   // Convert the boundaries to high-level quad coords..
   long lQuadX1 = lX1 >> cMaxLevel;
   long lQuadZ1 = lZ1 >> cMaxLevel;

   long lQuadX2 = lX2 >> cMaxLevel;
   long lQuadZ2 = lZ2 >> cMaxLevel;

   // If X2 falls on an even quad boundary, then don't increment it..
   // otherwise, do so.
   if (lX2 % mlMaxQuadTileSize != 0)
      ++lQuadX2;

   if (lZ2 % mlMaxQuadTileSize != 0)
      ++lQuadZ2;

   #ifdef DEBUG_UPDATEQUADTREE
   blog("    Updating High-Level Quad: (%ld, %ld) to (%ld, %ld)",
      lQuadX1, lQuadZ1, lQuadX2, lQuadZ2);
   #endif

   if (bAdd)
      addObstruction(hull, lPlayerMask);
   else
   {
      removeObstruction(lQuadX1, lQuadZ1, lQuadX2, lQuadZ2);
   }

   // Update all the low level tilesin the affected quads
   long lTileX1 = (lQuadX1 << cMaxLevel) - 1;
   if (lTileX1 < 0)
      lTileX1 = 0;
   long lTileZ1 = (lQuadZ1 << cMaxLevel) - 1;
   if (lTileZ1 < 0)
      lTileZ1 = 0;
   long lTileX2 = (lQuadX2 << cMaxLevel) + 1;
   if (lTileX2 > lMaxX)
      lTileX2 = lMaxX;
   long lTileZ2 = (lQuadZ2 << cMaxLevel) + 1;
   if (lTileZ2 > lMaxZ)
      lTileZ2 = lMaxZ;

   // Independant of quad updating.. set the pathing connections for
   // the level 0 buckets..
   // Update each bucket..
   for (long k = 0; k < clNumBuckets; k++)
      for (long x = lTileX1; x < lTileX2; x++)
         for (long z = lTileZ1; z < lTileZ2; z++)
         {
            setPathingConnections(k, 0, x, z);
         }

   // Now update the entire tree..
   // Update each bucket..
   for (k = 0; k < clNumBuckets; k++)
      for (long x = lQuadX1; x < lQuadX2; x++)
         for (long z = lQuadZ1; z < lQuadZ2; z++)
         {
            updateQuad(k, cMaxLevel, x, z);
         }

   #ifdef DEBUG_UPDATEQUADTREE
   debug("<---- updateQuadTree");
   #endif

}
#endif

//==============================================================================
// BLrpTree::findClosestPassableTile
//==============================================================================
bool BLrpTree::findClosestPassableTile(const BVector &vReference, BVector &vResult)
{
#if 0
   // Converts the world coords to tile locations, searchs surrounding tiles
   // for some number of iterations to find a passable one, and returns
   // true if it found one, false if it ran out of iterations.
   #ifdef DEBUG_FINDCLOSESTPASSABLETILE
   debug("----> findClosestPassableTile");
   #endif

   vResult = vReference;
   long lX1 = (long)(vReference.x * mfRecipCellSize);
   long lZ1 = (long)(vReference.z * mfRecipCellSize);

   // Is point on terrain?
   if (!game->getWorld())
   {
      BASSERT(0);
      return false;
   }
   BTerrainBase *pterrain = game->getWorld()->getTerrain();
   if (!pterrain)
   {
      BASSERT(0);
      return false;
   }
   bool bAdjusted = false;
   if (vReference.x < 0.0f)
   {
      lX1 = 0;
      bAdjusted = true;
   }
   if (vReference.z < 0.0f)
   {
      lZ1 = 0;
      bAdjusted = true;
   }
   if (vReference.x > pterrain->getTileSize() * pterrain->getNumberXTiles())
   {
      lX1 = mlTerrainSizeX[0] - 1;
      bAdjusted = true;
   }
   if (vReference.z > pterrain->getTileSize() * pterrain->getNumberZTiles())
   {
      lZ1 = mlTerrainSizeZ[0] - 1;
      bAdjusted = true;
   }      


   if (lX1 < 0 || lX1 > mlTerrainSizeX[0] - 1 ||
      lZ1 < 0 || lZ1 > mlTerrainSizeZ[0] - 1)
      return false;

   if (mpTree[0][lX1][lZ1].btPathingConns[0] != cConnNone)
   {
      #ifdef DEBUG_FINDCLOSESTPASSABLETILE
      debug("Returning on first check: lX1: %d lZ1: %d Conn: %X", lX1, lZ1, mpTree[0][lX1][lZ1].btPathingConns[0]);
      #endif
      if (bAdjusted)
      {
         float fHalfCell = mfCellSize * 0.5f;
         vResult.x = lX1 * mfCellSize + fHalfCell;
         vResult.z = lZ1 * mfCellSize + fHalfCell;
         vResult.y = 0.0f;
      }
      return true;
   }

   long lXStart = lX1;
   long lZStart = lZ1;
   long lXEnd = lX1;
   long lZEnd = lZ1;
   long x = -1;
   long z = -1;
   bool bFound = false;
   long lExpansions = 0;
   while (lExpansions < clMaxFindExpansions && !bFound)
   {
      lXStart -= 1;
      lZStart -= 1;
      lXEnd += 1;
      lZEnd += 1;
      for (z = lZStart; z <= lZEnd; z++)
      {
         if (z < 0 || z > mlTerrainSizeZ[0] - 1)
            continue;
         if (z == lZStart || z == lZEnd)
         {
            for (x = lXStart; x <= lXEnd; x++)
            {
               if (x < 0 || x > mlTerrainSizeX[0] - 1)
                  continue;
               if ((mpTree[0][x][z].btPathingConns[0]) != cConnNone)
               {
                  #ifdef DEBUG_FINDCLOSESTPASSABLETILE
                  debug("Found it on the top/botom check x: %d z: %d Conn: %X", x, z, mpTree[0][x][z].btPathingConns[0]);
                  #endif
                  bFound = true;
                  break;
               }
            }
         }
         else
         {
            if (lXStart < 0 || lXStart > mlTerrainSizeX[0] - 1)
               continue;
            if ((mpTree[0][lXStart][z].btPathingConns[0]) != cConnNone)
            {
               #ifdef DEBUG_FINDCLOSESTPASSABLETILE
               debug("Found it on the left edge x: %d z: %d Conn: %X", lXStart, z, mpTree[0][lXStart][z].btPathingConns[0]);
               #endif
               x = lXStart;
               bFound = true;
            }
            if (lXEnd < 0 || lXEnd > mlTerrainSizeX[0] - 1)
               continue;
            if ((mpTree[0][lXEnd][z].btPathingConns[0]) != cConnNone)
            {
               #ifdef DEBUG_FINDCLOSESTPASSABLETILE
               debug("Found it on the right edge x: %d z: %d Conn: %X", lXEnd, z, mpTree[0][lXEnd][z].btPathingConns[0]);
               #endif
               x = lXEnd;
               bFound = true;
            }
         }
         if (bFound)
            break;
      }
      ++lExpansions;
   }

   // If we didn't find a single passable tile.. bail.
   if (!bFound)
   {
      #ifdef DEBUG_FINDCLOSESTPASSABLETILE
      debug("Didn't find one at all.");
      #endif
      return false;
   }

   // Convert x & z to mid-tile locations.
   float fHalfCell = mfCellSize * 0.5f;
   vResult.x = x * mfCellSize + fHalfCell;
   vResult.z = z * mfCellSize + fHalfCell;
   vResult.y = 0.0f;
#endif
   return true;
}

//==============================================================================
// BLrpTree::isObstructedTile
//==============================================================================
bool BLrpTree::isObstructedTile(const BVector &vPos)
{
  
/*   BTerrainBase *pterrain = game->getWorld()->getTerrain();
   if (!pterrain)
   {
      BASSERT(0);
      return true;
   }*/
   if (vPos.x < 0.0f || vPos.x > gTerrainSimRep.getDataTileScale() * gTerrainSimRep.getNumXDataTiles() ||
       vPos.z < 0.0f || vPos.z > gTerrainSimRep.getDataTileScale() *  gTerrainSimRep.getNumXDataTiles())
       return true;

   long lX1 = (long)(vPos.x * mfRecipCellSize);
   long lZ1 = (long)(vPos.z * mfRecipCellSize);
   if (lX1 < 0 || lX1 > mlSizeX[0] - 1)
      return true;
   if (lZ1 < 0 || lZ1 > mlSizeZ[0] - 1)
      return true;

   if ((mpTree[0][lX1][lZ1].btPassability[0] & cAllInvalid) == cAllInvalid)
      return true;

   return false;

}

//==============================================================================
// BLrpTree::initStats()
//==============================================================================
void BLrpTree::initStats()
{

   QueryPerformanceFrequency(&mQPFreq);
   mlPCCalls = 0L;
   mfPCSum = 0.0f;
   mfPCMax = 0.0f;

   mlFPCalls = 0L;
   mfFPSum = 0.0f;
   mfFPMax = 0.0f;

   mlOLCalls = 0L;
   mfOLSum = 0.0f;
   mfOLMax = 0.0f;

   mlAPCalls = 0L;
   mfAPSum = 0.0f;
   mfAPMax = 0.0f;

   return;

}

//==============================================================================
// BLrpTree::initStats()
//==============================================================================
void BLrpTree::dumpStats()
{

   #ifdef DEBUG_LRPTIMING_STATS
   blog("===============================================");
   blog("LrpTree Stats:");
   blog("Calls to setPathingConnections:   % 8ld", mlPCCalls);
   blog("Avg Time of Call:                 %8.3f", (mlPCCalls)?mfPCSum / (float)mlPCCalls: 0.0f);
   blog("Max Time of Call:                 %8.3f", mfPCMax);
   blog("Total Time in setPathingConns:    %8.3f", mfPCSum);
   blog(".");
   blog("Calls to findPath:                % 8ld", mlFPCalls);
   blog("Avg Time of Call:                 %8.3f", (mlFPCalls)?mfFPSum / (float)mlFPCalls: 0.0f);
   blog("Max Time of Call:                 %8.3f", mfFPMax);
   blog(".");
   blog("Calls to addToOpenList:           % 8ld", mlOLCalls);
   blog("Avg Time of Call:                 %8.3f", (mlOLCalls)?mfOLSum / (float)mlOLCalls: 0.0f);
   blog("Max Time of Call:                 %8.3f", mfOLMax);
   blog("Total Time in addToOpenList:      %8.3f", mfOLSum);
   blog(".");
   blog("Calls to addPathNode:             % 8ld", mlAPCalls);
   blog("Avg Time of Call:                 %8.3f", (mlAPCalls)?mfAPSum / (float)mlAPCalls: 0.0f);
   blog("Max Time of Call:                 %8.3f", mfAPMax);
   blog("Total Time in addPathNode:        %8.3f", mfAPSum);
   blog(".");
   blog("=============================================");
   #endif
}

//==============================================================================
// BLrpTree::markQuadInPath
//==============================================================================
void BLrpTree::markQuadInPath(long n, long x, long z)
{
   long lIndex = 0L;
   for (long m = 0; m < n; m++)
      lIndex += mlSizeGrid[m];

   lIndex += (x * mlSizeZ[n]) + z;

   if (lIndex > mbaExclusions.getNumber() - 1)
   {
      BASSERT(0);
      return;
   }

   mbaExclusions.setBit(lIndex);

   return;

}

//==============================================================================
// BLrpTree::checkQuadInPath
//==============================================================================
bool BLrpTree::checkQuadInPath(long n, long x, long z)
{
   long lIndex = 0L;
   for (long m = 0; m < n; m++)
      lIndex += mlSizeGrid[m];

   lIndex += (x * mlSizeZ[n]) + z;

   if (lIndex > mbaExclusions.getNumber() - 1)
   {
      BASSERT(0);
      return false;
   }
   return (mbaExclusions.isBitSet(lIndex) != 0);

}
//Halwes - 10/9/2006 - Remove recursion
#if !defined( PERF_REMOVE_RECURSION )
//==============================================================================
// BLrpTree::invalidateQuad
//==============================================================================
void BLrpTree::invalidateQuad(long k, long n, long x, long z)
{
   // Mark the specificed quad as invalid, and all of it's children.
   BLrpNode *pnode = &mpTree[n][x][z];
   pnode->btPassability[k] |= cAllInvalid;
   if (n > 0)
   {
      long lNextLevel = n-1;
      long x1 = x << 1;
      long x2 = x1 + 1;
      long z1 = z << 1;
      long z2 = z1 + 1;

      invalidateQuad(k, lNextLevel, x1, z1);
      invalidateQuad(k, lNextLevel, x1, z2);
      invalidateQuad(k, lNextLevel, x2, z1);
      invalidateQuad(k, lNextLevel, x2, z2);

   }

   return;
}
#else // PERF_REMOVE_RECURSION
//==============================================================================
// BLrpTree::pushStack
//
// Very simple array based stack push
//==============================================================================
long BLrpTree::pushStack( nodeData* pStackArray, long stackIndex, const nodeData cStackEntry )
{
   stackIndex++;
   BASSERTM( stackIndex < cMaxDepth, "BLrpTree::pushStack - Stack full!\n" );

   pStackArray[stackIndex] = cStackEntry;

   return( stackIndex );
}

//==============================================================================
// BLrpTree::popStack
//
// Very simple array based stack pop
//==============================================================================
long BLrpTree::popStack( nodeData* pStackArray, long stackIndex )
{
   stackIndex--;

   return( stackIndex < -1 ? -1 : stackIndex );
}

//==============================================================================
// BLrpTree::invalidateQuad
//==============================================================================
void BLrpTree::invalidateQuad(long k, long n, long x, long z)
{
   long       stackIndex = -1;
   nodeData   nodeStack[cMaxDepth];

   // Set and push root node
   nodeData tempNode;
   tempNode.n     = n;
   tempNode.x     = x;
   tempNode.z     = z;
   tempNode.visit = 0;
   stackIndex     = pushStack( nodeStack, stackIndex, tempNode );

   BLrpNode* pNode = NULL;
   
   // Iterate through children nodes
   while( stackIndex >= 0 )
   {      
      if( ( nodeStack[stackIndex].n > 0 ) && ( nodeStack[stackIndex].visit < 4 ) )
      {
         long x1 = nodeStack[stackIndex].x << 1;
         long z1 = nodeStack[stackIndex].z << 1;

         switch( nodeStack[stackIndex].visit )
         {
            // X1Z1
            case 0:                                             
               tempNode.x = x1;
               tempNode.z = z1;
               break;

            // X1Z2
            case 1:
               tempNode.x = x1;
               tempNode.z = z1 + 1;
               break;

            // X2Z1
            case 2:
               tempNode.x = x1 + 1;
               tempNode.z = z1;
               break;

            // X2Z2
            case 3:
               tempNode.x = x1 + 1;
               tempNode.z = z1 + 1;
               break;         
         }                  

         tempNode.n = nodeStack[stackIndex].n - 1;
         nodeStack[stackIndex].visit++;
         stackIndex = pushStack( nodeStack, stackIndex, tempNode );
      }
      else
      {
         // Invalidate node and pop
         pNode                   =  &mpTree[nodeStack[stackIndex].n][nodeStack[stackIndex].x][nodeStack[stackIndex].z];
         pNode->btPassability[k] |= cAllInvalid;
         stackIndex              =  popStack( nodeStack, stackIndex );
      }
   }
}
#endif // PERF_REMOVE_RECURSION

//==============================================================================
// BLrpTree::findClosestPassableTileEx
// New supa expanded version, does a flood fill, flags checked quads by
// setting the cInPath flag.
//==============================================================================
bool BLrpTree::findClosestPassableTileEx(const BVector &vReference, BVector &vResult)
{

   // Converts the world coords to tile locations, searchs surrounding tiles
   // for some number of iterations to find a passable one, and returns
   // true if it found one, false if it ran out of iterations.
   vResult = vReference;
   long lX1 = (long)(vReference.x * mfRecipCellSize);
   long lZ1 = (long)(vReference.z * mfRecipCellSize);

   mNodeStack.setNumber(0);
   uint firstIndex = 0;
   uint numToEval = 0;

   // Put the first on in the stack.. 
   mNodeStack.add(BLrpNodeAddy(0, lX1, lZ1));
   numToEval++;

   // And begin evaluating.
   while (numToEval > 0)
   {
      // Get next to eval.  For speed, just increment the index to the next item.
      // To keep from getting an array that's too large, remove items once threshold reached.
      BLrpNodeAddy addy = mNodeStack[firstIndex];
      if (mNodeStack.getNumber() > cMaxPassableTileArraySize)
      {
         mNodeStack.erase(0, firstIndex + 1);
         firstIndex = 0;
      }
      else
         firstIndex++;
      numToEval--;

      // If Off Terrain, continue
      if (addy.x < 0 || addy.x > mlTerrainSizeX[0] - 1 ||
          addy.z < 0 || addy.z > mlTerrainSizeZ[0] - 1)
         continue;

      // Get node
      BLrpNode *pnode = &mpTree[addy.n][addy.x][addy.z];

      // If we've searched this node already, continue.
      if ((pnode->btStatus & cInPath) == cInPath)
         continue;

      // If this one is valid, set and return.
      if ((pnode->btPassability[0] & cAllInvalid) != cAllInvalid)
      {
         // Convert tile location and place it in vResult
         float fHalfCell = mfCellSize / 2.0f;
         vResult.x = addy.x * mfCellSize + fHalfCell;
         vResult.z = addy.z * mfCellSize + fHalfCell;
         vResult.y = 0.0f;

         // Clear flags.
         for (long x = 0; x < mlSizeX[0]; x++)
            for (long z = 0; z < mlSizeZ[0]; z++)
               mpTree[0][x][z].btStatus &= ~cInPath;

         return true;
      }

      // Mark it.
      pnode->btStatus |= cInPath;

      // Okay, stuff the 8 surrounding positions on the stack.
      mNodeStack.add(BLrpNodeAddy(0, addy.x - 1, addy.z - 1));
      mNodeStack.add(BLrpNodeAddy(0, addy.x,     addy.z - 1));
      mNodeStack.add(BLrpNodeAddy(0, addy.x + 1, addy.z - 1));
      mNodeStack.add(BLrpNodeAddy(0, addy.x - 1, addy.z));
      mNodeStack.add(BLrpNodeAddy(0, addy.x + 1, addy.z));
      mNodeStack.add(BLrpNodeAddy(0, addy.x - 1, addy.z + 1));
      mNodeStack.add(BLrpNodeAddy(0, addy.x,     addy.z + 1));
      mNodeStack.add(BLrpNodeAddy(0, addy.x + 1, addy.z + 1));
      numToEval += 8;
   }
      
   // Clear flags.
   for (long x = 0; x < mlSizeX[0]; x++)
      for (long z = 0; z < mlSizeZ[0]; z++)
         mpTree[0][x][z].btStatus &= ~cInPath;

   return false;

}

//==============================================================================
// BLrpTree::checkAndSearch
// Recursive flood fill check.
//==============================================================================
bool BLrpTree::checkAndSearch(long lX1, long lZ1, BVector &vResult)
{

   // If Off Terrain, return immediately.
   if (lX1 < 0 || lX1 > mlTerrainSizeX[0] - 1 ||
      lZ1 < 0 || lZ1 > mlTerrainSizeZ[0] - 1)
      return false;

   // If we've checked this one already, return immediately.
   if ((mpTree[0][lX1][lZ1].btStatus & cInPath) == cInPath)
      return false;

   // Check our tile.
   if ((mpTree[0][lX1][lZ1].btPassability[0] & cAllInvalid) != cAllInvalid)
   {
      // Convert tile location and place it in vResult
      float fHalfCell = mfCellSize / 2.0f;
      vResult.x = lX1 * mfCellSize + fHalfCell;
      vResult.z = lZ1 * mfCellSize + fHalfCell;
      vResult.y = 0.0f;
      return true;
   }

   // Set our Tile.  
   mpTree[0][lX1][lZ1].btStatus |= cInPath;

   // Check the eight surrounding tiles.
   if (checkAndSearch(lX1-1,lZ1-1, vResult))
      return true;
   if (checkAndSearch(lX1,  lZ1-1, vResult))
      return true;
   if (checkAndSearch(lX1+1,lZ1-1, vResult))
      return true;
   if (checkAndSearch(lX1-1,lZ1,   vResult))
      return true;
   if (checkAndSearch(lX1+1,lZ1,   vResult))
      return true;
   if (checkAndSearch(lX1-1,lZ1+1, vResult))
      return true;
   if (checkAndSearch(lX1,  lZ1+1, vResult))
      return true;
   if (checkAndSearch(lX1+1,lZ1+1, vResult))
      return true;

   return false;
}


//==============================================================================
// BLrpTree::getTileConnctions
// Pass in a world coord x & z, and get back the connection info for that tile.
//==============================================================================
long BLrpTree::getTileConnections(float fX, float fZ)
{
   long lX1 = (long)(fX * mfRecipCellSize);
   long lZ1 = (long)(fZ * mfRecipCellSize);

   // If Off Terrain, return immediately.
   if (lX1 < 0 || lX1 > mlTerrainSizeX[0] - 1 ||
      lZ1 < 0 || lZ1 > mlTerrainSizeZ[0] - 1)
      return (long)cConnNone;

   // Get the pathing connections for bucket zero at the specified location
   long lPathingConns = (long)mpTree[0][lX1][lZ1].btPathingConns[0];

   return lPathingConns;

};

//==============================================================================
// BLrpTree::adjustForRadius
//==============================================================================
void BLrpTree::adjustForRadius(const BVector &vClosest, const BVector &vGoal, BVector &vAdjClosest)
{
   BVector vDir = vGoal - vClosest;
   if (vDir.length() < cFloatCompareEpsilon)
      vAdjClosest = vClosest;
   else
   {
      vDir.normalize();
      // We use an ADJUSTED radius, because this is what the sim (BUnit::inRange) uses to
      // make radius to radius checks.
      float fRadius1 = cSqrt2*mfRadius;
      vAdjClosest = vClosest - (vDir * fRadius1);
   }
   return;

}

//==============================================================================
//==============================================================================
void BLrpTree::resetStatics()
{
   mNodeFreeList.clear();
   mOpenList.clear();
   mClosedList.clear();
   mFixupList.clear();
   mNodeStack.clear();
   mStaticHull.releaseMemory();
   #ifndef BUILD_FINAL
      mDebugPath.reset(true);
   #endif
   sTempObs.clear();
   sSegResults.clear();
}

//==============================================================================
// eof: lrptree.cpp
//==============================================================================

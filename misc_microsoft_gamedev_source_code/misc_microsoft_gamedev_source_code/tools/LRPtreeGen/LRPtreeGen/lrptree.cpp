//==============================================================================
// lrptree.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes

//#include "common.h"
//#include "convexhull.h"
//#include "entity.h"
//#include "game.h"
#include "lrptree.h"
#include "obstructionmanager.h"
//#include "path.h"
//#include "pather.h"
//#include "syncmacros.h"
//#include "world.h"
//#include "unit.h"
#include "terrainsimrep.h"
//#include "terrain.h"

// debug stuff
//#include "boundingbox.h"
//#include "usermanager.h"
//#include "user.h"
//#include "render.h"
//#include "gamedirectories.h"

// xcore
//#include "consoleOutput.h"
//#include "resource\ecfUtils.h"

// xsystem
//#include "xsystem.h"
#include "bfileStream.h"
//#include "convexhull.h"

#include "file\win32FileStream.h"

//==============================================================================
// Defines

// Debug aids only
#ifndef BUILD_FINAL
  #define DEBUG_LRPTREE_GRAPHPATHS
#endif

#define DEBUG_LISTEXCLUSIONS

const long cLrpRenderingMode = 0;
const long cLrpPathArrayInitSize = 4096L;
const long cLrpMaxTargetExpansion = 3;
const long clIterationCap = 2048;
const long clNodeStackInitSize = 512;
const long clMaxFindExpansions = 4;


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
   mpTree = new BLrpNode **[cMaxDepth];
   if (!mpTree)
   {
      BASSERT(0);
      return false;
   }

   // Allocate the rows & columns
   long lNodeCount = 0L;
   for (long n = 0; n < cMaxDepth; n++)
   {
      mlSizeX[n] = lXCurrent;
      mlSizeZ[n] = lZCurrent;
      mlSizeGrid[n] = mlSizeX[n] * mlSizeZ[n];
      mpTree[n] = new BLrpNode *[lXCurrent];

      BYTE* tempNodeGrid = new BYTE[sizeof(BLrpNode) * lXCurrent * lZCurrent];
      if (!mpTree[n])
      {
         BASSERT(0);
         return false;
      }
      for (long x = 0; x < lXCurrent; x++)
      {
         mpTree[n][x] = (BLrpNode*) tempNodeGrid;
         tempNodeGrid += lZCurrent * sizeof(BLrpNode);
         //         mpTree[n][x] = new BLrpNode[lZCurrent];
         if (!mpTree[n][x])
         {
            BASSERT(0);
            return false;
         }
         lNodeCount += lZCurrent;
         memset(mpTree[n][x], '\0', sizeof(BLrpNode) * lZCurrent);
      }
      lXCurrent >>= 1;
      lZCurrent >>= 1;
   }

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
         for (long x = 0; x < mlSizeX[n]; x++)
         {
            delete [] mpTree[n][x];
            mpTree[n][x] = NULL;
         }
         delete [] mpTree[n];
         mpTree[n] = NULL;
      }
      delete mpTree;
      mpTree = NULL;
   }

   mlTreeSizeX = mlTreeSizeZ = 0;
   mfCellSize = 0.0f;
   mfRecipCellSize = 0.0f;
   mbInitialized = false;

} // BLrpTree::reset


//==============================================================================
// BLrpTree::saveECFLRP
//==============================================================================
bool BLrpTree::saveECFLRP(const char* fileNameLRP)
{
   if (!mbInitialized)
      return false;

   BECFFileBuilder ecfBuilder;
   ecfBuilder.setID(cLRP_ECFFileID);

   int version =cLRPVersion;
   ecfBuilder.addChunk(cLRPHeaderID,(BYTE*)&version,sizeof(int));


   if (mpTree)
   {
      for (uint8 i=0; i<cMaxDepth; i++ )
         ecfBuilder.addChunk(i+1, (BYTE*) *(mpTree[i]), sizeof(BLrpNode) * mlSizeX[i] * mlSizeZ[i]);
   }
   else
      return false;

   BWin32FileStream fileStream;

   if (!fileStream.open(fileNameLRP, cSFWritable ))
      return false;

   ecfBuilder.writeToStream(fileStream);

   return true;

} // BLrpTree::saveECFLRP


//==============================================================================
// BLrpTree::addObstruction
//==============================================================================
bool BLrpTree::addObstruction(BOPObstructionNode *pObNode)
{
   // Get the Player Mask from THE node.
   long lPlayerMask = 0x0000;
/*
   if (pObNode->mProperties & BObstructionManager::cObsPropertyMultiPlayerOwned)
   {
      //BUnit *pUnit = pObNode->mUnit;
      // newer safer mode -- dlm 7/24/02
      BEntity *pUnit = mpObMgr->getObject(pObNode);
      if (pUnit)
         lPlayerMask = ObsSystemPlayerMasks[pUnit->getPlayerID()];
   }
*/

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


//==============================================================================
// BLrpTree::removeObstruction
//==============================================================================
bool BLrpTree::removeObstruction(long lX1, long lZ1, long lX2, long lZ2)
{
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

   BObstructionNodePtrArray &obstructions = mpObMgr->getFoundObstructionResults();
   // NOTE: These parameters have already been passed to begin.. it'd be nice to not have to pass them again.
   if (mlObMgrOptions == 0L)
   {
      BASSERT(0);
      return false;
   }
   mpObMgr->findObstructions(mStaticHull, 0.0f, mlObMgrOptions, BObstructionManager::cObsNodeTypeAllSolid,
      BObstructionManager::cNoPlayerOwnedIgnores, false, obstructions);

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
         if (!addObstruction(pObNode))
            return false;
      }
   }         

   return true;
}


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

/*
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

   static BObstructionNodePtrArray tempObs;
   static BSegIntersectResultArray segResults;

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
         BVector vDir(0.0f);
         float fScale = 0.0f;
         bool bBothOutside = false;

         vExteriorPoints[m].x = -1;

         long lInIdx = -1;
         long lOutIdx = -1;

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
            mpObMgr->findObstructions(mStaticPoints[lOutIdx], false, false, tempObs);
            if (tempObs.getNumber() == 0)
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
                  mStaticPoints[lOutIdx], mStaticPoints[lInIdx], false, segResults);
            
               // Find the closest Intersect hull that's not a hull the outside point is within.
               long lNumInts = segResults.getNumber();
               long lNumObs = tempObs.getNumber();
               float fBestDistSqr = cMaximumFloat;
               // FoundCurrHull is a hack to handle the case where "inside" reports that
               // the point at lInIdx is inside our current hull, but "segIntersect" between lOutIdx &
               // lInIdx doesn't find our current hull.
               bool bFoundCurrHull = false;
               for (long i = 0; i < lNumInts; i++)
               {
                  if (segResults[i].pObNode == pObNode)
                     bFoundCurrHull = true;
                  long j;
                  for (j = 0; j < lNumObs; j++)
                     if (segResults[i].pObNode == tempObs[j])
                        break;
                  if (j != lNumObs)
                     continue;

                  if (segResults[i].fDistSqr < fBestDistSqr)
                  {
                     fBestDistSqr = segResults[i].fDistSqr;
                  }
               }
               if (fBestDistSqr == cMaximumFloat)
               {
                  if (!bFoundCurrHull)
                     fBestDistSqr = mfCellSize * mfCellSize;
               }
               if (fBestDistSqr < cMaximumFloat)
               {
                  tempObs.setNumber(0);
                  tempObs.add(pObNode);

                  fScale = (float)sqrt((double)fBestDistSqr) - 0.05f;
                  vDir = mStaticPoints[lInIdx] - mStaticPoints[lOutIdx];
                  vDir.normalize();
                  vDir *= fScale;
                  vExteriorPoints[m] = mStaticPoints[lOutIdx] + vDir;
                  // Get the intersection point, and see if it's inside any other hulls..
                  mpObMgr->findObstructions(vExteriorPoints[m], false, true, tempObs);
                  if (tempObs.getNumber() > 1)
                     pnode->btPathingConns[0] &= ~lEdges[m];
               }
            
            }
         }
         else
         {
            // Do something with both outside..
         }

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


//==============================================================================
// eof: lrptree.cpp
//==============================================================================

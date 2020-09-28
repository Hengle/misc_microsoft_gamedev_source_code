//=============================================================================
// Copyright (c) 1999 Ensemble Studios
//
// Obstruction manager
//=============================================================================

//=============================================================================
// Includes
#include "common.h"
#include "obquadnode.h"
#include "maximumsupportedplayers.h"
#include "TerrainSimRep.h"
#include "unit.h"
#include "chunker.h"
#include "pointin.h"
#include "world.h"
#include "protoobject.h"
#include "segintersect.h"
#include "mathutil.h"
#include "database.h"
#include "protoobject.h"
#include "squad.h"
#include "pather.h"
#include "terrain.h"
#include "configsgame.h"

// xcore
#include "math\fastFloat.h"

// xgamerender
#include "BoundingSphere.h"
#include "boundingBox.h"

// xrender
#include "debugprimitives.h"
#include "render.h"

//Use the player passible definition for warthog jumping instead
#define CANJUMP_CHANGE

BObstructionManager gObsManager;

//DWORD OBrenderColor[BObstructionManager::cNumQuadTrees] = {cDWORDCyan, cDWORDBlue, cDWORDGreen, cDWORDRed, cDWORDMagenta, cDWORDBlack, cDWORDGreen, cDWORDGreen, cDWORDRed, cDWORDPurple, cDWORDGreen};
DWORD OBrenderColor[BObstructionManager::cNumQuadTrees] = {cDWORDCyan, cDWORDBlue, cDWORDGreen, cDWORDRed, cDWORDMagenta};

long ObsSystemPlayerMasks[cMaximumSupportedPlayers];

//#define DEBUGGRAPH_SUGGESTPLACEMENT

//=============================================================================
// Array of Cos & Sin Values for Hull Expansion
//=============================================================================
bool gExpandAngleTableInititalized=false;
float gExpandCOS[256];
float	gExpandSIN[256];

void initObstructionRotationTables(void)
{
   // Don't do this twice.
   if(gExpandAngleTableInititalized)
      return;

   // Build the table.
	double Sqrt2   = 1.4142135623730950488016887242097;
	double PiOver4 = 0.78539816339744830961566084581988;

	for (long n = 0; n < 128; n++)
	{
		// Scale fisrt to -64 .. + 64, then to -1.0f ... +1.0f
		double cosTheta = (double ((128 - n) - 64)) / 64.0;

		// Get the angle in Radians
		double Theta = acos(cosTheta);

		// Rotate 45 degrees  clockwise
		double ExpandTheta = Theta - PiOver4;

		// Multipley by sqrt(2) to get correct x/z amounts

		gExpandCOS[n] = (float) (cos(ExpandTheta) * Sqrt2);
		gExpandSIN[n] = (float) (sin(ExpandTheta) * Sqrt2);
	}

	for (long n = 128; n < 256; n++)
	{
		gExpandCOS[n] = -gExpandCOS[n-128];
		gExpandSIN[n] = -gExpandSIN[n-128];
	}

   // Mark this as initialized.
   gExpandAngleTableInititalized=true;
}


//=============================================================================
// Updated constants
//=============================================================================



const float	BObstructionManager::cWorldObstructionDensityFactor = 0.10f;			// factor to estimate # of objects in world based on number of tiles

const long  BObstructionManager::cMetersPerSmallestQuadCellSide = 16;				// Size of smallest quad node in meters (must be ^ of 2?)
const long  BObstructionManager::cTilesPerSmallestQuad = 4;								// Size of the smallest quad node in Tiles
const long  BObstructionManager::cMetersToQuadCellShift = 4;							// This the N in 2^N = cMetersPerSmallestQuadCellSide; 
const long  BObstructionManager::cMetersToQuadCellULANDMask = 0x7FFFFFF0;			// Mask to round down a coord in meters to the Upper/Left edge of a quad cell
const long  BObstructionManager::cMetersToQuadCellLRORMask  = 0x0000000F;		   // Mask to round up a coord in meters to the Lower/Right edge of a quad cell
const float BObstructionManager::cRoundUpFactor = 0.98f;									// Used to round up floats to next highest integer
const	float	BObstructionManager::cEdgeOfWorldThickness = 0.02f;						// Thickness of Edge of World Obstructions

const float BObstructionManager::cNodeDropDownFactor = 0.00001f;						// Adjustment to make tile aligned obstructions "drop" to a lower node
const float cOnSegmentEpsilon = 0.01f;

//=============================================================================
// Consts

const float          cfDepth = 0.75;

//=============================================================================
// static float BObstructionManager::getWaterPathingDepth
//=============================================================================
float BObstructionManager::getWaterPathingDepth()
{
   return(cfDepth);
}


// New Obstruction system values to replace the above 4...

long BObstructionManager::cDefaultQuadTreesToScanforLand  = BObstructionManager::cIsNewTypeBlockLandUnits;						
long BObstructionManager::cDefaultQuadTreesToScanforFlood  = BObstructionManager::cIsNewTypeBlockFloodUnits;						
long BObstructionManager::cDefaultQuadTreesToScanforScarab  = BObstructionManager::cIsNewTypeBlockScarabUnits;						
		

long BObstructionManager::cDefaultQuadTreesToScanforHover = BObstructionManager::cIsNewTypeBlockAllMovement;

//=============================================================================
// BObstructionManager::BObstructionManager
//=============================================================================
BObstructionManager::BObstructionManager(void) :
   mTileWidth(0),
   mTileHeight(0),
   mWidth(0),
   mHeight(0),
   mfWidth(0.0f),
   mfHeight(0.0f),
   mfTileWidth(0.0f),
   mfTileHeight(0.0f),
   mfTileSize(0.0f),
   mfConvertToTiles(0.0f),
   mNumQuadTreeLevels(0),
	mNumQuadTreeNodes(0),
   mNodeManager(NULL),
   mListManager(NULL),
   mHullManager(NULL),
   //mThePather(NULL),
	mQuadTreesToScan(0),
   mUpdatePlayerRelationMasks(false),
   mInUse(false),
   mRadius(0.0f),
   mRelaxedRadius(0.0f),
   mMode(cBeginNone),
   mValidNodeTypes(0),
   mlPlayerID(0),
   mbCanJump(false)
{
  long n;

   for (n = 0; n < cNumQuadTrees; n++)
   {
		mQTLevelInfo[n] = NULL;	
   	mQuadTreeNodes[n] = NULL;
   }

   for (n = 0; n < cMaxQuadTreeSizeinCells; n++)
   {
		mNodeLevelLookup[n] = 0;
   }
		
   for (n = 0; n < cMaximumSupportedPlayers; n++)
   {
		ObsSystemPlayerMasks[n] = 0;
   }
	
   // jce [7/8/2002] -- table is global and initialized from game startup now.
	//initRotationTables();

	mExpandCheckoutList.setNumber(4096);
	mExpandCheckoutList.setNumber(0);

	// Setup temportary storage spaces

	mFoundObstructionResults.setNumber(512);

}

//=============================================================================
// BObstructionManager::~BObstructionManager
//=============================================================================
BObstructionManager::~BObstructionManager(void)
{
   #ifdef DEBUG_DUMPQUADONDTOR
   dumpQuadTree();
   #endif

   deinitialize();

}

//=============================================================================
// BObstructionManager::initialize
//=============================================================================
bool BObstructionManager::initialize(void)
{
   SCOPEDSAMPLE(BObstructionManager_initialize)
   mfTileSize = gTerrainSimRep.getDataTileScale();
   mfConvertToTiles = 1.0f / mfTileSize;
	
   mTileWidth  = gTerrainSimRep.getNumXDataTiles();
	mTileHeight = mTileWidth;

	mfTileWidth  = (float) mTileWidth;
	mfTileHeight = (float) mTileHeight;

	mfWidth = mfTileWidth * mfTileSize;
	mfHeight = mfTileHeight * mfTileSize;

	mWidth  = (long) (mfWidth + cRoundUpFactor);
	mHeight = (long) (mfHeight + cRoundUpFactor);

	// Setup the Obstruction node memory manager

	float numTiles = mfTileWidth * mfTileHeight;
	long numObs = (long) (numTiles * cWorldObstructionDensityFactor);

	mNodeManager = new BOPObNodeArrayManager(numObs, numObs);				

	// Get other needed pointers

	//mThePather = theWorld->getPather();
	
	// Determine how big we need to make the quadtree
	// Remember maps csn be non square

	long maxDimX2 = max(mWidth, mHeight);
	maxDimX2 *= 2;

	// Get smallest power of 2 size that is larger than map (in meters)

	long cellsize = 8192;
	
	while (cellsize > maxDimX2)
	{
		cellsize>>=1;
	}

	long QuadTreeSize = cellsize;

	// Now we have one large node that encompases the whole map 
	// We subdivide it until we hit the minimum size for a quadnode

	mNumQuadTreeLevels = 0;

	long nodeCount = 1;
	mNumQuadTreeNodes = 0;
	long treeSizeinMeters = cellsize;


	while (cellsize >= cMetersPerSmallestQuadCellSide)
	{
		mNumQuadTreeNodes += nodeCount;			// Start @ top level, total up quad nodes
		mNumQuadTreeLevels++;

		nodeCount *= 4;							// 4 times the nodes per level :-)
		cellsize>>= 1;						// but only 1/2 the distance per side
	}

	// Now we set up the acutal Quad Tree Stuff, The Level Info and the Node Array

	for (long qNum = 0; qNum < cNumQuadTrees; qNum++)
	{
		// Allocate storage for the Level Info

		mQTLevelInfo[qNum] = HEAP_NEW_ARRAY(BOPQuadTreeLevelInfo, mNumQuadTreeLevels, gSimHeap);

		// Allocate Storage for the Node Data

		mQuadTreeNodes[qNum] = HEAP_NEW_ARRAY(BOPQuadTreeNode, mNumQuadTreeNodes, gSimHeap);


		// Fill Out the Level Info For each level

		long n, p, nodes;
		for (n = 0, p = 0, nodes = 1; n < mNumQuadTreeLevels; n++ )
		{
			mQTLevelInfo[qNum][n].mLevel = n;
			mQTLevelInfo[qNum][n].mShiftCount = cMetersToQuadCellShift + (mNumQuadTreeLevels-1) - n;	

			mQTLevelInfo[qNum][n].mScaleZShift = QuadTreeSize >> n;
			mQTLevelInfo[qNum][n].mQTNodeIndex = p;
			mQTLevelInfo[qNum][n].mQTNodes     = &(mQuadTreeNodes[qNum][p]);

		
			mQTLevelInfo[qNum][n].mMetersPerNode = treeSizeinMeters >> n;
			mQTLevelInfo[qNum][n].mfMetersPerNode = (float) (treeSizeinMeters >> n);

			p+= nodes;
			nodes*=4;
		}

		// Initialize each node of the quadtree;

		for (n = 0; n < mNumQuadTreeNodes; n++)
		{
			mQuadTreeNodes[qNum][n].mObstructionList = NULL;
			mQuadTreeNodes[qNum][n].mNumObstructions = 0;
			mQuadTreeNodes[qNum][n].mListType = BOPObQuadNodeListManager::cInvalidNodeListType;

		}
	}

	// Now fill out the Node Level Quick Index Array

	for (long z = 0; z < cMaxQuadTreeSizeinCells; z++)
	{
		long n = mNumQuadTreeLevels - 1;		// Start at top level
		long w = z;
		while (w > 0)
		{
			n--;
			w>>=1;
		}
		if (n < 0) 
		{
			n = -1;
		}
		mNodeLevelLookup[z] = n;
	}

	// Do We flag the Quad Nodes for Edge of Map Objects?

	// Now we setup the Pointer List Manager


	long numLowLevelQuadNodes = (mNumQuadTreeNodes + (mNumQuadTreeNodes>>1) + 1) >> 1;

	mListManager = new BOPObQuadNodeListManager(numLowLevelQuadNodes);						// Our QuadTree obstruction List Manager

	// Setup the Expanded Hull Managet

	mHullManager = new BOPHullArrayManager(1024, 1024);											


	// Updated masks for player owned obstructions

	updatePlayerOwnedMasks();
   enablePathingUpdatesAndRebuild();

   return(true);
}




//=============================================================================
// BObstructionManager::deinitialize
//=============================================================================
void BObstructionManager::deinitialize(void)
{
   releaseExpandedHulls();

	// Delete Obstructions First

	if (mNodeManager != NULL)
	{
		delete mNodeManager;
		mNodeManager = NULL;
	}

	// Then any Expanded Hulls

	if (mHullManager != NULL)
	{
		delete mHullManager;
		mHullManager = NULL;
	}


	// Then the Quad Node Lists

	if (mListManager != NULL)
	{
		delete mListManager;
		mListManager = NULL;
	}

	// Finally the Quad Tree Nodes and their info structs

	for (long n= 0; n < cNumQuadTrees; n++)
	{
		if (mQTLevelInfo[n] != NULL)
		{
			HEAP_DELETE_ARRAY(mQTLevelInfo[n], gSimHeap);
			mQTLevelInfo[n] = NULL;
		}

		if (mQuadTreeNodes[n] != NULL)
		{
			HEAP_DELETE_ARRAY(mQuadTreeNodes[n], gSimHeap);
			mQuadTreeNodes[n] = NULL;
		}
	}


	// All Done


}


//==============================================================================
// BObstructionManager::disablePathingUpdates
//==============================================================================
void BObstructionManager::disablePathingUpdates(void)
{
gPather.enableQuadUpdate(false);
/*	if (mThePather)
	{
	   mThePather->enableQuadUpdate(false);
	}*/

}


//==============================================================================
// BObstructionManager::EnablePathingUpdatesAndRebuild
//==============================================================================
void BObstructionManager::enablePathingUpdatesAndRebuild(void)
{
   SCOPEDSAMPLE(BObstructionManager_enablePathingUpdatesAndRebuild)

   gPather.buildPathingQuads(this);
   gPather.enableQuadUpdate(true);


	/*if (mThePather)
	{
      mThePather->buildPathingQuads(this);

      mThePather->enableQuadUpdate(true);
   }*/

}

//==============================================================================
// BObstructionManager::generateWorldBoundries
//==============================================================================
void BObstructionManager::generateWorldBoundries(void)
{
   // jce [10/3/2008] -- changing this to support playable bounds more efficiently.  Basically we're going to
   // not update the LRP with these obstructions and add a check during the pather loop for out of bounds.  This
   // simple check will add a tiny bit of runtime but won't incur the hundreds+ of milliseconds to update the pathing
   // quadtrees when the bounds change.

	// First, Remove all existing world boundary elements
	emptyVirtualQuadtreeofDataType(cObsTypeBlockAllMovement, cObsNodeTypeEdgeofMap);

	float edgeThickness = 16.0f;

   int minX=(int)gWorld->getSimBoundsMinX();
   int minZ=(int)gWorld->getSimBoundsMinZ();
   int maxX=(int)gWorld->getSimBoundsMaxX();
   int maxZ=(int)gWorld->getSimBoundsMaxZ();

   // Create Edge of World Obstructions along top and bottom sides of the world
   // DLM - converting this back to making multiple obstructions.  Please do not change these into
   // long obstructions that run the length of the world.  You will DESTROY the pather if you do so.  
	//for (long x = minX; x < maxX; x+=cMetersPerSmallestQuadCellSide)
	// jce [10/6/2008] -- extend this row to fill in corners with obstructions
	for (long x = minX-cMetersPerSmallestQuadCellSide; x < maxX+cMetersPerSmallestQuadCellSide; x+=cMetersPerSmallestQuadCellSide)
	{
		BOPObstructionNode* EdgeOb1 = mNodeManager->getNode();
      //mpEdgeOb[0] = EdgeOb1;
		resetObstructionNode(EdgeOb1);
		setAlignedObstruction(EdgeOb1, (float)x, minZ-edgeThickness, (float) min((x+cMetersPerSmallestQuadCellSide), maxX), minZ+cEdgeOfWorldThickness);
		EdgeOb1->mType = cObsNodeTypeEdgeofMap;
		//EdgeOb1->mProperties |= cObsPropertyUpdatePatherQuadTree;

		installObjectObstruction(EdgeOb1, cObsTypeBlockAllMovement);

		BOPObstructionNode* EdgeOb2 = mNodeManager->getNode();
      //mpEdgeOb[1] = EdgeOb2;
		resetObstructionNode(EdgeOb2);
		setAlignedObstruction(EdgeOb2, (float)x, (float) maxZ-cEdgeOfWorldThickness, (float) min((x+cMetersPerSmallestQuadCellSide), maxX), (float) maxZ+edgeThickness);
		EdgeOb2->mType = cObsNodeTypeEdgeofMap;
		//EdgeOb2->mProperties |= cObsPropertyUpdatePatherQuadTree;

		installObjectObstruction(EdgeOb2, cObsTypeBlockAllMovement);
	}

	// Create Edge of World Obstructions along left and right sides of the world
	for (long z = minZ; z < maxZ; z+=cMetersPerSmallestQuadCellSide)
	{
		BOPObstructionNode* EdgeOb1 = mNodeManager->getNode();
      //mpEdgeOb[2] = EdgeOb1;
		resetObstructionNode(EdgeOb1);
		setAlignedObstruction(EdgeOb1, minX-edgeThickness, (float)z, minX+cEdgeOfWorldThickness, (float) min((z+cMetersPerSmallestQuadCellSide), maxZ) );
		EdgeOb1->mType = cObsNodeTypeEdgeofMap;
		//EdgeOb1->mProperties |= cObsPropertyUpdatePatherQuadTree;

		installObjectObstruction(EdgeOb1, cObsTypeBlockAllMovement);

		BOPObstructionNode* EdgeOb2 = mNodeManager->getNode();
      //mpEdgeOb[3] = EdgeOb2;
		resetObstructionNode(EdgeOb2);
		setAlignedObstruction(EdgeOb2,  (float) maxX-cEdgeOfWorldThickness, (float)z, (float) maxX+edgeThickness, (float) min((z+cMetersPerSmallestQuadCellSide), maxZ) );
		EdgeOb2->mType = cObsNodeTypeEdgeofMap;
		//EdgeOb2->mProperties |= cObsPropertyUpdatePatherQuadTree;

		installObjectObstruction(EdgeOb2, cObsTypeBlockAllMovement);
	}


	// Need to re-enable quadpather updating

}





//==============================================================================
// BObstructionManager::generateWorldBoundries
//==============================================================================
void BObstructionManager::resetObstructionNode(BOPObstructionNode* theNode)
{

	theNode->mObject = NULL;
	theNode->mThisNodeIndex = -1;
	theNode->mThisNodePointerIndex = -1;
	theNode->mThisNodeQuadTree = -1;

	theNode->mExpandedHull = NULL;
	theNode->mEntityID = -1;
	theNode->mNodeProperties = 0;
	theNode->mBoundingIndicies = -1;

}



//==============================================================================
// BObstructionManager::emptyVirtualQuadtree
//==============================================================================
//
// Deallocates all nodes from a virtual QuadTree
// This does not take care of any external entity with links to the lists or items
//
void BObstructionManager::emptyVirtualQuadtree(const long ObsType)
{

	BOPQuadTreeNode*	NodeList = mQuadTreeNodes[ObsType];
	
	// Just rip through all the nodes

	for (long n = 0; n < mNumQuadTreeNodes; n++)
	{
		long numObs = NodeList[n].mNumObstructions;
		if (numObs > 0)											// Any valid nodes in it's list?
		{
			BOPObstructionNode** p = NodeList[n].mObstructionList;	
			for (long z = 0; z < numObs; z++, p++)			// If so, release every node in the list
			{
				mNodeManager->freeNode(*p);
			}
			// Now, reset the list
			mListManager->freeList(NodeList[n].mObstructionList, NodeList[n].mListType);

			NodeList[n].mObstructionList = NULL;
			NodeList[n].mNumObstructions = 0;
			NodeList[n].mListType = BOPObQuadNodeListManager::cInvalidNodeListType;
		}

	}

}

//==============================================================================
// BObstructionManager::emptyVirtualQuadtreeofDataType
//==============================================================================
void BObstructionManager::emptyVirtualQuadtreeofDataType(const long obsType, const long nodeTypes)
{


	if (nodeTypes == cObsNodeTypeAll)
	{
		emptyVirtualQuadtree(obsType);
		return;
	}

	BOPQuadTreeNode*	NodeList = mQuadTreeNodes[obsType];
	
	// Just rip through all the nodes

	for (long n = 0; n < mNumQuadTreeNodes; n++)
	{
		long numObs = NodeList[n].mNumObstructions;
		if (numObs > 0)											// Any valid nodes in it's list?
		{
			BOPObstructionNode** p = NodeList[n].mObstructionList;	
			for (long z = numObs-1; z >= 0; z--)			// If so, release every node in the list
			{
				BOPObstructionNode* theNode = p[z];

				if (nodeTypes == cObsNodeTypeAll || (theNode->mType & nodeTypes) != 0)  // Node Types Match
				{
               // Let the pather know....
               if (theNode->mProperties & cObsPropertyUpdatePatherQuadTree)
                  gPather.updatePathingQuad(this, theNode, false);

					mNodeManager->freeNode(theNode);
					removeNodeFromList(&NodeList[n], z);

               // ajl 9/4/07 - the mObstructionList variable can change during the call to removeNodeFromList,
               // so reset the temp "p" var here.
               p = NodeList[n].mObstructionList;
				}
			}
		}
	}

}


//==============================================================================
// BObstructionManager::emptyVirtualQuadtreeNodes
//==============================================================================
// Coordinates are in meters
//
void BObstructionManager::emptyVirtualQuadtreeNodes(const long obsType, float x1, float z1, float x2, float z2, const long nodeTypes)
{

	BOPQuadTreeLevelInfo*	LevelInfo = mQTLevelInfo[obsType];

	long Level;
	long X1, X2, Z1, Z2;
	long x, z;
	long startX, startZ, endX, endZ;
	bool flushAll;
	float	fX1, fX2, fZ1, fZ2;
	
	X1 = (long) x1;
	X2 = (long) (x2 - cNodeDropDownFactor);

	Z1 = (long) z1;
	Z2 = (long) (z2- cNodeDropDownFactor);

	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidX(X2);
	makeValidZ(Z1);
	makeValidZ(Z2);


   Level = 0;     // Scan *all* levels of the quadtree.. ;) 

	while (Level < mNumQuadTreeLevels)
	{

		startX = X1 >> LevelInfo[Level].mShiftCount;
		startZ = Z1 >> LevelInfo[Level].mShiftCount;

		endX = X2 >> LevelInfo[Level].mShiftCount;
		endZ = Z2 >> LevelInfo[Level].mShiftCount;

		// look at all the nodes

		for (x = startX; x <= endX; x++)
		{
			for (z = startZ; z <= endZ; z++)
			{
				// Determine which Node we are in..

				long NodeIndex = mQTLevelInfo[obsType][Level].mQTNodeIndex + (z << Level) + x;

				// Get Pointer to Quad Node

				BOPQuadTreeNode* curNode = &mQuadTreeNodes[obsType][NodeIndex];

				if (curNode->mNumObstructions <= 0)		// Skip if empty node
				{
					continue;
				}
				
				// Now see if the node is fully inside the update region

				flushAll = true;
				long shift = mQTLevelInfo[obsType][Level].mShiftCount;

				// compute bounding area of node in meters

				fX1 = (float) (x << shift);
				fX2 = (float) ((x+1) << shift);

				fZ1 = (float) (z<< shift);
				fZ2 = (float) ((z+1) << shift);

				// Not at a bottom quad node?
				// Then check to see if the entire node is inside the update area

				if (Level < mNumQuadTreeLevels-1)
				{
					if ( fX1 < x1 || fX2 > x2 || fZ1 < z1 || fZ2 > z2 )
					{
						flushAll = false;
					}
				}

				// Now either just delete everything, or check each obstruction in the node
            // You can't just delete everything, because you still need to do the type check
            // against the obsNodeType.. dlm 2/4/02
				if (flushAll)		// Delete all obstructions in the quad node
				{
					for (long n = curNode->mNumObstructions -1; n >= 0; n--)
					{
						BOPObstructionNode* obsData = curNode->mObstructionList[n];

						// is the obsturction fully in the deleta area?
						if (nodeTypes == cObsNodeTypeAll || (obsData->mType & nodeTypes) != 0 )		// Node Types Match?
						{
							mNodeManager->freeNode(obsData);
							removeNodeFromList(curNode, n);
						}
					}
						
				}
				else		// Delete obstructions in the node that are inside the bounding coordinates
				{
					// Scan each individual obstruction

					for (long n = curNode->mNumObstructions -1; n >= 0; n--)
					{
						BOPObstructionNode* obsData = curNode->mObstructionList[n];

						// is the obsturction fully in the deleta area?

						if (obsData->mDirectVal[obsData->mIdxMinX] >= x1 &&
							 obsData->mDirectVal[obsData->mIdxMaxX] <= x2 &&
							 obsData->mDirectVal[obsData->mIdxMinZ] >= z1 &&
							 obsData->mDirectVal[obsData->mIdxMaxZ] <= z2)
						{
							if (nodeTypes == cObsNodeTypeAll || (obsData->mType & nodeTypes) != 0 )		// Node Types Match?
							{
								mNodeManager->freeNode(obsData);
								removeNodeFromList(curNode, n);
							}
						}
					}

				}


			}  // end Z for loop
		} // end X for loop

		// expand area for next tree level
			
		Level++;
	}


}







//==============================================================================
// BObstructionManager::InstallObjectObstruction
//==============================================================================
void BObstructionManager::installObjectObstruction(BOPObstructionNode* theNode, long obsType)
{
	long	X1, X2, Z1, Z2;
	long  L1, L2;
	long	x, z;

	// verify this node not in use

	BASSERT(theNode->mThisNodeIndex == -1);
	BASSERT(theNode->mThisNodePointerIndex == -1);
	BASSERT(theNode->mThisNodeQuadTree == -1);

	// Determine which node level the obstruction belongs in.

	X1 = (long) theNode->mDirectVal[theNode->mIdxMinX];
	X2 = (long) (theNode->mDirectVal[theNode->mIdxMaxX] - cNodeDropDownFactor);

	Z1 = (long) theNode->mDirectVal[theNode->mIdxMinZ];
	Z2 = (long) (theNode->mDirectVal[theNode->mIdxMaxZ] - cNodeDropDownFactor);

	// Clamp objects partially off the world
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);
	
	// WORLD BOUNDRY CHECK

	if (X2 < 0 || Z2 < 0 || X1 >= mWidth || Z1 >= mHeight)
	{
      {setBlogError(358); 		blogerrortrace("OBSTRUCTION ERROR - OBSTRUCTION OFF WORLD MAP");}
		BASSERT(0);
		return;
	}

	//BASSERT(X2 >= 0);
	//BASSERT(Z2 >= 0);
	//BASSERT(X1 <  mWidth);
	//BASSERT(Z1 <  mHeight);
	
	// Get QuadTree Level for each Axis

	long nodeLLX = (X1^X2) >> cMetersToQuadCellShift;
	long nodeLLZ = (Z1^Z2) >> cMetersToQuadCellShift;
	if(nodeLLX < 0 || nodeLLZ < 0 || nodeLLX >= countof(mNodeLevelLookup) || nodeLLZ >= countof(mNodeLevelLookup))
	{
	   BFAIL("out of range node lookup");
	   return;
	}
	
	L1 = mNodeLevelLookup[nodeLLX];
	L2 = mNodeLevelLookup[nodeLLZ];


	if (L2 < L1) L1 = L2;		// Choose the higher (smaller #) of the 2 levels

	// More sanity checks

	BASSERT(L1 >= 0);
	BASSERT(L1 < mNumQuadTreeLevels);

	// Now Compute the Index into the QuadTree of the node this items belongs in

	x = X1 >> mQTLevelInfo[obsType][L1].mShiftCount;
	z = Z1 >> mQTLevelInfo[obsType][L1].mShiftCount;
	z = z << mQTLevelInfo[obsType][L1].mLevel;		// Move z Left the number of bits used 

	long NodeIndex = mQTLevelInfo[obsType][L1].mQTNodeIndex + z + x;

	// Add the node to the list

	BOPQuadTreeNode* QuadNode = mQTLevelInfo[obsType][L1].mQTNodes + z + x;

	addNodeToList(QuadNode, theNode);

	// Fill out the remaining node data

	theNode->mThisNodeIndex = NodeIndex;

	theNode->mThisNodeQuadTree = obsType;

	// Does this node forward data to the pather

	if (theNode->mProperties &	cObsPropertyUpdatePatherQuadTree)
	{
	   gPather.updatePathingQuad(this, theNode, true);
	}

	// All Done :-)

}


//==============================================================================
// BObstructionManager::UpdateObstructionLocation
//==============================================================================
bool BObstructionManager::updateObstructionLocation(BOPObstructionNode* theNode, long obsType, bool bForce)
{
   SCOPEDSAMPLE(BObstructionManager_updateObstructionLocation);
	long	X1, X2, Z1, Z2;
	long  L1, L2;
	long	x, z;

	// Determine which node level the obstruction belongs in.

	X1 = (long) theNode->mDirectVal[theNode->mIdxMinX];
	X2 = (long) (theNode->mDirectVal[theNode->mIdxMaxX] - cNodeDropDownFactor);

	Z1 = (long) theNode->mDirectVal[theNode->mIdxMinZ];
	Z2 = (long) (theNode->mDirectVal[theNode->mIdxMaxZ] - cNodeDropDownFactor);

	// Clamp objects partially off the world

	if (X1 < 0) X1 = 0;
	if (Z1 < 0) Z1 = 0;
	if (X2 >= mWidth) X2 = mWidth-1;
	if (Z2 >= mHeight) Z2 = mHeight-1;
	
	// WORLD BOUNDRY CHECK
	if (X2 < 0 || Z2 < 0 || X1 >= mWidth || Z1 >= mHeight)
	{
      return false;
//		blogtrace("OBSTRUCTION ERROR - OBSTRUCTION OFF WORLD MAP");
//		BASSERT(0);
	}

	//BASSERT(X2 >= 0);
	//BASSERT(Z2 >= 0);
	//BASSERT(X1 <  mWidth);
	//BASSERT(Z1 <  mHeight);
	
	// Get QuadTree Level for each Axis
	long nodeLLX = (X1^X2) >> cMetersToQuadCellShift;
	long nodeLLZ = (Z1^Z2) >> cMetersToQuadCellShift;
	if(nodeLLX < 0 || nodeLLZ < 0 || nodeLLX >= countof(mNodeLevelLookup) || nodeLLZ >= countof(mNodeLevelLookup))
	{
	   BFAIL("out of range node lookup");
	   return(false);
	}
	
	L1 = mNodeLevelLookup[nodeLLX];
	L2 = mNodeLevelLookup[nodeLLZ];

	if (L2 < L1) L1 = L2;		// Choose the higher (smaller #) of the 2 levels

	// More sanity checks

	BASSERT(L1 >= 0);
	BASSERT(L1 < mNumQuadTreeLevels);

	// Now Compute the Index into the QuadTree of the node this items belongs in

	x = X1 >> mQTLevelInfo[obsType][L1].mShiftCount;
	z = Z1 >> mQTLevelInfo[obsType][L1].mShiftCount;
	z = z << mQTLevelInfo[obsType][L1].mLevel;		// Move z Left the number of bits used 

	long NodeIndex = mQTLevelInfo[obsType][L1].mQTNodeIndex + z + x;

	// See if we haven't actually moved nodes

	if (theNode->mThisNodeQuadTree == obsType && theNode->mThisNodeIndex == NodeIndex && !bForce)
	{
		return true;		// Nothing more to do
	}

	// Remove the node from its current Quad Node list

	BOPQuadTreeNode* OldQuadNode = &mQuadTreeNodes[theNode->mThisNodeQuadTree][theNode->mThisNodeIndex];
   BASSERT( OldQuadNode );

   if (OldQuadNode && OldQuadNode->mObstructionList)
	   removeNodeFromList(OldQuadNode, theNode->mThisNodePointerIndex);

   // Let the pather know....
   if (theNode->mProperties & cObsPropertyUpdatePatherQuadTree)
		   gPather.updatePathingQuad(this, theNode, false);

	// Add the node to the new list

	BOPQuadTreeNode* QuadNode = mQTLevelInfo[obsType][L1].mQTNodes + z + x;

	addNodeToList(QuadNode, theNode);

	// update the remaining node data

	theNode->mThisNodeIndex = NodeIndex;

	theNode->mThisNodeQuadTree = obsType;

	// Does this node forward data to the pather
   // If the new obs type is noncollideable, turn off it's need to 
   // update the quad tree. dlm 4/15/02 (tax day!)
   if (obsType == cObsTypeNonCollidableUnit)
      theNode->mProperties &= ~cObsPropertyUpdatePatherQuadTree;

	if (theNode->mProperties &	cObsPropertyUpdatePatherQuadTree)
	{
		   gPather.updatePathingQuad(this, theNode, true);
	}
	
	// All Done :-)
   return true;

}


//==============================================================================
// BObstructionManager::deleteObstruction
//==============================================================================
void BObstructionManager::deleteObstruction(BOPObstructionNode *theNode)
{

	long obsType = theNode->mThisNodeQuadTree;
	long NodeIndex = theNode->mThisNodeIndex;
	long ListIndex = theNode->mThisNodePointerIndex;

   // Local copy of the node..
   BOPObstructionNode node = *theNode;

   BOPQuadTreeNode* QuadNode = mQuadTreeNodes[obsType] + NodeIndex;

	// Return Node to Memory Pool

	mNodeManager->freeNode(theNode);

	// Delete the list entry
   if (QuadNode->mObstructionList)
	   removeNodeFromList(QuadNode, ListIndex);

   // Let the pather know....
   if (node.mProperties & cObsPropertyUpdatePatherQuadTree)
		   gPather.updatePathingQuad(this, &node, false);

}



//==============================================================================
// BObstructionManager::AddNodeToList
//==============================================================================
void BObstructionManager::addNodeToList(BOPQuadTreeNode* theQuadNode, BOPObstructionNode* theNode)
{
   // jce [4/27/2005] -- Crazy check because somehow invalid types are getting in.
   BASSERTM(theNode->mType==cObsNodeTypeUnit || theNode->mType==cObsNodeTypeDoppleganger || theNode->mType==cObsNodeTypeTerrain || theNode->mType==cObsNodeTypeCorpse ||
      theNode->mType==cObsNodeTypeEdgeofMap || theNode->mType==cObsNodeTypeMesh || theNode->mType==cObsNodeTypeSquad, "Invalid obstruction node type being added.");

	// Is there no list attached to this node?

	if (theQuadNode->mListType == BOPObQuadNodeListManager::cInvalidNodeListType)
	{
		theQuadNode->mObstructionList = mListManager->getList(0);
		theQuadNode->mNumObstructions = 0;
		theQuadNode->mListType = 0;
	}

	// Is this list full?

	BASSERT(theQuadNode->mObstructionList != NULL);

	short listType = theQuadNode->mListType;
	long numItems = theQuadNode->mNumObstructions;

	if (numItems >= mListManager->mExpandSize[listType])
	{
      BASSERT((listType + 1) < BOPObQuadNodeListManager::cMaxNumNodeTypes);

		BOPObstructionNode** newList = mListManager->getList(listType+1);									// Get a new list

		memcpy(newList, theQuadNode->mObstructionList, numItems * sizeof(BOPObstructionNode*));	// copy existing data over

		mListManager->freeList(theQuadNode->mObstructionList, listType);									// release the old list

		listType++;																	// Update the node information
		theQuadNode->mObstructionList = newList;
		theQuadNode->mListType = listType;
	}

	// Add the element to the end of the list

	theQuadNode->mObstructionList[numItems] = theNode;
   BASSERT(theQuadNode->mNumObstructions < UINT16_MAX);           // If this goes off then need to consider increasing size of mNumObstructions variable
	theQuadNode->mNumObstructions++;

	theNode->mThisNodePointerIndex = numItems;

}


//==============================================================================
// BObstructionManager::RemoveNodeFromList
//==============================================================================
void BObstructionManager::removeNodeFromList(BOPQuadTreeNode* theQuadNode, long index)
{
	unsigned short numItems = theQuadNode->mNumObstructions;

	BASSERT(theQuadNode->mObstructionList != NULL);
	BASSERT(index >= 0);
	BASSERT(index < numItems);

   if (theQuadNode->mObstructionList == NULL || index <0 || index >= numItems )
      return;

	// is the item in the middle of the list?

	numItems--;

	// Did we delete the last item?

	if (numItems == 0)
	{
		mListManager->freeList(theQuadNode->mObstructionList, theQuadNode->mListType);
		theQuadNode->mObstructionList = NULL;
		theQuadNode->mNumObstructions = 0;
		theQuadNode->mListType = BOPObQuadNodeListManager::cInvalidNodeListType;
		return;
	}

	// Did we not delete the last item in the list, leaving a 'hole'?

	if (index < numItems)			// Move last element to 'hole' where deleted item went
	{
		theQuadNode->mObstructionList[index] = theQuadNode->mObstructionList[numItems];
		theQuadNode->mObstructionList[index]->mThisNodePointerIndex = index;
	}
	
	theQuadNode->mNumObstructions = numItems;

	// Should the list be shrunk to a smaller allocated size?

	short listType = theQuadNode->mListType;

	if (numItems <= mListManager->mContractSize[listType])
	{
		BOPObstructionNode** newList = mListManager->getList(listType-1);									// Get a new list

		memcpy(newList, theQuadNode->mObstructionList, numItems * sizeof(BOPObstructionNode*));	// copy existing data over

		mListManager->freeList(theQuadNode->mObstructionList, listType);									// release the old list

		listType--;																	// Update the node information
		theQuadNode->mObstructionList = newList;
		theQuadNode->mListType = listType;
	}

}


//==============================================================================
// BObstructionManager::getNewObstructionNode
//==============================================================================
BOPObstructionNode* BObstructionManager::getNewObstructionNode(void)
{
   BOPObstructionNode *pNode = mNodeManager->getNode();
   // Just clear out the unit pointer when we get this bad boy.  dlm 6/18/02
   if (pNode)
      pNode->mObject = NULL;
   return pNode;
}



//==============================================================================
// BObstructionManager::updatePathingInformation
//==============================================================================
void BObstructionManager::updatePathingInformation(BOPObstructionNode *theNode)
{
	if (theNode->mProperties &	cObsPropertyUpdatePatherQuadTree)
	{
		   gPather.updatePathingQuad(this, theNode, false);
	}

}

//==============================================================================
// BObstructionManager::generateTerrainObstructions
//==============================================================================
void BObstructionManager::generateTerrainObstructions(BYTE *obstructionMap)
{

   if (!obstructionMap)
      return;
      
	// Rebuilding the whole map... so...

	// remove all existing obstructions
   /* Don't do this here.. just pass true to the function, and let that
      code handle it.  That way, this code is centralized in one location.
      dlm 4/17/02
	emptyVirtualQuadtree(cObsTypeBlockLandMovement);
	emptyVirtualQuadtree(cObsTypeBlockWaterMovement);
	emptyVirtualQuadtreeofDataType(cObsTypeBlockAllMovement, cObsNodeTypeTerrain);
   */


	// And specify the entire map area to rebuild
//   generateTerrainObstructions(obstructionMap, 0, 0, mTileWidth-1, mTileHeight-1, true);
   generateTerrainObstructions(obstructionMap, 0, 0, mTileWidth-1, mTileHeight-1, false);


}



//==============================================================================
// BObstructionManager::regenerateTerrainObstructions
//==============================================================================
void BObstructionManager::regenerateTerrainObstructions(BYTE *obstructionMap, long startX, long startZ, long endX, long endZ)
{
   // Make sure cliffs get updated before we update obstructions if
   // we're in editor mode
/*   BTerrainBase *terrain = game->getWorld()->getTerrain();
   if (terrain && terrain->getCliffManager() && game->getIsEditorMode())
   {
      terrain->getCliffManager()->updateAll(true);
   }*/

	generateTerrainObstructions(obstructionMap, startX, startZ, endX, endZ, true);
}


//==============================================================================
// BObstructionManager::generateTerrainObstructions
//==============================================================================
// Parameters are tile coordinates
//
void BObstructionManager::generateTerrainObstructions(BYTE *obstructionMap, long startX, long startZ, long endX, long endZ, bool clearExisting)
{
   SCOPEDSAMPLE(BObstructionManager_generateTerrainObstructions)

#ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigNoUpdatePathingQuad))
      return;
#endif

   #ifndef BUILD_FINAL
      DWORD startTime = timeGetTime();
   #endif

   // Determine the rectangular area in meters that we want to process
   long x1, x2, z1, z2;
   long tileX1, tileX2, tileZ1, tileZ2;

   // get Area coords in meters
   x1 = (long) (startX * mfTileSize);
   z1 = (long) (startZ * mfTileSize);
   x2 = (long) (endX * mfTileSize);
   z2 = (long) (endZ * mfTileSize);

   // Now Round to smalled quad node boundries
   x1 = (x1 & cMetersToQuadCellULANDMask);
   z1 = (z1 & cMetersToQuadCellULANDMask);

   x2 = min((x2 | cMetersToQuadCellLRORMask), mWidth - 1);
   z2 = min((z2 | cMetersToQuadCellLRORMask), mHeight - 1);

   // now convert back to tiles
   tileX1 = max((long) (x1 * mfConvertToTiles + 0.001f), 0);
   tileZ1 = max((long) (z1 * mfConvertToTiles + 0.001f), 0);

   tileX2 = min((long) (x2 * mfConvertToTiles + 0.001f), mTileWidth - 1);		// Clamp at edge of world...
   tileZ2 = min((long) (z2 * mfConvertToTiles + 0.001f), mTileHeight - 1);

   bool bBuildLRPPathTree = !gPather.isLRPTreeLoaded();

   bool bSaveEnableQuadUpdate = gPather.getEnableQuadUpdate();
   if (!bBuildLRPPathTree)
      gPather.enableQuadUpdate(false);

   // do we need to clear the quad tree of obstructions in a given area?
   if (clearExisting)
   {
      emptyVirtualQuadtreeNodes(cObsTypeBlockLandMovement,           (float) x1, (float) z1, (float) (x2 + 1), (float) (z2 + 1), cObsNodeTypeTerrain);
      emptyVirtualQuadtreeNodes(cObsTypeBlockFloodMovement,          (float) x1, (float) z1, (float) (x2 + 1), (float) (z2 + 1), cObsNodeTypeTerrain);
      emptyVirtualQuadtreeNodes(cObsTypeBlockScarabMovement,         (float) x1, (float) z1, (float) (x2 + 1), (float) (z2 + 1), cObsNodeTypeTerrain);
      emptyVirtualQuadtreeNodes(cObsTypeBlockLandAndFloodMovement,   (float) x1, (float) z1, (float) (x2 + 1), (float) (z2 + 1), cObsNodeTypeTerrain);
      emptyVirtualQuadtreeNodes(cObsTypeBlockLandAndScarabMovement,  (float) x1, (float) z1, (float) (x2 + 1), (float) (z2 + 1), cObsNodeTypeTerrain);
      emptyVirtualQuadtreeNodes(cObsTypeBlockFloodAndScarabMovement, (float) x1, (float) z1, (float) (x2 + 1), (float) (z2 + 1), cObsNodeTypeTerrain);
      emptyVirtualQuadtreeNodes(cObsTypeBlockAllMovement,	         (float) x1, (float) z1, (float) (x2 + 1), (float) (z2 + 1), cObsNodeTypeTerrain);
      emptyVirtualQuadtreeNodes(cObsTypeBlockAirMovement,        	   (float) x1, (float) z1, (float) (x2 + 1), (float) (z2 + 1), cObsNodeTypeTerrain);
   }

   // Setup the obstruction Matrix
   long	ObstructionMatrixSize = cTilesPerSmallestQuad * cTilesPerSmallestQuad;
   BYTE* *ObstructionMatrix = new BYTE*[cTilesPerSmallestQuad];
   BYTE  *ObstructionMatrixData = new BYTE[ObstructionMatrixSize];

   for (long n = 0; n < cTilesPerSmallestQuad; n++)
   {
      ObstructionMatrix[n] = ObstructionMatrixData + cTilesPerSmallestQuad * n;
   }

   // Now we need to setup the scanning loop
   long quadX, quadZ;
   long x, z;
   long obsType;
   int count = 0;

   // Loop to scan the area of the map to be updated (plus padding to match quad sizes)
   for (quadX = tileX1; quadX <= tileX2; quadX += cTilesPerSmallestQuad)
   {
      if (((tileX2 - tileX1) > 64) && ((count & 3) == 0))
      {
         gRender.startupStatus("BObstructionManager::generateTerrainObstructions: %02.1f%%", (quadX - tileX1) / float(tileX2 - tileX1) * 100.0f);
      }
      count++;
      
      for (quadZ = tileZ1; quadZ <= tileZ2; quadZ += cTilesPerSmallestQuad)
      {
         // Clear the obstruction matrix
         memset(ObstructionMatrixData, cNOObstruction, ObstructionMatrixSize);

         // Now Scan the tiles in this quad cell
         for (x = quadX; x < quadX+cTilesPerSmallestQuad && x <= tileX2; x++)
         {
            for (z = quadZ; z < quadZ+cTilesPerSmallestQuad && z <= tileZ2; z++)
            {
               ObstructionMatrix[x-quadX][z-quadZ] = obstructionMap[x * mTileWidth + z];
            }
         }

         // Now we pass through and combine blocks to make as large an obstruction as possible
         bool scanForObs = true;

         while (scanForObs)
         {
            scanForObs = false;  // assume we will find no obstructions

            for (x = 0; x < cTilesPerSmallestQuad; x++)
            {
               for (z = 0; z < cTilesPerSmallestQuad; z++)
               {
                  BYTE oType = ObstructionMatrix[x][z];

                  // Convert NonSolid terrain code from the editor value (0xFF) to the sim code value (4)
//                  if (oType == 0xFF)
//                     oType = cIsNonSolid;

                  // No Obstruction ??
                  if (oType == cNOObstruction)
                     continue;

                  // Ok, found a block of a particular type, now.. how big a square can we get?
                  long wide = 1;
                  long tall = 1;

                  expandSquare(ObstructionMatrix, x, z, wide, tall, cTilesPerSmallestQuad, oType);

                  // can it be made wider?
                  long wide1 = wide;
                  long tall1 = tall;

                  expandWidth(ObstructionMatrix, x, z, wide1, tall1, cTilesPerSmallestQuad, oType);

                  // can it be made taller?
                  long wide2 = wide;
                  long tall2 = tall;

                  expandHeight(ObstructionMatrix, x, z, wide2, tall2, cTilesPerSmallestQuad, oType);

                  long size = wide * tall;
                  long size1 = wide1 * tall1;
                  long size2 = wide2 * tall2;

                  // Wide rectangle valid?
                  if (size1 > size)
                  {
                     wide = wide1;
                     tall = tall1;
                     size = size1;
                  }

                  // Tall Rectangle bigger than everything else?
                  if (size2 > size)
                  {
                     wide = wide2;
                     tall = tall2;
                     size = size2;
                  }

                  // we found something, so make sure we keep checking after this
                  scanForObs = true;

                  // Lets make an obstruction for this
                  // First Get a new node and reset its contents
                  BOPObstructionNode* obsNode = mNodeManager->getNode();
                  resetObstructionNode(obsNode);

                  // Fill out the dimensions
                  setAlignedObstruction(obsNode, (float) (quadX + x)*mfTileSize, (float) (quadZ + z)*mfTileSize, (float) (quadX + x+ wide)*mfTileSize, (float) (quadZ + z + tall)*mfTileSize );

                  // Determine which tree to put it in
                  obsType = cObsTypeUnknown;

                  if (oType == cObstructs_Land_Movement)
                     obsType = cObsTypeBlockLandMovement;

                  if (oType == cObstructs_Flood_Movement)
                     obsType = cObsTypeBlockFloodMovement;

                  if (oType == cObstructs_Scarab_Movement)
                     obsType = cObsTypeBlockScarabMovement;

                  if (oType == cObstructs_Land_Flood_Movement)
                     obsType = cObsTypeBlockLandAndFloodMovement;

                  if (oType == cObstructs_Land_Scarab_Movement)
                     obsType = cObsTypeBlockLandAndScarabMovement;

                  if (oType == cObstructs_Flood_Scarab_Movement)
                     obsType = cObsTypeBlockFloodAndScarabMovement;

                  if (oType == cObstructsAllMovement)
                     obsType = cObsTypeBlockAllMovement;

                  if (oType == cObstructsAirMovement)
                     obsType = cObsTypeBlockAirMovement;

                  // Note that it is a terrain obstruction,
                  // And that the pather need to know about it...
                  obsNode->mType = cObsNodeTypeTerrain;
                  obsNode->mProperties |= cObsPropertyUpdatePatherQuadTree;

                  // And Place it in the quadtree
                  if (obsType != cObsTypeUnknown)
                  {
                     installObjectObstruction(obsNode, obsType);
                  }
                  else
                  {
                     BASSERT(0);			// Something bad happened - unknown terrain type?
                  }

                  // And clear the obstruction data matrix
                  for (long xc = x; xc < x + wide; xc++)
                  {
                     for (long zc = z; zc < z + tall; zc++)
                     {
                        ObstructionMatrix[xc][zc] = cNOObstruction;
                     }
                  }
               }	// end matrix z scan loop
            }	// end matrix x scan loop
         } // Scan for obstruction (in matrix) loop
      }	// end quad processing z loop
   }	// end quad processing x loop


   // Whoo hoo! done (re-)generating terrain obstructions
   // Clean up after ourselves
   if (ObstructionMatrix != NULL)
      delete[] ObstructionMatrix;

   if (ObstructionMatrixData != NULL)
      delete[] ObstructionMatrixData;

   if (bBuildLRPPathTree)
   {
      // Update the quad tree.. yeahh..
      BVector vMin((float)(tileX1 * mfTileSize), 0.0f, (float)(tileZ1 * mfTileSize));
      BVector vMax((float)(tileX2 * mfTileSize), 0.0f, (float)(tileZ2 * mfTileSize));
      gPather.updatePathingQuad(this, vMin, vMax);
   }
   else
      gPather.enableQuadUpdate(bSaveEnableQuadUpdate);

   #ifndef BUILD_FINAL
      DWORD endTime = timeGetTime();
      blogtrace("generateTerrainObstructions: %d ms", endTime - startTime);
   #endif
   
   gRender.startupStatus("BObstructionManager::generateTerrainObstructions: %02.1f%%", 100.0f);

}


#pragma warning (push)
#pragma warning (disable : 4127)
//==============================================================================
// BObstructionManager::expandSquare
//==============================================================================
void BObstructionManager::expandSquare(BYTE* *Matrix, long StartX, long StartZ, long &width, long &height, long maxSize, BYTE obsType)
{
   // Sanity/overflow checks
   BASSERT(width >= 0);
   BASSERT(height >= 0);
   BASSERT(maxSize >= 0);

	while (true)
	{
      // overflow check
      BASSERT(StartX + width >= StartX);
      BASSERT(StartZ + height >= StartZ);

		// Can we expand at all?

		if (StartX + width >= maxSize)
			return;
		if (StartZ + height >= maxSize)
			return;

		// Bottom Right diagonal ok?

		if (Matrix[StartX + width][StartZ + height] != obsType)
			return;

		for (long n = 0; n < width; n++)
		{
			// Bottom Row ok?
			if (Matrix[StartX+n][StartZ + height] != obsType)
				return;

			// Right side ok?
			if(Matrix[StartX + width][StartZ + n] != obsType)
				return;
		}

		// all checked out, so we can expand the square

		width++;
		height++;
	}

}

//==============================================================================
// BObstructionManager::expandWidth
//==============================================================================
void BObstructionManager::expandWidth(BYTE* *Matrix, long StartX, long StartZ, long &width, long &height, long maxSize, BYTE obsType)
{
   // Sanity/overflow checks
   BASSERT(width >= 0);
   BASSERT(height >= 0);
   BASSERT(maxSize >= 0);
   BASSERT(StartZ + height >= StartZ);

	if (StartZ + height >= maxSize)
		return;

	while (true)
	{
		// Can we expand at all?

      // overflow check
      BASSERT(StartX + width >= StartX);
      
		if (StartX + width >= maxSize)
			return;

		for (long n = 0; n < height; n++)
		{
			// Right side ok?
			if(Matrix[StartX + width][StartZ + n] != obsType)
				return;
		}

		// all checked out, so we can expand to the right

		width++;
	}

}

//==============================================================================
// BObstructionManager::expandHeight
//==============================================================================
void BObstructionManager::expandHeight(BYTE* *Matrix, long StartX, long StartZ, long &width, long &height, long maxSize, BYTE obsType)
{
   // Sanity/overflow checks
   BASSERT(width >= 0);
   BASSERT(height >= 0);
   BASSERT(maxSize >= 0);
   BASSERT(StartX + width >= StartX);

	if (StartX + width >= maxSize)
		return;

	while (true)
	{
		// Can we expand at all?

      // overflow check
      BASSERT(StartZ + height >= StartZ);
      
		if (StartZ + height >= maxSize)
			return;


		for (long n = 0; n < width; n++)
		{
			// Bottom Row ok?
			if (Matrix[StartX+n][StartZ + height] != obsType)
				return;
		}

		// all checked out, so we can expand downward

		height++;
	}

}
#pragma warning (pop)


//==============================================================================
// BObstructionManager::setAlignedObstruction
//==============================================================================
void BObstructionManager::setAlignedObstruction(BOPObstructionNode *theNode, float x1, float z1, float x2, float z2)
{
	// Insure X2>X1 && Z2>Z1

	float t;

	if (x1 > x2)
	{
		t = x1; x1 = x2; x2 = t;
	}

	if (z1 > z2)
	{
		t = z1; z1 = z2; z2 = t;
	}

	// Fill out the points

	theNode->mX1 = x1;		// Set Lower Left point
	theNode->mZ1 = z1;

	theNode->mX2 = x1;		// Set Upper Left point
	theNode->mZ2 = z2;

	theNode->mX3 = x2;		// Set Upper Right point
	theNode->mZ3 = z2;

	theNode->mX4 = x2;		// Set Lower Right point
	theNode->mZ4 = z1;

	//fill out the bounding info 

	theNode->mIdxMinX = 0;
	theNode->mIdxMinZ = 1;
	theNode->mIdxMaxX = 4;
	theNode->mIdxMaxZ = 5;

	theNode->mRotation = cNoRotation;		// Not rotated

}


//==============================================================================
// BObstructionManager::FillOutRotatedPosition
//==============================================================================
void BObstructionManager::fillOutRotatedPosition(BOPObstructionNode* theNode, float x, float z, float radiusX, float radiusZ, float rotationX, float rotationZ )
{
   SCOPEDSAMPLE(BObstructionManager_fillOutRotatedPosition);

	// Compute rotation compenets
   float forwardX=radiusZ*rotationX;     
   float forwardZ=radiusZ*rotationZ;

   // Right vector is (z, -x)
   float rightX=radiusX*rotationZ;
   float rightZ=-radiusX*rotationX;

	// Compute coordinates

	theNode->mX1 = x - forwardX - rightX;
	theNode->mZ1 = z - forwardZ - rightZ;

	theNode->mX2 = x + forwardX - rightX;
	theNode->mZ2 = z + forwardZ - rightZ;

	theNode->mX3 = x + forwardX + rightX;
	theNode->mZ3 = z + forwardZ + rightZ;

	theNode->mX4 = x - forwardX + rightX;
	theNode->mZ4 = z - forwardZ + rightZ;


	// Determine bounding box

	fillOutMinAndMax(theNode);

	// now we need to quantize the angle of rotation

	long QuantCos = 64 + ((long) (rotationX * 64.0f));			// Scale -64.0 to +64.0, then to 0 to 128
	if (rotationZ < 0.0f)
		QuantCos = QuantCos + 128;										// Should not be 0 or 128 if this occures, ut-oh...
	else
		QuantCos = 128 - QuantCos;										// Reverse for circular mapping (0->360 degrees = 0 to 256)

	theNode->mRotation = (BYTE) QuantCos;    // Magic Numbers to f*# with Dusty's brain....


}


//==============================================================================
// BObstructionManager::fillOutRotatedPosition
//==============================================================================
void BObstructionManager::fillOutNonRotatedPosition(BOPObstructionNode* theNode, float x, float z, float radiusX, float radiusZ)
{
   SCOPEDSAMPLE(BObstructionManager_fillOutNonRotatedPosition);
	// Compute coordinates

	theNode->mX1 = x - radiusX;		// Lower Left Point
	theNode->mZ1 = z - radiusZ;

	theNode->mX2 = x - radiusX;		// Upper Left Point
	theNode->mZ2 = z + radiusZ;

	theNode->mX3 = x + radiusX;		// Upper Right Point
	theNode->mZ3 = z + radiusZ;

	theNode->mX4 = x + radiusX;		// Lower Right Point
	theNode->mZ4 = z - radiusZ;




	// Fillout Min & Max for bounding box

	theNode->mIdxMaxX = 4;
	theNode->mIdxMaxZ = 5;
	theNode->mIdxMinX = 0;
	theNode->mIdxMinZ = 1;

	// This obstruction Not rotated

	theNode->mRotation = cNoRotation;		

}











//==============================================================================
// BObstructionManager::FillOutMinAndMax
//==============================================================================
void BObstructionManager::fillOutMinAndMax(BOPObstructionNode* theNode)
{
	long n;
	long minX = 0;
	long maxX = 0;

	for (n = 2; n < 7; n+=2)
	{
		if (theNode->mDirectVal[n] < theNode->mDirectVal[minX])
		{
			minX = n;
		}
		else if (theNode->mDirectVal[n] > theNode->mDirectVal[maxX])
		{
			maxX = n;
		}
	}

	long minZ = 1;
	long maxZ = 1;

	for (n = 3; n < 8; n+=2)
	{
		if (theNode->mDirectVal[n] < theNode->mDirectVal[minZ])
		{
			minZ = n;
		}
		else if (theNode->mDirectVal[n] > theNode->mDirectVal[maxZ])
		{
			maxZ = n;
		}
	}


	theNode->mIdxMinX = (BYTE) minX;
	theNode->mIdxMaxX = (BYTE) maxX;
	theNode->mIdxMinZ = (BYTE) minZ;
	theNode->mIdxMaxZ = (BYTE) maxZ;
}



//==============================================================================
// BObstructionManager::markObstructionsToBeIgnored
//==============================================================================
void BObstructionManager::markObstructionsToBeIgnored(BObstructionNodePtrArray &theList, const long numItems)
{

	for (long n = 0; n < min(numItems, theList.getNumber()); n++)
	{
		theList.get(n)->mProperties |= cObsPropertyInIgnoreList;
	}


}


//==============================================================================
// BObstructionManager::unmarkObstructionsToBeIgnored
//==============================================================================
void BObstructionManager::unmarkObstructionsToBeIgnored(BObstructionNodePtrArray &theList, const long numItems)
{

	for (long n = 0; n < min(numItems, theList.getNumber()); n++)
	{
		theList.get(n)->mProperties &= ~cObsPropertyInIgnoreList;
	}


}





//==============================================================================
// BObstructionManager::releaseExpandedHulls
//==============================================================================
void BObstructionManager::releaseExpandedHulls(void)
{
	for (long n = 0; n < mExpandCheckoutList.getNumber(); n++)
	{
		mExpandCheckoutList[n]->mExpandedHull = NULL;
	}
	mExpandCheckoutList.setNumber(0);

   if (mHullManager)
	   mHullManager->reclaimAllNodes();
}



//==============================================================================
// BObstructionManager::getExpandedHullPoints
//==============================================================================
BOPQuadHull* BObstructionManager::getExpandedHullPoints(BOPObstructionNode *theNode, const float expansionRadius)
{
  /* #ifndef BUILD_FINAL
   if(expansionRadius == 0.0f)
   {
      if(theNode->mType==cObsNodeTypeUnit && theNode->mUnit && theNode->mUnit->getProtoUnit())
         blogtrace("0 expansion radius for unit %s", theNode->mUnit->getProtoUnit()->getName());
      else
         blogtrace("0 expansion radius for unknown unit/not a unit");
   }
   #endif*/
   
	BASSERT(expansionRadius != 0.0f);

	// Is there already a an expanded node for this radius?

	BOPQuadHull* expandData = theNode->mExpandedHull;

	// Are there expanded hulls for this node

	if (expandData != NULL)
	{
		while (expandData)
		{
			// Check to see if one already exists for this radius
			if (expandData->mRadius == expansionRadius)
			{
				return(expandData);
			}
			// Otherwise see if more expanded hulls are chained on the end of this one
			if (expandData->mNextNode)
			{
				expandData = expandData->mNextNode;
			}
			else	// ok, make a new expanded hull and put on the end of the chain
			{
				expandData->mNextNode = mHullManager->getNode();
				expandData = expandData->mNextNode;
				break;
			}
		}
	}
	else // make expanded hull and attach to obstruction
	{
		expandData = mHullManager->getNode();
		theNode->mExpandedHull = expandData;
		mExpandCheckoutList.add(theNode);
	}

   if (expandData)
      expandData->expandFrom(theNode->getHull(), expansionRadius);

	return(expandData);

}

//==============================================================================
// BObstructionManager::
//==============================================================================
bool BObstructionManager::testObstructions(float tx1, float tz1, float tx2, float tz2, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID)
{
	// Handle Begin/End Mode

	if (mInUse)
	{
		expansionRadius = mRadius;
		QuadTreesToScan |= mQuadTreesToScan;
		nodeTypes |=mValidNodeTypes;
		lPlayerID = mlPlayerID;
	}

   // Set up Player Mask
   long lPlayerMask = 0;
   if (lPlayerID > 0)
      lPlayerMask = 0x0001 << (lPlayerID - 1);

	// Are we expanding hulls?
	bool expand = (expansionRadius != 0.0f);

	// Get True Coordinates

	if (tx1 > tx2)
	{
		float t = tx1; tx1 = tx2; tx2 = t;
	}
	if (tz1 > tz2)
	{
		float t = tz1; tz1 = tz2; tz2 = t;
	}

	// copy the coordinates for area expansion
	float unrotatedX1 = tx1;
	float unrotatedX2 = tx2;
	float unrotatedZ1 = tz1;
	float unrotatedZ2 = tz2;
	
	float rotatedX1 = unrotatedX1;
	float rotatedZ1 = unrotatedZ1;
	float rotatedX2 = unrotatedX2;
	float rotatedZ2 = unrotatedZ2;

	// Get expanded search area
	if (expand)
	{
		unrotatedX1 -= expansionRadius;
		unrotatedX2 += expansionRadius;
		unrotatedZ1 -= expansionRadius;
		unrotatedZ2 += expansionRadius;
		
	   // jce [10/13/2008] -- expanding by the hypotenuse here because this is used for overlap
	   // checks with rotated obstructions so at the corners using expansionRadius is not enough
	   // for a proper test.
	   float expandBy = sqrtf(2.0f*expansionRadius*expansionRadius);
	   
		rotatedX1 -= expandBy;
		rotatedX2 += expandBy;
		rotatedZ1 -= expandBy;
		rotatedZ2 += expandBy;
	}


	// clamp to world
	if (rotatedX1 < 0.0f)
	   rotatedX1 = 0.0f;
	if (rotatedZ1 < 0.0f)
	   rotatedZ1 = 0.0f;
	if (rotatedX2 > mfWidth)
	   rotatedX2 = mfWidth;
	if (rotatedZ2 > mfHeight)
	   rotatedZ2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) rotatedX1;
	X2 = (long) (rotatedX2 - cNodeDropDownFactor);

	Z1 = (long) rotatedZ1;
	Z2 = (long) (rotatedZ2 - cNodeDropDownFactor);

	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);

	long CurTree = 0;
	long CurLevel, StartX, EndX, StartZ, EndZ;

	BOPQuadHull	*ProcessedHull;

	while (QuadTreesToScan != 0)
	{
		if (QuadTreesToScan & 0x01)
		{
			// Start at Bottom level ofQuadtree
			CurLevel = mNumQuadTreeLevels - 1;	

			while (CurLevel >= 0)
			{
				// Compute range of QuadCells to scan for this level
				StartX = X1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndX   = X2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				StartZ = Z1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndZ   = Z2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				// Do we want to check for wholly contained nodes?

				bool insideCells = ((EndX - StartX) > 1) && ((EndZ - StartZ) > 1);

				// Process a 2d array range of Quad Nodes

				for (long z = StartZ; z <= EndZ; z++)
				{
					long NodeIndex = mQTLevelInfo[CurTree][CurLevel].mQTNodeIndex + (z << CurLevel);
					bool processAll = (insideCells && z > StartZ && z < EndZ);

					BOPQuadTreeNode* CurQuadNode = &mQuadTreeNodes[CurTree][NodeIndex + StartX];

					for (long x = StartX; x <= EndX; x++, CurQuadNode++)
					{
						bool BoundsCheck = !(processAll && x > StartX && x < EndX);

						for (long n = 0; n < CurQuadNode->mNumObstructions; n++)
						{
							// Get the Obstruction to process

							BOPObstructionNode * theNode = CurQuadNode->mObstructionList[n];
                     if (!theNode)
                     {
                        // What's up here.. how is it possible to get
                        // a null object inside this list.. 
                        BASSERT(0);
                        continue;
                     }

							// Is this item flagged to be ignored?

							if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
								continue;


							// See if the object is rotated
							bool rotated = ((theNode->mRotation & cIsObstructionRotated) != 0);

							// Check to see if the bounding boxes don't overlap
							if (BoundsCheck) // outright reject?
							{
							   if(rotated)
							   {
                           if(FastFloat::compareLessThan(rotatedX2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                              FastFloat::compareLessThan(rotatedZ2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],rotatedX1) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],rotatedZ1))
                              continue;
							   }
							   else
							   {
                           if(FastFloat::compareLessThan(unrotatedX2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                              FastFloat::compareLessThan(unrotatedZ2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],unrotatedX1) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],unrotatedZ1))
                              continue;
                        }
							}


							// Perform object propery checks (Node type matching)
							if (nodeTypes != cObsNodeTypeAll)
							{
								if ((theNode->mType & (BYTE) nodeTypes) == 0)
									continue;
							}

							// Check if object is player owned, and is on player ignore list

							if (lPlayerMask)
							{
								if (theNode->mPlayerID != 0)
								{
#ifdef CANJUMP_CHANGE
                           if (mbCanJump)
                              continue;
#else
									if ((ObsSystemPlayerMasks[theNode->mPlayerID] & lPlayerMask) == lPlayerMask)
										continue;
#endif
								}
							}


							// If no expansion takes place, note there is no expanded hull
							if (!expand)
								theNode->mExpandedHull = NULL;

							// If it's not rotated, then the item overlaps, and we add it
							// If it's rotated, we have to make some checks

							if (rotated)
							{
								// Get either the expanded hull, or the actual

								if (expand)
									ProcessedHull = getExpandedHullPoints(theNode, expansionRadius);
								else
									ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);

								// Test Center Point
                        if (!ProcessedHull)
                        {
                           BASSERT(0);
                           continue;
                        }

								float px = (ProcessedHull->mDirectVal[ProcessedHull->mIdxMinX] + ProcessedHull->mDirectVal[ProcessedHull->mIdxMaxX]) * 0.5f;
								float pz = (ProcessedHull->mDirectVal[ProcessedHull->mIdxMinZ] + ProcessedHull->mDirectVal[ProcessedHull->mIdxMaxZ]) * 0.5f;

								if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
								{
									// it overlaps, we're in
								}
								else	// Test Corner Point #1
								{
									px = ProcessedHull->mX1; pz = ProcessedHull->mZ1;
									if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
									{
										// it overlaps, we're in
									}
									else  // Test Corner Point #2
									{
										px = ProcessedHull->mX2; pz = ProcessedHull->mZ2;
										if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
										{
											// it overlaps, we're in
										}
										else  // Test Corner Point #3
										{
											px = ProcessedHull->mX3; pz = ProcessedHull->mZ3;
											if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
											{
												// it overlaps, we're in
											}
											else  // Test Corner Point #4
											{
												px = ProcessedHull->mX4; pz = ProcessedHull->mZ4;
												if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
												{
													// it overlaps, we're in
												}
												else	// Need to do a more complete test 
												{
													if (!ProcessedHull->overlapsBox(tx1, tz1, tx2, tz2))
													{
														continue;
													}
												}
											}
										}
									}
								}
							} // end if(rotated) check


							// If we got to this point, we found an overlapping obstruction
                     return(true);

						} //end for n loop

					} // end for x loop
				}	// End for z loop

				// Process next highest level in quadtree
				CurLevel--;		

			}  // End Loop to process all quadtree levels


		}  
		CurTree++;
		QuadTreesToScan >>= 1;
	}

   // nothing overlapping
   return(false);
}

//==============================================================================
// BObstructionManager::
//
// Test obstructions around a point with given radius
//==============================================================================
bool BObstructionManager::testObstructions( const BVector cPoint, const float cTestRadius, const float expansionRadius, long quadTreesToScan, long nodeTypes, long playerID )
{
   float x1 = cPoint.x + cTestRadius;
   float z1 = cPoint.z + cTestRadius;
   float x2 = cPoint.x - cTestRadius;
   float z2 = cPoint.z - cTestRadius;

   return( testObstructions( x1, z1, x2, z2, expansionRadius, quadTreesToScan, nodeTypes, playerID ) );
}

//==============================================================================
// BObstructionManager::testObstructionsInCachedList
//==============================================================================
bool BObstructionManager::testObstructionsInCachedList(float tx1, float tz1, float tx2, float tz2, float expansionRadius, BObstructionNodePtrArray &cachedObstructions)
{
   // Handle Begin/End Mode
   if (mInUse)
      expansionRadius = mRadius;

   // Are we expanding hulls?
   bool expand = (expansionRadius != 0.0f);

   // Get True Coordinates
   if (tx1 > tx2)
   {
      float t = tx1; tx1 = tx2; tx2 = t;
   }
   if (tz1 > tz2)
   {
      float t = tz1; tz1 = tz2; tz2 = t;
   }

	// copy the coordinates for area expansion
	float unrotatedX1 = tx1;
	float unrotatedX2 = tx2;
	float unrotatedZ1 = tz1;
	float unrotatedZ2 = tz2;
	
	float rotatedX1 = unrotatedX1;
	float rotatedZ1 = unrotatedZ1;
	float rotatedX2 = unrotatedX2;
	float rotatedZ2 = unrotatedZ2;

	// Get expanded search area
	if (expand)
	{
		unrotatedX1 -= expansionRadius;
		unrotatedX2 += expansionRadius;
		unrotatedZ1 -= expansionRadius;
		unrotatedZ2 += expansionRadius;
		
	   // jce [10/13/2008] -- expanding by the hypotenuse here because this is used for overlap
	   // checks with rotated obstructions so at the corners using expansionRadius is not enough
	   // for a proper test.
	   float expandBy = sqrtf(2.0f*expansionRadius*expansionRadius);
	   
		rotatedX1 -= expandBy;
		rotatedX2 += expandBy;
		rotatedZ1 -= expandBy;
		rotatedZ2 += expandBy;
	}


	// clamp to world
	if (rotatedX1 < 0.0f)
	   rotatedX1 = 0.0f;
	if (rotatedZ1 < 0.0f)
	   rotatedZ1 = 0.0f;
	if (rotatedX2 > mfWidth)
	   rotatedX2 = mfWidth;
	if (rotatedZ2 > mfHeight)
	   rotatedZ2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) rotatedX1;
	X2 = (long) (rotatedX2 - cNodeDropDownFactor);

	Z1 = (long) rotatedZ1;
	Z2 = (long) (rotatedZ2 - cNodeDropDownFactor);

   BOPQuadHull	*ProcessedHull;

   for (long n = 0; n < cachedObstructions.getNumber(); n++)
   {
      // Get the Obstruction to process
      BOPObstructionNode * theNode = cachedObstructions[n];
      if (!theNode)
      {
         // What's up here.. how is it possible to get
         // a null object inside this list.. 
         BASSERT(0);
         continue;
      }

      // Is this item flagged to be ignored?
      if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
         continue;

      // See if the object is rotated
      bool rotated = ((theNode->mRotation & cIsObstructionRotated) != 0);

      // Check to see if the bounding boxes don't overlap
	   if(rotated)
	   {
         if(FastFloat::compareLessThan(rotatedX2,theNode->mDirectVal[theNode->mIdxMinX]) ||
            FastFloat::compareLessThan(rotatedZ2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
            FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],rotatedX1) ||
            FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],rotatedZ1))
            continue;
	   }
	   else
	   {
         if(FastFloat::compareLessThan(unrotatedX2,theNode->mDirectVal[theNode->mIdxMinX]) ||
            FastFloat::compareLessThan(unrotatedZ2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
            FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],unrotatedX1) ||
            FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],unrotatedZ1))
            continue;
      }

      // If no expansion takes place, note there is no expanded hull
      if (!expand)
         theNode->mExpandedHull = NULL;

      // If it's not rotated, then the item overlaps, and we add it
      // If it's rotated, we have to make some checks
      if (rotated)
      {
         // Get either the expanded hull, or the actual
         if (expand)
            ProcessedHull = getExpandedHullPoints(theNode, expansionRadius);
         else
            ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);

         // Test Center Point
         if (!ProcessedHull)
         {
            BASSERT(0);
            continue;
         }

         float px = (ProcessedHull->mDirectVal[ProcessedHull->mIdxMinX] + ProcessedHull->mDirectVal[ProcessedHull->mIdxMaxX]) * 0.5f;
         float pz = (ProcessedHull->mDirectVal[ProcessedHull->mIdxMinZ] + ProcessedHull->mDirectVal[ProcessedHull->mIdxMaxZ]) * 0.5f;

         if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
         {
            // it overlaps, we're in
         }
         else	// Test Corner Point #1
         {
            px = ProcessedHull->mX1; pz = ProcessedHull->mZ1;
            if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
            {
               // it overlaps, we're in
            }
            else  // Test Corner Point #2
            {
               px = ProcessedHull->mX2; pz = ProcessedHull->mZ2;
               if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
               {
                  // it overlaps, we're in
               }
               else  // Test Corner Point #3
               {
                  px = ProcessedHull->mX3; pz = ProcessedHull->mZ3;
                  if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
                  {
                     // it overlaps, we're in
                  }
                  else  // Test Corner Point #4
                  {
                     px = ProcessedHull->mX4; pz = ProcessedHull->mZ4;
                     if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
                     {
                        // it overlaps, we're in
                     }
                     else	// Need to do a more complete test 
                     {
                        if (!ProcessedHull->overlapsBox(tx1, tz1, tx2, tz2))
                        {
                           continue;
                        }
                     }
                  }
               }
            }
         }
      } // end if(rotated) check

      // If we got to this point, we found an overlapping obstruction
      return(true);
   } //end for n loop

   // nothing overlapping
   return(false);
}

//==============================================================================
// BObstructionManager::
//==============================================================================
void BObstructionManager::findObstructions(float tx1, float tz1, float tx2, float tz2, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID,
														  bool ignoreCurrentList, BObstructionNodePtrArray &theList)
{
	SCOPEDSAMPLE(BObstructionManager_findObstructions)
	long	incomingSize = theList.getNumber();

	if (ignoreCurrentList)
		markObstructionsToBeIgnored(theList, incomingSize);
	else
		theList.setNumber(0);


	// Handle Begin/End Mode

	if (mInUse)
	{
		expansionRadius = mRadius;
		QuadTreesToScan |= mQuadTreesToScan;
		nodeTypes |=mValidNodeTypes;
		lPlayerID = mlPlayerID;
	}

   // Set up Player Mask
   long lPlayerMask = 0;
   if (lPlayerID > 0)
      lPlayerMask = 0x0001 << (lPlayerID - 1);

	// Are we expanding hulls?
	bool expand = (expansionRadius != 0.0f);

	// Get True Coordinates

	if (tx1 > tx2)
	{
		float t = tx1; tx1 = tx2; tx2 = t;
	}
	if (tz1 > tz2)
	{
		float t = tz1; tz1 = tz2; tz2 = t;
	}

	// copy the coordinates for area expansion
	float unrotatedX1 = tx1;
	float unrotatedX2 = tx2;
	float unrotatedZ1 = tz1;
	float unrotatedZ2 = tz2;
	
	float rotatedX1 = unrotatedX1;
	float rotatedZ1 = unrotatedZ1;
	float rotatedX2 = unrotatedX2;
	float rotatedZ2 = unrotatedZ2;

	// Get expanded search area
	if (expand)
	{
		unrotatedX1 -= expansionRadius;
		unrotatedX2 += expansionRadius;
		unrotatedZ1 -= expansionRadius;
		unrotatedZ2 += expansionRadius;
		
	   // jce [10/13/2008] -- expanding by the hypotenuse here because this is used for overlap
	   // checks with rotated obstructions so at the corners using expansionRadius is not enough
	   // for a proper test.
	   float expandBy = sqrtf(2.0f*expansionRadius*expansionRadius);
	   
		rotatedX1 -= expandBy;
		rotatedX2 += expandBy;
		rotatedZ1 -= expandBy;
		rotatedZ2 += expandBy;
	}


	// clamp to world
	if (rotatedX1 < 0.0f)
	   rotatedX1 = 0.0f;
	if (rotatedZ1 < 0.0f)
	   rotatedZ1 = 0.0f;
	if (rotatedX2 > mfWidth)
	   rotatedX2 = mfWidth;
	if (rotatedZ2 > mfHeight)
	   rotatedZ2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) rotatedX1;
	X2 = (long) (rotatedX2 - cNodeDropDownFactor);

	Z1 = (long) rotatedZ1;
	Z2 = (long) (rotatedZ2 - cNodeDropDownFactor);

	// jce [7/6/2005] -- this should have been pasted here too:
	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);
	
	long CurTree = 0;
	long CurLevel, StartX, EndX, StartZ, EndZ;

	BOPQuadHull	*ProcessedHull;
   BFixedString<256> temp;
	while (QuadTreesToScan != 0)
	{
		if (QuadTreesToScan & 0x01)
		{
			// Start at Bottom level ofQuadtree
			CurLevel = mNumQuadTreeLevels - 1;	

			while (CurLevel >= 0)
			{
				// Compute range of QuadCells to scan for this level
				StartX = X1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndX   = X2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				StartZ = Z1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndZ   = Z2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				// Do we want to check for wholly contained nodes?

				bool insideCells = ((EndX - StartX) > 1) && ((EndZ - StartZ) > 1);

				// Process a 2d array range of Quad Nodes

				for (long z = StartZ; z <= EndZ; z++)
				{
					long NodeIndex = mQTLevelInfo[CurTree][CurLevel].mQTNodeIndex + (z << CurLevel);
					bool processAll = (insideCells && z > StartZ && z < EndZ);

               // jce [2/25/2005] -- sanity checks to figure out why this is crashing right now.
               if(CurTree<0 || CurTree>=cNumQuadTrees)
               {
                  temp.format(B("Bad CurTree (%d) in obstruction manager."), CurTree);
                  BFAIL(temp);
                  return;
               }
               long fullIndex = NodeIndex+StartX;
               if(fullIndex<0 || fullIndex >= mNumQuadTreeNodes)
               {
                  temp.format(B("Bad index (%d+%d = %d) in obstruction manager, max is %d."), NodeIndex, StartX, fullIndex, mNumQuadTreeNodes);
                  BFAIL(temp);
                  return;
               }
					BOPQuadTreeNode* CurQuadNode = &mQuadTreeNodes[CurTree][NodeIndex + StartX];

					for (long x = StartX; x <= EndX; x++, CurQuadNode++)
					{
						bool BoundsCheck = !(processAll && x > StartX && x < EndX);

						for (long n = 0; n < CurQuadNode->mNumObstructions; n++)
						{
							// Get the Obstruction to process

							BOPObstructionNode * theNode = CurQuadNode->mObstructionList[n];
                     if (!theNode)
                     {
                        // What's up here.. how is it possible to get
                        // a null object inside this list.. 
                        BASSERT(0);
                        continue;
                     }

							// Is this item flagged to be ignored?

							if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
								continue;


							// See if the object is rotated
							bool rotated = ((theNode->mRotation & cIsObstructionRotated) != 0);

							// Check to see if the bounding boxes don't overlap
							if (BoundsCheck) // outright reject?
							{
							   if(rotated)
							   {
                           //CLM [04.30.08] removal of multiple FCMP 
                           if(FastFloat::compareLessThan(rotatedX2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                              FastFloat::compareLessThan(rotatedZ2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],rotatedX1) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],rotatedZ1))
                              continue;
							   }
							   else
							   {
                           //CLM [04.30.08] removal of multiple FCMP 
                           if(FastFloat::compareLessThan(unrotatedX2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                              FastFloat::compareLessThan(unrotatedZ2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],unrotatedX1) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],unrotatedZ1))
                              continue;
                        }

                        //origional
								/*if (theNode->mDirectVal[theNode->mIdxMinX] > x2 ||
									 theNode->mDirectVal[theNode->mIdxMinZ] > z2 ||
									 theNode->mDirectVal[theNode->mIdxMaxX] < x1 ||
									 theNode->mDirectVal[theNode->mIdxMaxZ] < z1)
									continue;*/
							}

							// Perform object propery checks (Node type matching)

							if (nodeTypes != cObsNodeTypeAll)
							{
								if ((theNode->mType & (BYTE) nodeTypes) == 0)
									continue;
							}

							// Check if object is player owned, and is on player ignore list

							if (lPlayerMask)
							{
								if (theNode->mPlayerID != 0)
								{
#ifdef CANJUMP_CHANGE
                           if (mbCanJump)
                              continue;
#else
									if ((ObsSystemPlayerMasks[theNode->mPlayerID] & lPlayerMask) == lPlayerMask)
										continue;
#endif
								}
							}

							// If no expansion takes place, note there is no expanded hull

							if (!expand)
								theNode->mExpandedHull = NULL;

							// If it's not rotated, then the item overlaps, and we add it
							// If it's rotated, we have to make some checks

							if (rotated)
							{
								// Get either the expanded hull, or the actual

								if (expand)
									ProcessedHull = getExpandedHullPoints(theNode, expansionRadius);
								else
									ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);

								// Test Center Point
                        if (!ProcessedHull)
                        {
                           BASSERT(0);
                           continue;
                        }

								float px = (ProcessedHull->mDirectVal[ProcessedHull->mIdxMinX] + ProcessedHull->mDirectVal[ProcessedHull->mIdxMaxX]) * 0.5f;
								float pz = (ProcessedHull->mDirectVal[ProcessedHull->mIdxMinZ] + ProcessedHull->mDirectVal[ProcessedHull->mIdxMaxZ]) * 0.5f;

								if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
								{
									// it overlaps, we're in
								}
								else	// Test Corner Point #1
								{
									px = ProcessedHull->mX1; pz = ProcessedHull->mZ1;
									if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
									{
										// it overlaps, we're in
									}
									else  // Test Corner Point #2
									{
										px = ProcessedHull->mX2; pz = ProcessedHull->mZ2;
										if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
										{
											// it overlaps, we're in
										}
										else  // Test Corner Point #3
										{
											px = ProcessedHull->mX3; pz = ProcessedHull->mZ3;
											if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
											{
												// it overlaps, we're in
											}
											else  // Test Corner Point #4
											{
												px = ProcessedHull->mX4; pz = ProcessedHull->mZ4;
												if (px >= tx1 && px <= tx2 && pz >= tz1 && pz <= tz2)
												{
													// it overlaps, we're in
												}
												else	// Need to do a more complete test 
												{
													if (!ProcessedHull->overlapsBox(tx1, tz1, tx2, tz2))
													{
														continue;
													}
												}
											}
										}
									}
								}
							} // end if(rotated) check
							else
							{
								// If we are to return an expanded hull, make one and attach it to this node
								if (expand)
									getExpandedHullPoints(theNode, expansionRadius);
							}


							// Now we add the obstruction to our results list 

							theList.add(theNode);

						} //end for n loop

					} // end for x loop
				}	// End for z loop

				// Process next highest level in quadtree
				CurLevel--;		

			}  // End Loop to process all quadtree levels


		}  
		CurTree++;
		QuadTreesToScan >>= 1;
	}

	
	// Now we reset the ignore list if we were using that....

	if (ignoreCurrentList)
		unmarkObstructionsToBeIgnored(theList, incomingSize);


}


//==============================================================================
// BObstructionManager::findObstructions
//==============================================================================
void BObstructionManager::findObstructions(const BVector &point, const bool relaxed, const bool add, BObstructionNodePtrArray &obstructions)
{
   // If not adding, clear the list.
   if(!add)
      obstructions.setNumber(0);

   // You should have called begin before using this.
   if(!mInUse)
   {
      BASSERT(0);
      return;
   }

   float expansionRadius;
	if(relaxed)
		expansionRadius = mRelaxedRadius;
	else
		expansionRadius = mRadius;

   // Do the check.  Pass in a bunch of defaults.
   mInUse=false;
   findObstructionsOnPoint(point, expansionRadius, mQuadTreesToScan, mValidNodeTypes, mlPlayerID, add, obstructions);
   mInUse=true;
}


//==============================================================================
// BObstructionManager::findObstructionsNoExpansion
//==============================================================================
void BObstructionManager::findObstructionsNoExpansion(const BConvexHull &theHull, bool ignoreCurrentList, BObstructionNodePtrArray &obstructions)
{

	BASSERT(mInUse);


	float ExpansionRadius = 0.0f;

	mInUse = false;				// Disable override...

	findObstructions(theHull, ExpansionRadius, mQuadTreesToScan, mValidNodeTypes, mlPlayerID, ignoreCurrentList, obstructions);

	mInUse = true;

}



//==============================================================================
// BObstructionManager::findObstructions
//==============================================================================
void BObstructionManager::findObstructions(const BConvexHull &theHull, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID,
														  bool ignoreCurrentList, BObstructionNodePtrArray &theList, CHECKUNIT_CALLBACK checkUnitCB, void *checkUnitParam)
{
	
	long	incomingSize = theList.getNumber();

	if (ignoreCurrentList)
		markObstructionsToBeIgnored(theList, incomingSize);
	else
		theList.setNumber(0);

	// Handle Begin/End Mode

	if (mInUse)
	{
		expansionRadius = mRadius;
		QuadTreesToScan |= mQuadTreesToScan;
		nodeTypes |=mValidNodeTypes;
		lPlayerID = mlPlayerID;
	}

   // Set up Player Mask
   long lPlayerMask = 0;
   if (lPlayerID > 0)
      lPlayerMask = 0x0001 << (lPlayerID - 1);

	// Are we expanding hulls?

	bool expand = (expansionRadius != 0.0f);

	// Get True Coordinates for Hull Bounding Box

	float tx1 = theHull.getBoundingMin().x;
	float tz1 = theHull.getBoundingMin().z;
	float tx2 = theHull.getBoundingMax().x;
	float tz2 = theHull.getBoundingMax().z;

	// copy the coordinates for area expansion
		
	float x1 = tx1, x2 = tx2, z1 = tz1, z2 = tz2;
	
	// Get expanded search area

	if (expand)
	{
	   // jce [10/13/2008] -- expanding by the hypotenuse here because this is used for overlap
	   // checks with rotated obstructions so at the corners using expansionRadius is not enough
	   // for a proper test.
	   float expandBy = sqrtf(2.0f*expansionRadius*expansionRadius);
	   
		x1 -= expandBy;
		x2 += expandBy;
		z1 -= expandBy;
		z2 += expandBy;
	}

	// clamp to world

	if (x1 < 0.0f) x1 = 0.0f;
	if (z1 < 0.0f) z1 = 0.0f;
	if (x2 > mfWidth)  x2 = mfWidth;
	if (z2 > mfHeight) z2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) x1;
	X2 = (long) (x2 - cNodeDropDownFactor);

	Z1 = (long) z1;
	Z2 = (long) (z2 - cNodeDropDownFactor);

	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);

	long CurTree = 0;
	long CurLevel, StartX, EndX, StartZ, EndZ;

	BOPQuadHull	*ProcessedHull;

   BFixedString<256> temp;
	while (QuadTreesToScan != 0)
	{
		if (QuadTreesToScan & 0x01)
		{
			// Start at Bottom level ofQuadtree
			CurLevel = mNumQuadTreeLevels - 1;	

			while (CurLevel >= 0)
			{
				// Compute range of QuadCells to scan for this level
				StartX = X1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndX   = X2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				StartZ = Z1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndZ   = Z2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				// Do we want to check for wholly contained nodes?

				bool insideCells = ((EndX - StartX) > 1) && ((EndZ - StartZ) > 1);

				// Process a 2d array range of Quad Nodes

				for (long z = StartZ; z <= EndZ; z++)
				{
					long NodeIndex = mQTLevelInfo[CurTree][CurLevel].mQTNodeIndex + (z << CurLevel);
					bool processAll = (insideCells && z > StartZ && z < EndZ);

               // jce [2/25/2005] -- sanity checks to figure out why this is crashing right now.
               if(CurTree<0 || CurTree>=cNumQuadTrees)
               {
                 
                  temp.format(B("Bad CurTree (%d) in obstruction manager."), CurTree);
                  BFAIL(temp);
                  return;
               }
               long fullIndex = NodeIndex+StartX;
               if(fullIndex<0 || fullIndex >= mNumQuadTreeNodes)
               {
                  
                  temp.format(B("Bad index (%d+%d = %d) in obstruction manager, max is %d."), NodeIndex, StartX, fullIndex, mNumQuadTreeNodes);
                  BFAIL(temp);
                  return;
               }
               
					BOPQuadTreeNode* CurQuadNode = &mQuadTreeNodes[CurTree][NodeIndex + StartX];

					for (long x = StartX; x <= EndX; x++, CurQuadNode++)
					{
						bool BoundsCheck = !(processAll && x > StartX && x < EndX);

						for (long n = 0; n < CurQuadNode->mNumObstructions; n++)
						{
							// Get the Obstruction to process

							BOPObstructionNode * theNode = CurQuadNode->mObstructionList[n];
                     if (!theNode)
                     {
                        // What's up here.. how is it possible to get
                        // a null object inside this list.. 
                        BASSERT(0);
                        continue;
                     }

							// Is this item flagged to be ignored?

							if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
								continue;

							// Check to see if the bounding boxes don't overlap

							if (BoundsCheck) // outright reject?
							{
                        if(FastFloat::compareLessThan(x2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                           FastFloat::compareLessThan(z2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],x1) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],z1))
                           continue;
							}

							// Perform object propery checks (node Type Matching)

							if (nodeTypes != cObsNodeTypeAll)
							{
								if ((theNode->mType & (BYTE) nodeTypes) == 0)
									continue;
							}
                     
                     // Allow the caller a crack at rejecting it.
                     if(checkUnitCB)
                     {
                        bool checkResult = checkUnitCB(theNode->mObject, checkUnitParam);
                        if(!checkResult)
                           continue;
                     }
                     
							// Check if object is player owned, and is on player ignore list
							if (lPlayerMask)
							{
								if (theNode->mPlayerID != 0)
								{
#ifdef CANJUMP_CHANGE
                           if (mbCanJump)
                              continue;
#else
									if ((ObsSystemPlayerMasks[theNode->mPlayerID] & lPlayerMask) == lPlayerMask)
										continue;
#endif
								}
							}

							// If no expansion takes place, note there is no expanded hull attached

							if (expand)
							{
								ProcessedHull = getExpandedHullPoints(theNode, expansionRadius);
							}
							else
							{
								ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);
							}

							// Test Center Point
                     if (!ProcessedHull)
                     {
                        BASSERT(0);
                        continue;
                     }
							if (ProcessedHull->overlapsHull(theHull))
							{
								// Now we add the obstruction to our results list 

								theList.add(theNode);
							}

						} //end for n loop

					} // end for x loop
				}	// End for z loop

				// Process next highest level in quadtree
				CurLevel--;		

			}  // End Loop to process all quadtree levels


		}  
		CurTree++;
		QuadTreesToScan >>= 1;
	}

	
	// Now we reset the ignore list if we were using that....
	if (ignoreCurrentList)
		unmarkObstructionsToBeIgnored(theList, incomingSize);


}

//==============================================================================
// BObstructionManager::findObstructionsNoExpansion
//==============================================================================
void BObstructionManager::findObstructionsNoExpansionQuadHull(const BOPQuadHull *pHull, bool ignoreCurrentList, 
                                                               BObstructionNodePtrArray &obstructions)
{

   if (!mInUse)
   {
	   BASSERT(0);
      return;
   }


	float ExpansionRadius = 0.0f;

	mInUse = false;				// Disable override...

	findObstructionsQuadHull(pHull, ExpansionRadius, mQuadTreesToScan, mValidNodeTypes, 
      mlPlayerID, ignoreCurrentList, obstructions);

	mInUse = true;

}


//==============================================================================
// BObstructionManager::findObstructions
//==============================================================================
void BObstructionManager::findObstructionsQuadHull(const BOPQuadHull *pHull, float expansionRadius, long QuadTreesToScan, 
                                                   long nodeTypes, long lPlayerID, bool ignoreCurrentList, BObstructionNodePtrArray &theList)
{
	
	long	incomingSize = theList.getNumber();

	if (ignoreCurrentList)
		markObstructionsToBeIgnored(theList, incomingSize);
	else
		theList.setNumber(0);

	// Handle Begin/End Mode

	if (mInUse)
	{
		expansionRadius = mRadius;
		QuadTreesToScan |= mQuadTreesToScan;
		nodeTypes |=mValidNodeTypes;
		lPlayerID = mlPlayerID;
	}

   // Set up Player Mask
   long lPlayerMask = 0;
   if (lPlayerID > 0)
      lPlayerMask = 0x0001 << (lPlayerID - 1);

	// Are we expanding hulls?

	bool expand = (expansionRadius != 0.0f);

	// Get True Coordinates for Hull Bounding Box

   float tx1 = pHull->mDirectVal[pHull->mIdxMinX];
   float tz1 = pHull->mDirectVal[pHull->mIdxMinZ];
   float tx2 = pHull->mDirectVal[pHull->mIdxMaxX];
   float tz2 = pHull->mDirectVal[pHull->mIdxMaxZ];

	// copy the coordinates for area expansion		
	float x1 = tx1, x2 = tx2, z1 = tz1, z2 = tz2;
	
	// Get expanded search area

	if (expand)
	{
	   // jce [10/13/2008] -- expanding by the hypotenuse here because this is used for overlap
	   // checks with rotated obstructions so at the corners using expansionRadius is not enough
	   // for a proper test.
	   float expandBy = sqrtf(2.0f*expansionRadius*expansionRadius);
	   
		x1 -= expandBy;
		x2 += expandBy;
		z1 -= expandBy;
		z2 += expandBy;
	}

	// clamp to world

	if (x1 < 0.0f) x1 = 0.0f;
	if (z1 < 0.0f) z1 = 0.0f;
	if (x2 > mfWidth)  x2 = mfWidth;
	if (z2 > mfHeight) z2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) x1;
	X2 = (long) (x2 - cNodeDropDownFactor);

	Z1 = (long) z1;
	Z2 = (long) (z2 - cNodeDropDownFactor);

	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);

	long CurTree = 0;
	long CurLevel, StartX, EndX, StartZ, EndZ;

	BOPQuadHull	*ProcessedHull;
   BFixedString<256> temp;
	while (QuadTreesToScan != 0)
	{
		if (QuadTreesToScan & 0x01)
		{
			// Start at Bottom level ofQuadtree
			CurLevel = mNumQuadTreeLevels - 1;	

			while (CurLevel >= 0)
			{
				// Compute range of QuadCells to scan for this level
				StartX = X1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndX   = X2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				StartZ = Z1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndZ   = Z2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				// Do we want to check for wholly contained nodes?

				bool insideCells = ((EndX - StartX) > 1) && ((EndZ - StartZ) > 1);

				// Process a 2d array range of Quad Nodes

				for (long z = StartZ; z <= EndZ; z++)
				{
					long NodeIndex = mQTLevelInfo[CurTree][CurLevel].mQTNodeIndex + (z << CurLevel);
					bool processAll = (insideCells && z > StartZ && z < EndZ);

               // jce [2/25/2005] -- sanity checks to figure out why this is crashing right now.
               if(CurTree<0 || CurTree>=cNumQuadTrees)
               {
                  
                  temp.format(B("Bad CurTree (%d) in obstruction manager."), CurTree);
                  BFAIL(temp);
                  return;
               }
               long fullIndex = NodeIndex+StartX;
               if(fullIndex<0 || fullIndex >= mNumQuadTreeNodes)
               {
                 
                  temp.format(B("Bad index (%d+%d = %d) in obstruction manager, max is %d."), NodeIndex, StartX, fullIndex, mNumQuadTreeNodes);
                  BFAIL(temp);
                  return;
               }

					BOPQuadTreeNode* CurQuadNode = &mQuadTreeNodes[CurTree][NodeIndex + StartX];

					for (long x = StartX; x <= EndX; x++, CurQuadNode++)
					{
						bool BoundsCheck = !(processAll && x > StartX && x < EndX);

						for (long n = 0; n < CurQuadNode->mNumObstructions; n++)
						{
							// Get the Obstruction to process

							BOPObstructionNode * theNode = CurQuadNode->mObstructionList[n];
                     if (!theNode)
                     {
                        // What's up here.. how is it possible to get
                        // a null object inside this list.. 
                        BASSERT(0);
                        continue;
                     }

							// Is this item flagged to be ignored?

							if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
								continue;


							// Check to see if the bounding boxes don't overlap

							if (BoundsCheck) // outright reject?
							{
                        if(FastFloat::compareLessThan(x2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                           FastFloat::compareLessThan(z2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],x1) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],z1))
                           continue;
							}

							// Perform object propery checks (node Type Matching)

							if (nodeTypes != cObsNodeTypeAll)
							{
								if ((theNode->mType & (BYTE) nodeTypes) == 0)
									continue;
							}

							// Check if object is player owned, and is on player ignore list

							if (lPlayerMask)
							{
								if (theNode->mPlayerID != 0)
								{
#ifdef CANJUMP_CHANGE
                           if (mbCanJump)
                              continue;
#else
									if ((ObsSystemPlayerMasks[theNode->mPlayerID] & lPlayerMask) == lPlayerMask)
										continue;
#endif
								}
							}

							// If no expansion takes place, note there is no expanded hull attached

							if (expand)
							{
								ProcessedHull = getExpandedHullPoints(theNode, expansionRadius);
							}
							else
							{
								ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);
							}

                     if (!ProcessedHull)
                     {
                        BASSERT(0);
                        continue;
                     }
							if (ProcessedHull->overlapsHull(pHull))
							{
								// Now we add the obstruction to our results list 

								theList.add(theNode);
							}

						} //end for n loop

					} // end for x loop
				}	// End for z loop

				// Process next highest level in quadtree
				CurLevel--;		

			}  // End Loop to process all quadtree levels


		}  
		CurTree++;
		QuadTreesToScan >>= 1;
	}

	
	// Now we reset the ignore list if we were using that....

	if (ignoreCurrentList)
		unmarkObstructionsToBeIgnored(theList, incomingSize);


}

//==============================================================================
// BObstructionManager::findObstructionsOnPoint
//==============================================================================
bool BObstructionManager::findObstructionsOnPoint(const BVector thePoint, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID, bool ignoreCurrentList, BObstructionNodePtrArray& theList)
{
   SCOPEDSAMPLE(BObstructionManager_findObstructionsOnPoint);
	long	incomingSize = 0;
   if (ignoreCurrentList)
   {
      incomingSize = theList.getNumber();
		markObstructionsToBeIgnored(theList, incomingSize);
   }
	else
	{
	   theList.setNumber(0);
	}


	// Handle Begin/End Mode

	if (mInUse)
	{
		expansionRadius = mRadius;
		QuadTreesToScan |= mQuadTreesToScan;
		nodeTypes |=mValidNodeTypes;
		lPlayerID = mlPlayerID;
	}

   // Set up Player Mask
   long lPlayerMask = 0;
   if (lPlayerID > 0)
      lPlayerMask = 0x0001 << (lPlayerID - 1);

	// Are we expanding hulls?
	bool expand = (expansionRadius != 0.0f);

	// Get True Coordinates

	float unrotatedX1 = thePoint.x;
	float unrotatedZ1 = thePoint.z;
	float unrotatedX2 = unrotatedX1;
	float unrotatedZ2 = unrotatedZ1;

	float rotatedX1 = unrotatedX1;
	float rotatedZ1 = unrotatedZ1;
	float rotatedX2 = unrotatedX2;
	float rotatedZ2 = unrotatedZ2;

	// Get expanded search area

	if (expand)
	{
		unrotatedX1 -= expansionRadius;
		unrotatedX2 += expansionRadius;
		unrotatedZ1 -= expansionRadius;
		unrotatedZ2 += expansionRadius;
		
	   // jce [10/13/2008] -- expanding by the hypotenuse here because this is used for overlap
	   // checks with rotated obstructions so at the corners using expansionRadius is not enough
	   // for a proper test.
	   float expandBy = sqrtf(2.0f*expansionRadius*expansionRadius);
	   
		rotatedX1 -= expandBy;
		rotatedX2 += expandBy;
		rotatedZ1 -= expandBy;
		rotatedZ2 += expandBy;
	}

	// clamp to world

	if (rotatedX1 < 0.0f)
	   rotatedX1 = 0.0f;
	if (rotatedZ1 < 0.0f)
	   rotatedZ1 = 0.0f;
	if (rotatedX2 > mfWidth)
	   rotatedX2 = mfWidth;
	if (rotatedZ2 > mfHeight)
	   rotatedZ2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) rotatedX1;
	X2 = (long) (rotatedX2 - cNodeDropDownFactor);

	Z1 = (long) rotatedZ1;
	Z2 = (long) (rotatedZ2 - cNodeDropDownFactor);

	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);

	long CurTree = 0;
	long CurLevel, StartX, EndX, StartZ, EndZ;

	BOPQuadHull	*ProcessedHull;

   BFixedString<256> temp;
	while (QuadTreesToScan != 0)
	{
		if (QuadTreesToScan & 0x01)
		{
			// Start at Bottom level ofQuadtree
			CurLevel = mNumQuadTreeLevels - 1;	

			while (CurLevel >= 0) 
			{
				// Compute range of QuadCells to scan for this level
				StartX = X1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndX   = X2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				StartZ = Z1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndZ   = Z2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				// Do we want to check for wholly contained nodes?

				bool insideCells = ((EndX - StartX) > 1) && ((EndZ - StartZ) > 1);

				// Process a 2d array range of Quad Nodes

				for (long z = StartZ; z <= EndZ; z++)  
				{
					long NodeIndex = mQTLevelInfo[CurTree][CurLevel].mQTNodeIndex + (z << CurLevel);               
					bool processAll = (insideCells && z > StartZ && z < EndZ); 

               // jce [2/25/2005] -- sanity checks to figure out why this is crashing right now.
               if(CurTree<0 || CurTree>=cNumQuadTrees)
               {
                  temp.format(B("Bad CurTree (%d) in obstruction manager."), CurTree);
                  BFAIL(temp);
                  return false;
               }
               long fullIndex = NodeIndex+StartX;
               if(fullIndex<0 || fullIndex >= mNumQuadTreeNodes) 
               {
                  temp.format(B("Bad index (%d+%d = %d) in obstruction manager, max is %d."), NodeIndex, StartX, fullIndex, mNumQuadTreeNodes);
                  BFAIL(temp);
                  return false;
               }

					BOPQuadTreeNode* CurQuadNode = &mQuadTreeNodes[CurTree][NodeIndex + StartX];

					for (long x = StartX; x <= EndX; x++, CurQuadNode++) 
					{
						bool BoundsCheck = !(processAll && x > StartX && x < EndX); 

						for (long n = 0; n < CurQuadNode->mNumObstructions; n++) 
						{
							// Get the Obstruction to process

							BOPObstructionNode * theNode = CurQuadNode->mObstructionList[n];
                     if (!theNode)
                     {
                        // What's up here.. how is it possible to get
                        // a null object inside this list.. 
                        BASSERT(0);
                        continue;
                     }

							// Is this item flagged to be ignored?

							if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
								continue;


							// See if the object is rotated
							bool rotated = ((theNode->mRotation & cIsObstructionRotated) != 0);

							// Check to see if the bounding boxes don't overlap
							if (BoundsCheck) // outright reject?
							{
							   if(rotated)
							   {
                           //CLM [04.30.08] removal of multiple FCMP 
                           if(FastFloat::compareLessThan(rotatedX2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                              FastFloat::compareLessThan(rotatedZ2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],rotatedX1) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],rotatedZ1))
                              continue;
							   }
							   else
							   {
                           //CLM [04.30.08] removal of multiple FCMP 
                           if(FastFloat::compareLessThan(unrotatedX2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                              FastFloat::compareLessThan(unrotatedZ2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],unrotatedX1) ||
                              FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],unrotatedZ1))
                              continue;
                        }
							}

							// Perform object propery checks (node type matching)

							if (nodeTypes != cObsNodeTypeAll)
							{
								if ((theNode->mType & (BYTE) nodeTypes) == 0)
									continue;
							}

							// Check if object is player owned, and is on player ignore list

							if (lPlayerMask)
							{
								if (theNode->mPlayerID != 0)
								{
#ifdef CANJUMP_CHANGE
                           if (mbCanJump)
                              continue;
#else
									if ((ObsSystemPlayerMasks[theNode->mPlayerID] & lPlayerMask) == lPlayerMask)
										continue;
#endif
								}
							}

							// If it's not rotated, then the item overlaps, and we add it
							// If it's rotated, we have to make some checks

							if (rotated)
							{
								// Get either the expanded hull, or the actual hull

								if (expand)
									ProcessedHull = getExpandedHullPoints(theNode, expansionRadius);
								else
									ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);

                        if (!ProcessedHull)
                        {
                           BASSERT(0);
                           continue;
                        }

								if (!ProcessedHull->inside(thePoint))
								{
									continue;
								}

							} // end if(rotated) check


							// Now we add the obstruction to our results list 
							theList.add(theNode);

						} //end for n loop

					} // end for x loop
				}	// End for z loop

				// Process next highest level in quadtree
				CurLevel--;		

			}  // End Loop to process all quadtree levels


		}  
		CurTree++;
		QuadTreesToScan >>= 1;
	}
	
	// Now we reset the ignore list if we were using that....
   if (ignoreCurrentList)
		unmarkObstructionsToBeIgnored(theList, incomingSize);

   return (theList.getNumber() > 0);
}


//==============================================================================
// BObstructionManager::saveOrdering
//==============================================================================
bool BObstructionManager::saveOrdering(BChunkWriter* chunkWriter)
{
   // ajl 6/7/02 - Save the ordering of obstruction nodes within each quadtree node. This
   // is needed to prevent record games from going out of sync because if it's not done
   // then the results from the various find functions will sometimes be ordered differently
   // between the original game and the played back game.

   long result;
   long version=3;
   CHUNKWRITESAFE(chunkWriter, Long, version);

   //Write tag.
   long mainHandle;
   result=chunkWriter->writeTagPostSized(BCHUNKTAG("OO"), mainHandle);
   if (!result)
   {
      {setBlogError(359); blogerror("BObstructionManager::saveOrdering -- error writing tag.");}
      return(false);
   }

   long numTrees = cObsTypeDoppleganger + 1; // Ignoring land stuff
//   long numTrees = cNumQuadTrees;

   CHUNKWRITESAFE(chunkWriter, Long, numTrees);
   CHUNKWRITESAFE(chunkWriter, Long, mNumQuadTreeNodes);
   
   for(long i=0; i<numTrees; i++)
   {
      BOPQuadTreeNode* nodeList=mQuadTreeNodes[i];

      for (long n = 0; n < mNumQuadTreeNodes; n++)
      {
         long numObs = nodeList[n].mNumObstructions;

         CHUNKWRITESAFE(chunkWriter, Long, numObs);

         if (numObs > 0)
         {
            BOPObstructionNode** p=nodeList[n].mObstructionList;	
            for (long z = 0; z < numObs; z++)
               CHUNKWRITESAFE(chunkWriter, Long, p[z]->mEntityID);
         }
      }
   }

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(360); blogerror("BObstructionManager::saveOrdering -- failed to write chunk size!");}
      return(false);
   }

   return (true);
}


//==============================================================================
// BObstructionManager::loadOrdering
//==============================================================================
bool BObstructionManager::loadOrdering(BChunkReader* chunkReader)
{
   // ajl 6/7/02 - Load the ordering of obstruction nodes within each quadtree node. This
   // is needed to prevent record games from going out of sync because if it's not done
   // then the results from the various find functions will sometimes be ordered differently
   // between the original game and the played back game.

   long result;
   long version;
   CHUNKREADSAFE(chunkReader, Long, version);

   if (version==1)
      return(true);

   if(version>=3)
   {
      // Read tag.
      result=chunkReader->readExpectedTag(BCHUNKTAG("OO"));
      if (!result)
      {
         {setBlogError(361); blogerror("BObstructionManager::loadOrdering -- error reading tag.");}
         return(false);
      }
   }

   // Read the data.
   int error=0;

   long numTrees;
   CHUNKREADSAFE(chunkReader, Long, numTrees);
   if(numTrees!=cObsTypeDoppleganger+1) // Ignoring land stuff
   {
      error=1;
      {setBlogError(362); blogerror("BObstructionManager::loadOrdering -- Invalid numTrees (%d vs %d)", numTrees, cObsTypeDoppleganger+1);}
   }

   long numNodes;
   CHUNKREADSAFE(chunkReader, Long, numNodes);
   if (!error && numNodes!=mNumQuadTreeNodes)
   {
      error=2;
      {setBlogError(363); blogerror("BObstructionManager::loadOrdering -- Invalid numNodes (%d vs %d)", numNodes, mNumQuadTreeNodes);}
   }

   BDynamicSimLongArray loadedIDs;
   for(long i=0; i<numTrees; i++)
   {
      BOPQuadTreeNode* nodeList=(error?NULL:mQuadTreeNodes[i]);
      for (long n=0; n<numNodes; n++)
      {
         long numObs;
         CHUNKREADSAFE(chunkReader, Long, numObs);
         if(!error && numObs!=nodeList[n].mNumObstructions)
         {
            error=3;
            {setBlogError(364); blogerror("BObstructionManager::loadOrdering -- Invalid numObs (%d vs %d) for node %d", numObs, nodeList[n].mNumObstructions, n);}
         }

         long matchCount=0;
         for (long z=0; z<numObs; z++)
         {
            long entityID;
            CHUNKREADSAFE(chunkReader, Long, entityID);
            if(!error)
            {
               loadedIDs.add(entityID);
               BOPObstructionNode** p=nodeList[n].mObstructionList;	
               for (long c=0; c<numObs; c++)
               {
                  if(p[c]->mEntityID==entityID)
                  {
                     matchCount++;
                     break;
                  }
               }
            }
         }
         if(!error && matchCount!=numObs)
         {
            error=4;
            {setBlogError(365); blogerror("BObstructionManager::loadOrdering -- Invalid matchCount (%d vs %d) for node %d", matchCount, numObs, n);}
         }
      }
   }

   if(version>=3)
   {
      // Validate our reading of the chunk.
      result=chunkReader->validateChunkRead(BCHUNKTAG("OO"));
      if(!result)
      {
         {setBlogError(366); blogerror("BObstructionManager::loadOrdering -- did not read chunk properly!");}
         return(false);
      }
   }

   if(error)
      return(true);

   // Actually do the node reordering.
   BDynamicSimArray<BOPObstructionNode*> orderedNodes;
   long index=0;
   for(long i=0; i<numTrees; i++)
   {
      BOPQuadTreeNode* nodeList=mQuadTreeNodes[i];
      for (long n=0; n<numNodes; n++)
      {
         long numObs=nodeList[n].mNumObstructions;
         if(numObs>0)
         {
            orderedNodes.setNumber(0);
            BOPObstructionNode** p=nodeList[n].mObstructionList;	
            bool allEntities=true;
            for (long z=0; z<numObs; z++)
            {
               long entityID=loadedIDs[index];
               if(entityID==-1) // Have to check this because versions < 3 were saving out the cObsTypeNonSolid tree (passable but not buildable land).
                  allEntities=false;
               else
               {
                  for (long c=0; c<numObs; c++)
                  {
                     if(p[c]->mEntityID==entityID)
                     {
                        orderedNodes.add(p[c]);
                        break;
                     }
                  }
               }
               index++;
            }
            if(allEntities)
            {
               BASSERT(orderedNodes.getNumber()==numObs);
               for (long c=0; c<numObs; c++)
               {
                  if(p[c] != orderedNodes[c])
                  {
                     p[c] = orderedNodes[c];
                     p[c]->mThisNodePointerIndex = c;
                  }
               }
            }
         }
      }
   }

   BASSERT(index==loadedIDs.getNumber());

   return (true);
}



//==============================================================================
// BObstructionManager::getSessionState
//==============================================================================
bool BObstructionManager::getSessionState(BSaveObsManagerState &theData)
{
	theData.mSRadius							= mRadius;				
	theData.mSRelaxedRadius					= mRelaxedRadius;	
	theData.mSMode								= mMode;				

	theData.mSQuadTreestoScan				= mQuadTreesToScan;
	theData.mSValidNodeTypes				= mValidNodeTypes;
	theData.mSPlayerID     				   = mlPlayerID;

	theData.mSEntityIgnoreList          = mEntityIgnoreList;				

	return(mInUse);

}


//==============================================================================
// BObstructionManager::setSessionState
//==============================================================================
void BObstructionManager::setSessionState(BSaveObsManagerState &theData)
{
	mRadius									= theData.mSRadius;				
	mRelaxedRadius							= theData.mSRelaxedRadius;	
	mMode										= theData.mSMode;				

	mQuadTreesToScan						= theData.mSQuadTreestoScan;
	mValidNodeTypes						= theData.mSValidNodeTypes;
	mlPlayerID	      					= theData.mSPlayerID;

   // Don't call the array assignment operator since that will free the array's memory and the realloc it.
   // The lists being copied here always seemed to be empty so it probably wasn't hurting things much.
   mEntityIgnoreList.setNumber(theData.mSEntityIgnoreList.getNumber());
   for (int i = 0; i < theData.mSEntityIgnoreList.getNumber(); i++)
   {
      mEntityIgnoreList[i] = theData.mSEntityIgnoreList[i];
   }
}

//==============================================================================
// BObstructionManager::setupEntityObstructionData
//==============================================================================
void BObstructionManager::setupEntityObstructionData(long& QuadTreesToScan, long& NodeTypes, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   mFoundObstructionResults.setNumber(0);

   if (isSquadCheck)
   {
      // VAT: 11/03/08: the addition of block air movement and nonmovable units are 
      // huge hacks, since buildings never have a squad obstruction. instead, 
      // we find their unit obstructions and validate in the unit query
      QuadTreesToScan = cIsNewTypeAllCollidableSquads | 
                        cIsNewTypeBlockAirMovement | cIsNewTypeCollidableNonMovableUnit; 
      NodeTypes = cObsNodeTypeSquad | cObsNodeTypeUnit;      

      // yes, this can be added onto unit or squad checks, for objects that have a noncollidable unit obstruction but not a squad obstruction
      if (includeNonCollidableUnitObs)
         QuadTreesToScan |= cIsNewTypeNonCollidableUnit;
      else
         QuadTreesToScan |= cIsNewTypeNonCollidableSquad;
   }
   else
   {
      QuadTreesToScan = cIsNewTypeAllCollidableUnits;
      NodeTypes = cObsNodeTypeUnit;

      if (includeNonCollidableUnitObs)
         QuadTreesToScan |= cIsNewTypeNonCollidableUnit;
   }

}

//==============================================================================
// BObstructionManager::findEntityObstructions
//==============================================================================
void BObstructionManager::findEntityObstructions(const BConvexHull &theHull, bool includeNonCollidableUnitObs, bool isSquadCheck, CHECKUNIT_CALLBACK checkUnitCB, void *checkUnitParam)
{
   long QuadTreesToScan = 0;
   long NodeTypes = 0;
   setupEntityObstructionData(QuadTreesToScan, NodeTypes, includeNonCollidableUnitObs, isSquadCheck);
	findObstructions(theHull, 0.0f, QuadTreesToScan, NodeTypes, cNoPlayerOwnedIgnores, false, mFoundObstructionResults, checkUnitCB, checkUnitParam);
}

//==============================================================================
// BObstructionManager::findEntityObstructions
//==============================================================================
void BObstructionManager::findEntityObstructions(const float minX, const float minZ, const float maxX, const float maxZ, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   long QuadTreesToScan = 0;
   long NodeTypes = 0;
   setupEntityObstructionData(QuadTreesToScan, NodeTypes, includeNonCollidableUnitObs, isSquadCheck);

   //Figure out the min and max points (add/sub 0.02f in to make sure that we don't have a degenerate box).
   float myMinX=min(minX, maxX)-0.02f;
   float myMaxX=max(minX, maxX)+0.02f;
   float myMinZ=min(minZ, maxZ)-0.02f;
   float myMaxZ=max(minZ, maxZ)+0.02f;

	findObstructions(myMinX, myMinZ, myMaxX, myMaxZ, 0.0f, QuadTreesToScan, NodeTypes, cNoPlayerOwnedIgnores, false, mFoundObstructionResults);
}

//==============================================================================
// BObstructionManager::findEntityObstructions
//==============================================================================
void BObstructionManager::findEntityObstructions(const BVector p0, const BVector p1, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   long QuadTreesToScan = 0;
   long NodeTypes = 0;
   setupEntityObstructionData(QuadTreesToScan, NodeTypes, includeNonCollidableUnitObs, isSquadCheck);

   //Figure out the min and max points (add/sub 0.02f in to make sure that we don't have a degenerate box).
   float myMinX=min(p0.x, p1.x)-0.02f;
   float myMaxX=max(p0.x, p1.x)+0.02f;
   float myMinZ=min(p0.z, p1.z)-0.02f;
   float myMaxZ=max(p0.z, p1.z)+0.02f;

   findObstructions(myMinX, myMinZ, myMaxX, myMaxZ, 0.0f, QuadTreesToScan, NodeTypes, cNoPlayerOwnedIgnores, false, mFoundObstructionResults);
}

//==============================================================================
// BObstructionManager::findEntityObstructions
//==============================================================================
void BObstructionManager::findEntityObstructions(const BVector point, const float radius, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   long QuadTreesToScan = 0;
   long NodeTypes = 0;
   setupEntityObstructionData(QuadTreesToScan, NodeTypes, includeNonCollidableUnitObs, isSquadCheck);

   findObstructionsOnPoint(point, radius, QuadTreesToScan, NodeTypes, cNoPlayerOwnedIgnores, false, mFoundObstructionResults);
}

//==============================================================================
// BObstructionManager::findEntityObstructionsQuadHull
//==============================================================================
void BObstructionManager::findEntityObstructionsQuadHull(const BOPQuadHull *pHull, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   long QuadTreesToScan = 0;
   long NodeTypes = 0;
   setupEntityObstructionData(QuadTreesToScan, NodeTypes, includeNonCollidableUnitObs, isSquadCheck);

	findObstructionsQuadHull(pHull, 0.0f, QuadTreesToScan, NodeTypes, cNoPlayerOwnedIgnores, false, mFoundObstructionResults);
}

//==============================================================================
// BObstructionManager::GetObjectsIntersections
//==============================================================================
bool BObstructionManager::getObjectsIntersections(const ObsSegIntersectMode mode, const BVector &point1, const BVector &point2, const bool useRelaxed,
                                                       BVector &intersectPoint, BObstructionNodePtrArray &obstructions)
{
	// We had better be in a Beg/End cycle

	if (!mInUse)
	{
		BASSERT(0);
		return(false);
	}

   // Clear the list..
   obstructions.setNumber(0);

	// Get variables from Begin() call

	long QuadTreesToScan = mQuadTreesToScan;
	long nodeTypes = mValidNodeTypes;

	float	expansionRadius = 0.0f;

	if (useRelaxed)
		expansionRadius = mRelaxedRadius;
	else
		expansionRadius = mRadius;

   // Set up Player Mask
   long lPlayerMask = 0;
   if (mlPlayerID > 0)
      lPlayerMask = 0x0001 << (mlPlayerID - 1);

   // Are we expanding hulls?
	bool expand = (expansionRadius != 0.0f);

	// Get True Coordinates

	float x1 = min(point1.x, point2.x);
	float z1 = min(point1.z, point2.z);
	float x2 = max(point1.x, point2.x);
	float z2 = max(point1.z, point2.z);

	// Get expanded search area

	if (expand)
	{
	   // jce [10/13/2008] -- expanding by the hypotenuse here because this is used for overlap
	   // checks with rotated obstructions so at the corners using expansionRadius is not enough
	   // for a proper test.
	   float expandBy = sqrtf(2.0f*expansionRadius*expansionRadius);
	   
		x1 -= expandBy;
		x2 += expandBy;
		z1 -= expandBy;
		z2 += expandBy;
	}

	// clamp to world

	if (x1 < 0.0f) x1 = 0.0f;
	if (z1 < 0.0f) z1 = 0.0f;
	if (x2 > mfWidth)  x2 = mfWidth;
	if (z2 > mfHeight) z2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) x1;
	X2 = (long) (x2 - cNodeDropDownFactor);

	Z1 = (long) z1;
	Z2 = (long) (z2 - cNodeDropDownFactor);

	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);

	// Setup scanning loop

	long CurTree = 0;
	long CurLevel, StartX, EndX, StartZ, EndZ;

	BOPQuadHull	*ProcessedHull;

   float		bestDistSqr = cMaximumFloat;
	float		thisDistSqr;
	bool		intersectionFound = false;
	BVector	localIntersectPoint;
	long		intersectSegment;
	long		numIntersections;

   BFixedString<64> temp;
	while (QuadTreesToScan != 0)
	{
		if (QuadTreesToScan & 0x01)
		{
			// Start at Bottom level ofQuadtree
			CurLevel = mNumQuadTreeLevels - 1;	

			while (CurLevel >= 0)
			{
				// Compute range of QuadCells to scan for this level
				StartX = X1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndX   = X2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				StartZ = Z1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndZ   = Z2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				// Do we want to check for wholly contained nodes?

				bool insideCells = ((EndX - StartX) > 1) && ((EndZ - StartZ) > 1);

				// Process a 2d array range of Quad Nodes

				for (long z = StartZ; z <= EndZ; z++)
				{
					long NodeIndex = mQTLevelInfo[CurTree][CurLevel].mQTNodeIndex + (z << CurLevel);
					bool processAll = (insideCells && z > StartZ && z < EndZ);

               // jce [2/25/2005] -- sanity checks to figure out why this is crashing right now.
               if(CurTree<0 || CurTree>=cNumQuadTrees)
               {
                 
                  temp.format(B("Bad CurTree (%d) in obstruction manager."), CurTree);
                  BFAIL(temp);
                  return(false);
               }
               long fullIndex = NodeIndex+StartX;
               if(fullIndex<0 || fullIndex >= mNumQuadTreeNodes)
               {
                 
                  temp.format(B("Bad index (%d+%d = %d) in obstruction manager, max is %d."), NodeIndex, StartX, fullIndex, mNumQuadTreeNodes);
                  BFAIL(temp);
                  return(false);
               }

					BOPQuadTreeNode* CurQuadNode = &mQuadTreeNodes[CurTree][NodeIndex + StartX];

					for (long x = StartX; x <= EndX; x++, CurQuadNode++)
					{
						bool BoundsCheck = !(processAll && x > StartX && x < EndX);

						//for (long n = 0; n < CurQuadNode->mNumObstructions; n++)
						for (long n = CurQuadNode->mNumObstructions-1; n >= 0; n--)
						{
							// Get the Obstruction to process

							BOPObstructionNode * theNode = CurQuadNode->mObstructionList[n];
                     if (!theNode)
                     {
                        // What's up here.. how is it possible to get
                        // a null object inside this list.. 
                        BASSERT(0);
                        continue;
                     }

							// Perform object propery checks (node type matching)
							//if (nodeTypes != cObsNodeTypeAll)
							//{
								if ((theNode->mType & (BYTE) nodeTypes) == 0)
									continue;
							//}

							// Is this item flagged to be ignored?

							if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
								continue;


							// Check to see if the bounding boxes don't overlap

							if (BoundsCheck) // outright reject?
							{
                        //CLM [04.30.08] removal of multiple FCMP 
                        if(FastFloat::compareLessThan(x2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                           FastFloat::compareLessThan(z2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],x1) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],z1))
                           continue;

                       //origional
								/*if (theNode->mDirectVal[theNode->mIdxMinX] > x2 ||
									 theNode->mDirectVal[theNode->mIdxMinZ] > z2 ||
									 theNode->mDirectVal[theNode->mIdxMaxX] < x1 ||
									 theNode->mDirectVal[theNode->mIdxMaxZ] < z1)
									continue;*/
							}


							// Check if object is player owned, and is on player ignore list

							if (lPlayerMask)
							{
								if (theNode->mPlayerID != 0)
								{
#ifdef CANJUMP_CHANGE
                           if (mbCanJump)
                              continue;
#else
									if ((ObsSystemPlayerMasks[theNode->mPlayerID] & lPlayerMask) == lPlayerMask)
										continue;
#endif
								}
							}

							// Get either the expanded hull, or the actual

							if (expand)
								ProcessedHull = getExpandedHullPoints(theNode, expansionRadius);
							else
							{
								ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);
							}

                     if (!ProcessedHull)
                     {
                        BASSERT(0);
                        continue;
                     }
							// Peform Check
							bool intersect = ProcessedHull->segmentIntersects(point1, point2, -1, localIntersectPoint, intersectSegment, thisDistSqr, numIntersections, true);

							if (intersect)
							{
								intersectionFound = true;

								// Keep track of intersection closest to point1

								if(thisDistSqr < bestDistSqr)
								{
									intersectPoint = localIntersectPoint;
									bestDistSqr = thisDistSqr;
								}
							}
							else if(ProcessedHull->inside(point1))
							{
								// If inside the set as closet intersection
								intersectionFound = true;
								intersect = true;
								bestDistSqr = 0.0f;
								intersectPoint = point1;

							}

							// Getting a list of all intersections?
							if (intersect && mode == cGetAllIntersections)
							{
                        obstructions.add(theNode);
							}

							// Are we looking for any single intersection?

							if (intersect && mode == cGetAnyIntersect)
							{
								return(intersectionFound);
							}

						} //end for n loop

					} // end for x loop
				}	// End for z loop

				// Process next highest level in quadtree
				CurLevel--;		

			}  // End Loop to process all quadtree levels
		}  
		CurTree++;
		QuadTreesToScan >>= 1;
	}

	// Return our findings

	return(intersectionFound);

}


//==============================================================================
// BObstructionManager::GetObjectsIntersections (the complete version)
// NOTE:  this currently does not sort the results according to closest fDistDqr.
// if you find that you need this, it should prolly be a sep routine, to 
// keep this one "fast".  dlm 5/9/02
//==============================================================================
bool BObstructionManager::getObjectsIntersections(const ObsSegIntersectMode mode, const BVector &point1, const BVector &point2, const bool useRelaxed,
                                                       BSegIntersectResultArray &results)
{
	// We had better be in a Beg/End cycle

	if (!mInUse)
	{
		BASSERT(0);
		return(false);
	}

   // Clear the list..
   results.setNumber(0);

   BSegIntersectResult tempResult;

	// Get variables from Begin() call

	long QuadTreesToScan = mQuadTreesToScan;
	long nodeTypes = mValidNodeTypes;
	
	float	expansionRadius = 0.0f;

	if (useRelaxed)
		expansionRadius = mRelaxedRadius;
	else
		expansionRadius = mRadius;

   // Set up Player Mask
   long lPlayerMask = 0;
   if (mlPlayerID > 0)
      lPlayerMask = 0x0001 << (mlPlayerID - 1);

   // Are we expanding hulls?
	bool expand = (expansionRadius != 0.0f);

	// Get True Coordinates

	float x1 = min(point1.x, point2.x);
	float z1 = min(point1.z, point2.z);
	float x2 = max(point1.x, point2.x);
	float z2 = max(point1.z, point2.z);

	// Get expanded search area

	if (expand)
	{
	   // jce [10/13/2008] -- expanding by the hypotenuse here because this is used for overlap
	   // checks with rotated obstructions so at the corners using expansionRadius is not enough
	   // for a proper test.
	   float expandBy = sqrtf(2.0f*expansionRadius*expansionRadius);
	   
		x1 -= expandBy;
		x2 += expandBy;
		z1 -= expandBy;
		z2 += expandBy;
	}

	// clamp to world

	if (x1 < 0.0f) x1 = 0.0f;
	if (z1 < 0.0f) z1 = 0.0f;
	if (x2 > mfWidth)  x2 = mfWidth;
	if (z2 > mfHeight) z2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) x1;
	X2 = (long) (x2 - cNodeDropDownFactor);

	Z1 = (long) z1;
	Z2 = (long) (z2 - cNodeDropDownFactor);

	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);

	// Setup scanning loop

	long CurTree = 0;
	long CurLevel, StartX, EndX, StartZ, EndZ;

	BOPQuadHull	*ProcessedHull;

	float		thisDistSqr;
	bool		intersectionFound = false;
	BVector	localIntersectPoint;
	long		intersectSegment;
	long		numIntersections;

   BFixedString<64> temp;

	while (QuadTreesToScan != 0)
	{
		if (QuadTreesToScan & 0x01)
		{
			// Start at Bottom level ofQuadtree
			CurLevel = mNumQuadTreeLevels - 1;	

			while (CurLevel >= 0)
			{
				// Compute range of QuadCells to scan for this level
				StartX = X1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndX   = X2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				StartZ = Z1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndZ   = Z2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				// Do we want to check for wholly contained nodes?

				bool insideCells = ((EndX - StartX) > 1) && ((EndZ - StartZ) > 1);

				// Process a 2d array range of Quad Nodes

				for (long z = StartZ; z <= EndZ; z++)
				{
					long NodeIndex = mQTLevelInfo[CurTree][CurLevel].mQTNodeIndex + (z << CurLevel);
					bool processAll = (insideCells && z > StartZ && z < EndZ);

               // jce [2/25/2005] -- sanity checks to figure out why this is crashing right now.
               if(CurTree<0 || CurTree>=cNumQuadTrees)
               {
                 
                  temp.format(B("Bad CurTree (%d) in obstruction manager."), CurTree);
                  BFAIL(temp);
                  return(false);
               }
               long fullIndex = NodeIndex+StartX;
               if(fullIndex<0 || fullIndex >= mNumQuadTreeNodes)
               {
                 
                  temp.format(B("Bad index (%d+%d = %d) in obstruction manager, max is %d."), NodeIndex, StartX, fullIndex, mNumQuadTreeNodes);
                  BFAIL(temp);
                  return(false);
               }
					BOPQuadTreeNode* CurQuadNode = &mQuadTreeNodes[CurTree][NodeIndex + StartX];

					for (long x = StartX; x <= EndX; x++, CurQuadNode++)
					{
						bool BoundsCheck = !(processAll && x > StartX && x < EndX);

						//for (long n = 0; n < CurQuadNode->mNumObstructions; n++)
						for (long n = CurQuadNode->mNumObstructions-1; n >= 0; n--)
						{
							// Get the Obstruction to process

							BOPObstructionNode * theNode = CurQuadNode->mObstructionList[n];
                     if (!theNode)
                     {
                        // What's up here.. how is it possible to get
                        // a null object inside this list.. 
                        BASSERT(0);
                        continue;
                     }

							// Perform object propery checks (node type matching)
							//if (nodeTypes != cObsNodeTypeAll)
							//{
								if ((theNode->mType & (BYTE) nodeTypes) == 0)
									continue;
							//}

							// Is this item flagged to be ignored?

							if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
								continue;


							// Check to see if the bounding boxes don't overlap

							if (BoundsCheck) // outright reject?
							{
                        if(FastFloat::compareLessThan(x2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                           FastFloat::compareLessThan(z2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],x1) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],z1))
                           continue;
							}


							// Check if object is player owned, and is on player ignore list

							if (lPlayerMask)
							{
								if (theNode->mPlayerID != 0)
								{
#ifdef CANJUMP_CHANGE
                           if (mbCanJump)
                              continue;
#else
									if ((ObsSystemPlayerMasks[theNode->mPlayerID] & lPlayerMask) == lPlayerMask)
										continue;
#endif
								}
							}

							// Get either the expanded hull, or the actual

							if (expand)
								ProcessedHull = getExpandedHullPoints(theNode, expansionRadius);
							else
							{
								ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);
							}

                     if (!ProcessedHull)
                     {
                        BASSERT(0);
                        continue;
                     }

							// Peform Check
							bool intersect = ProcessedHull->segmentIntersects(point1, point2, -1, localIntersectPoint, intersectSegment, thisDistSqr, numIntersections, true);

							if (intersect)
							{
								intersectionFound = true;
                        // Fill up the result struct..
                        tempResult.fDistSqr = thisDistSqr;
                        tempResult.lSegmentIdx = intersectSegment;
                        tempResult.pObNode = theNode;
                        tempResult.vIntersect = localIntersectPoint;
                        results.add(tempResult);
							}

							// Early out?
							if (intersect && mode == cGetAnyIntersect)
							{
								return(intersectionFound);
							}

						} //end for n loop

					} // end for x loop
				}	// End for z loop

				// Process next highest level in quadtree
				CurLevel--;		

			}  // End Loop to process all quadtree levels
		}  
		CurTree++;
		QuadTreesToScan >>= 1;
	}

	// Return our findings

	return(intersectionFound);

}





//==============================================================================
// BObstructionManager::PerformNewWallCheck
//==============================================================================
long BObstructionManager::performNewWallCheck( const BVector &point1, const BVector &point2, long ignoreWallForPlayerID, BWallCheckResultArray &results,bool ignoreWalls)
{

   long lInsideCount = 0;
   // We had better be in a Beg/End cycle
	if (!mInUse)
	{
		BASSERT(0);
		return(false);
	}


   // Clear results.
   results.setNumber(0);

   // Get direction vector.
   BVector	dir	= point2 - point1;
   dir.y=0.0f;

   float length = dir.length();
   if(length<cFloatCompareEpsilon)
      return(-1);

   dir /= length;

   // Get normal.
   BVector normal(-dir.z, 0.0f, dir.x);

   // Get two test segment points.

   BVector segs[4];

   /*
   // "Plus" segment.
   segs[0]=point1 + mRadius*normal;
   segs[1]=point2 + mRadius*normal;

   // "Minus" segment.  In a directed sense, this segment goes from segs[3] to seg[2].  The points are
   // organized in clockwise order for inside-hull checks.

   segs[3]=point1 - mRadius*normal;
   segs[2]=point2 - mRadius*normal;
   */
   
   // "Plus" segment.
   segs[0]=point1 + mRadius*normal - mRadius*dir;
   segs[1]=point2 + mRadius*normal + mRadius*dir;

   // "Minus" segment.  In a directed sense, this segment goes from segs[3] to seg[2].  The points are
   // organized in clockwise order for inside-hull checks.
   segs[3]=point1 - mRadius*normal - mRadius*dir;
   segs[2]=point2 - mRadius*normal + mRadius*dir;


#ifdef DEBUG_WALLS
#ifndef BUILD_FINAL
      BConvexHull tempHull;
      tempHull.initialize(segs, 4, true);
      tempHull.addDebugLines("debugwalls", cColorYellow);
      debugAddPoint("debugwalls", point1, cColorWhite);
      debugAddPoint("debugwalls", point2, cColorBlack);
#endif
#endif 

	// Get variables from Begin() call

	long QuadTreesToScan = mQuadTreesToScan;
	long nodeTypes = mValidNodeTypes;


   // Set up Player Mask
   long lPlayerMask = 0;   
   if (ignoreWallForPlayerID > 0)
      lPlayerMask = 0x0001 << (ignoreWallForPlayerID - 1);

	// Get True Coordinates
   float x1 = cMaximumFloat;
   float z1 = cMaximumFloat;
   float x2 = -cMaximumFloat;
   float z2 = -cMaximumFloat;
   for (long m = 0; m < 4; m++)
   {
      if (segs[m].x < x1)
         x1 = segs[m].x;
      if (segs[m].z < z1)
         z1 = segs[m].z;
      if (segs[m].x > x2)
         x2 = segs[m].x;
      if (segs[m].z > z2)
         z2 = segs[m].z;
   }

	// clamp to world

	if (x1 < 0.0f) x1 = 0.0f;
	if (z1 < 0.0f) z1 = 0.0f;
	if (x2 > mfWidth)  x2 = mfWidth;
	if (z2 > mfHeight) z2 = mfHeight;

	// Determine which node level the obstruction belongs in.

	long X1, X2, Z1, Z2;

	X1 = (long) x1;
	X2 = (long) (x2 - cNodeDropDownFactor);

	Z1 = (long) z1;
	Z2 = (long) (z2 - cNodeDropDownFactor);


	// jce [3/2/2005] -- Clamp coords onto map since the below code is happy to crash if they aren't. 
	makeValidX(X1);
	makeValidZ(Z1);
	makeValidX(X2);
	makeValidZ(Z2);


	// Setup scanning loop

	long CurTree = 0;
	long CurLevel, StartX, EndX, StartZ, EndZ;

	BOPQuadHull	*ProcessedHull;

   BFixedString<64> temp;
	while (QuadTreesToScan != 0)
	{
		if (QuadTreesToScan & 0x01)
		{
			// Start at Bottom level ofQuadtree
			CurLevel = mNumQuadTreeLevels - 1;	

			while (CurLevel >= 0)
			{
				// Compute range of QuadCells to scan for this level
				StartX = X1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndX   = X2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				StartZ = Z1 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;
				EndZ   = Z2 >> mQTLevelInfo[CurTree][CurLevel].mShiftCount;

				// Do we want to check for wholly contained nodes?

				bool insideCells = ((EndX - StartX) > 1) && ((EndZ - StartZ) > 1);

				// Process a 2d array range of Quad Nodes

				for (long z = StartZ; z <= EndZ; z++)
				{
					long NodeIndex = mQTLevelInfo[CurTree][CurLevel].mQTNodeIndex + (z << CurLevel);
					bool processAll = (insideCells && z > StartZ && z < EndZ);

               // jce [2/25/2005] -- sanity checks to figure out why this is crashing right now.
               if(CurTree<0 || CurTree>=cNumQuadTrees)
               {
                 
                  temp.format(B("Bad CurTree (%d) in obstruction manager."), CurTree);
                  BFAIL(temp);
                  return(false);
               }
               long fullIndex = NodeIndex+StartX;
               if(fullIndex<0 || fullIndex >= mNumQuadTreeNodes)
               {
                 
                  temp.format(B("Bad index (%d+%d = %d) in obstruction manager, max is %d."), NodeIndex, StartX, fullIndex, mNumQuadTreeNodes);
                  BFAIL(temp);
                  return(false);
               }
					BOPQuadTreeNode* CurQuadNode = &mQuadTreeNodes[CurTree][NodeIndex + StartX];

					for (long x = StartX; x <= EndX; x++, CurQuadNode++)
					{
						bool BoundsCheck = !(processAll && x > StartX && x < EndX);

						for (long n = 0; n < CurQuadNode->mNumObstructions; n++)
						{
							// Get the Obstruction to process

							BOPObstructionNode * theNode = CurQuadNode->mObstructionList[n];
                     if (!theNode)
                     {
                        // What's up here.. how is it possible to get
                        // a null object inside this list.. 
                        BASSERT(0);
                        continue;
                     }

							// Is this item flagged to be ignored?

							if ((theNode->mProperties & cObsPropertyInIgnoreList) != 0)
								continue;


							// Check to see if the bounding boxes don't overlap

							if (BoundsCheck) // outright reject?
							{
                        if(FastFloat::compareLessThan(x2,theNode->mDirectVal[theNode->mIdxMinX]) ||
                           FastFloat::compareLessThan(z2,theNode->mDirectVal[theNode->mIdxMinZ]) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxX],x1) ||
                           FastFloat::compareLessThan(theNode->mDirectVal[theNode->mIdxMaxZ],z1))
                           continue;
							}

							// Perform object propery checks (node type matching)

							if (nodeTypes != cObsNodeTypeAll)
							{
								if ((theNode->mType & (BYTE) nodeTypes) == 0)
									continue;
							}

							// Check if object is player owned, and is on player ignore list

							if (lPlayerMask)
							{
								if (theNode->mPlayerID != 0)
								{
#ifdef CANJUMP_CHANGE
                           if (mbCanJump)
                              continue;
#else
									if ((ObsSystemPlayerMasks[theNode->mPlayerID] & lPlayerMask) == lPlayerMask)
										continue;
#endif
								}
							}

                     // Try.. if the obstruction is another wall, ignore it.  We'll build directly on top of walls. (especially connectors).
                     BEntity *pObject = NULL;
                     if (theNode->mProperties & cObsPropertyDeletedNode)
                     {
                        pObject = NULL;
                     }
                     /*else if(theNode->mType==cObsNodeTypeDoppleganger)
                     {
                        BTypedID entity;

                        entity.setID(theNode->mEntityID);
                        entity.setType(BEntity::cDopple);

                        long dopplePlayerID = -1, doppleIndex = -1;
                        entity.unpackDopplePackedID(dopplePlayerID, doppleIndex);

                        if(dopplePlayerID != ignoreWallForPlayerID)
                           continue;

                        // See if the thing we're doppling is collideable.  If so.. use the obstruction.
                        // if not, ignore it.
                        BDopple *dopple=game->getWorld()->getDoppleManager()->getDoppleByIndex(dopplePlayerID, doppleIndex);
                        if (dopple)
                        {
                           long protoID = dopple->getProtoUnitID();
                           if (protoID >= 0)
                           {
                              BPlayer *pplayer = game->getWorld()->getPlayer(dopplePlayerID);
                              if (pplayer)
                              {
                                 const BProtoUnit *pproto = pplayer->getProtoUnit(protoID);
                                 if (pproto && pproto->getFlag(BProtoUnit::cCollideable) == false)
                                     continue;
                              }
                           }
                        }
                     }*/
                     else
                     {
                        pObject = theNode->mObject;
                        if (pObject != NULL)
                        {
                           /*if (ignoreWalls)
                           {
                              if (pUnit->getProtoUnitFlag(BProtoUnit::cWallBuild))
                                 continue;
                           }

                           if(pUnit->isVisibleToPlayer(ignoreWallForPlayerID) == false)
                              continue;                           
                              
                           // skip trees for right now. If they are in the way, we are going to delete them.
                           // Note: We will probably want to move this to its own DB flag.s
                           /*
                           if ( (pUnit->getProtoUnitFlag(BProtoUnit::cImmoveable) == true) && 
                              (pUnit->isAbstractType(cAbstractUnitTypeTree) ) )
                              continue;
                           */
                        }
                     }

							// Uee the Actual Hull
							ProcessedHull = (BOPQuadHull*) &(theNode->mExpandedHull);

                     /*               
                     long lTempResult = singleWallCheck(ProcessedHull, point1, 
                        point2, dir, normal, mRadius, segs, results, theNode);
                     */
                     long lTempResult = singleWallCheck(ProcessedHull, point1 - mRadius * dir, 
                        point2 + mRadius * dir, dir, normal, mRadius, segs, results, theNode);
 

                     if (lTempResult < -1)
                        return lTempResult;
                     if (lTempResult >= 0)
                        lInsideCount += lTempResult;

						} //end for n loop

					} // end for x loop
				}	// End for z loop

				// Process next highest level in quadtree
				CurLevel--;		

			}  // End Loop to process all quadtree levels

		}  
		CurTree++;
		QuadTreesToScan >>= 1;
	}

   //-- look for black map.
   /*
   BTerrainBase *pTerrain = mWorld->getTerrain();
   if(pTerrain != NULL)
   {
      BObBlackTileSearch info;
      info.pTerrain = pTerrain;
      info.normal = normal;
      info.radius = mRadius;
      info.segs[0] = segs[0];
      info.segs[1] = segs[1];
      info.segs[2] = segs[2];
      info.segs[3] = segs[3];
      info.direction = dir;
      info.point1 = point1 - mRadius * dir;
      info.point2 = point2 + mRadius * dir;
      info.pResults = &results;
      info.pPlayer = mWorld->getPlayer(ignoreWallForPlayerID);
      info.pObMgr = this;
      info.insideCount = lInsideCount;

      BVector temp1, temp2;
      temp1 = info.point1;
      temp2 = info.point2;

      pTerrain->convertWorldToTile(&temp1);
		pTerrain->convertWorldToTile(&temp2);

      scanLineForward((long)temp1.x, (long)temp1.z, (long)temp2.x, (long)temp2.z, NULL, findBlackTiles, (void*)&info);
   }
   */

	// Return our findings

   // Init the y component to 0 for sync issues.

   for(long i=0; i<results.getNumber()-1; i++)
	{
      results[i].mPoint.y=0.0f;
	}

   // Dumb slow selection sort.

   BWallCheckResult tempWCR;
   for(long i=0; i<results.getNumber()-1; i++)
   {
      for(long j=i+1; j<results.getNumber(); j++)
      {  
         // If out of order...
         // if(results[i].mDist > results[j].mDist)
         if ( (results[i].mDist - results[j].mDist) > cFloatCompareEpsilon)
         {
            // Swap.
            tempWCR=results[i];
            results[i]=results[j];
            results[j]=tempWCR;
         }
         else if  ((fabs(results[i].mDist - results[j].mDist)) < cFloatCompareEpsilon)
         {
            // equal but wrong order
            if ( (results[i].mIn == true) && (results[j].mIn == false))
            {
               // IN compares > than OUT
               tempWCR=results[i];
               results[i]=results[j];
               results[j]=tempWCR;
            }
         }
      }
   }

   return lInsideCount;

}

//==============================================================================
// BObstructionManager::findBlackTiles
//==============================================================================
void CALLBACK BObstructionManager::findBlackTiles(long x, long z, void* pParam)
{
   /*BObBlackTileSearch* pInfo = (BObBlackTileSearch*)pParam;
   if((pInfo==NULL) || (pInfo->pPlayer == NULL) || (pInfo->pObMgr==NULL) || (pInfo->pTerrain==NULL))
      return;

   BOPQuadHull hull;
   BVector points[4];
   BVector point(0.0f);
   float expand = 0.0f, tileSize = pInfo->pTerrain->getTileSize();

   long localX = x, localZ = z;
   static long blackTiles = 0;

   blackTiles  = 0;
   blackTiles  = (pInfo->pPlayer->isBlack(x--, z--) == true)   << 0;
   blackTiles |= (pInfo->pPlayer->isBlack(x, z--) == true)     << 1;
   blackTiles |= (pInfo->pPlayer->isBlack(x++, z--) == true)   << 2;
   blackTiles |= (pInfo->pPlayer->isBlack(x--, z) == true)     << 3;
   blackTiles |= (pInfo->pPlayer->isBlack(x, z) == true)       << 4;
   blackTiles |= (pInfo->pPlayer->isBlack(x++, z) == true)     << 5;
   blackTiles |= (pInfo->pPlayer->isBlack(x--, z++) == true)   << 6;
   blackTiles |= (pInfo->pPlayer->isBlack(x, z++) == true)     << 7;
   blackTiles |= (pInfo->pPlayer->isBlack(x++, z++) == true)   << 8;
   //-- if all tiles are black, then move on.
   if(blackTiles == 0x01FF)
      return;

   for(long i=0; i<9; i++)
   {
      if( (blackTiles & (1 << i)) == false )
         continue;

      localX = x;
      localZ = z;

      switch(i)
      {
         case 0:
            localX--;
            localZ--;
            break;
         case 1:
            localX;
            localZ--;
            break;
         case 2:
            localX++;
            localZ--;
            break;
         case 3:
            localX--;
            localZ;
            break;
         case 4:
            break;
         case 5:
            localX++;
            localZ;
            break;
         case 6:
            localX--;
            localZ++;
            break;
         case 7:
            localX;
            localZ++;
            break;
         case 8:
            localX++;
            localZ++;
            break;
      }

      point.set((float)localX, 0.0f, (float)localZ);
      pInfo->pTerrain->convertTileToWorld(&point);
      
      points[0].x = point.x - expand;
      points[0].z = point.z - expand;
      points[1].x = point.x - expand;
      points[1].z = point.z + tileSize + expand;
      points[2].x = point.x + tileSize + expand;
      points[2].z = point.z + tileSize + expand;
      points[3].x = point.x + tileSize + expand;
      points[3].z = point.z - expand;

      if(hull.createSimpleHull(points) == false)
         continue;

      long results = pInfo->pObMgr->singleWallCheck(&hull, pInfo->point1, pInfo->point2, pInfo->direction, pInfo->normal, 
                                                      pInfo->radius, pInfo->segs, *(pInfo->pResults), NULL);
      if (results >= 0)
         pInfo->insideCount += results;
   }*/
}


//==============================================================================
// BObQuadNode::singleWallCheck
// New return codes for SingleWallCheck:
// -2 = segments are completely inside of referenced obstruction
// -1 = segments didn't intersect referenced obstruction
// >=0 = some intersections occured, this is the "startInside" count.
//==============================================================================
/*
long BObstructionManager::singleWallCheck(const BOPQuadHull *hull, const BVector &point1, const BVector &point2, const BVector &dir, 
               const BVector &normal, float width,  const BVector segs[4],  BWallCheckResultArray &results, BOPObstructionNode *pObNode)
{

   dir;
   point2;

   // Some vars.
   bool hit=false;
   long startInsideCount=0;
   
   // Check our obstructions.

   // Set up for figuring out enter/exit points.
   BVector enterPoint(cOriginVector);
   bool gotEnter=false;
   BVector exitPoint(cOriginVector);
   bool gotExit=false;
   float enterDistSqr=cMaximumFloat;
   float exitDistSqr=-cMaximumFloat;


// Check it.

	// Check plus segment.
	BVector thisEnterPoint, thisExitPoint;
	long result=hull->minMaxSegIntersect(segs[0], segs[1], thisEnterPoint, thisExitPoint);

	// Grab enter point.
	if(result==BConvexHull::cMinMaxEnter || result==BConvexHull::cMinMaxThrough)
	{
		// Adjust point for width/move to main segment.
		enterPoint=thisEnterPoint-width*normal;

		// Save info.
		enterDistSqr=point1.xzDistanceSqr(enterPoint);
		gotEnter=true;
	}

	// Grab exit point.
	if(result==BConvexHull::cMinMaxExit || result==BConvexHull::cMinMaxThrough)
	{
		// Adjust point for width/move to main segment.
		exitPoint=thisExitPoint-width*normal;

		// Save info.
		exitDistSqr=point1.xzDistanceSqr(exitPoint);
		gotExit=true;
	}


	// Check minus segment.
	long result2=hull->minMaxSegIntersect(segs[3], segs[2], thisEnterPoint, thisExitPoint);

	// Grab enter point.
	if(result2==BConvexHull::cMinMaxEnter || result2==BConvexHull::cMinMaxThrough)
	{
		// Adjust point for width/move to main segment.
		thisEnterPoint=thisEnterPoint+width*normal;
		float thisEnterDistSqr=point1.xzDistanceSqr(thisEnterPoint);
   
		if(thisEnterDistSqr<enterDistSqr)
		{
			// Save info.
			enterPoint=thisEnterPoint;
			enterDistSqr=thisEnterDistSqr;
			gotEnter=true;
		}
	}

	// Grab exit point.
	if(result2==BConvexHull::cMinMaxExit || result2==BConvexHull::cMinMaxThrough)
	{
		// Adjust point for width/move to main segment.
		thisExitPoint=thisExitPoint+width*normal;
		float thisExitDistSqr=point1.xzDistanceSqr(thisExitPoint);
   
		if(thisExitDistSqr>exitDistSqr)
		{
			// Save info.
			exitPoint=thisExitPoint;
			exitDistSqr=thisExitDistSqr;
			gotExit=true;
		}
	}

	// If either segment is stuck in hull, we need to block out enter or exit as appropriate.
	if((result==BConvexHull::cMinMaxEnter) || (result2==BConvexHull::cMinMaxEnter))
		gotExit=false;
	if((result==BConvexHull::cMinMaxExit) || (result2==BConvexHull::cMinMaxExit))
		gotEnter=false;


	// If we got any hits, add points.
	if(gotEnter || gotExit)
	{
		hit=true;

		// Keep count of how many things the start point is inside.
		if(gotExit && !gotEnter)
			startInsideCount++;


      // check if any points of the obstruction are inside my hull
      BConvexHull segmentHull;
      segmentHull.addPoints(segs, 4, true);

      // for each point in the hull, see if its inside our hull
      for (long i = 0; i<4; i++)
      {
         BVector t;
         t.x = hull->mPoint[i].mX;
         t.z = hull->mPoint[i].mZ;
         t.y = 0.0f;

         if ( segmentHull.inside(t) )
         {
            // get the normal to our main line and see where this point intersects that line
            BVector t1 = t + normal*2.0f;
            BVector t2 = t - normal*2.0f;

            BVector intersect;
            long result = segmentIntersect(t1, t2, point1, point2, intersect);
            if(result==cIntersection)
            {
               // get Distance from start
               float thisDistSqr=point1.xzDistanceSqr(intersect);

               if (gotEnter && (thisDistSqr<enterDistSqr) ) // if we have an enter and this is closer, use it
               {
                  enterPoint=intersect;
                  enterDistSqr=thisDistSqr;
               }
               else if (gotExit && (thisDistSqr>exitDistSqr) ) // else if we have an exit and this is farther, use it.
               {
                  exitPoint=intersect;
                  exitDistSqr=thisDistSqr;
               }
            }
            else
            {
               // this shouldn't happen.
            }
         }
      }


		if(gotExit)
		{
			// Add only exit point.
         results.add(BWallCheckResult(exitPoint, float(sqrt(exitDistSqr)), pObNode, false));
		}
		if(gotEnter)
		{
			// Only add entrance point.
			results.add(BWallCheckResult(enterPoint, float(sqrt(enterDistSqr)), pObNode, true));
		}
	}

   if(!hit)
   {
      if (result == BConvexHull::cMinMaxInside && result2 == BConvexHull::cMinMaxInside)
         return -2;
      else
         return -1;
   }
   else
      return(startInsideCount);


}
*/
//==============================================================================
// BObQuadNode::singleWallCheck
// New return codes for SingleWallCheck:
// -2 = segments are completely inside of referenced obstruction
// -1 = segments didn't intersect referenced obstruction
// >=0 = some intersections occured, this is the "startInside" count.
//==============================================================================
long BObstructionManager::singleWallCheck(const BOPQuadHull *hull, const BVector &point1, const BVector &point2, const BVector &dir, 
               const BVector &normal, float width,  const BVector segs[4],  BWallCheckResultArray &results, BOPObstructionNode *pObNode)
{


   point2;

   // Some vars.
   bool hit=false;
   long startInsideCount=0;
   
   // Check our obstructions.

   // Set up for figuring out enter/exit points.
   BVector enterPoint(cOriginVector);
   bool gotEnter=false;
   BVector exitPoint(cOriginVector);
   bool gotExit=false;
   float enterDistSqr=cMaximumFloat;
   float exitDistSqr=-cMaximumFloat;


// Check it.

	// Check plus segment.
	BVector thisEnterPoint, thisExitPoint;
	long result=hull->minMaxSegIntersect(segs[0], segs[1], thisEnterPoint, thisExitPoint);

	// Grab enter point.
	if(result==BConvexHull::cMinMaxEnter || result==BConvexHull::cMinMaxThrough)
	{
		// Adjust point for width/move to main segment.
		enterPoint=thisEnterPoint-width*normal;

		// Save info.
		enterDistSqr=point1.xzDistanceSqr(enterPoint);
		gotEnter=true;
	}

	// Grab exit point.
	if(result==BConvexHull::cMinMaxExit || result==BConvexHull::cMinMaxThrough)
	{
		// Adjust point for width/move to main segment.
		exitPoint=thisExitPoint-width*normal;

		// Save info.
		exitDistSqr=point1.xzDistanceSqr(exitPoint);
		gotExit=true;
	}


	// Check minus segment.
	long result2=hull->minMaxSegIntersect(segs[3], segs[2], thisEnterPoint, thisExitPoint);

	// Grab enter point.
	if(result2==BConvexHull::cMinMaxEnter || result2==BConvexHull::cMinMaxThrough)
	{
		// Adjust point for width/move to main segment.
		thisEnterPoint=thisEnterPoint+width*normal;
		float thisEnterDistSqr=point1.xzDistanceSqr(thisEnterPoint);
   
		if(thisEnterDistSqr<enterDistSqr)
		{
			// Save info.
			enterPoint=thisEnterPoint;
			enterDistSqr=thisEnterDistSqr;
			gotEnter=true;
		}
	}

	// Grab exit point.
	if(result2==BConvexHull::cMinMaxExit || result2==BConvexHull::cMinMaxThrough)
	{
		// Adjust point for width/move to main segment.
		thisExitPoint=thisExitPoint+width*normal;
		float thisExitDistSqr=point1.xzDistanceSqr(thisExitPoint);
   
		if(thisExitDistSqr>exitDistSqr)
		{
			// Save info.
			exitPoint=thisExitPoint;
			exitDistSqr=thisExitDistSqr;
			gotExit=true;
		}
	}

	// If either segment is stuck in hull, we need to block out enter or exit as appropriate.
	if((result==BConvexHull::cMinMaxEnter) || (result2==BConvexHull::cMinMaxEnter))
		gotExit=false;
	if((result==BConvexHull::cMinMaxExit) || (result2==BConvexHull::cMinMaxExit))
		gotEnter=false;

	// Check for points of obstruction hull inside of wide segment.  Ugh, this can probably be
	// streamlined a bunch.
	for(long i=0; i<4; i++)
	{
      mTempVectors[2].set(hull->mPoint[i].mX, 0.0f, hull->mPoint[i].mZ);
		bool inside=pointInXZProjection(segs, 4, mTempVectors[2]);
		if(inside)
		{
			// Get closest point on line.
			mTempVectors[2].xzDistanceToLine(point1, dir, &mTempVectors[3]);

			// Get distance to endpoint.
			float distSqr=mTempVectors[3].xzDistanceSqr(point1);

         // If distance is less than onSegmentEpsilon.. just ignore..
         if (distSqr < cOnSegmentEpsilon)
            continue;

			// Update enter.
			if((gotEnter && distSqr<enterDistSqr) || (!gotEnter))
			{
				// Save info.
				enterPoint   = mTempVectors[3];
				enterDistSqr = distSqr;
				gotEnter     = true;
			}

			// Update exit.
			if((gotExit && distSqr>exitDistSqr) || (!gotExit))
			{
				// Save info.
				exitPoint   = mTempVectors[3];
				exitDistSqr = distSqr;
				gotExit     = true;
			}
		}
	}

	// If we got any hits, add points.
	if(gotEnter || gotExit)
	{
		hit=true;

		// Keep count of how many things the start point is inside.
		if(gotExit && !gotEnter)
			startInsideCount++;
   
		if(gotExit)
		{
			// Add only exit point.
			results.add(BWallCheckResult(exitPoint+width*dir, float(sqrt(exitDistSqr))+width, pObNode, false));
		}
		if(gotEnter)
		{
			// Only add entrance point.
			results.add(BWallCheckResult(enterPoint-width*dir, float(sqrt(enterDistSqr))-width, pObNode, true));
		}
	}

   if(!hit)
   {
      if (result == BConvexHull::cMinMaxInside && result2 == BConvexHull::cMinMaxInside)
         return -2;
      else
         return -1;
   }
   else
      return(startInsideCount);
}


//==============================================================================
// BObstructionManager::rebuildPlayerRelationMasks
//==============================================================================
void BObstructionManager::rebuildPlayerRelationMasks()
{
	mUpdatePlayerRelationMasks = true;
	updatePlayerOwnedMasks();
}

//==============================================================================
// BObstructionManager::updatePlayerOwnedMasks
//==============================================================================
bool BObstructionManager::updatePlayerOwnedMasks()
{

	// Are we disabled
	if (!mUpdatePlayerRelationMasks)
		return(false);

   // gaia is an invalid player for our purposes.  

	for (long p = 1; p < gWorld->getNumberPlayers(); p++)
	{
		BPlayer *pPlayer = gWorld->getPlayer(p);
		if (!pPlayer)
		{
			BASSERT(0);
			return(false);
		}

		long PlayerMask = 0;

		for (long n = 1; n < gWorld->getNumberPlayers(); n++)
		{
			if (!pPlayer->isAlly(n))
				continue;

			// Player ID's in this mask are zero based.
			// Shift the player id to the correct bit location..
			PlayerMask |=  (0x0001 << (n-1)); 

		}

		ObsSystemPlayerMasks[p] = PlayerMask;
	}

   // Run through the list of all obstructions that are owned by players.. and update them, so that 
   // the quad tree is updated accordingly.
   static BObstructionNodePtrArray obstructions;   
   findObstructions(0.0f, 0.0f, mfWidth, mfHeight, 0.0f, cIsNewTypeCollidableNonMovableUnit, cObsNodeTypeUnit, -1L, false, obstructions);
   long lObCount = obstructions.getNumber();
   for (long n = 0; n < lObCount; n++)
   {
      BOPObstructionNode *pnode = obstructions[n];
      if (pnode->mPlayerID)
      {
         this->updateObstructionLocation(pnode, cObsTypeCollidableNonMovableUnit, true);
      }
   }

   return true;
}



//==============================================================================
// BObstructionManager::begin
//==============================================================================
bool BObstructionManager::begin(ObsBeginType mode, float radius, long QuadTreeToScan, long validNodeTypes, long lPlayerID, 
																	float ExpansionScale, const BEntityIDArray* ignoreList, bool canJump)
{
	mode;
	mRadius = radius;

	return(begin(cBeginNone, 0, 0, QuadTreeToScan, validNodeTypes, lPlayerID, ExpansionScale, ignoreList, canJump));

}

//==============================================================================
// BObstructionManager::begin
//==============================================================================
bool BObstructionManager::begin(ObsBeginType mode, long modeval1, long modeval2, long QuadTreeToScan, long validNodeTypes, long lPlayerID, 
																	float ExpansionScale, const BEntityIDArray* ignoreList, bool canJump)
{

	BUnit* theUnit;

	modeval2;



	if (mInUse)
	{
		BASSERT(0);
		return(false);
	}

	// Clear the Session Ignore Unit List

	mSessionUnitIgnoreList.setNumber(0);

	// Store session variables

	mQuadTreesToScan = QuadTreeToScan;
	mValidNodeTypes = validNodeTypes;
	mlPlayerID =lPlayerID;
   mbCanJump = canJump;

   bool isFloodMovement = false;
   bool isScarabMovement = false;
   bool isHoverMovement = false;

	switch (mode)
	{
		case cBeginNone:
		{
			break;
		}

		case cBeginUnit:
		{
			theUnit = gWorld->getUnit(modeval1);
			if (theUnit == NULL || theUnit->getProtoObject() == NULL)
			{
				BASSERT(0);
				return(false);
			}

			mRadius = theUnit->getObstructionRadiusX();
         isFloodMovement = (theUnit->getProtoObject()->getMovementType() == cMovementTypeFlood);
         isScarabMovement = (theUnit->getProtoObject()->getMovementType() == cMovementTypeScarab);
         isHoverMovement = (theUnit->getProtoObject()->getMovementType() == cMovementTypeHover);
				
			break;
		}

		case cBeginProtoUnit:
		{
			const BProtoObject *protoUnit = NULL;
			if (gWorld->getPlayer(modeval1))
				protoUnit = gWorld->getPlayer(modeval1)->getProtoObject(modeval2);

			if(protoUnit == NULL)
			{
				BASSERT(0);
				return(false);
			}

		   mRadius = protoUnit->getObstructionRadiusX();
         isFloodMovement = (protoUnit->getMovementType() == cMovementTypeFlood);
         isScarabMovement = (protoUnit->getMovementType() == cMovementTypeScarab);
         isHoverMovement = (protoUnit->getMovementType() == cMovementTypeHover);

			break;
		}

		case cBeginEntity:		// Modeval1 = entityID, modeval2 = entitytype
		{
         theUnit = NULL;

			if (modeval2 == BEntity::cClassTypeUnit)              // Entity is a UNIT
			{
				theUnit = gWorld->getUnit(modeval1);
				if(theUnit == NULL)
				{
					BASSERT(0);
					return(false);
				}

				mRadius = theUnit->getProtoObject()->getObstructionRadiusX();
            mlPlayerID=theUnit->getPlayerID();
			}
         else if (modeval2 == BEntity::cClassTypeSquad)        // Entity is a Squad
			{
		      BSquad* unitGroup = gWorld->getSquad(modeval1);
				if (unitGroup == NULL)
				{
					BASSERT(0);
					return(false);
				}

				mRadius = unitGroup->getObstructionRadius();
            mlPlayerID=unitGroup->getPlayerID();

				// HACK HACK HACK  Get the first unit from the group.. use this unit to determine
				// if we're a water group or a land group. This assumes you can't have mixed water
				// and land units in the same group.  (Which would just drive pathing (and it's programmer) INSANE!

	         theUnit = gWorld->getUnit(unitGroup->getChild(0));
			   if (theUnit == NULL || theUnit->getProtoObject() == NULL)
				{
					return(false);
				}
			}
         else if (modeval2 == BEntity::cClassTypePlatoon)      // Entity is a Platoon
         {
				BPlatoon* pPlatoon = gWorld->getPlatoon(modeval1);
				if (pPlatoon == NULL)
				{
					BASSERT(0);
					return(false);
				}

            // TRB 7/18/07:  Yeah, yeah, yeah.  I know hardcoding the value is bad, but
            // the radius must be positive or it causes an assert.  I changed the pather
            // too so hopefully it won't come down this branch.  This should probably
            // use the radius of the largest unit, but a default value will do.
            mRadius = pPlatoon->getPathingRadius();
            if (mRadius <= 0.0f)
            {
               mRadius = 2.0f;
            }
            mlPlayerID = pPlatoon->getPlayerID();

            // Break out now since there isn't a unit to access in the code below
            break;
         }

         if (theUnit != NULL)
         {
            isFloodMovement = (theUnit->getProtoObject()->getMovementType() == cMovementTypeFlood);
            isScarabMovement = (theUnit->getProtoObject()->getMovementType() == cMovementTypeScarab);
            isHoverMovement = (theUnit->getProtoObject()->getMovementType() == cMovementTypeHover);
         }

			break;
		}

		default:
		{
			BASSERT(0);
			return(false);
		}

	}

	// Flush the Expanded Hull Cache
	releaseExpandedHulls();

   // jce 1/4/2002 -- subtract off a few centimeters to make pathing work when guys
   // get wedged exactly inside things of their radius... perhaps gross, but I have plenty
   // of rationalizations about why this makes sense.
	mRadius-=0.05f;

   // Don't use negative expansions (dlm 4/10/02)
   if (mRadius < cFloatCompareEpsilon)
      mRadius = 0.0f;

   // Compute relaxed radius.
   mRelaxedRadius = ExpansionScale*mRadius;

	// if no tree to scan are specified, use defaults based on Movement type

   if (mQuadTreesToScan == -1)
   {
      if (isFloodMovement)
         mQuadTreesToScan = cDefaultQuadTreesToScanforFlood;
      if (isScarabMovement)
         mQuadTreesToScan = cDefaultQuadTreesToScanforScarab;
      if (isHoverMovement)
         mQuadTreesToScan = cDefaultQuadTreesToScanforHover;
	   else
		   mQuadTreesToScan = cDefaultQuadTreesToScanforLand;
   }

	// Mark units in the Entity Ignore List as to be ignored
	if(ignoreList)
   {
      // Don't call the array assignment operator since that will free the array's memory and the realloc it
      mEntityIgnoreList.setNumber(ignoreList->getNumber());
      for (int i = 0; i < ignoreList->getNumber(); i++)
      {
         mEntityIgnoreList[i] = ignoreList->get(i);
      }
   }
   else
      mEntityIgnoreList.setNumber(0);

   for (uint n = 0; n < mEntityIgnoreList.getSize(); n++)
   {
      uint type = mEntityIgnoreList.get(n).getType();
      if (type == BEntity::cClassTypeUnit)
      {
         theUnit = gWorld->getUnit(mEntityIgnoreList.get(n), true);
         if ((theUnit != NULL) && (theUnit->getObstructionNode() != NULL))
         {
            const_cast<BOPObstructionNode*>(theUnit->getObstructionNode())->mProperties |= cObsPropertyInIgnoreList;
         }
      }
      else if (type == BEntity::cClassTypeSquad)
      {
         BSquad* pSquad = gWorld->getSquad(mEntityIgnoreList.get(n), true);
         if ((pSquad != NULL) && (pSquad->getObstructionNode() != NULL))
         {
            const_cast<BOPObstructionNode*>(pSquad->getObstructionNode())->mProperties |= cObsPropertyInIgnoreList;
         }
      }
   }

   // Remember that we've called begun.
   mInUse=true;

	return(true);

}

//==============================================================================
// BObstructionManager::end
//==============================================================================
bool BObstructionManager::end()
{


	//BUnit* theUnit;


	if (!mInUse)
	{
		BASSERT(0);
		return(false);
	}

	// Flush the Expanded Hull Cache

	releaseExpandedHulls();

	// Clear the ignore status for any units to be ignored this session

	for (long n = 0; n < mSessionUnitIgnoreList.getNumber(); n++)
	{
		mSessionUnitIgnoreList[n]->mProperties &= ~cObsPropertyInIgnoreList;				
	}

	mSessionUnitIgnoreList.setNumber(0);

	// Unmark units in the Entity Ignore list at to be ignored

	for (uint n = 0; n < mEntityIgnoreList.getSize(); n++)
	{
      uint type = mEntityIgnoreList.get(n).getType();
      if (type == BEntity::cClassTypeUnit)
		{
			BUnit* pUnit = gWorld->getUnit(mEntityIgnoreList.get(n), true);
         if ((pUnit != NULL) && (pUnit->getObstructionNode() != NULL))
			{
				const_cast<BOPObstructionNode*>(pUnit->getObstructionNode())->mProperties &= ~cObsPropertyInIgnoreList;
			}
		}
      else if (type == BEntity::cClassTypeSquad)
      {
         BSquad* pSquad = gWorld->getSquad(mEntityIgnoreList.get(n), true);
         if ((pSquad != NULL) && (pSquad->getObstructionNode() != NULL))
         {
				const_cast<BOPObstructionNode*>(pSquad->getObstructionNode())->mProperties &= ~cObsPropertyInIgnoreList;
         }
      }
	}

	// Reset variables

	mEntityIgnoreList.setNumber(0);

	mQuadTreesToScan = 0;
	mValidNodeTypes = 0;
	mlPlayerID = 0;

	mSessionUnitIgnoreList.setNumber(0);

	mRadius = 0.0f;
	mRelaxedRadius = 0.0f;

   mbCanJump = false;

	mInUse = false;

	return(true);

}


//==============================================================================
// BObstructionManager::getExpandedHull
//==============================================================================
const BOPQuadHull*  BObstructionManager::getExpandedHull(const BOPObstructionNode *obs)
{
   if (!mInUse)
      return(NULL);

   // jce -- ick, it's either cast away const here or give out non-const pointers, so
   // this seems the lesser of two evils.
   return(getExpandedHullPoints((BOPObstructionNode*)obs, mRadius));
}

//==============================================================================
// BObstructionManager::getExpandedHull
//==============================================================================
const BOPQuadHull*  BObstructionManager::getExpandedHull(const BOPObstructionNode *obs, float extraRadius)
{
   if (!mInUse)
   {
      // Must call begin first.
      BASSERT(0);
      return(NULL);
   }

   // jce -- ick, it's either cast away const here or give out non-const pointers, so
   // this seems the lesser of two evils.
   return(getExpandedHullPoints((BOPObstructionNode*)obs, mRadius + extraRadius));
}


//==============================================================================
// BObstructionManager::getRelaxedExpandedHull
// DLM 10/1/08 - I'm not sure why this was commented out.. but as I need it,
// and I don't see any reason why it shouldn't work.. I'm putting it back in.
//==============================================================================
const BOPQuadHull *BObstructionManager::getRelaxedExpandedHull(const BOPObstructionNode *obs)
{
   if (!mInUse)
   {
      // Must call begin first.
      BASSERT(0);
      return(NULL);
   }

   // jce -- ick, it's either cast away const here or give out non-const pointers, so
   // this seems the lesser of two evils.
   return(getExpandedHullPoints((BOPObstructionNode*)obs, mRelaxedRadius));
}


//==============================================================================
// BObstructionManager::findObstructions
//==============================================================================
void BObstructionManager::findObstructions(const BConvexHull &hull, const bool relaxed, const bool add, BObstructionNodePtrArray &obstructions)
{
   // If not adding to the list, clear it.
   if(!add)
      obstructions.setNumber(0);

   // Make sure begin has been called.
   if (!mInUse)
   {
      BASSERT(0);
      return;
   }

   float expansionRadius;
	if(relaxed)
		expansionRadius = mRelaxedRadius;
	else
		expansionRadius = mRadius;

   findObstructions(hull, expansionRadius, 0, 0, 0, add, obstructions);
}

//==============================================================================
// BObstructionManager::findObstructionsQuadHull
//==============================================================================
void BObstructionManager::findObstructionsQuadHull(const BOPQuadHull *pHull, const bool relaxed, const bool add, BObstructionNodePtrArray &obstructions)
{
   // If not adding to the list, clear it.
   if(!add)
      obstructions.setNumber(0);

   // Make sure begin has been called.
   if (!mInUse)
   {
      BASSERT(0);
      return;
   }

   float expansionRadius;
	if(relaxed)
		expansionRadius = mRelaxedRadius;
	else
		expansionRadius = mRadius;

   findObstructionsQuadHull(pHull, expansionRadius, 0, 0, 0, add, obstructions);
}

//==============================================================================
// BObstructionManager::segmentIntersects
//==============================================================================
bool BObstructionManager::segmentIntersects(const BVector &point1, const BVector &point2,
   const bool relaxed, BVector &iPoint)
{
   // Make sure begin has been called.
   if (!mInUse)
   {
      BASSERT(0);
      return false;
   }

   // Get for hits.
   bool hit=getObjectsIntersections(cGetNearestIntersect, point1, point2, relaxed, iPoint, mSegmentIntersectResults);
   return(hit);


}


//==============================================================================
// BObstructionManager::segmentIntersects
//==============================================================================
bool BObstructionManager::segmentIntersects(const BVector &point1, const BVector &point2, 
   const bool relaxed)
{
   // Make sure begin has been called.
   if (!mInUse)
   {
      BASSERT(0);
      return false;
   }

   // Temp to feed in to the function.
   BVector iPoint;

   // Get for hits.
   bool hit=getObjectsIntersections(cGetAnyIntersect, point1, point2, relaxed, iPoint, mSegmentIntersectResults);
   return(hit);
}


//==============================================================================
// BObstructionManager::render
//==============================================================================
void BObstructionManager::render( long lRenderMode )
{
//   if(lRenderMode<1 || lRenderMode>4)
   if(lRenderMode<1 || lRenderMode>4)
      return;
    
   //gRenderDraw.stateBlockCapture();
	long QuadTreesToRender = 0;

	long clipHint = 0;//BClipHints::cClipFlagsMask;


	switch (lRenderMode)
	{
		case 1:
			QuadTreesToRender = cRenderNodesMode1;
			break;

		case 2:
			QuadTreesToRender = cRenderNodesMode2;
			break;

		case 3:
			QuadTreesToRender = cRenderNodesMode3;
			break;
      case 4:
         QuadTreesToRender = cRenderNodesMode4;
         break;
	}

	renderObstructions(QuadTreesToRender, clipHint, 0, 0,0, 0.0f);
   //gRenderDraw.stateBlockApply();

}



//==============================================================================
// BObstructionManager::renderObstructions
//==============================================================================
void BObstructionManager::renderObstructions(long QuadTreesToRender, long clipHint, long level, long x, long z, float delta)
{  
   // jce [10/2/2008] -- Return of a hacky culling routine.  This is especially desireable in HW since the number of debug lines that can be rendered
   // is pretty limited.
   float nodeSize = mQTLevelInfo[0][level].mfMetersPerNode;
   bool onScreen = gRenderDraw.getMainActiveVolumeCuller().isAABBVisibleBounds(BVector(x*nodeSize, gTerrain.getMin().y, z*nodeSize), BVector((x+1)*nodeSize, gTerrain.getMax().y, (z+1)*nodeSize));
   if(!onScreen)
      return;
         
	// process all quads in this level
   for (long curTree=0, treeFlags=QuadTreesToRender; treeFlags != 0 && curTree < cNumQuadTrees; curTree++, treeFlags>>=1)
   {
      if ((treeFlags & 0x0001) == 0)
         continue;
	
		BOPQuadTreeNode* quadNode = mQTLevelInfo[curTree][level].mQTNodes + (z << mQTLevelInfo[curTree][level].mLevel) + x;

		for (long n = 0; n < quadNode->mNumObstructions; n++)
		{
			BOPObstructionNode *theNode = quadNode->mObstructionList[n];
			if (theNode == NULL)
				continue;

			long startPoint = 3;
         
         /*BSimString propertiesStr;
         propertiesStr.format("Properties: %d, player: %d, rot: %d, type: %d", (long)(theNode->mProperties), (long)(theNode->mPlayerID), (long)(theNode->mRotation), (long)(theNode->mType));
         BVector point(theNode->mPoint[startPoint].mX, 0, theNode->mPoint[startPoint].mZ);
         gpDebugPrimitives->addDebugText(propertiesStr, point, 0.5f, cDWORDRed);*/
         // DLM - set up some good 'ol Age 3 Unit Obstruction Colors 
         DWORD dwColor = cDWORDRed; 
         if (theNode->mThisNodeQuadTree == cObsTypeNonCollidableUnit)
            dwColor = cDWORDBlack;
         else if (theNode->mThisNodeQuadTree == cObsTypeCollidableNonMovableUnit)
            dwColor = cDWORDRed;
         else if (theNode->mThisNodeQuadTree == cObsTypeCollidableStationaryUnit)
            dwColor = cDWORDBlue;
         else if (theNode->mThisNodeQuadTree == cObsTypeCollidableMovingUnit)
            dwColor = cDWORDGreen;
         else if (theNode->mThisNodeQuadTree == cObsTypeBlockAirMovement)
            dwColor = cDWORDPurple;
         else if (theNode->mThisNodeQuadTree == cObsTypeCollidableMovingSquad)
            dwColor = cDWORDCyan;
         else if (theNode->mThisNodeQuadTree == cObsTypeCollidableStationarySquad)
            dwColor = cDWORDOrange;


         // Use some colors here

			for(long endPoint=0; endPoint<4; endPoint++)
			{
				mTempVectors[0].set(theNode->mPoint[startPoint].mX, 0.0f, theNode->mPoint[startPoint].mZ);
				mTempVectors[1].set(theNode->mPoint[endPoint].mX, 0.0f, theNode->mPoint[endPoint].mZ);

            gTerrainSimRep.addDebugLineOverTerrain(mTempVectors[0], mTempVectors[1], dwColor, dwColor, 0.2f);

				startPoint=endPoint;
			}
		}
	}

	// Not at the bottom level of the tree?
	// Then break it up into 4 equal size chunks and display separately
	if (level+1 < mNumQuadTreeLevels)
	{
		x = x*2;
		z = z*2;
		renderObstructions(QuadTreesToRender, clipHint, level+1, x,   z,   delta);
		renderObstructions(QuadTreesToRender, clipHint, level+1, x+1, z,   delta);
		renderObstructions(QuadTreesToRender, clipHint, level+1, x,   z+1, delta);
		renderObstructions(QuadTreesToRender, clipHint, level+1, x+1, z+1, delta);
	}
}



//==============================================================================
// BObstructionManager::addUnitIgnoreList
// inputs:  IDList   List of ID's to convert into types.
// output:  none
//==============================================================================
void BObstructionManager::addUnitIgnoreList(const BEntityIDArray& IDList)
{
	for (long n = 0; n < IDList.getNumber(); n++)
	{
		addUnitIgnore(IDList[n]);
	}

}

//==============================================================================
// BObstructionManager::addUnitIgnore
//==============================================================================
void BObstructionManager::addUnitIgnore(BEntityID unitID)
{
	BEntity* theUnit = gWorld->getEntity(unitID);
	if (theUnit)
	{
      BOPObstructionNode* obsNode = const_cast<BOPObstructionNode*>(theUnit->getObstructionNode());
		if (obsNode)
		{
			obsNode->mProperties |= cObsPropertyInIgnoreList;
			mSessionUnitIgnoreList.add(obsNode);
		}
	}
}

//==============================================================================
// BObstructionManager::clearUnitIgnoreList
// inputs:  IDList   List of Units to to unclear the ignore bit from.
// output:  none
//==============================================================================
void BObstructionManager::clearUnitIgnoreList(const BEntityIDArray& IDList)
{
	for (long n = IDList.getNumber()-1; n >= 0; n--)
	{
		clearUnitIgnore(IDList[n]);
	}
}

//==============================================================================
// BObstructionManager::clearUnitIgnore
//==============================================================================
void BObstructionManager::clearUnitIgnore(BEntityID unitID)
{
	BEntity* theUnit = gWorld->getEntity(unitID);
	if (theUnit)
	{
      BOPObstructionNode* obsNode = const_cast<BOPObstructionNode*>(theUnit->getObstructionNode());
		if (obsNode)
		{
			obsNode->mProperties &= ~cObsPropertyInIgnoreList;
			mSessionUnitIgnoreList.removeValue(obsNode);
		}
	}
}


//==============================================================================
// BObstructionManager::makeValidX
//==============================================================================
void BObstructionManager::makeValidX(long &x)
{
   if(x<0)
      x=0;
   if(x>=mWidth)
      x=mWidth-1;
}


//==============================================================================
// BObstructionManager::makeValidZ
//==============================================================================
void BObstructionManager::makeValidZ(long &z)
{
   if(z<0)
      z=0;
   if(z>=mHeight)
      z=mHeight-1;
}

//=============================================================================
// BTerrain::getHeightRange
//=============================================================================
bool BObstructionManager::getAllInHeightRange(const BConvexHull &hull, float &minHeight, float &maxHeight)
{
   // Get bounding box of convex hull.
   float tileSize = gTerrainSimRep.getDataTileScale();
   float recipTileSize = 1.0f / tileSize;
   long minXVertex = (long)(hull.getBoundingMin().x*recipTileSize);
   long minZVertex = (long)(hull.getBoundingMin().z*recipTileSize);
   long maxXVertex = (long)(hull.getBoundingMax().x*recipTileSize+1.0f);
   long maxZVertex = (long)(hull.getBoundingMax().z*recipTileSize+1.0f);
   // Clamp to edge.
   makeValidX(minXVertex);
   makeValidX(maxXVertex);
   makeValidZ(minZVertex);
   makeValidZ(maxZVertex);

   // Check each terrain vertex in bounding box of hull to see if it actually falls within
   // the convex hull itself.
   BVector vertex;
   for(long x=minXVertex; x<=maxXVertex; x++)
   {
      for(long z=minZVertex; z<=maxZVertex; z++)
      {
         computeVertex(x,z,vertex);
         bool inside = hull.inside(vertex);
         if (inside && (vertex.y < minHeight) || (vertex.y > maxHeight))
            return (false);
      }
   }

   return (true);
}

//=============================================================================
// BTerrain::computeVertex
//=============================================================================
void BObstructionManager::computeVertex(long x, long z, BVector &vertex) const
{
   // Note: this is protected and as such expects that any checks that x and z are
   // valid are already done by the time this function is called!
   float tileSize = gTerrainSimRep.getDataTileScale();

   vertex.x = x * tileSize;
   vertex.z = z * tileSize;

   gTerrainSimRep.getHeight(vertex, true);
}



//==============================================================================
// BOPQuadHull methods
//==============================================================================



//==============================================================================
// BOPQuadHull::segmentIntersects
//==============================================================================
bool BOPQuadHull::segmentIntersects(const BVector &point1, const BVector &point2, const long ignorePoint,
   BVector &iPoint, long &segmentIndex, float &distanceSqr, long &lNumIntersections,
   bool checkBBox, bool bCheckInside, float ignoreDistSqr) const
{
   lNumIntersections = 0L;

   // If the segment can't possibly intersect the bounding box, there can't
   // be any intersection.
   // jce todo: bring back this check?
   checkBBox;

   BVector thisIPoint(cOriginVector);
   distanceSqr = cMaximumFloat;
   segmentIndex = -1;

   // Check each segment
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; startPoint=endPoint, endPoint++)
   {
      // Skip segment if it contains the ignore point (if any)
      if((ignorePoint>=0) && ((ignorePoint==endPoint) || (ignorePoint==startPoint)))
         continue;

      // Check this hull segment.
      // jce todo: get rid of temp BVector creation.

      long result = segmentIntersect(point1, point2, BVector(mPoint[startPoint].mX, 0.0f, mPoint[startPoint].mZ), 
         BVector(mPoint[endPoint].mX, 0.0f, mPoint[endPoint].mZ), thisIPoint);
      if(result==cIntersection)
      {
         // Ignore the intersection with an endpoint.  We've either already got it,
         // or we'll get it eventually.
         /*
         if ((_fabs(mPoints[endPoint].x - thisIPoint.x) < cFloatCompareEpsilon) &&
             (_fabs(mPoints[endPoint].z - thisIPoint.z) < cFloatCompareEpsilon))
             continue;
         */

         // If we've already got one zero distance intersection
         // Check if this is the closest to point1 so far.
         float dx = point1.x-thisIPoint.x;
         float dz = point1.z-thisIPoint.z;
         float thisDistSqr = dx*dx + dz*dz;

         // Ignore intersections closer than ignore dist.
         if(thisDistSqr<ignoreDistSqr)
            continue;

         // If we already have a zero distance intersection.. ignore any others..
         if (thisDistSqr < cFloatCompareEpsilon && distanceSqr < cFloatCompareEpsilon)
            continue;

         ++lNumIntersections;
         if(thisDistSqr < distanceSqr)
         {
            distanceSqr = thisDistSqr;
            iPoint = thisIPoint;
            segmentIndex = startPoint;
         } 
      }
   }

   // If no intersection found, maybe the whole damn thing is inside the hull?
   if (bCheckInside && segmentIndex == -1)
   {
      if(inside(point1))
      {
         segmentIndex=-1;
         distanceSqr=0.0f;
         iPoint = point1;
         return(true);
      }
   }

   return(segmentIndex>=0);
}


//==============================================================================
// BOPQuadHull::inside
//==============================================================================
bool BOPQuadHull::inside(const BVector &point) const
{
   // jce todo: do bounding check here?

   // Check the points.
   // jce todo: get rid of stupid conversion to BVector.
   BVector tempPoints[4];
   for(long i=0; i<4; i++)
      tempPoints[i].set(mPoint[i].mX, 0.0f, mPoint[i].mZ);
   bool result = pointInXZProjection(tempPoints, 4, point);
   return(result);
}


//==============================================================================
// BOPQuadHull::segmentIntersects
//==============================================================================
bool BOPQuadHull::segmentIntersects(const BVector &point1, const BVector &point2, bool checkBBox) const
{
   // If the segment can't possibly intersect the bounding box, there can't
   // be any intersection.
   // jce todo: add this check back?
   checkBBox;


   BVector iPoint(cOriginVector);

   // Check each segment
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; endPoint++)
   {
      // Check this hull segment.
      // jce todo: eliminate temp bvector creation.
      long result = segmentIntersect(point1, point2, BVector(mPoint[startPoint].mX, 0.0f, mPoint[startPoint].mZ), 
         BVector(mPoint[endPoint].mX, 0.0f, mPoint[endPoint].mZ), iPoint);
      if(result==cIntersection)
         return(true);

      startPoint=endPoint;
   }

   return(false);
}


//==============================================================================
// BOPQuadHull::checkSide
//
// Used by overlapsHull
//==============================================================================
bool BOPQuadHull::checkSide(float dx, float dz, float vx, float vz, float errorEpsilon) const
{
   // Vertices are projected to the form v+t*d.
   // Return value is +1 if all t > 0, -1 if all t < 0, 0 otherwise, in
   // which case the line splits the polygon.
   for (long i = 3; i>=0; i--)
   {
      float x=mPoint[i].mX-vx;
      float z=mPoint[i].mZ-vz;
      float t = x*dx+z*dz;
      if(t<errorEpsilon)
         return(false);
   }
   return(true);
}


//==============================================================================
// BOPQuadHull::overlapsHull
//==============================================================================
bool BOPQuadHull::overlapsHull(const BConvexHull &hull, float errorEpsilon) const
{
   // Check if bounding boxes overlap, fail now if they don't.
   if(getMinX() > hull.getBoundingMax().x)
      return(false);
   if(getMinZ() > hull.getBoundingMax().z)
      return(false);
   if(getMaxX() < hull.getBoundingMin().x)
      return(false);
   if(getMaxZ() < hull.getBoundingMin().z)
      return(false);

   // Test edges of C0 for separation. Because of the clockwise ordering,
   // the projection interval for C0 is [m,0] where m <= 0. Only try to determine
   // if C1 is on the ‘positive’ side of the line.
   long i0, i1;
   for (i0 = 0, i1 = 3; i0 < 4; i1 = i0, i0++)
   {
      float nx=mPoint[i1].mZ-mPoint[i0].mZ;
      float nz=mPoint[i0].mX-mPoint[i1].mX;
      if (::checkSide(hull.getPoints().getPtr(), hull.getPointCount(), nx, nz, mPoint[i0].mX, mPoint[i0].mZ, errorEpsilon))
      { // C1 is entirely on ‘positive’ side of line C0.V(i0)+t*D
         return(false);
      }
   }
   // Test edges of C1 for separation. Because of the clockwise ordering,
   // the projection interval for C1 is [m,0] where m <= 0. Only try to determine
   // if C0 is on the ‘positive’ side of the line.
   for (i0 = 0, i1 = hull.getPointCount()-1; i0 < hull.getPointCount(); i1 = i0, i0++)
   {
      float nx=hull.getPoint(i1).z-hull.getPoint(i0).z;
      float nz=hull.getPoint(i0).x - hull.getPoint(i1).x;
      if (checkSide(nx, nz, hull.getPoint(i0).x, hull.getPoint(i0).z, errorEpsilon))
      { // C0 is entirely on ‘positive’ side of line C1.V(i0)+t*D
         return(false);
      }
   }

   return(true);
}


//==============================================================================
// BOPQuadHull::overlapsHull
//==============================================================================
bool BOPQuadHull::overlapsHull(const BOPQuadHull *hull, float errorEpsilon) const
{
   // Check if bounding boxes overlap.
   if(getMinX() > hull->getMaxX())
      return(false);
   if(getMinZ() > hull->getMaxZ())
      return(false);
   if(getMaxX() < hull->getMinX())
      return(false);
   if(getMaxZ() < hull->getMinZ())
      return(false);

   // jce [8/9/2002] -- If they are both axis aligned, the real points and the bounding boxes are the same, 
   // so the bounding box check we just did is sufficient.
   if(!isRotated() && !hull->isRotated())
      return(true);

   // Test edges of C0 for separation. Because of the clockwise ordering,
   // the projection interval for C0 is [m,0] where m <= 0. Only try to determine
   // if C1 is on the ‘positive’ side of the line.
   long i0, i1;
   for (i0 = 0, i1 = 3; i0 < 4; i1 = i0, i0++)
   {
      float nx=mPoint[i1].mZ-mPoint[i0].mZ;
      float nz=mPoint[i0].mX-mPoint[i1].mX;
      if (hull->checkSide(nx, nz, mPoint[i0].mX, mPoint[i0].mZ, errorEpsilon))
      { // C1 is entirely on ‘positive’ side of line C0.V(i0)+t*D
         return(false);
      }
   }
   // Test edges of C1 for separation. Because of the clockwise ordering,
   // the projection interval for C1 is [m,0] where m <= 0. Only try to determine
   // if C0 is on the ‘positive’ side of the line.
   for (i0 = 0, i1 = 3; i0 < 4; i1 = i0, i0++)
   {
      float nx=hull->mPoint[i1].mZ-hull->mPoint[i0].mZ;
      float nz=hull->mPoint[i0].mX-hull->mPoint[i1].mX;
      if (checkSide(nx, nz, hull->mPoint[i0].mX, hull->mPoint[i0].mZ, errorEpsilon))
      { // C0 is entirely on ‘positive’ side of line C1.V(i0)+t*D
         return(false);
      }
   }
   return(true);
}


//=============================================================================
// BOPQuadHull::overlapsBox
//=============================================================================
bool BOPQuadHull::overlapsBox(const float minX, const float minZ, const float maxX, const float maxZ) const
{
   // First check if bounding boxes overlap.
   if(getMinX() > maxX)
      return(false);
   if(getMinZ() > maxZ)
      return(false);
   if(getMaxX() < minX)
      return(false);
   if(getMaxZ() < minZ)
      return(false);

   // If not rotated, we just need a bbox check.
   if(!isRotated())
      return(true);

   // jce todo: make this whole thing smarter.

   // Now check if the actual hull overlaps the box.

   // Check if hulls points are inside box.
   for(long i=0; i<4; i++)
   {
      if(mPoint[i].mX>=minX && mPoint[i].mX<=maxX && mPoint[i].mZ>=minZ && mPoint[i].mZ<=maxZ)
         return(true);
   }

   // Set up points of box.
   BVector points[4] = {BVector(minX, 0.0f, minZ), BVector(minX, 0.0f, maxZ),
      BVector(maxX, 0.0f, maxZ), BVector(maxX, 0.0f, minZ)};

   // jce todo: get rid of stupid conversion to BVector.
   BVector tempPoints[4];
   for(long i=0; i<4; i++)
      tempPoints[i].set(mPoint[i].mX, 0.0f, mPoint[i].mZ);

   // Check if points are inside the hull.
   for(long i=0; i<4; i++)
   {
      // jce 12/6/2000 -- instead of calling the inside function, we just do a check ourselves here.
      // This makes sense because we've already checked the bounding box, etc.
      bool result = pointInXZProjection(tempPoints, 4, points[i]);
      if(result)
         return(true);
   }

   // Check segments of box against our segments.
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; startPoint=endPoint, endPoint++)
   {
      bool result = segmentIntersects(points[startPoint], points[endPoint], false);
      if(result)
         return(true);
   }

   // If we got here, the two don't overlap.
   return(false);
}


//==============================================================================
// BOPQuadHull::minMaxSegIntersect
//==============================================================================
long BOPQuadHull::minMaxSegIntersect(const BVector &point1, const BVector &point2, 
   BVector &minPoint, BVector &maxPoint) const
{
   // Check bbox.
   // jce todo: fix up this check.
   //if(!segmentMightIntersectBox(point1, point2))
      //return(BConvexHull::cMinMaxNone);

   BVector iPoint(cOriginVector);
   long minSeg=-1;
   long maxSeg=-1;
   float minDistSqr=cMaximumFloat;
   float maxDistSqr=-cMaximumFloat;

   // Check each segment
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; endPoint++)
   {
      // Check this hull segment.
      // jce todo: get rid of temp BVector crap.]
      long result = segmentIntersect(point1, point2, BVector(mPoint[startPoint].mX, 0.0f, mPoint[startPoint].mZ), 
         BVector(mPoint[endPoint].mX, 0.0f, mPoint[endPoint].mZ), iPoint);
      if(result==cIntersection)
      {
         // Get dist from first point.
         float distSqr=iPoint.distanceSqr(point1);

         // Update min.
         if(distSqr<minDistSqr)
         {
            minDistSqr=distSqr;
            minSeg=endPoint;
            minPoint=iPoint;
         }

         // Update max.
         if(distSqr>maxDistSqr)
         {
            maxDistSqr=distSqr;
            maxSeg=endPoint;
            maxPoint=iPoint;
         }
      }
      startPoint=endPoint;
   }

   // Check for no intersection.
   if(minSeg<0 && maxSeg<0)
   {
      if (inside(point1))
         return BConvexHull::cMinMaxInside;
      else
         return(BConvexHull::cMinMaxNone);
   }

   // Check for single intersection.
   if(minSeg==maxSeg)
   {
      // jce 6/8/2001 -- we need to figure out whether this is an "enter" segment or
      // an "exit" segment.  Super cheesy version for now is to just check this at the last instant
      // instead of figuring it out cleverly as we go.
      /*
      */

      // Newer super cheesy version.  If the intersection is near point 1, generate 
      // cMinMaxExit.  Otherwise, generate cMinMaxEnter.  dlm 5/21/02
      if (minDistSqr < cOnSegmentEpsilon)
         return BConvexHull::cMinMaxExit;
      else
      {
         // Do John's super cheesy check.
         if(inside(point1))
            return(BConvexHull::cMinMaxExit);
         else
         {
            //-- if there was only one intersection point, and its "really" close to a point on the hull, and point2 is not inside the hull
            //-- (meaning that the line basically hits the corner only and failed a seqIntersect because the errorEpsilon was too low)
            //-- set the points equal and send back cMinMaxThrough. JER 6/26/02
            if(inside(point2) == false)
            {
               float cfNearHullPointError = 0.01f;
               BVector testPoint = minPoint;
               for(long i=0; i < 4; i++)
               {
                  if(testPoint.xzDistanceSqr(BVector(mPoint[i].mX, 0.0f, mPoint[i].mZ)) < cfNearHullPointError)
                  {
                     minPoint = testPoint;
                     maxPoint = testPoint;
                     return(BConvexHull::cMinMaxThrough);
                  }
               }
            }
            else
               return BConvexHull::cMinMaxEnter;
         }
      }
   }
   else
   {
      // If the min and the max are both less than cFloatCompareEpsilon, then
      // just call it exit as well.
      if (minDistSqr < cOnSegmentEpsilon && maxDistSqr < cOnSegmentEpsilon)
      {
         return BConvexHull::cMinMaxExit;
      }
   }

   // Otherwise, we got two.
   return(BConvexHull::cMinMaxThrough);
}


//==============================================================================
// BOPQuadHull::rayIntersects
//==============================================================================
bool BOPQuadHull::rayIntersects(const BVector &point, const BVector &vector, BVector &iPoint, long *segmentIndex, 
                                 float *distSqr) const
{
   // Do Ray Bounding checks
   if((vector.x>0.0f) && (point.x > getMaxX()))
      return(false);
   if((vector.x<0.0f) && (point.x < getMinX()))
      return(false);
   if((vector.z>0.0f) && (point.z > getMaxZ()))
      return(false);
   if((vector.z<0.0f) && (point.z < getMinZ()))
      return(false);


   BVector thisIPoint(cOriginVector);
   float bestDistSqr = cMaximumFloat;
   bool hit=false;

   // Check each segment
   long startPoint = 3;
   BVector v1(0.0f), v2(0.0f);
   for(long endPoint=0; endPoint < 4; endPoint++)
   {
      // Check this hull segment.
      v1.x = mPoint[startPoint].mX;
      v1.z = mPoint[startPoint].mZ;
      v2.x = mPoint[endPoint].mX;
      v2.z = mPoint[endPoint].mZ;
      long result = segmentIntersectRay(v1, v2, point, vector, thisIPoint);
      if(result==cIntersection)
      {
         // Check if this is the closest to point so far.
         float dx = point.x-thisIPoint.x;
         float dz = point.z-thisIPoint.z;
         float thisDistSqr = dx*dx + dz*dz;
         if(thisDistSqr < bestDistSqr)
         {
            bestDistSqr = thisDistSqr;
            iPoint = thisIPoint;
            if(segmentIndex)
               *segmentIndex = startPoint;
            hit=true;
         } 
      }
      startPoint=endPoint;
   }

   // Record best distance squared if desired.
   if(distSqr)
      *distSqr = bestDistSqr;

   return(hit);

}



//==============================================================================
// BOPQuadHull::rayIntersectsFar
//==============================================================================
bool BOPQuadHull::rayIntersectsFar(const BVector &point, const BVector &vector, BVector &iPoint, long *segmentIndex, 
                                 float *distSqr) const
{
   // Do Ray Bounding checks
   if((vector.x>0.0f) && (point.x > getMaxX()))
      return(false);
   if((vector.x<0.0f) && (point.x < getMinX()))
      return(false);
   if((vector.z>0.0f) && (point.z > getMaxZ()))
      return(false);
   if((vector.z<0.0f) && (point.z < getMinZ()))
      return(false);


   BVector thisIPoint(cOriginVector);
   float bestDistSqr = -1.0f;
   bool hit=false;

   // Check each segment
   long startPoint = 3;
   BVector v1(0.0f), v2(0.0f);
   for(long endPoint=0; endPoint < 4; endPoint++)
   {
      // Check this hull segment.
      v1.x = mPoint[startPoint].mX;
      v1.z = mPoint[startPoint].mZ;
      v2.x = mPoint[endPoint].mX;
      v2.z = mPoint[endPoint].mZ;
      long result = segmentIntersectRay(v1, v2, point, vector, thisIPoint);
      if(result==cIntersection)
      {
         // Check if this is the farthest to point so far.
         float dx = point.x-thisIPoint.x;
         float dz = point.z-thisIPoint.z;
         float thisDistSqr = dx*dx + dz*dz;
         if(thisDistSqr > bestDistSqr)
         {
            bestDistSqr = thisDistSqr;
            iPoint = thisIPoint;
            if(segmentIndex)
               *segmentIndex = startPoint;
            hit=true;
         } 
      }
      startPoint=endPoint;
   }

   // Record best distance squared if desired.
   if(distSqr)
      *distSqr = bestDistSqr;

   return(hit);

}

//==============================================================================
// BOPQuadHull::distance
//==============================================================================
float BOPQuadHull::distance(const BVector &point) const
{
   return((float)sqrt(distanceSqr(point)));
}


//==============================================================================
// BOPQuadHull::distanceSqr
//==============================================================================
float BOPQuadHull::distanceSqr(const BVector &point) const
{
   // If inside the hull, range is 0.
   if(inside(point))
      return(0.0f);

   // Get distance to each segment making up the hull and keep the smallest.
   float bestDistSqr = cMaximumFloat;
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; endPoint++)
   {
      // jce todo: get rid of temp vector stuff.
      float thisDistSqr = point.xzDistanceToLineSegmentSqr(BVector(mPoint[startPoint].mX, 0.0f, mPoint[startPoint].mZ),
         BVector(mPoint[endPoint].mX, 0.0f, mPoint[endPoint].mZ));
      if(thisDistSqr<bestDistSqr)
         bestDistSqr=thisDistSqr;
      startPoint=endPoint;
   }

   return(bestDistSqr);
}


//==============================================================================
// BOPQuadHull::distance
//==============================================================================
float BOPQuadHull::distance(const BOPQuadHull *hull) const
{
   return(float(sqrt(distanceSqr(hull))));
}


//==============================================================================
// BOPQuadHull::distanceSqr
//==============================================================================
float BOPQuadHull::distanceSqr(const BOPQuadHull *hull) const
{
   // Check param.
   if(!hull)
      return(cMaximumFloat);

   // Check for overlap.
   if(overlapsHull(hull))
      return(0.0f);

   // Check segments.
   float bestDistSqr = cMaximumFloat;
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; endPoint++)
   {
      long hullStartPoint = 3;
      for(long hullEndPoint=0; hullEndPoint<4; hullEndPoint++)
      {
         float thisDistSqr = distanceBetweenSegmentsSqr(mPoint[startPoint].mX, mPoint[startPoint].mZ, 
            mPoint[endPoint].mX, mPoint[endPoint].mZ, hull->mPoint[hullStartPoint].mX, hull->mPoint[hullStartPoint].mZ, 
            hull->mPoint[hullEndPoint].mX, hull->mPoint[hullEndPoint].mZ);
         if(thisDistSqr<bestDistSqr)
            bestDistSqr=thisDistSqr;
         hullStartPoint=hullEndPoint;
      }
      startPoint=endPoint;
   }

   return(bestDistSqr);   
}

//==============================================================================
// BOPQuadHull::distance
//==============================================================================
float BOPQuadHull::distance(const BConvexHull &hull) const
{
   return(float(sqrt(distanceSqr(hull))));
}


//==============================================================================
// BOPQuadHull::distanceSqr
//==============================================================================
float BOPQuadHull::distanceSqr(const BConvexHull &hull) const
{
   //Check param.
   if (hull.getPointCount() < 2)
      return(cMaximumFloat);

   // Check for overlap.
   if (overlapsHull(hull))
      return(0.0f);
   const BDynamicSimVectorArray &hullPoints=hull.getPoints();

   // Check segments.
   float bestDistSqr = cMaximumFloat;
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; endPoint++)
   {
      long hullStartPoint = hullPoints.getNumber()-1;
      for(long hullEndPoint=0; hullEndPoint<hullPoints.getNumber(); hullEndPoint++)
      {
         float thisDistSqr = distanceBetweenSegmentsSqr(mPoint[startPoint].mX, mPoint[startPoint].mZ, 
            mPoint[endPoint].mX, mPoint[endPoint].mZ, hullPoints[hullStartPoint].x, hullPoints[hullStartPoint].z, 
            hullPoints[hullEndPoint].x, hullPoints[hullEndPoint].z);
         if(thisDistSqr<bestDistSqr)
            bestDistSqr=thisDistSqr;
         hullStartPoint=hullEndPoint;
      }
      startPoint=endPoint;
   }

   return(bestDistSqr);   
}

//==============================================================================
// BOPQuadHull::computeCenter
//==============================================================================
void BOPQuadHull::computeCenter(BVector &center) const
{
   // this actually calculates the center.  We don't store it,
   // so you should.  Hence the reference parm.
   float fSumX = 0.0f;
   float fSumZ = 0.0f;
   for (long i = 0; i < 4; i++)
   {
      fSumX += mPoint[i].mX;
      fSumZ += mPoint[i].mZ;
   }
   center.x = fSumX / 4.0f;
   center.y = 0.0f;
   center.z = fSumZ / 4.0f;

   return;

}


//==============================================================================
// BObstructionManager::debugAddPoint
//==============================================================================
void BObstructionManager::debugAddPoint(const char *szName, const BVector &start, DWORD color)
{

  /* static float fDepth = 0.2f;
   fDepth += 0.2f;
   if (fDepth > 1.4f)
      fDepth = 0.2f;
   BVector p1 = start;
   BVector p2 = start;
   p1.x -= 0.25f;
   p1.z -= 0.25f;
   p1.y = 0.5f;
   p2.x += 0.25f;
   p2.z += 0.25f;
   p2.y = 0.5f;
   BDebugPrimRender::addDebugLine(szName, 0L, p1, p2, color, color);
   //game->getWorld()->getTerrain()->addDebugLineOverTerrain(szName, 0L, p1, p2, color, color, fDepth);
   p1.z = start.z + 0.25f;
   p2.z = start.z - 0.25f;
   BDebugPrimRender::addDebugLine(szName, 0L, p1, p2, color, color);
//   game->getWorld()->getTerrain()->addDebugLineOverTerrain(szName, 0L, p1, p2, color, color, fDepth);*/
}

//=============================================================================
// BVector OPQuadHull::findClosestPointOnHull
// Finds the closest point on the nonconvex hull to the point in question.
// the vector returned is that point.  The distancSqr value is set with the
// distance squared between that point and the referenced point.
//=============================================================================
BVector BOPQuadHull::findClosestPointOnHull(const BVector &vStart, long *plSegmentIndex, float *pfClosestDistSqr) const
{

   // Find closest point on the concave hull to this location.
   float fBestDistSqr = cMaximumFloat;
   BVector vClosest(0.0f, 0.0f, 0.0f);
   BVector v1, v2;
   v1.y = 0.0f;
   v2.y = 0.0f;

   long lStartIdx = 3;
   long lBestIdx = -1L;

   for (long lEndIdx = 0; lEndIdx < 4; lEndIdx++)
   {
      BVector vTest(cOriginVector);
      v1.x = mPoint[lStartIdx].mX;
      v1.z = mPoint[lStartIdx].mZ;
      v2.x = mPoint[lEndIdx].mX;
      v2.z = mPoint[lEndIdx].mZ;

      float fDistSqr = vStart.xzDistanceToLineSegmentSqr(v1, v2, &vTest);
      if (fDistSqr < fBestDistSqr)
      {
         fBestDistSqr = fDistSqr;
         vClosest = vTest;
         lBestIdx = lStartIdx;
      }
      lStartIdx = lEndIdx;
   }
   if (plSegmentIndex)
      *plSegmentIndex = lBestIdx;

   if (pfClosestDistSqr)
      *pfClosestDistSqr = fBestDistSqr;

   vClosest.y = 0.0f;
   return vClosest;

}

//==============================================================================
// BOPQuadHull::computePerpToSegment
//==============================================================================
void BOPQuadHull::computePerpToSegment(long segIndex, float &x, float &z) const
{
   // Check for valid segindex.
   if(segIndex<0 || segIndex>3)
   {
      BASSERT(0);
      x=0.0f;
      z=0.0f;
      return;
   }

   // Get next index.
   long nextIndex=segIndex+1;
   if(nextIndex > 3)
      nextIndex = 0;

   // Get perp.
   x = mPoint[segIndex].mZ - mPoint[nextIndex].mZ;
   z = mPoint[nextIndex].mX - mPoint[segIndex].mX;

   // Normalize.
   float recipLen=1.0f/float(sqrt(x*x+z*z));
   x*=recipLen;
   z*=recipLen;
}


//==============================================================================
// BOPQuadHull::expandFrom
//==============================================================================
void BOPQuadHull::expandFrom(const BOPQuadHull *hull, float expansionRadius)
{
   // Should have initialized rotation table already.
   BASSERT(gExpandAngleTableInititalized);

   if(!hull)
   {
      BASSERT(0);
      return;
   }

	long angle = hull->mRotation;

	float xDelta = gExpandCOS[angle] * expansionRadius;
	float zDelta = gExpandSIN[angle] * expansionRadius;

	mX1 = hull->mX1 - xDelta;
	mZ1 = hull->mZ1 - zDelta;

	mX2 = hull->mX2 - zDelta;
	mZ2 = hull->mZ2 + xDelta;

	mX3 = hull->mX3 + xDelta;
	mZ3 = hull->mZ3 + zDelta;

	mX4 = hull->mX4 + zDelta;
	mZ4 = hull->mZ4 - xDelta;

	// fill out 1st 16 byte block

	mRadius = expansionRadius;
	mNextNode = NULL;
	mRotation = hull->mRotation;
	mBoundingIndicies = hull->mBoundingIndicies;
}


//==============================================================================
// BOPQuadHull::suggestPlacement
// So what's this about.  Basically, we take the passed in location, and
// push off to each of the four sides of the obstruction in an axis aligned
// direction.  We then record in the vSuggestion vector the position that was
// closest to the original location.  This is a quick & dirty "snap to edge" of
// obstruction thing.  This code was cannabalized from the original 
// unit::suggestPlacement, which in turn will be replaced to call this function.
//==============================================================================
bool BOPQuadHull::suggestPlacement(const BVector vLocation, float fTestRadiusX, float fTestRadiusZ, BVector &vSuggestion) const
{
#if 0
   float minX = mDirectVal[mIdxMinX];
   float minZ = mDirectVal[mIdxMinZ];
   float maxX = mDirectVal[mIdxMaxX];
   float maxZ = mDirectVal[mIdxMaxZ];
   float lengthX = maxX - minX;
   float lengthZ = maxZ - minZ;
   float halfLengthX = lengthX * 0.5f;
   float halfLengthZ = lengthZ * 0.5f;
   float centerX = minX + halfLengthX;
   float centerZ = minZ + halfLengthZ;

   gObsManager.test
#else
   float fDistSqr = 0.0f;
   float fClosestDistSqr = cMaximumFloat;

   BVector vTemp(0.0f);
   
   float fXLen = mDirectVal[mIdxMaxX] - mDirectVal[mIdxMinX];
   float fZLen = mDirectVal[mIdxMaxZ] - mDirectVal[mIdxMinZ];
   float fXRadius = fXLen * 0.5f;
   float fZRadius = fZLen * 0.5f;
   float fCenterX = mDirectVal[mIdxMinX] + fXRadius;
   float fCenterZ = mDirectVal[mIdxMinZ] + fZRadius;

   // If valid position cannot be suggested just send back current position
   vSuggestion = vLocation;
   bool result = false;   
   float testRadius = 0.0f;

   // Add a gap between test radii so obstruction test won't fail on edge cases
   const float cGap = 0.05f;

   // Define flags for quadtree 
   long obstructionQuadTrees = 
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit | // Unit that can't move                                                       
      BObstructionManager::cIsNewTypeBlockLandUnits;            // Terrain that blocks any combo of movement that includes land-based movement

   // Check
   for (long lDir = 0; lDir < 8; lDir++)
   {
      switch (lDir)
      {
         case 0:
            vTemp.x    = fCenterX + fXRadius + fTestRadiusX + cGap;
            vTemp.z    = fCenterZ;
            testRadius = fTestRadiusX;
            break;
         case 1:
            vTemp.x    = fCenterX;
            vTemp.z    = fCenterZ - fZRadius - fTestRadiusZ - cGap;
            testRadius = fTestRadiusZ;
            break;
         case 2:
            vTemp.x    = fCenterX - fXRadius - fTestRadiusX - cGap;
            vTemp.z    = fCenterZ;
            testRadius = fTestRadiusX;
            break;
         case 3:
            vTemp.x    = fCenterX;
            vTemp.z    = fCenterZ + fZRadius + fTestRadiusZ + cGap;
            testRadius = fTestRadiusZ;
            break;

         case 4:
            vTemp.x    = fCenterX + 0.707f * (fXRadius + fTestRadiusX + cGap);
            vTemp.z    = fCenterZ - 0.707f * (fZRadius + fTestRadiusZ + cGap);
            testRadius = fTestRadiusZ;
            break;
         case 5:
            vTemp.x    = fCenterX - 0.707f * (fXRadius + fTestRadiusX + cGap);
            vTemp.z    = fCenterZ - 0.707f * (fZRadius + fTestRadiusZ + cGap);
            testRadius = fTestRadiusZ;
            break;
         case 6:
            vTemp.x    = fCenterX - 0.707f * (fXRadius + fTestRadiusX + cGap);
            vTemp.z    = fCenterZ + 0.707f * (fZRadius + fTestRadiusZ + cGap);
            testRadius = fTestRadiusZ;
            break;
         case 7:
            vTemp.x    = fCenterX + 0.707f * (fXRadius + fTestRadiusX + cGap);
            vTemp.z    = fCenterZ + 0.707f * (fZRadius + fTestRadiusZ + cGap);
            testRadius = fTestRadiusZ;
            break;

      }
      fDistSqr = vLocation.distanceSqr(vTemp);
      // Test for obstructions on suggested position
      BOOL test = gObsManager.testObstructions( vTemp, testRadius, 0.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, 0 );
      #ifdef DEBUGGRAPH_SUGGESTPLACEMENT
      gTerrainSimRep.addDebugThickCircleOverTerrain(vTemp, 1.0f, .5f, (!test) ? cDWORDGreen : cDWORDRed, 1.0f, BDebugPrimitives::cCategoryNone, 10.0f);
      #endif
      if( ( fDistSqr < fClosestDistSqr ) && !test )
      {
         vSuggestion = vTemp;
         fClosestDistSqr = fDistSqr;
         result = true;
      }
   }
   #ifdef DEBUGGRAPH_SUGGESTPLACEMENT
   gTerrainSimRep.addDebugThickCircleOverTerrain(vSuggestion, 1.0f, .5f, cDWORDPurple, 1.1f, BDebugPrimitives::cCategoryNone, 10.0f);
   #endif

   return (result);
#endif
}

//==============================================================================
// BOPQuadHull::createSimpleHull
//==============================================================================
bool BOPQuadHull::createSimpleHull(const BVector* points)
{
   if(points==NULL)
      return(false);

   mX1 = points[0].x;
	mZ1 = points[0].z;

	mX2 = points[1].x;
	mZ2 = points[1].z;

	mX3 = points[2].x;
	mZ3 = points[2].z;

	mX4 = points[3].x;
	mZ4 = points[3].z;

	mRadius = 0.0f;
	mNextNode = NULL;
	mRotation = 0;
	mBoundingIndicies = 0;
   return(true);
}

//==============================================================================
// eof: obstructionmanager.cpp
//==============================================================================

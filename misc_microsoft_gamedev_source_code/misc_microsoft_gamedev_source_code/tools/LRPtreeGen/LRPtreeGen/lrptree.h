//==============================================================================
// lrptree.h
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================
#pragma once

// *******************************************************************
// THIS IS A STRIPPED DOWN VERSION OF THE GAME APP'S LRPTREE CLASS
// INTENDED ONLY TO SUPPORT OFFLINE GENERATION OF LRPTREE DATA SO
// THAT WE CAN WRITE IT TO A FILE FOR FASTER IN-GAME LOAD TIMES.
// *******************************************************************

//==============================================================================
// Defines
//#define DEBUG_NOLARGEOBS
#define DEBUG_UPDATEBASEQUAD3

//Halwes - 10/9/2006 - Remove recursion
#define PERF_REMOVE_RECURSION

//==============================================================================
// Includes
//XCORE
#include "xcore.h"
#include "xcorelib.h"
#include "math\vector.h"

//==============================================================================
// Forward declarations
class BObstructionManager;
class BConvexHull;
class BPath;
class BOPQuadHull;
class BOPObstructionNode;

//==============================================================================
// Const declarations

// NOTE:  VERY IMPORTANT - you must assign bucketsizes in order of increasing size.
// The Pathing quad tree will break if you do not.  dlm
__declspec(selectany) extern const long clNumBuckets = 3;
__declspec(selectany) extern const long clBucketSizes[clNumBuckets] =
{
   // ajl 11/14/06 - adjust bucket sizes to handle the large unit sizes in phoenix better
   //1, 2, 3
   // Changed this to a more reasonable set of levels. 
   1, 2, 4
};
// DLM - Hi.  If you modify something in the LRPTree data, like, oh, say, the bucket sizes, you should
// update the LRPVersion enum below, so that the file system can flag if you're using a scenario with
// a different version than the one that was used with the lrptree was baked. Word. 
enum eLRPChunkID
{
   cLRP_ECFFileID = 0x00706706,
   cLRPHeaderID =  0x34F00706,
   cLRPVersion  = 0x0002,
   cLRP1ID = 0x0001,
   cLRP2ID = 0x0002,
   cLRP3ID = 0x0003,
   cLRP4ID = 0x0004,
};

const uint64   cLandPassECFChunkID = 0xAAAA000000000000;
const uint64   cFloodPassECFChunkID = 0xAAAB000000000000;
const uint64   cScarabPassECFChunkID = 0xAAAC000000000000;


enum
{
   cMaxLevel      = 3,
   cMaxDepth
};


class BLrpNode
{
   public:
      BYTE                    btPathingConns[clNumBuckets]; // 3 sets of pathing connection info.. 1 for each bucket.  
      BYTE                    btPassability[clNumBuckets];
      BYTE                    btStatus;               // Status
      long                    lPlayerMask;            // If this quad is owned by players, these are the players that own it. 
};

class BLrpPathNode
{
   public:

      float                   fCost;
      float                   fEstimate;
      float                   fX;
      float                   fZ;

      BLrpPathNode           *lrpParent;

      short                   sX;
      short                   sZ;
      BYTE                    sN;
      BYTE                    sDir;
      
};

class BLrpNodeAddy
{
   public:
      BLrpNodeAddy()
      { n = 0; x = 0; z = 0; }
      BLrpNodeAddy(long a, long b, long c) 
      { n = a; x = b; z = c; }
      long n;
      long x;
      long z;
};

typedef BDynamicSimArray<BLrpPathNode> BLrpPathArray;
typedef BDynamicSimArray<BLrpPathNode *> BLrpPathPtrArray;
typedef BDynamicSimArray<BLrpNodeAddy> BLrpNodeAddyArray;


//==============================================================================
class BLrpTree
{
   public:

      // Passability Flags
      enum
      {
         cPassable      = 0x0000,
         cNWInvalid     = 0x0001,
         cNEInvalid     = 0x0002,
         cSEInvalid     = 0x0004,
         cSWInvalid     = 0x0008,
         cAllInvalid    = 0x000F,
         cInUse         = 0x0010,
         cBroken        = 0x0020,
         cPlayerQuad    = 0x0040
      };

      // Connectivity Flags
      enum
      {
         cConnNone      = 0x0000,
         cConnNorth     = 0x0001,
         cConnEast      = 0x0002,
         cConnSouth     = 0x0004,
         cConnWest      = 0x0008,
         cConnAll       = 0x000F,
      };

      // Status Flags
      enum
      {
         cUnknown          = 0x0000,
         cInQueue          = 0x0010,
         cInPath           = 0x0020,
         cInTarget         = 0x0040,
         cTargetClear      = 0x0080
      };

      // Directions
      enum
      {
         cDirNone       = -1,
         cDirNorth,
         cDirEast,
         cDirSouth,
         cDirWest
      }; 

      enum
      {
         cChildNW      = 0,
         cChildNE,
         cChildSE,
         cChildSW
      };
      // Constructors
      BLrpTree( void );

      // Destructors
      ~BLrpTree( void );

      // Functions
      bool                    init(BObstructionManager *pObMgr, float fMaxX, float fMaxZ, float fMinQuadSize);
      void                    reset(void);
      bool                    saveECFLRP(const char* fileNameLRP);

      void                    getQuadBoundaries(long n, long x, long z, BVector &vMin, BVector &vMax);
      void                    updateQuadTree(BOPObstructionNode *pObNode, bool bAdd);
      void                    updateQuadTree(const BVector &vMin, const BVector &vMax);

      void                    setObMgrOptions(long lOptions) { mlObMgrOptions = lOptions; }
      float                   getCellSize() const { return mfCellSize; }
      BLrpNode***             getTree() { return (mpTree); }
      long                    getSizeX(int i) { return (mlSizeX[i]); }
      long                    getSizeZ(int i) { return (mlSizeZ[i]); }
      bool                    isInitialized() { return (mbInitialized); }

      // Variables

   protected:

      struct nodeData
      {
         long      n;
         long      x;
         long      z;
         long      visit;
         BLrpNode* pNode;
         BLrpNode* pNodeChildren[4];
      };

      inline long              pushStack( nodeData* pStackArray, long stackIndex, const nodeData stackEntry );
      inline long              popStack( nodeData* pStackArray, long stackIndex );

      // Functions
      //Halwes - 10/9/2006 - nothing ever uses the bool returned from updateQuad and the 360 compiler does not like functions that return Booleans as per
      //                     "Xbox 360 CPU Performance Update" talk from Summer 2006 GameFest.
      void                    updateQuad(long k, long n, long x, long z);
      void                    updateBaseQuad(long x, long z, const BConvexHull &hull, long lPlayerMask);
      void                    updateBaseQuad2(long x, long z, const BConvexHull &hull, long lPlayerMask);
      void                    invalidateQuad(long k, long n, long x, long z);

      void                    updateBaseQuad3(long x, long z, BOPObstructionNode *pObNode, const BOPQuadHull *pQuadHull, long lPlayerMask);

      bool                    addObstruction(BOPObstructionNode *pObNode);
 
      bool                    removeObstruction(long lX1, long lZ1, long lX2, long lZ2);

      void                    setPathingConnections(long k, long n, long x, long z);

      //Halwes - 10/9/2006 - nothing ever uses the bool returned from initConnectivity and the 360 compiler does not like functions that return Booleans as per
      //                     "Xbox 360 CPU Performance Update" talk from Summer 2006 GameFest.
      void                    initConnectivity(long n, long x, long z);

      // Variables
      BLrpNode                ***mpTree;              // Three dimensional array of nodes to represent tree.  Fast and expensive.
      long                    mlSizeX[cMaxDepth];     // Size of each level in X dim (column)
      long                    mlSizeZ[cMaxDepth];     // Size of each level in Z dim (row)
      long                    mlSizeGrid[cMaxDepth];  // Total No. of nodes at this level
      long                    mlTerrainSizeX[cMaxDepth]; // Size of each level actually occupied by terrain 
      long                    mlTerrainSizeZ[cMaxDepth]; // ditto.
      long                    mlTreeSizeX;            // Actual size of tree in X dim
      long                    mlTreeSizeZ;            // Actual size of tree in Z dim
      float                   mfCellSize;             // Size of smallest quad
      float                   mfRecipCellSize;        // 1/Size of smallest quad
      long                    mlMaxQuadSize;          // Size of largest quad. (in world coords)
      long                    mlMaxQuadTileSize;      // Size of largest quad (in tiles)

      // Current path information
      long                    mlEntityID;             // Entity being pathed
      long                    mlPlayerID;             // Player ID requesting path
      float                   mfRadius;               // Radius of pathed unit.
      float                   mfRange;                // Range of pathed unit
      float                   mfTargetRadius;         // Radius of target
      long                    mlBucket;               // Current bucket we're pathing with
      long                    mlUnitWidth;            // Width of the pathing unit in tiles
      long                    mlTargetCellX;          // X & Z of target.
      long                    mlTargetCellZ;
      long                    mlBackPathOption;       // BackPath Option
      BVector                 mvStart;
      BVector                 mvGoal;
      long                    mGn;                    // Goal Quad Info
      long                    mGx;
      long                    mGz;
      long                    mlIterations;           // No. of iterations used in last path.

      BObstructionManager     *mpObMgr;               // Squirrel off a ptr to the ObMgr

      long                    mlObMgrOptions;

      // Path Queues

      static BConvexHull mStaticHull;                 // Temp Hull for misc. ConvexHull functions.
      static BVector mStaticPoints[5];                // Temp Point array

      // Timing Status
      LARGE_INTEGER           mQPFreq;
      long                    mlPCCalls;              // setPathingConnections
      float                   mfPCSum;
      float                   mfPCMax;

      long                    mlFPCalls;              // findPath
      float                   mfFPSum;
      float                   mfFPMax;

      long                    mlOLCalls;              // addToOpenList
      float                   mfOLSum;
      float                   mfOLMax;
   
      long                    mlAPCalls;              // addPathNode
      float                   mfAPSum;
      float                   mfAPMax;
      
      BDynamicSimArray<BLrpNode*> mTargetQuadList;
   
      bool                    mbInitialized;          // Are we inited?
      bool                    mbEnableSync;           // bool to programmatically control syncing.

   private:

      // Functions

      // Variables

}; // BLrpTree

//==============================================================================
// lrptree.h
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Defines
//#define DEBUG_NOLARGEOBS
#define DEBUG_UPDATEBASEQUAD3

//Halwes - 10/9/2006 - Remove recursion
#define PERF_REMOVE_RECURSION

// rg [1/28/08] - If USE_PACKED_LRP_NODES is defined, packed BLrpNode's are used 
// instead of the original bloated nodes.
#define USE_PACKED_LRP_NODES

//==============================================================================
// Includes

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
// Bucket sizes are in number of cells.
#ifdef DEBUG_NOLARGEOBS
__declspec(selectany) extern const long clNumBuckets = 1;
__declspec(selectany) extern const long clBucketSizes[clNumBuckets] = 
{ 1 
};
#else
__declspec(selectany) extern const long clNumBuckets = 3;
__declspec(selectany) extern const long clBucketSizes[clNumBuckets] =
{
   // ajl 11/14/06 - adjust bucket sizes to handle the large unit sizes in phoenix better
   //1, 2, 3
   // DLM 2/20/08 - bringing the bucket sizes back down to normal, as we don't have anything larger 
   // than four, and we have LOTS of things that are sized 1 to 2 that shouldn't be pathed as size 3. 
   1, 2, 4
};
#endif

// Hi - if you update the LRPTree data make sure you update the version number
// in the enum below (cLRPVersion).  This will allow the executable to verify
// current data against the version of the LRP tree that was used for baking. 
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

struct BUnpackedLrpNode
{
   BYTE                    btPathingConns[clNumBuckets]; // 3 sets of pathing connection info.. 1 for each bucket.  
   BYTE                    btPassability[clNumBuckets];
   BYTE                    btStatus;               // Status
   long                    lPlayerMask;
};

#ifdef USE_PACKED_LRP_NODES
   #pragma pack(push, 1)

   // Static nibble array helper object.
   template<uint cElements>
   struct BStaticNibbleArray
   {
      // Read/write and read-only imposter objects.
      class BAccessor
      {
         BStaticNibbleArray&  mArray;
         uint                 mIndex;
      public:
         BAccessor(BStaticNibbleArray& array, uint index) : mArray(array), mIndex(index) { }
         
         operator int () const { return mArray.get(mIndex); }
         
         int operator= (int newVal) { mArray.set(mIndex, newVal); return newVal; }
         
         int operator++(int) { int curVal = mArray.get(mIndex); mArray.set(curVal + 1); return curVal; }
         int operator--(int) { int curVal = mArray.get(mIndex); mArray.set(curVal - 1); return curVal; }
         
         int operator &= (int mask) { int newVal = mArray.get(mIndex) & mask; mArray.set(mIndex, newVal); return newVal; }
         int operator |= (int mask) { int newVal = mArray.get(mIndex) | mask; mArray.set(mIndex, newVal); return newVal; }
      };
      
      class BConstAccessor
      {
         const BStaticNibbleArray&  mArray;
         uint                       mIndex;
         
      public:
         BConstAccessor(const BStaticNibbleArray& array, uint index) : mArray(array), mIndex(index) { }
               
         operator int () const { return mArray.get(mIndex); }
      };
      
      const BConstAccessor operator[] (uint index) const  { return BConstAccessor(*this, index); }
                 BAccessor operator[] (uint index)        { return BAccessor(*this, index); }
            
      int get(uint index) const
      {
         BDEBUG_ASSERT(index < cElements);
         const uint v = mBytes[index >> 1];
         if (index & 1)
            return (v >> 4) & 0xF;
         else
            return v & 0xF;
      }
      
      void set(uint index, int newValue) 
      {
         BDEBUG_ASSERT(index < cElements);
         BDEBUG_ASSERT((newValue >= 0) && (newValue <= 0xF));
         uint v = mBytes[index >> 1];
         if (index & 1)
            v = (v & 0x0F) | (newValue << 4);
         else
            v = (v & 0xF0) | newValue;
         mBytes[index >> 1] = static_cast<BYTE>(v);
      }

      enum { cNumBytes = (cElements + 1) >> 1 };

      BYTE mBytes[cNumBytes];
   };
   
   // rg [1/28/08] - With 3 buckets this struct is only 5 bytes (2+3)
   class BLrpNode
   {
   public:
      // 3 sets of pathing connection info.. 1 for each bucket.  
      // The last value holds the status field, which only needs 4 bits.
      enum { cStatusFieldIndex = clNumBuckets };      
      
      BStaticNibbleArray<cStatusFieldIndex + 1>                         btPathingConns;      
      BYTE                                                              btPassability[clNumBuckets];

      __declspec(property(get = getStatus, put = putStatus))            int btStatus;            
      __declspec(property(get = getPlayerMask, put = putPlayerMask))    long lPlayerMask;

      inline long getPlayerMask() const { return 0; }
      inline void putPlayerMask(long playerMask)  { playerMask; }
      
      inline int getStatus() const { return btPathingConns.get(cStatusFieldIndex); }
      inline void putStatus(int status) { btPathingConns.set(cStatusFieldIndex, status); }
   };
   #pragma pack(pop)
#else
   class BLrpNode
   {
   public:
      BYTE                    btPathingConns[clNumBuckets]; // 3 sets of pathing connection info.. 1 for each bucket.  
      BYTE                    btPassability[clNumBuckets];
      BYTE                    btStatus;               // Status
      long                    lPlayerMask;
   };
#endif

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
typedef BFreeList<BLrpPathNode, 12> BLrpPathNodeFreeList;
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
         cUnknown          = 0x00,
         cInQueue          = 0x01,
         cInPath           = 0x02,
         cInTarget         = 0x04,
         cTargetClear      = 0x08
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
      bool                    isInitialized() const { return mbInitialized; }
      void                    reset(void);
      void                    saveECFLRP(const char* fileNameLRP);
      bool                    loadECFLRP(const uint64 treeID, const char* fileNameLRP);

      static void             resetStatics();

      void                    getQuadBoundaries(long n, long x, long z, BVector &vMin, BVector &vMax);
      #ifdef DEBUG_UPDATEBASEQUAD3
      void                    updateQuadTree(BOPObstructionNode *pObNode, bool bAdd);
      void                    updateQuadTree(const BVector &vMin, const BVector &vMax);
      #else
      void                    updateQuadTree(const BConvexHull &hull, long lPlayerMask, bool bAdd);
      #endif

      long                    findPath(long lEntityID, BVector &vStart, BVector &vGoal, float fRange, BPath *pPath,
                                 long lPlayerID, float fRadius, float fTargetRadius, long lBackPathOption, bool canJump);

      void                    excludeArea(long lBucket, const BVector &vMin, const BVector &vMax, bool bSet = true);

      bool                    findClosestPassableTile(const BVector &vReference, BVector &vResult);
      bool                    isObstructedTile(const BVector &vPos);

      bool                    findClosestPassableTileEx(const BVector &vReference, BVector &vResult);
      bool                    checkAndSearch(long lX1, long lZ1, BVector &vResult);

      bool                    segmentIntersect(long lBucket, const BVector &v1, const BVector &v2);

      long                    getTileConnections(float fX, float fZ);      // Returns the connections for bucket zero, at the tile level.

      bool                    isSyncEnabled() 
                              { return mbEnableSync; }
      void                    enableSync(bool bSync = true)
                              { mbEnableSync = bSync; }
      
      // Debug Render Function
      void                    render();               // This only renders quads. 
      #ifndef BUILD_FINAL
      void                    renderDebugPath();      // This renders the cached last debug path
      #endif


      BLrpNode                *getNorth(long n, long x, long z) { return (z == mlSizeZ[n] - 1)?NULL:&mpTree[n][x][z+1]; }
      BLrpNode                *getEast(long n, long x, long z)  { return (x == mlSizeX[n] - 1)?NULL:&mpTree[n][x+1][z]; }
      BLrpNode                *getSouth(long n, long x, long z) { return (z == 0)?NULL:&mpTree[n][x][z-1]; }
      BLrpNode                *getWest(long n, long x, long z)  { return (x == 0)?NULL:&mpTree[n][x-1][z]; }

      long                    getIterations() const { return mlIterations; }
      void                    setObMgrOptions(long lOptions)
                              { mlObMgrOptions = lOptions; }
      float                   getCellSize() const { return mfCellSize; }

      void                    initStats();
      void                    dumpStats();
      // Variables
      
      void                    analyzeNodes() const;

   protected:

      //Halwes - 10/9/2006 - Remove recursion
      #if defined( PERF_REMOVE_RECURSION )
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
      #endif

      // Functions
      //Halwes - 10/9/2006 - nothing ever uses the bool returned from updateQuad and the 360 compiler does not like functions that return Booleans as per
      //                     "Xbox 360 CPU Performance Update" talk from Summer 2006 GameFest.
      #if !defined( PERF_REMOVE_RECURSION )
      bool                    updateQuad(long k, long n, long x, long z);
      #else
      void                    updateQuad(long k, long n, long x, long z);
      #endif      
      void                    updateBaseQuad(long x, long z, const BConvexHull &hull, long lPlayerMask);
      void                    updateBaseQuad2(long x, long z, const BConvexHull &hull, long lPlayerMask);
      void                    invalidateQuad(long k, long n, long x, long z);

      #ifdef DEBUG_UPDATEBASEQUAD3
      void                    updateBaseQuad3(long x, long z, BOPObstructionNode *pObNode, const BOPQuadHull *pQuadHull, long lPlayerMask);
      #endif

      #ifdef DEBUG_UPDATEBASEQUAD3
      bool                    addObstruction(BOPObstructionNode *pObNode);
      #else
      bool                    addObstruction(const BConvexHull &hull, long lPlayerMask);
      #endif

      bool                    removeObstruction(long lX1, long lZ1, long lX2, long lZ2);
      void                    setPathingConnections(long k, long n, long x, long z);

      //Halwes - 10/9/2006 - nothing ever uses the bool returned from initConnectivity and the 360 compiler does not like functions that return Booleans as per
      //                     "Xbox 360 CPU Performance Update" talk from Summer 2006 GameFest.
      #if !defined( PERF_REMOVE_RECURSION )      
      bool                    initConnectivity(long n, long x, long z);
      #else
      void                    initConnectivity(long n, long x, long z);
      #endif
      void                    renderQuad(long n, long x, long z, long clipHint, bool bInvalid = false);
      bool                    setTargetQuad(long lBucket, BVector &vTarget, float fRange, bool bSet);
      BLrpNode                *getInUseQuadWithPoint(const BVector &vPoint, long lBucket, long &n, long &x,
                                 long &z);
      void                    addPathNode(long lBucket, long n, long x, long z, long lDir, BLrpPathNode *lrpParent);
      void                    getQuadEstimate(long n, long x, long z, BVector &vEstimate);
      void                    fixupWaypoints(long lBucket, BLrpPathNode *pnodePath);
      bool                    findWaypoint(long lBucket, BLrpPathNode *pnode, BVector &vLastWaypoint, 
                                 BVector &vWaypoint, BVector &vNextWaypoint);
      void                    estimateWaypoint(BLrpPathNode *pnode, BVector &vWaypoint);
      void                    pathCleanup(bool bDoExclusions);

      bool                    pointInQuad(BLrpPathNode *pnode, const BVector &vector);

      long                    findClosestPointInQuad(long k, long n, long x, long z, const BVector &vGoal, 
                                      const BVector &vStart, BVector &vClosest);
      long                    calcCellDistance(long k, long n, long lSrcX, long lSrcZ, long lDestX, long lDestZ);
      float                   getClosestPointInCell(long x, long z, const BVector &vGoal, BVector &vClosest);
      float                   getDistanceToQuad(long n, long x, long z, const BVector &vGoal, BVector &vClosest);

      void                    excludeQuad(long lBucket, long n, long x, long z, bool bSet);
      bool                    findClosestValidQuad(long lBucket, long n, long x, long z, long &n1, long &x1, long &z1);
      bool                    checkValidQuad(long lBucket, long n, long x, long z, long &n1, long &x1, long &z1);

      void                    markQuadInPath(long n, long x, long z);
      bool                    checkQuadInPath(long n, long x, long z);

      bool                    testCell(long lConnDir, long lBucket, long lX, long lZ);
      void                    backPath(long lBucket, BPath *path);

      BLrpNode                *findValidStart(const BVector &vStart, long lBucket, long &m, long &c, long &r);
      void                    adjustForRadius(const BVector &vClosest, const BVector &vGoal, BVector &vAdjClosest);

      void                    debug(char* v, ... ) const;
      void                    debugAddPoint(const char *szName, const BVector &start, DWORD color);

      // Path Node Functions
      BLrpPathNode            *getNewNode(BLrpPathNode *lrpParent);
      BLrpPathNode            *addToOpenList(BLrpPathNode *lrpParent, long n, long x, long z, long lDir, float fX, float fZ,
                                 float fAdditionalCost = 0.0f, bool skipBoundsCheck=false);
      void                    addToClosedList(BLrpPathNode *pnode) { mClosedList.add(pnode); }
      BLrpPathNode            *getOpenNode();
            
      // Variables
      BVector                 mvStart;
      BVector                 mvGoal;
      
      BLrpNode                ***mpTree;              // Three dimensional array of nodes to represent tree.  Fast and expensive. Lives on the sim heap.
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
//      long                    mlTerrainSizeX;         // Size of terrain in cells (X)
//      long                    mlTerrainSizeZ;         // Size of terrain in cells (Z)

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
      long                    mGn;                    // Goal Quad Info
      long                    mGx;
      long                    mGz;
      long                    mlIterations;           // No. of iterations used in last path.

      BBitArray               mbaExclusions;           // Bit Array of quads marked as "inPath"

      BObstructionManager     *mpObMgr;               // Squirrel off a ptr to the ObMgr

      long                    mlObMgrOptions;

      bool                    mCanJump;               // Whether the tree should allow passibility in the jump areas

      // Path Queues
      static BLrpPathNodeFreeList mNodeFreeList;
      static BLrpPathPtrArray mOpenList;
      static BLrpPathPtrArray mClosedList;
      static BLrpPathPtrArray mFixupList;
      static BLrpNodeAddyArray mNodeStack;

      static BConvexHull mStaticHull;                 // Temp Hull for misc. ConvexHull functions.
      static BVector mStaticPoints[5];                // Temp Point array

      #ifndef BUILD_FINAL
      static BPath            mDebugPath;             // Cache the path off for debugline purposes.  Currently we only keep 1 of these at at time.
      #endif

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
   
      bool                    mbInitialized : 1;          // Are we inited?
      bool                    mbEnableSync : 1;           // bool to programmatically control syncing.

   private:

      // Functions

      // Variables

}; // BLrpTree

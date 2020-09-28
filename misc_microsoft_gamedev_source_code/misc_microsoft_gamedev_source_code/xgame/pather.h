//==============================================================================
// pather.h
//
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
#include "path.h"
#include "typedid.h"
//#include "pathtree.h"
#include "nonconvexhull2.h"
#include "pathquad.h"
#include "convexhull.h"
#include "math\runningAverage.h"

#ifndef BUILD_FINAL
   #include "configsgame.h"
#endif

// Timing Status
#ifndef BUILD_FINAL
#ifndef DEBUG_TIMING_STATS
#define DEBUG_TIMING_STATS
#endif
#endif

// ON _PATH_3:
// This was an "optimization" attempt that feeds all the obstructions in the hulled area into a single nonconvex hull instance, "mMasterNCH",
// which does all the work of creating NCH's out of the overlapping obstruction clouds.  PostHullPather_3 then does the pathing around all
// of the NCH's.  Unfortunately, it's actually slower (a tad) than than the crappy overlapping hulls/convert to convex code we have now
// so we're turning it off, until I (or someone) can optimize it.  It's a *lot* less code than the current pather. DLM 4/21/08
//#define _PATH_3

//==============================================================================
// Forward declarations
class BObstructionManager;
class BConvexHull;
class BUnit;
class BPathQueues;
class BPathQuadMgr;
class BPathNode2;
class BLrpTree;
class BLrpNode;
class BBitQuadtree;
class BEntity;

		// Forward Declarations to support the new obstruction manager

class BOPObstructionNode;


//==============================================================================
// Const declarations
// Breakover point for the low level pather
#define _MOVE4
#ifdef _MOVE4
   const float cMaximumLowLevelDist = 80.0f;
#else
   const float cMaximumLowLevelDist = 40.0f;
#endif
const float cLowLevelRelaxFactor = 8.0f;
const float cLowLevelRelaxFactorSqr = cLowLevelRelaxFactor * cLowLevelRelaxFactor;
const float cMaximumLowLevelDistSqr = cMaximumLowLevelDist*cMaximumLowLevelDist;
const float cPathingRadiusReductionFactor = 0.50f;
const long  clInitAggregateObListSize = 64;

// Parameter collection for pathing calls.
class BPathParms
{
   public:
      // Entity Info
      long                       lEntityID;
      long                       lEntityType;
      float                      fRadius;
      long                       lPlayerID;
      BEntity                    *pEntity;

      // Target Info
      long                       lTargetID;
      float                      fTargetRadius;
      BUnit                      *pTarget;

      // Path Info
      long                       lBackPath;
      long                       lPathClass;
      long                       lPathSource;
      bool                       fullPathOnly;
      bool                       canJump;

      // Ob Manager Info
      BObstructionManager        *pObManager;
      const BEntityIDArray       *pIgnoreList;

};

typedef BFreeList<BNonconvexHull, 7> BNchFreeList;


class BPathIntersectResult
{
   public:
      float                   fDistSqr;
      long                    lHullIdx;
      long                    lSegmentIdx;
      long                    lNCHIdx;
      BVector                 vIntersect;
};

typedef BDynamicSimArray<BPathIntersectResult> BPathIntersectResultArray;


//==============================================================================
class BPather
{
   public:

      // Internal enums

      // Return codes from beginPathing
      enum 
      {
         cInitialized,
         cInvalid
      };

      // Path Types
      enum
      {
         cUndefined=0,           // Let the Pather determine what kind of path to get (default)
         cShortRange,            // Use the Short Range Pather
         cLongRange              // Use the Long Range Pather
      };

      // Path Classes - We're likely to have more classes in the future..
      // Wall units, units on boats maybe.. pathing without gates.. who knows.
      enum
      {
         cLandPath,              // We're pathing a land unit
         cSquadLandPath,         // Pathing a squad.  Path around squad obstructions too.
         cFloodPath,             // Pathing a flood unit
         cScarabPath,            // Pathing a scarab unit
         cAirPath,               // Pathing an air unit
         cHoverPath              // Pathing a hover unit
      };

      // The source of the path..
      enum
      {
         cUndefinedSource,
         cUnitSource,
         cUnitGroupSource,
         cProtoUnitSource,
         cPlatoonSource
      };

      // Back Path Options
      enum
      {
         cBackPathOff,
         cBackPathOn,
         cBackPathOnlyOnFull
      };

      // DLM - 6/18/08 - We're actually using this now, so bump it up to something that is meaningful in this game. 
      static const long cShortRangeLimitDist   = 16384;       //  128 meters squared

      // Constructors
      BPather( void );

      // Destructors
      ~BPather( void );

      // Primary Interfaces

      bool                    init(void);
      void                    reset(bool destruct = false);

      void                    resetTrees(void);

      long                    beginPathing(const long lEntityID, const long lEntityType,
                                 BObstructionManager *pObManager, const BEntityIDArray *pIgnoreList,
                                 const float fRadius, bool bSkipObBegin, bool fullPathOnly, bool canJump, long lTargetID = -1,
                                 long pathClass = cLandPath);

      long                    beginProtoPathing(const long lPlayerID, const long lProtoID,
                                 BObstructionManager *pObManager, const BEntityIDArray *pIgnoreList,
                                 const float fRadius, bool bSkipObBegin, bool fullPathOnly, bool canJump, long lTargetID = -1,
                                 long pathClass = cLandPath);

      void                    endPathing();
      
                              // This is the interface to use when you batch off your begin and ends.  Note..
                              // you can pass in different entity ID's and TargetID's for each path, but you
                              // should not expect the radius used by the pather to change during a batch of
                              // of calls, as this determines the size of all the cached hulls/obstructions.
      long                    findPath(long lEntityID, const BVector &vStart, const BVector &vGoal, float fRange, 
                                 BPath *pPath, long lPathType = cUndefined, long lTargetID = -1L);

      // Similar to old style, but accepts a simple long array of ignoreUnit ID's, versues the 
      // TypedID entity array above.
      long                    findPath(const long entityID, const long entityType,
                                 BObstructionManager *obManager, const BVector &start, 
                                 const BVector &goal, const float radius, const float range, 
                                 const BEntityIDArray& ignoreUnits, 
                                 BPath *path, bool skipBegin, bool fullPathOnly, bool canJump, long lTargetID = -1L,
                                 long lPathType = cUndefined, long pathClass = cLandPath);

      // Interface for finding a path for a proto unit..
      
      long                    findProtoPath(const long lPlayerID, const long lProtoID,
                                 BObstructionManager *obManager, const BVector &start,
                                 const BVector &goal, const float radius, const float range,
                                 const BEntityIDArray &ignoreUnits,
                                 BPath *path, bool skipBegin, bool fullPathOnly, bool canJump, long lTargetID = -1L, long lPathType = cUndefined);
      

      // YAPI (Yet Another Pathing Interface) - This one to just calc a path without an entity id.
      long                    findPath(BObstructionManager *obManager, const BVector &start, 
                                 const BVector &goal, const float radius, const float range, 
                                 const BEntityIDArray *ignoreList, BPath *path, bool skipBegin,
                                 bool fullPathOnly, bool canJump, long lTargetID = -1L, long lPathType = cUndefined, long pathClass = cLandPath);


      // Some new interfaces to the info the long range pather uses
      bool                    findClosestPassableTile(const BVector &vReference, BVector &vResult, long lLandFloodScarab = cLandPath);
      bool                    isObstructedTile(const BVector &vPos, long lLandFloodScarab = cLandPath);
      // Newer expanded flood fill version
      bool                    findClosestPassableTileEx(const BVector &vReference, BVector &vResult, long lLandFloodScarab = cLandPath);

      // checks tiles in the LRP along the segment and returns true if there's an intersection
      bool                    tileSegmentIntersect(const BVector &vStart, const BVector &vEnd);

      // Hijacked this internal routine and made it public, so platoon can use it to find locations
      // for squads to move to, so they're not standing on top of each other.  It ASSUMES you
      // have set up the obmanager correctly.  
      bool findUnobstructedPoint_3(const BVector &vPoint, BVector &vAdjPoint);

      // Debug render
      void                    render();

      // Quad Tree Updating
      void                    buildPathingQuads(BObstructionManager *pObManager);
      inline void             enableQuadUpdate(bool bUpdate = true)
                              { mbQuadUpdate = bUpdate; }
      inline bool             getEnableQuadUpdate(void) const { return(mbQuadUpdate); }

      // Debug Stats
      #ifndef BUILD_FINAL
      void                    initStats();
      void                    dumpStats();
      #endif

      void                    setBackPathOption(long lBackPathOption) { mPathParms.lBackPath = lBackPathOption; }
      long                    getBackPathOption(void) { return mPathParms.lBackPath; }

      void                    setTargetID(const long lTargetID) 
                              { mPathParms.lTargetID = lTargetID; }

		// Functions to support the new obstruction manager

		void							updatePathingQuad(BObstructionManager *pObManager, BOPObstructionNode* theNode, bool AddNode);
		void							updatePathingQuad(BObstructionManager *pObManager, const BVector &vMin, const BVector &vMax);

      // Info functions
      const BEntity           *getPathingEntity() const { return mPathParms.pEntity; }
      const BUnit             *getPathingTarget() const { return mPathParms.pTarget; }

      bool                    getLongRangeSyncing(long lLandFloodScarab = cLandPath);
      void                    setLongRangeSyncing(bool bEnable, long lLandFloodScarab = cLandPath);

      // Useful debug displays everyone should have a chance to use..
      // DLM 2/11/08 - Yes.. that's right.. 2008.  These debug routines will actually be
      // very useful eventually, but currently they're not in use, so I'm commenting them out
      #ifndef BUILD_FINAL
      void                    debugAddPoint(int category, const BVector &start, DWORD color);
      void                    debugAddHull(int category, const BConvexHull &hull, DWORD color);
      void                    debugAddHull(int category, const BOPQuadHull *pHull, DWORD color);
      void                    debugAddNch(int category, const BNonconvexHull &hull, DWORD color, long group=0);
      void                    debugClearRenderLines(int category);
      #endif

      // Here are the new debugRender routines
      #ifndef BUILD_FINAL
      void                    renderDebugLowLevelPath();
      #endif

//      void                    saveLRPTree(const char* fileNameLRP);
      bool                    loadLRPTree(const char* fileNameLRP);
      bool                    isLRPTreeLoaded() { return(mbLRPTreeLoaded); }

      #ifndef BUILD_FINAL
         static long          getShowPaths() { long val = -1; gConfig.get(cConfigShowPaths, &val); return val; }
      #endif

      // Pathing Stats
      #ifdef DEBUG_TIMING_STATS
      // DLM 11/5/08 - Updated

      // Low Level Pather Stats
      long                    mlLLCallsPerUpdate;
      BRunningAverage<long,double> mLLAverageCallsPerUpdate;

      BRunningAverage<double,double> mLLAverageTimePerCall;

      BRunningAverage<double, double> mHullAreaAverageTimePerCall;

      // High Level Pather Stats
      long                    mlHLCallsPerUpdate;
      BRunningAverage<long,double> mHLAverageCallsPerUpdate;

      BRunningAverage<double,double> mHLAverageTimePerCall;

      uint                    mReferenceFrame;

      // End of stuff I've udpated
      #endif

   protected:

      #ifndef BUILD_FINAL
      void                    sendRemoteDebuggerUpdate(void);
      #endif


      // Functions
      long                    findQuickPath(const BVector &start, const BVector &goal, 
                                 const float range, BPath *path);


      long                    findHighLevelPath2(const BVector &start,
                                 const BVector &goal, const float range, BPath *path);


      bool                    findUnobstructedPoint(const BVector &vOrig, const BVector &vGoal, BDynamicSimArray<BOPObstructionNode*> &alObs, BVector &vEscape);

      bool                    findNewPolyStart(const BVector &vStart, BVector &vEscape, BDynamicSimArray<BOPObstructionNode*> &obs, 
                                 long lAttempts, BVector *vAltPoint, bool checkingStartPoint);

      bool                    findOverlappingObstructions(const BVector &vTestPnt, BDynamicSimArray<BOPObstructionNode*> &obs, float limitDistSqr);


      void                    assemblePath(const BPath *source, BPath *dest, long lPathResult);

      // Even Newer Pathing Routines (IronHull Pather)
      long                    findLowLevelPath2(const BVector &start, 
                                 const BVector &goal, const float range, BPath *path);
      long                    postHullPather(const BVector &vStart, const BVector &vGoal, const BVector &vOrigGoal,
                                 float fRange, BPath *path, long lStartInsideIdx, long lGoalInsideIdx, 
                                 bool interior, long edgeHull=-1, long edgeSegment=-1, long edgeNchIndex=-1);
      long                    postHullPather_3(const BVector &vStart, const BVector &vGoal, const BVector &vOrigGoal,
                                 float fRange, BPath *path, long lStartInsideIdx, long lGoalInsideIdx, 
                                 bool interior);
         
      void                    hullArea();
      long                    doInsideCheck(const BVector &vPoint, long lExcludeIndex = -1);

                              // SegIntersect stores the results in the mIntersectResults Array
      bool                    doSegIntersectCheck(const BVector &vStart, const BVector &vGoal,
                                 long lCurrHull, long lCurrIdx, long startInsideIndex, bool interior, bool &hitInterior);
#ifdef _PATH_3
      bool                    doSegIntersectCheck_3(const BVector &vStart, const BVector &vGoal,
                                 long lCurrHull, long lCurrIdx, long startInsideIndex, 
                                 bool interior, bool &hitInterior);
#endif
      bool                    segIntersects(const BVector &vStart, const BVector &vGoal, float errorEpsilon=0.01f);
      long                    doNchBuild(long lPostHullIdx, const BVector &start, bool &bOnEdge, BVector &vAdjStart, long &segIndex, long &nchIndex);
      bool                    doInteriorBuild(long lNchIdx, const BVector &vInteriorPnt);
      
      long                    doNewSegIntersectCheck(const BVector &vStart, const BVector &vGoal,
                                 BVector &iPoint, long &hullIndex, long &segIndex);


      void                    updateEntityInfo();

      bool                    installObstructions(BBitQuadtree* tree);
      bool                    removeObstructions(BBitQuadtree* tree);

      // Debug Displays
      void                    debug(char* v, ... ) const;
               
      // Variables
      LARGE_INTEGER           mQPFreq;

      BPathParms              mPathParms;
      long                    mlPathType;
      BPath                   mTempPath;

      BPathQueues             *mQueues;               // Our Pathing Queues

      BSimpleNonconvexPtrArray mNonconvexArray;
      BNchFreeList             *mpNchFreeList;

      BVector                 mvLimitPos;             // Limiting factors on overlapping hull gathering
      float                   mfLimitDist;
      float                   mfTileSize;             // Terrain Tile Size, we use it for some limit checks in the quad pather
      float                   mfTileSizeSqr;          // Guess..

      BDynamicSimArray<BOPObstructionNode*> mObArray;

      // Limits
      long                    mlBadPathThresholdLow;  // Low Level pathing threshold
      long                    mlBadPathThresholdHigh; // High Level Pathing threshold
      
      // New Parms for IronHull (tm) pather
      BConvexHull             mHulledArea;
      BNonconvexArray         mHullArray1;
      BNonconvexArray         mHullArray2;
      BDynamicSimVectorArray      mAddPoints;
      BDynamicSimArray<BOPObstructionNode*> mPostObs;
      BPathIntersectResultArray mIntersectResults;

      BNonconvexArray*        mPreHulls;
      BNonconvexArray*        mPostHulls;

      // New Parms for Lrp Pather
      BLrpTree                *mLrpTreeLand;
      BLrpTree                *mLrpTreeFlood;
      BLrpTree                *mLrpTreeScarab;
      BLrpTree                *mLrpTreeHover;
      BLrpTree                *mLrpTreeAir;

      // Old Pathing Stats
      #ifndef BUILD_FINAL
      long                    mLLFullCalls;
      long                    mLLExpansions;

      long                    mlPolyCalls;
      float                   mfPolySum;
      float                   mfPolyMax;
      float                   mlPolyNewIntersects;    // No. of times that re-testing intersects actually gave us a new intersect.
      long                    mlPolyIterations;
      long                    mlPolyIterationMax;
      long                    mlPolyStartCalls;
      long                    mlPolyEndCalls;

      long                    mlNCCalls;
      long                    mlNCFails;
      float                   mfNCSum;
      float                   mfNCMax;
      long                    mlNCMaxObs;
      long                    mlNCObsAtMax;
      long                    mStartStas;

      long                    mlHLIterations;         // Total No. of Iterations
      long                    mlHLIterationSum;
      long                    mlHLIterationMax;       // Max No. of iterations
      
      long                    mlPHCalls;
      float                   mfPHSum;
      float                   mfPHMax;


      long                    mlOLCalls;
      float                   mfOLSum;
      float                   mfOLMax;

      #endif

      BDynamicSimLongArray        mBannedUnits;

      bool                    mbQuadUpdate;           // Enable/Disable quad updating
      bool                    mbSkipObBegin;
      bool                    mbInitialized;
      bool                    mbLRPTreeLoaded;        // Long Range Path tree has been loaded from disk

      // DLM - Static Debug Stuff -- added in 2/11/8
#ifndef BUILD_FINAL
      static BPath            mDebugPath;
#endif

#ifdef _PATH_3
   protected:
      long findLowLevelPath_3(const BVector &vStart, const BVector &vGoal, const float fRange, BPath *path);

      BNonconvexHull       mMasterHull;
#endif
   private:

      // Functions

      // Variables

}; // BPather

extern BPather gPather;

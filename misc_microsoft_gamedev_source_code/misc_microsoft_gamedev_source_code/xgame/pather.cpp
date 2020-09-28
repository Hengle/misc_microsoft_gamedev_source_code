//==============================================================================
// pather.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "pather.h"
#include "pathingLimiter.h"
#include "protoobject.h"
//#include "aconfigenums.h"
#include "game.h"
#include "world.h"
//#include "terrainbase.h"
#include "player.h"
#include "obstructionmanager.h"
#include "path.h"
//#include "renderdevice.h"
//#include "debugprimrender.h"
#include "segintersect.h"
#include "pathquad.h"
#include "unit.h"
#include "pathqueues.h"
//#include "aperfenum.h"
//#include "coreoutput.h"
#include "database.h"
//#include "unitgroup.h"
#include "syncmacros.h"
#include "lrptree.h"
#include "bitquadtree.h"
//#include "remotegamedebugger.h"
#include "squad.h"
#include "terrainsimrep.h"
#include "configsgame.h"
#include "debugprimitives.h"
#include "econfigenum.h"



//#define DEBUG_HALFTILEQUADS
BPather gPather;
//==============================================================================
// Constants
const float cOnSegmentEpsilon       = 0.01f;         // How close to be defined as on a segment
const long cMaxEscapeAttempts       = 3;              // Maximum retries for escaping obstructions
//const long cMaxEscapeAttempts       = 0;              // Maximum retries for escaping obstructions
const float cEscapeVelocity         = 2.0f;           // Distance to travel each iteration when trying to escape obstructions
const long cMaxExteriors            = 10;             // Number of Exterior hulls to collect in any one direction.
const long cMaxObstructionDepth     = 100;            // Maximum recursion depth for finding overlapping obstructions
const float cPathOutErrorTolerance  = 0.5f;           // Used for adding nodes to the open/closed lists
const long cMaxTileIterations       = 1000L;          // Maximum Iterations used in TilePather
const long clFailedMax              = 1000L;           // Maximum allowed failed nodes before bailing.
const long clHullArraySize          = 128;            // Initial size of Pre & Post Hull Arrays
const long clVectorArraySize        = 32;             // Initial size of Vector Scratch Space 
const long clIntersectResultSize    = 32;             // Initial size of PathIntersect Result Array
const float cfMinHulledAreaLength   = 8.0f;           // Minimum size of Hulled Area
const long  clAllowedExpansions     = 0;              // Number of times we allow the hulled area to expand
#ifdef DEBUG_HALFTILEQUADS
const float cfMinQuadFactor         = 0.5f;           // Size of minimum quad in relation to a tile size.  (ie., 1.0 means min quad == 1 tile )
#else
// DLM 7/31/08 - doubling the size of a minimum quad in the quad tree.
const float cfMinQuadFactor         = 1.0f;           // Size of minimum quad in relation to a tile size.  (ie., 1.0 means min quad == 1 tile )
#endif

const float cfPolyTimeLimit         = 60.0f;          // Debugging use only.. time limit at which to log a bad path..
const float cfQuadTimeLimit         = 100.0f;         // Same for quad paths.

// HACK - This should be a data driven thing.  However, we do *not* want to make it
// on a per-unit basis thing, as this would drive me crazy. dlm.
const float cfWadingDepth           = 1.00f;          // Maximum Depth at which water is still passable.


/*
#ifndef _PATH_3
#define _PATH_3
#endif
*/

// New constants for FindLowLevelPath_3
const float cFindUnobstructedLimit  = 48.0f;          // The max we'll allow ourselves to look for an unobstructed position to begin or end pathing
const float cFindUnobstructedNudge  = 1;           // The amount we nudge the "get off" vector to push ourselves outside the obstruction.

#ifdef _PATH_3
//#define DEBUG_FINDLOWLEVELPATH_3
//#define DEBUG_POSTHULLPATHER_3

//#define DEBUGGRAPH_FINDUNOBSTRUCTED_3
//#define DEBUGGRAPH_MASTERHULL_3

#endif

static BVector sScratchPoints[4];                     // Vector scratch pad

// jce [10/7/2008] -- moved these out of doNchBuild to allow them to get cleared on reset to make inter-scenario
// memory checks happy.
// Static to hold currently overlapping hulls.
static BDynamicSimArray<BOPObstructionNode*> sCurrentList;

//#define DEBUG_USENEWPOLYSTART
#ifndef BUILD_FINAL
BPath BPather::mDebugPath;
#endif

#ifdef DEBUG_USENEWPOLYSTART
// TEST
const cNumEscapeDirections = 16;
const BVector cDirections[] = 
{
   BVector(1.0f, 0.0f, 0.0f), 
   BVector(-1.0f, 0.0f, 0.0f), 
   BVector(0.0f, 0.0f, 1.0f),
   BVector(0.0f, 0.0f, -1.0f), 

   BVector(cOneOverSqrt2, 0.0f, cOneOverSqrt2), 
   BVector(-cOneOverSqrt2, 0.0f, cOneOverSqrt2), 
   BVector(cOneOverSqrt2, 0.0f, -cOneOverSqrt2), 
   BVector(-cOneOverSqrt2, 0.0f, -cOneOverSqrt2),

   BVector((float)cos(cPiOver8), 0.0f, (float)sin(cPiOver8)),
   BVector((float)sin(cPiOver8), 0.0f, (float)cos(cPiOver8)),
   BVector(-(float)sin(cPiOver8), 0.0f, (float)cos(cPiOver8)),
   BVector(-(float)cos(cPiOver8), 0.0f, (float)sin(cPiOver8)),
   BVector(-(float)cos(cPiOver8), 0.0f, -(float)sin(cPiOver8)),
   BVector(-(float)sin(cPiOver8), 0.0f, -(float)cos(cPiOver8)),
   BVector((float)sin(cPiOver8), 0.0f, -(float)cos(cPiOver8)),
   BVector((float)cos(cPiOver8), 0.0f, -(float)sin(cPiOver8))
};
#endif
//==============================================================================
// Defines
//#define NEW_SEG_CHECK

#ifndef BUILD_FINAL
#define DEBUG_GRAPHICAL
#endif

//#define DEBUG_SINGLEPASS_POLY

#ifndef BUILD_FINAL
//#define DEBUG_DUPLICATE_OBSTRUCTION_CHECK
//#define DEBUG_SKIP_QUICKPATH
//#define DEBUG_NOBACKPATH
//#define DEBUG_FORCE_LOWLEVEL
//#define DEBUG_NEW_QUEUES
//#define DEBUG_NOLIMITCHECK
//#define DEBUG_USELRPTREE
//#define DEBUG_PATHSYNC
//#define DEBUG_LRPPATHSYNC
//#define DEBUG_GRAPH_FINDNEWSTART

//#define DEBUG_HASHSTATS

// Plotting Aids
// Pathing Categories!
/*
cCategoryPatherLowLevelPather,
cCategoryPatherHulls,
cCategoryPatherQuadPather,
cCategoryPatherUnobstructedPoints,
cCategoryPatherStartEndPoints,
cCategoryPatherPostHullPather,
cCategoryPatherHulledArea,
cCategoryPatherHulledItems,
cCategoryPatherPostHulls,
cCategoryPatherNonConvex,
cCategoryPatherNonConvex2,
cCategoryPatherNewPolyStart,
cCategoryPatherPathNodes,
cCategoryPatherPostNchs,
*/
// DLM - 2/18/08 Returning on low level pathing debugging goodness
//#define DEBUGGRAPH_HULLEDAREA
//#define DEBUGGRAPH_HULLEDITEMS
//#define DEBUGGRAPH_STARTENDPOINTS
//#define DEBUGGRAPH_POSTNCHS
//#define DEBUGGRAPH_POSTHULLS
//#define DEBUGGRAPH_NEWPOLYSTART

//#define DEBUG_GRAPHPATHNODES
//#define DEBUG_GRAPHHULLNODES
//#define DEBUG_INVALIDHULLPNT
//#define DEBUG_NONCONVEX_GRAPHICAL         // Use this one in conjuction with the one in nonconvexhull.cpp
//#define DEBUG_GRAPHPOSTNCHS
//#define DEBUG_GRAPHNEWPOLYSTART

//#define DEBUG_GRAPH_UNOBSTRUCTEDPOINTS


// Logging Aids
//#define DEBUG_FINDPATH
//#define DEBUG_QUICKPATH
//#define DEBUG_FINDLOWLEVELPATH
//#define DEBUG_FINDLOWLEVELPATH2
//#define DEBUG_POLYPATHER
//#define DEBUG_FINDNEWPOLYSTART
//#define DEBUG_POSTHULLPATHER

//#define DEBUG_ESCAPEOBSTRUCTIONS
//#define DEBUG_ASSEMBLEPATH

//#define DEBUG_FINDHIGHLEVELPATH2
//#define DEBUG_FINDTILEPATH
//#define DEBUG_FINDQUADPATH
//#define DEBUG_DOINSIDECHECK
//#define DEBUG_DOINTERIORBUILD
//#define DEBUG_BEGINEND
//#define DEBUG_DONCHBUILD
//#define DEBUG_HULLAREA

// Timing Stuff
// Timing limits MUST be enabled for timing cap's to work.
//#define DEBUG_PATHING_LIMITS

//#define DEBUG_TIMING


//#define DEBUG_OVERLAPTIMING

//#define DEBUG_VTUNE

//#define DEBUG_POLYPATHER2
#endif

const long clBadPathThresholdLow = 120L;
const long clBadPathThresholdHigh = 1000L;

const long clObStackSize = 512;

#ifdef DEBUG_VTUNE
#include "vtuneapi.h"
#endif

#ifdef DEBUG_GRAPHICAL
const DWORD cColorHullComplete = cDWORDCyan;                   // Complete Convex Hull    (Cyan)
const DWORD cColorHullIncomplete = cDWORDYellow;               // Incomplete Convex Hull  (Yellow)
const DWORD cColorOrigStartPnt = cDWORDBlue;                   // Original Start Pnt      (Blue)
const DWORD cColorOrigGoalPnt = cDWORDGreen;                   // Original Goal Pnt       (Green)
const DWORD cColorPathStartPnt = cDWORDCyan;                   // Actual Path Start       (Cyan);
const DWORD cColorPathGoalPnt = packRGB(0, 191, 255);          // Actual Path Goal       (Lt. Green);
const DWORD cColorExaminedPoint = cDWORDOrange;                // Examined node           (Orange)
const DWORD cColorHulledArea = cDWORDRed;                      // Hulled Area             (Red);
const DWORD cColorHulledObs = cDWORDBlue;                      // Expanded Obstructions in Hulled Area  (Blue);
const DWORD cColorPostHulls = cDWORDCyan;                      // PostHulls               (Cyan)
const DWORD cColorPostHullPather = cDWORDMagenta;              // PostHullPather          (Magenta)
const DWORD cColorNchPather = cDWORDBlack;                     // NCH pather              (Black)
const DWORD cColorPostNchs = cDWORDYellow;                     // Nch's from 1 post hull  (Yellow)
const DWORD cColorHullActualExitPnt = cDWORDCyan;              // Actual Hull Exit Point  (Cyan)
const DWORD cColorPartialBestPnt = cDWORDMagenta;              // Closest point found on a partial path (Magenta)
const DWORD cColorInteriorNch = cDWORDDarkOrange;              // Interior Nonconvex Hull (Dark Orange)
#endif


#ifndef BUILD_FINAL
const DWORD cTrackedPathColor[] = {cDWORDRed, cDWORDBlue, cDWORDYellow, cDWORDCyan};
const DWORD cTrackedPathWaypointsColor[] = {0x007F0000, 0x0000007F, 0x007F7F00, 0x00007F7F};
#endif


//==============================================================================
// BPather::BPather
//==============================================================================
BPather::BPather(void) : 
   mbInitialized(false),
   mfLimitDist(0.0f),
   mbQuadUpdate(true),
   mlBadPathThresholdLow(clBadPathThresholdLow),
   mlBadPathThresholdHigh(clBadPathThresholdHigh),
   mpNchFreeList(NULL),
   mLrpTreeLand(NULL),
   mLrpTreeFlood(NULL),
   mLrpTreeScarab(NULL), 
   mLrpTreeHover(NULL),
   mLrpTreeAir(NULL),
   mbSkipObBegin(false),
   mlPathType(cLongRange),
   mvLimitPos(cOriginVector),
   mfTileSize(0.0f),
   mfTileSizeSqr(0.0f),
   mbLRPTreeLoaded(false),
#ifdef DEBUG_TIMING_STATS
   mlLLCallsPerUpdate(0),
   mlHLCallsPerUpdate(0),
#endif
   mQueues(NULL)
{
   // The Pathing Queues
} // BPather::BPather

//==============================================================================
// BPather::init
//==============================================================================
bool BPather::init()
{
   mQueues = new BPathQueues;
   if (!mQueues)
   {
      BASSERT(0);
      return false;
   }

   BNonconvexHull::startup();

   // Allocate the NonconvexHull free list;
   if (mpNchFreeList)
      delete mpNchFreeList;

   mpNchFreeList = new BNchFreeList;
   if (!mpNchFreeList)
   {
      BASSERT(0);
      return false;
   }

   // Give ourselves some initial space in this array... It can always grow later.
   mObArray.setNumber(256);
   mNonconvexArray.setNumber(0);

   mHullArray1.setNumber(clHullArraySize);
   mHullArray2.setNumber(clHullArraySize);
   mAddPoints.setNumber(clVectorArraySize);
   mPostObs.setNumber(clInitAggregateObListSize);
   mIntersectResults.setNumber(clIntersectResultSize);

#ifndef BUILD_FINAL
   initStats();
#endif

   return true;
}  // BPather::init

//==============================================================================
// BPather::~BPather
//==============================================================================
BPather::~BPather(void)
{
   reset(true);
}

//==============================================================================
// BPather::reset
//==============================================================================
void BPather::reset(bool destruct)
{

   #ifndef BUILD_FINAL
   dumpStats();
   #endif

   mHullArray1.clear();
   mHullArray2.clear();
   mAddPoints.clear();
   mIntersectResults.clear();

   resetTrees();

   if (mQueues)
   {
      delete mQueues;
      mQueues = NULL;
   }

   if (mpNchFreeList)
   {
      delete mpNchFreeList;
      mpNchFreeList = NULL;
   }

   if (!destruct)
   {
      BLrpTree::resetStatics();
      BNonconvexHull::shutdown();
   }
   
   // jce [10/7/2008] -- adding more stuff to be cleaned up here for inter-scenario cleanliness.  This stuff 
   // will all get realloced the next scenario, however.
   mPostObs.clear();
   sCurrentList.clear();
   mTempPath.reset(true);
   mNonconvexArray.clear();
   mObArray.clear();
   mHulledArea.clear();
   mHulledArea.releaseMemory();
   mBannedUnits.clear();
   #ifndef BUILD_FINAL
      mDebugPath.reset(true);
   #endif
   

} // BPather::reset


//==============================================================================
// BPather::beginPathing
// Sets up all the parameter info, and initalizes memory storage. 
//
//==============================================================================
long BPather::beginPathing(const long lEntityID, const long lEntityType,
                           BObstructionManager *pObManager, const BEntityIDArray *pIgnoreList,
                           const float fRadius, bool bSkipObBegin, bool fullPathOnly, bool canJump, long lTargetID,
                           long pathClass)
{

#ifdef DEBUG_VTUNE
   VTResume();
#endif

   #ifdef DEBUG_LRPPATHSYNC
   syncPathingData("lEntityID", lEntityID);
   syncPathingData("fRadius", fRadius);
   #endif

   long lResult = BPather::cInitialized;

   if (!pObManager)
   {
      BASSERT(0);
      return BPather::cInvalid;
   }

   #ifdef DEBUG_BEGINEND
   debug("Begin Pathing Called..");
   debug("Entity ID: %d", lEntityID);
   if (bSkipObBegin)
      debug("bSkipObBegin = true");
   else
      debug("bSkipObBegin = false");
   #endif

   // Initialize the Parm Block those things we get from parms..
   // a batch of pathing calls.  
   mPathParms.lEntityID = lEntityID;
   mPathParms.lEntityType = lEntityType;
   mPathParms.pObManager = pObManager;
   mPathParms.pIgnoreList = pIgnoreList;
   mPathParms.fRadius = fRadius;
   mPathParms.lTargetID = lTargetID;
   mPathParms.fullPathOnly=fullPathOnly;
   mPathParms.canJump = canJump;
   if (lTargetID != -1)
   {
      BUnit *pTarget = gWorld->getUnit(lTargetID);
      mPathParms.pTarget = pTarget;
   }

   // Place logical defaults in the rest..
   mPathParms.lPlayerID = -1L;
   mPathParms.pEntity = NULL;
   mPathParms.fTargetRadius = 0.0f;
   mPathParms.lBackPath = cBackPathOn;
   mPathParms.pTarget = NULL;
   mPathParms.lPathClass = pathClass;
   mPathParms.lPathSource = cUndefinedSource;

   // Determine Path Class
   // And appropraite obstruction options based on that.

   long quadtrees=BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeAllCollidableSquads;

   // If this is a generic path request, set up the options appropriately for land based pathing.
   if (lEntityID == -1L)
   {
      if (pathClass == cLandPath || pathClass == cSquadLandPath)
         quadtrees |= BObstructionManager::cIsNewTypeBlockLandUnits;
      else if (pathClass == cFloodPath)
         quadtrees |= BObstructionManager::cIsNewTypeBlockFloodUnits;
      else if (pathClass == cScarabPath)
         quadtrees |= BObstructionManager::cIsNewTypeBlockScarabUnits;
      else if (pathClass == cAirPath)
         quadtrees = BObstructionManager::cIsNewTypeBlockAirMovement;
   }
   else if (lEntityType == BEntity::cClassTypePlatoon)
   {
      BPlatoon* pPlatoon = gWorld->getPlatoon(lEntityID);
      if (pPlatoon == NULL)
      {
         BASSERT(0);
         return BPather::cInvalid;
      }

      mPathParms.lPathSource = cPlatoonSource;
      mPathParms.pEntity = pPlatoon;
      if (pathClass == cLandPath || pathClass == cSquadLandPath)
         quadtrees |= BObstructionManager::cIsNewTypeBlockLandUnits;
      else if (pathClass == cFloodPath)
         quadtrees |= BObstructionManager::cIsNewTypeBlockFloodUnits;
      else if (pathClass == cScarabPath)
         quadtrees |= BObstructionManager::cIsNewTypeBlockScarabUnits;
      else if (pathClass == cAirPath)
         quadtrees = BObstructionManager::cIsNewTypeBlockAirMovement;

      // Determine the Player..
      mPathParms.lPlayerID = pPlatoon->getPlayerID();
   }
   else
   {
      // Get unit or unit group.
      BUnit *unit = NULL;
      long lID = -1L;

      if (lEntityType != BEntity::cClassTypeUnit)
      {
         BSquad* uG = NULL;
       
         uG = gWorld->getSquad(lEntityID);
         if (uG == NULL)
         {
            BASSERT(0);
            return BPather::cInvalid;
         }

         // Get the pathing type from the first unit in the group.  The unit grouper
         // should prevent land and water units from being grouped together.
         //const long *pGroupUnits = uG->getUnits();
         lID = uG->getChild(0).asLong();

         // Set the source..
         mPathParms.lPathSource = cUnitGroupSource;
         mPathParms.pEntity = uG;
      }
      else
      {
         lID = lEntityID;
         mPathParms.lPathSource = cUnitSource;
      }

      
      unit = gWorld->getUnit(lID);
      if(!unit || !unit->getProtoObject())
      {
         BASSERT(0);
         return BPather::cInvalid;
      }

      if (mPathParms.lPathSource == cUnitSource)
         mPathParms.pEntity = unit;

      if (unit->getProtoObject()->getMovementType() == cMovementTypeAir)
         // path things by their movement type, not obstruction type.  dlm 8/16/02
         // || unit->isCollideable() == false)
      {
         // By Air.. 
         mPathParms.lPathClass = cAirPath;

         quadtrees=BObstructionManager::cIsNewTypeBlockAirMovement;
      }
      else if (unit->getProtoObject()->getMovementType() == cMovementTypeLand)
      {
         // Land.. 
         mPathParms.lPathClass = cLandPath;

         quadtrees|=BObstructionManager::cIsNewTypeBlockLandUnits;
      }
      else if (unit->getProtoObject()->getMovementType() == cMovementTypeFlood)
      {
         // Flood.. 
         mPathParms.lPathClass = cFloodPath;

         quadtrees|=BObstructionManager::cIsNewTypeBlockFloodUnits;
      }
      else if (unit->getProtoObject()->getMovementType() == cMovementTypeScarab)
      {
         // Scarab.. 
         mPathParms.lPathClass = cScarabPath;

         quadtrees|=BObstructionManager::cIsNewTypeBlockScarabUnits;
      }
      else if (unit->getProtoObject()->getMovementType() == cMovementTypeHover)
      {
         mPathParms.lPathClass = cHoverPath;

         quadtrees|=BObstructionManager::cIsNewTypeBlockAllMovement;
      }
      // Determine the Player..
      mPathParms.lPlayerID = unit->getPlayerID();
   }

   // Unless we have the config set, pathing ignores moving units -- so zap that bit
   // out of the bitfield.
   bool pathingMoving;
#ifdef BUILD_FINAL
   pathingMoving = false;
#else
   pathingMoving = false;
#endif
   if(!pathingMoving)
   {
      quadtrees &= ~BObstructionManager::cIsNewTypeCollidableMovingUnit;
      quadtrees &= ~BObstructionManager::cIsNewTypeCollidableMovingSquad;
   
      // jce [9/25/2008] -- There really is no such thing as a collideable, moveable unit any more.  If you
      // move you are in a squad, whose obstruction is the thing we care about.   
      quadtrees &= ~BObstructionManager::cIsNewTypeCollidableStationaryUnit;
   }

   // Intialize Ob Manager
   if(!bSkipObBegin)
   {
      if ((lEntityID == -1) || (lEntityType == BEntity::cClassTypePlatoon))
      {
         mPathParms.pObManager->begin(BObstructionManager::cBeginNone, fRadius, quadtrees, BObstructionManager::cObsNodeTypeAll, 
            mPathParms.lPlayerID, cDefaultRadiusSofteningFactor, pIgnoreList, mPathParms.canJump);
      }
      else
      {
         mPathParms.pObManager->begin(BObstructionManager::cBeginEntity, lEntityID, lEntityType, quadtrees, 
            BObstructionManager::cObsNodeTypeAll, mPathParms.lPlayerID, cDefaultRadiusSofteningFactor, 
            pIgnoreList, mPathParms.canJump);
      }

      #ifdef DEBUG_BEGINEND
      debug("\tPlayer Ignores set for Player %d", mPathParms.lPlayerID);
      #endif
   }

   mbSkipObBegin = bSkipObBegin;
   if (mNonconvexArray.getNumber())
   {
      // If this is not null, it means we're being called without being ended.
      BASSERT(0);
   }
   mNonconvexArray.setNumber(0);
   mbInitialized = true;
   return lResult;

}


//==============================================================================
// BPather::beginProtoPathing
//==============================================================================
long BPather::beginProtoPathing(const long lPlayerID, const long lProtoID,
                           BObstructionManager *pObManager, const BEntityIDArray *pIgnoreList,
                           const float fRadius, bool bSkipObBegin, bool fullPathOnly, bool canJump, long lTargetID,
                           long pathClass)
{

#ifdef DEBUG_VTUNE
   VTResume();
#endif
//   perfBeginPathing();

   #ifdef DEBUG_LRPPATHSYNC
   syncPathingData("lPlayerID", lPlayerID);
   syncPathingData("lProto", lProtoID);
   syncPathingData("fRadius", fRadius);
   #endif

   long lResult = BPather::cInitialized;

   if (!pObManager)
   {
      BASSERT(0);
      return BPather::cInvalid;
   }

   #ifdef DEBUG_BEGINEND
   debug("Begin ProtoPathing Called...");
   debug("Player ID: %ld", lPlayerID);
   debug("Proto ID: %ld", lProtoID);
   if (bSkipObBegin)
      debug("bSkipObBegin = true");
   else
      debug("bSkipObBegin = false");
   #endif
      
   // Use the beginPathing function to initialize the parameter set for
   // a batch of pathing calls.  
   mPathParms.lEntityID = -1L;
   mPathParms.lEntityType = -1L;
   mPathParms.pEntity = NULL;
   mPathParms.pObManager = pObManager;
   mPathParms.pIgnoreList = pIgnoreList;
   mPathParms.fRadius = fRadius;
   mPathParms.lBackPath = cBackPathOn;
   mPathParms.lPlayerID = lPlayerID;
   mPathParms.lPathSource = cProtoUnitSource;
   mPathParms.lTargetID = lTargetID;
   mPathParms.fullPathOnly=fullPathOnly;
   mPathParms.lPathClass = pathClass;
   if (lTargetID != -1)
   {
      BUnit *pTarget = gWorld->getUnit(lTargetID);
      mPathParms.pTarget = pTarget;
   }
   else
      mPathParms.pTarget = NULL;
   
   // Determine Path Class
   // And appropraite obstruction options based on that.
   long quadtrees=BObstructionManager::cIsNewTypeAllCollidableUnits;

   // Consider squad obstructions too
   if (pathClass == cSquadLandPath)
      quadtrees |= BObstructionManager::cIsNewTypeAllCollidableSquads;

   BASSERT(lProtoID >= 0);
   BASSERT(lPlayerID >= 0);

   // Get unit.
   const BProtoObject *protoUnit=NULL;
 
   BPlayer *player = gWorld->getPlayer(lPlayerID);
   if(player)
      protoUnit = player->getProtoObject(lProtoID);
 

   if(!protoUnit)
   {
      BASSERT(0);
      return BPather::cInvalid;
   }

   if (protoUnit->getMovementType() == cMovementTypeAir)
   // Path things according to their movementtype, not according to their obstruction type.  dlm 8/16/02
      // || protoUnit->getFlag(BProtoUnit::cCollideable) == false)
   {
      mPathParms.lPathClass = cAirPath;
      quadtrees=BObstructionManager::cIsNewTypeBlockAirMovement;
   }
   else if (protoUnit->getMovementType() == cMovementTypeLand)
   {
      mPathParms.lPathClass = cLandPath;
      quadtrees|=BObstructionManager::cIsNewTypeBlockLandUnits;
   }
   else if (protoUnit->getMovementType() == cMovementTypeFlood)
   {
      mPathParms.lPathClass = cFloodPath;
      quadtrees|=BObstructionManager::cIsNewTypeBlockFloodUnits;
   }
   else if (protoUnit->getMovementType() == cMovementTypeScarab)
   {
      mPathParms.lPathClass = cScarabPath;
      quadtrees|=BObstructionManager::cIsNewTypeBlockScarabUnits;
   }
   else if (protoUnit->getMovementType() == cMovementTypeHover)
   {
      mPathParms.lPathClass = cHoverPath;
      quadtrees|=BObstructionManager::cIsNewTypeBlockAllMovement;
   }

   // Initialize Ob Manager
   if(!bSkipObBegin)
   {      
      mPathParms.pObManager->begin(BObstructionManager::cBeginProtoUnit, lPlayerID, lProtoID, quadtrees, 
         BObstructionManager::cObsNodeTypeAll, mPathParms.lPlayerID, cDefaultRadiusSofteningFactor, 
         pIgnoreList, mPathParms.canJump);

      #ifdef DEBUG_BEGINEND
      debug("\tPlayer Ignores set for Player %d", mPathParms.lPlayerID);
      #endif
   }
   
   mbSkipObBegin = bSkipObBegin;
   if (mNonconvexArray.getNumber())
   {
      // If this is not null, it means we're being called without being ended.
      BASSERT(0);
   }
   mNonconvexArray.setNumber(0);
   mbInitialized = true;
   return lResult;

}


//==============================================================================
// BPather::endPathing
//==============================================================================
void BPather::endPathing()
{
   if(!mbSkipObBegin)
   {
      mPathParms.pObManager->end();
   }

   #ifdef DEBUG_BEGINEND
   debug("End Pathing Called");
   #endif

      long lNumNchHulls = mNonconvexArray.getNumber();
   for (long l = 0; l < lNumNchHulls; l++)
   {
      mpNchFreeList->release(mNonconvexArray[l]);
   }
   mNonconvexArray.setNumber(0);

   #ifdef DEBUG_VTUNE
   VTPause();
   #endif

//   perfEndPathing();

   return;

}

//==============================================================================
// BPather::findPath
//==============================================================================
long BPather::findPath(const long entityID, const long entityType,
                       BObstructionManager *obManager, const BVector &start, 
                       const BVector &goal, const float radius, const float range, 
                       const BEntityIDArray& ignoreUnits, 
                       BPath *path, bool skipBegin, bool fullPathOnly, bool canJump, long lTargetID, long lPathType, long pathClass)
{
   //Do the begin.
   if (beginPathing(entityID, entityType, obManager, &ignoreUnits, radius, skipBegin, fullPathOnly, canJump, lTargetID, pathClass) != cInitialized)
      return(BPath::cFailed);

   long lResult=findPath(entityID, start, goal, range, path, lPathType, lTargetID);

   endPathing();

   return(lResult);
}


//==============================================================================
// BPather::findPath
//==============================================================================
long BPather::findProtoPath(const long lPlayerID, const long lProtoID,
                       BObstructionManager *obManager, const BVector &start, 
                       const BVector &goal, const float radius, const float range, 
                       const BEntityIDArray &ignoreUnits, 
                       BPath *path, bool skipBegin, bool fullPathOnly, bool canJump, long lTargetID, long lPathType)
{
   //Do the begin.
   if (beginProtoPathing(lPlayerID, lProtoID, obManager, &ignoreUnits, radius, skipBegin, fullPathOnly, canJump, lTargetID) != cInitialized)
      return(BPath::cFailed);

   // -1 for entity id, as we're not pathing an entity, but for a proto unit.
   long lResult=findPath(-1, start, goal, range, path, lPathType, lTargetID);

   endPathing();

   return(lResult);
}


//==============================================================================
// BPather::findPath
// This is a "general use" pathing call, that doesn't require any sort of
// entity ID.  You do, still though, have to give it an entity ID.
//==============================================================================
long BPather::findPath(BObstructionManager *obManager, const BVector &start, 
   const BVector &goal, const float radius, const float range, 
   const BEntityIDArray *ignoreList, BPath *path, bool skipBegin,
   bool fullPathOnly, bool canJump, long lTargetID, long lPathType, long pathClass)
{
   // Normal Begin Pathing Call, no entity information..
   if (beginPathing(-1L, -1L, obManager, ignoreList, radius, skipBegin, fullPathOnly, canJump, lTargetID, pathClass) != cInitialized)
      return(BPath::cFailed);

   // -1 for entity id, as we're just running a generic path, with a prescribed
   // radius, to a specific target.
   long lResult=findPath(-1L, start, goal, range, path, lPathType, lTargetID);

   endPathing();

   return(lResult);
}



//==============================================================================
// BPather::findPath
//==============================================================================
long BPather::findPath(long lEntityID, const BVector &vStart, const BVector &vGoal, float fRange,
                       BPath *pPath, long lPathType, long lTargetID)
{
   SCOPEDSAMPLE(BPather_findPath);

   // Sanity check
   if (!pPath)
   {
      BASSERT(0);
      return BPath::cError; 
   }
   
   // jce [4/25/2008] -- Make sure we start with a clean path.
   pPath->reset();
   // DLM - 11/10/08 - removed all these interfaces from limiter - we weren't using them anywhere
   /*
   BPathingLimiter *pathLimiter = gWorld->getPathingLimiter();
   if (pathLimiter)
   {
      pathLimiter->setPathStartTime(startTime);

      if (pathLimiter->getInGenPathFromPltn())
         pathLimiter->incTmpPathCalls();
      if (pathLimiter->getInGenPathingHints())
         pathLimiter->incTmpPathCalls();
      if (pathLimiter->getInFindMorePathSeg())
         pathLimiter->incTmpPathCallsInFindMorePathSeg();
   }
   */

   long lFullResult = BPath::cFailed;

   #ifdef DEBUG_BEGINEND
   debug("The REAL findPath is called..");
   #endif

   #ifndef BUILD_FINAL
   // See if we need to update the frame counts.
   if (gWorld->getUpdateNumber() != mReferenceFrame)
   {
      mLLAverageCallsPerUpdate.addSample(mlLLCallsPerUpdate);
      mlLLCallsPerUpdate = 0;

      mHLAverageCallsPerUpdate.addSample(mlHLCallsPerUpdate);
      mlHLCallsPerUpdate = 0;

      mReferenceFrame = gWorld->getUpdateNumber();

   }
   #endif

   // Sanity check for dead guys.   
   if(mPathParms.lEntityType == BEntity::cClassTypeUnit && (lEntityID>=0))
   {
      BUnit *unit = gWorld->getUnit(lEntityID);
      if(!unit || !unit->isAlive())
      {
         BFAIL("Dead unit pathing.");
         return(BPath::cError);
      }
   }

   if (!mbInitialized)
      return BPath::cError;

   //perfBegin(Pathing);
   
   // Assign the new EntityID and TargetID with each call.  Note, you cannot change Entity Type and/or radius
   // within a batch of pathing calls (between begin and end) as these determine the types of obstructions and
   // the sizes of obstructions used, which is hopefully the kind of stuff we'd like to cache.
   mPathParms.lEntityID = lEntityID;
   mPathParms.lTargetID = lTargetID;
   updateEntityInfo();


   long forceLRP = false;
   #ifdef DEBUG_GRAPHICAL

   if (getShowPaths() == 2 && lPathType == cShortRange)
   {
      debugClearRenderLines(BDebugPrimitives::cCategoryPatherLowLevelPather);
      debugClearRenderLines(BDebugPrimitives::cCategoryPatherHulls);
   }
   #endif

#if 0
   #ifdef BUILD_FINAL
   forceLRP = false;
   #else
   forceLRP = gConfig.isDefined(cConfigForceLongRangePaths);
   #endif

   #ifdef DEBUG_GRAPHICAL
   if (game->getShowPaths() == 3 && (lPathType == cLongRange || forceLRP))
   {
      debugClearRenderLines(BDebugPrimitives::cCategoryPatherQuadPather);
   }
   #endif
#endif

   // Don't really path if turned off.
   /*if(!game->getDoPathing())
   {
      pPath->addWaypointAtEnd(vStart);
      pPath->addWaypointAtEnd(vGoal);
      // Even if pathing is turned off, insert waypoints along the straight line
      // distance so entity movement doesn't freak out.
      if (lPathType == cLongRange)
      {
         BVector vCurrent = pPath->getWaypoint(0);
         for (long l = 1; l < pPath->getNumberWaypoints(); l++)
         {
            BVector vNext = pPath->getWaypoint(l);
            if (vCurrent.xzDistanceSqr(vNext) > cMaximumLowLevelDistSqr)
            {
               // Project a point along this vector and insert a waypoint here..
               BVector vDir = vNext - vCurrent;
               vDir.y = 0.0f;
               vDir.normalize();
               BVector vNew = vCurrent + (vDir * (cMaximumLowLevelDist - 0.1f));
               pPath->addWaypointBefore(l, vNew);
               vCurrent = vNew;            
            }
            else
               vCurrent = vNext;
         }
      }
      
      perfEnd(Pathing);
      return(BPath::cFull);
   }*/

#ifdef DEBUG_FINDPATH
   debug("----> findPath");
   debug("Entity ID: %ld", mPathParms.lEntityID);
   debug("Target ID: %ld", mPathParms.lTargetID);
   debug("Start: (%5.2f, %5.2f) Goal: (%5.2f, %5.2f)",
      vStart.x, vStart.z, vGoal.x, vGoal.z);
#endif

   // Do radius to radius check from start to goal.
   float distance = vStart.xzDistance(vGoal);
   // jce 1/16/2002 -- skip this radius stuff since the target radius is only valid at the end of the path
   // and to be consistent we'd really need to call inRange on the entities repeatedly during pathing which
   // is already done at a high level for the starting point.
   //distance -= cSqrt2*mPathParms.fTargetRadius;
   //distance -= cSqrt2*mPathParms.fRadius;
   if (distance <= fRange)
   {
#ifdef DEBUG_FINDPATH
      debug("Return Code: %d", BPath::cInRangeAtStart);
      debug("<---- Returned INRANGEATSTART from findPath (used distance <= fRange)");
#endif
//      perfEnd(Pathing);
      pPath->addWaypointAtEnd(vStart);
      pPath->addWaypointAtEnd(vGoal);

      return(BPath::cInRangeAtStart);
   }

   // See if we're a flying unit or a non-collideable thing.  If so,
   // then just hook up the path and go home. 
   if (mPathParms.lPathClass == cAirPath)
   {

      if (lPathType == cLongRange)
      {
         // Even for flying units, we still need to do the clamps on the terrain limit.
         BVector vAdjStart = vStart;
         BVector vAdjGoal = vGoal;
         
         // Get min/max tiles.
         float tileSize = gTerrainSimRep.getDataTileScale();
         float fMinX = 0.0f, fMaxX = 0.0f, fMinZ = 0.0f, fMaxZ = 0.0f;

         fMaxX = gTerrainSimRep.getNumXDataTiles() * tileSize;
         fMaxZ = gTerrainSimRep.getNumXDataTiles() * tileSize;

         if (vAdjGoal.x < fMinX)
            vAdjGoal.x = fMinX;
         if (vAdjGoal.z < fMinZ)
            vAdjGoal.z = fMinZ;
         if (vAdjGoal.x >= fMaxX)
            vAdjGoal.x = fMaxX - 0.1f;
         if (vAdjGoal.z >= fMaxZ)
            vAdjGoal.z = fMaxZ - 0.1f;

         if (vAdjStart.x < fMinX)
            vAdjStart.x = fMinX;
         if (vAdjStart.z < fMinZ)
            vAdjStart.z = fMinZ;
         if (vAdjStart.x >= fMaxX)
            vAdjStart.x = fMaxX - 0.1f;
         if (vAdjStart.z >= fMaxZ)
            vAdjStart.z = fMaxZ - 0.1f;

         pPath->addWaypointAtEnd(vAdjStart);
         BVector pathVec = vAdjGoal - vAdjStart;
         pathVec.safeNormalize();

         BVector currPoint = vAdjStart;
         float dist = currPoint.xzDistanceSqr(vAdjGoal);
         while (dist > cMaximumLowLevelDistSqr - cLowLevelRelaxFactorSqr)
         {
            currPoint += (pathVec * (cMaximumLowLevelDist - (cLowLevelRelaxFactor + 1.0f)));
            pPath->addWaypointAtEnd(currPoint);
            dist = currPoint.xzDistanceSqr(vAdjGoal);
         }
         pPath->addWaypointAtEnd(vAdjGoal);
      
//      perfEnd(Pathing);
         #ifdef DEBUG_FINDPATH
         debug("Return Code: %d", BPath::cFull);
         debug("<---- Returned FULL from findPath");
         #endif
            
         return(BPath::cFull);
      }
      else
         mlPathType = cShortRange;

   }

   // No dice.. determine the type of path to calculate, and do it.
   else if (lPathType == cUndefined)
   {
      float fDistSqr = vStart.xzDistanceSqr(vGoal);
      if (fDistSqr > cMaximumLowLevelDistSqr)
         mlPathType = cLongRange;
      else
         mlPathType = cShortRange;
   }
   else
      mlPathType = lPathType;

   if (forceLRP)
      mlPathType = cLongRange;

#ifdef DEBUG_FORCE_LOWLEVEL
   mlPathType = cShortRange;
#endif

#ifndef DEBUG_SKIP_QUICKPATH
   // if a short range path, attempt to find quick path..
   if (mlPathType == cShortRange)
   {
      long lQuickResult = findQuickPath(vStart, vGoal, fRange, pPath);

      // If we found a full path, return it.
      if(lQuickResult == BPath::cFull)
      {
         #ifdef DEBUG_FINDPATH
         debug("QuickPath found path.");
         debug("Return Code: %d", BPath::cFull);
         debug("<---- findPath");
         #endif
         
         return(BPath::cFull);
      }
   }
#endif

   
   if (mlPathType == cShortRange)
   {
      //perfBegin(PathingShort);

      #ifdef DEBUG_TIMING
      LARGE_INTEGER startTime;
      QueryPerformanceCounter(&startTime);
      #endif
      
      #ifndef _PATH_3
      lFullResult = findLowLevelPath2(vStart, vGoal, fRange, pPath);
      #else
      lFullResult = findLowLevelPath_3(vStart, vGoal, fRange, pPath);
      #endif

      #ifdef DEBUG_TIMING
      LARGE_INTEGER endTime;
      QueryPerformanceCounter(&endTime);
      int64delta=endTime.QuadPart-startTime.QuadPart;
      float elapsed = 1000.0f*(float)delta/(float)(mQPFreq.QuadPart);
      blogtrace("findLowLevelPath2: %0.2f ms", elapsed);
      #endif

     // perfEnd(PathingShort);
   }
   else
   {
      //perfBegin(PathingLong);

      lFullResult = findHighLevelPath2(vStart, vGoal, fRange, pPath);

      //perfEnd(PathingLong);
   }

   // Only do this when it's a short range path we're checking for.
   // jce [10/2/2008] -- actually going to do this for high level paths too... Hopefully it won't break anything.  I know it fixes
   // some things :)
   // Final check to make sure that the path really takes us closer to the goal than the start
   // point.  This can happen because we sometimes path to artificial goal points that are pushed off of obstructions
   // and probably other ways too :)
   if((lFullResult==BPath::cFull || lFullResult==BPath::cPartial || lFullResult==BPath::cOutsideHulledAreaPartial || lFullResult==BPath::cOutsideHulledAreaFailed) && pPath && pPath->getNumberWaypoints()>0)
   {
      float lastDistSqr=pPath->getWaypoint(pPath->getNumberWaypoints()-1).xzDistanceSqr(vGoal);
      
      // jce [9/22/2008] -- changed this to >= since a path that gets us no closer to the goal is still pretty much a loser
      if(lastDistSqr>=distance*distance)
         lFullResult=BPath::cFailed;
   }


   // Little Syncotron.
   #ifdef DEBUG_PATHSYNC
   syncPathingCode("Returning from findPath");
   syncPathingData("Return Code", lFullResult);
   syncPathingData("Number of Waypoints in Path", pPath->getNumberWaypoints());
   BVector vWaypoint(0.0f);
   for (long m = 0; m < pPath->getNumberWaypoints(); m++)
   {
      vWaypoint = pPath->getWaypoint(m);
      syncPathingData("   Waypoint", vWaypoint);
   }
   #endif
      
#ifdef DEBUG_FINDPATH
   debug("Return Code: %d", lFullResult);
   debug("<---- findPath");
#endif

//   perfEnd(Pathing);
   // DLM - 11/10/08 - we're tracking time elsewhere - removed these interfaces
   /*
   if (pathLimiter)
   {
      DWORD totalTime = pathLimiter->getTotalPathTime() + (timeGetTime() - startTime);
      pathLimiter->setTotalPathTime(totalTime);
      if (pathLimiter->getTotalPathTime() > pathLimiter->getMaxPathTime())
         pathLimiter->setMaxPathTime(pathLimiter->getTotalPathTime());
   }
   */

   return lFullResult;
}

//==============================================================================
// BPather::findQuickPath
//==============================================================================
long BPather::findQuickPath(const BVector &start, const BVector &goal, 
                            const float range, BPath *path)
{

   SCOPEDSAMPLE(BPather_findQuickPath);
   // Check for bogus parameters.
   if(!path || range<0.0f)
   {
      BASSERT(0);
      return(BPath::cError);
   }

   #ifdef DEBUG_QUICKPATH
   debug("---> QuickPath");
   debug("Start: (%f, %f)", start.x, start.z);
   debug("Goal:  (%f, %f)", goal.x, goal.z);
   #endif   

   // Check the line segment from from start to goal.
   BVector iPoint(cOriginVector);
   //BDynamicSimLongArray obstructions;
   // Clear the ObStack
   mObArray.setNumber(0);

   //long lObIdx, lSegment;
   // jce 11/21/2001 -- changed to relaxed check.
   // jce [10/9/2008] -- changing back to non relaxed for consistency with other pathing methods (and lol)

   //bool result = mPathParms.pObManager->segmentIntersects(start, goal, true, false, iPoint, lObIdx, lSegment, mlObStack);
	// MGP - switch to use single obstruction check,
	// All we want to know is that we hit something or not..
   bool result = mPathParms.pObManager->segmentIntersects(start, goal, false);


   //bool result = mPathParms.pObManager->segmentIntersects(start, goal, true, false, iPoint, lObIdx, lSegment, mlObStack);
   if(!result)
   {
      // We can get from start to finish without hitting anything.  Just fill those points in.
      path->addWaypointAtEnd(start);
      path->addWaypointAtEnd(goal);
      #ifdef DEBUG_QUICKPATH
      debug("Returning cFull with no avoidpoint");
      debug("<--- Quickpath");
      #endif
      return(BPath::cFull);
   }

   // Quick path failed.
   #ifdef DEBUG_QUICKPATH
   debug("Returning Failed");
   debug("<--- Quickpath");
   #endif
   return(BPath::cFailed);
}


//==============================================================================
// findNewPolyStart
//==============================================================================
bool BPather::findNewPolyStart(const BVector &vStart, BVector &vEscape, BDynamicSimArray<BOPObstructionNode*> &obs, 
                               long lAttempts, BVector *pvAltPoint, bool checkingStartPoint) 
{

   bool bResult = true;
   bool bComplete;
   bool bAltInside = false;
   BVector vTemp(cOriginVector);
   const BOPQuadHull *testHull = NULL;
   BNonconvexHull *pncHull;
   long segIndex = -1;
   long hulIndex =  0;
   //long lOrig;
   float fDist;

   #ifdef DEBUG_FINDNEWPOLYSTART
   debug("---> findNewPolyStart ");
   #endif


   #ifdef DEBUG_FINDNEWPOLYSTART
   debug("Gathering %d Overlapping Obs starting at vector: (%f, %f)", obs.getNumber(), vStart.x, vStart.z);
   #endif
   
   // DLM 8/18/08 - Ugh.  Hate making these kinds of tweaks.  But 3 * our pathing radius is allowing us 
   // to pass through objects.  In an imperfect world, we have to settle for imperfect solutions.
   //float maxAllowedDist = 3.0f*mPathParms.fRadius;
   // jce [9/3/2008] -- Ugh++.  Tweaking this back some based on some other cases/discussion :/
   float maxAllowedDist = 2.7f*mPathParms.fRadius;

   // Don't Overlap, even on the first attempt.  Just initialized with the array of hulls passed to us..

   // Gather Overlapping Obstructions, with Limit Checks.  Only do this on our first attempt.
   // after that, we just initialized with the hulls passed to us.. 
   // This isn't good enough.  We need to do the overlapping.  DLM 10/15/01

   if (lAttempts == 0)
   {
      obs.setNumber(1);
      
      if(checkingStartPoint)
         bComplete = findOverlappingObstructions(vStart, obs, maxAllowedDist*maxAllowedDist);
      else
         bComplete = findOverlappingObstructions(vStart, obs, mfLimitDist);
   }
   else 
      bComplete = false;

   pncHull = mpNchFreeList->acquire(false);
   if (!pncHull)
   {
      BASSERT(0);
      return false;
   }
   if (!pncHull->initialize(obs, mPathParms.pObManager))
   {
      // If we fail to initialize the hull, then return false.
      #ifdef DEBUG_INVALIDHULLPNT
      debugAddPoint("invalidhullpnt", pncHull->getPoint(0), 0xff000000);
      //gWorld->setPaused(true);
      #endif

      #ifdef DEBUG_FINDNEWPOLYSTART
      debug("\tUnable to initialize NonconvexHull.. bailing with FALSE.");
      debug("<---- findNewPolyStart");
      #endif
      
      // Release hull.
      mpNchFreeList->release(pncHull);

      return false;
   }

   #ifdef DEBUG_GRAPHNEWPOLYSTART
   debugAddNch(BDebugPrimitives::cCategoryPatherNewPolyStart, *pncHull, 0x00ffff00);
   #endif

   // See if the alt point is in the hull as well.  This would be our 'start' if we're doing
   // a goal, and vice versa.
   if (pvAltPoint && pncHull->inside(*pvAltPoint, hulIndex))
   {
      #ifdef DEBUG_FINDNEWPOLYSTART
      debug("\tThe alt point is also inside the hull.  Bailing with FALSE.");
      debug("<---- findNewPolyStart");
      #endif

      // If it *is* inside the hull we just created, try just getting off our blocking obstruction.
      testHull = mPathParms.pObManager->getExpandedHull(obs[0]);
      vEscape = testHull->findClosestPointOnHull(vStart, &segIndex, NULL);
      // Set attempts to cMaxAttemts, so we don't try to recurse into this routine.
      lAttempts = cMaxEscapeAttempts;
      bAltInside = true;
   }
   else
   {
      // Before we go any further, verify that Nonconvex hull think's were inside as well.  Because if it doesn't,
      // it's going to cause the path to fail later.
      #ifdef DEBUG_FINDNEWPOLYSTART
      debug("   Verifying if nonconvexhull thinks start is inside..");
      #endif
      if (pncHull->inside(vStart, hulIndex))
      {
         vEscape = pncHull->findClosestPointOnHull(vStart, &segIndex, &hulIndex);
         #ifdef DEBUG_FINDNEWPOLYSTART
         debug("   NCH does think we're inside.  Closest point on hull is (%5.2f, %5.2f)", vEscape.x, vEscape.z);
         #endif
      }
      else
      {
         #ifdef DEBUG_FINDNEWPOLYSTART
         debug("   NCH does NOT think we're inside.  Just using original start value and returning...", vEscape.x, vEscape.z);
         #endif
         vEscape = vStart;

         // jce [11/17/2008] -- adding missing release call to this code path
         mpNchFreeList->release(pncHull);

         return true;
      }
   }

   #ifdef DEBUG_GRAPHNEWPOLYSTART
   debugAddPoint(BDebugPrimitives::cCategoryPatherNewPolyStart, vEscape, 0xffff0000);
   #endif

   vTemp = vEscape - vStart;
   vTemp.y = 0.0f;
   fDist = vTemp.length();
   #ifdef DEBUG_FINDNEWPOLYSTART
   debug("   fDist to Closest Point on Hull is: %f", fDist);
   #endif

   if (fDist < cFloatCompareEpsilon)
   {
      #ifdef DEBUG_FINDNEWPOLYSTART
      debug("\tDistance to closest point on overlapping hull < cFloatCompareEpsilon.");
      debug("\backing off perp to segment.");
      debug("<---- findNewPolyStart");
      #endif

      // Just back off perp to segment.
      if (bAltInside && testHull != NULL)
         testHull->computePerpToSegment(segIndex, vTemp.x, vTemp.z);
      else
         pncHull->computePerpToSegment(segIndex, vTemp.x, vTemp.z, hulIndex);
   }
   // jce [6/29/2005] -- changed it so this "not too far" check occurs only when looking for a start
   // point since we don't want this to allow us to move too far through an obstruction in that case... but with
   // a goal point, it's ok wherever we find. 
   // jce [10/6/2008] -- Added additional check for point being outside of the playable bounds
   else if(checkingStartPoint && (fDist>maxAllowedDist) || (gWorld && gWorld->isOutsidePlayableBounds(vEscape, true)))
   {
      #ifdef DEBUG_FINDNEWPOLYSTART
      debug("\tClosest point is too far.  Bailing.");
      #endif

      // Release hull.
      mpNchFreeList->release(pncHull);

      return(false);
   }
   else
      vTemp.normalize();

   vTemp *= (fDist + 0.1f);
   vEscape = vStart + vTemp;

   if (bComplete)
   {
      #ifdef DEBUG_FINDNEWPOLYSTART
      debug("\tGot closest point on a complete hull.  Done.");
      #endif
      bResult = true;
   }
   else
   {
      BDynamicSimArray<BOPObstructionNode*> newObs;
      mPathParms.pObManager->findObstructions(vEscape, false, false, newObs);
      
      // Assume we're good.
      bResult=true;
      
      // For each new obstruction, see if it was in the list we used to build the hull.
      // If it's not then we've got a problem.  If so, then it should theoretically be a
      // float precision thing.
      for(long i=0; i<newObs.getNumber(); i++)
      {
         if(!obs.contains(newObs[i]))
         {
            bResult = false;
            break;
         }
      }

      /*
      // See if our point was unobstructed.  If so, we succeeded.  If not, we didn't.
      // Use *relaxed* obstructions, as we generated our point with *expanded* obstructions,
      // so hopefully we'll be outside.
      lOrig = obs.getNumber();
      mPathParms.pObManager->findObstructions(vEscape, false, true, obs);
      if (obs.getNumber() == lOrig)
      {
         #ifdef DEBUG_FINDNEWPOLYSTART
         debug("\tClosest point on incomplete hull is clear.  Done.");
         #endif
         bResult = true;
      }
      else
      {
         // Add this obstruction to our list, and try again, if we have retries left.
         ++lAttempts;
         if (lAttempts >= cMaxEscapeAttempts)
         {
            #ifdef DEBUG_FINDNEWPOLYSTART
            debug("\tClosest point on incomplete hull is blocked.  Out of retries.");
            #endif
            bResult = false;
         }
         else
         {
            #ifdef DEBUG_FINDNEWPOLYSTART
            debug("\tClosest point on incomplete hull is blocked.  Retry Attempt %d.", lAttempts);
            #endif
            bResult = findNewPolyStart(vStart, vEscape, obs, lAttempts, pvAltPoint, checkingStartPoint);
         }
      }
      */
   }

   #ifdef DEBUG_FINDNEWPOLYSTART
   debug("Returned with %s", (bResult)?"TRUE":"FALSE");
   debug("Final Escape Vector found: %f,%f", vEscape.x, vEscape.z);
   debug("<---- findNewPolyStart");
   #endif

   // Release hull.
   mpNchFreeList->release(pncHull);

   return bResult;
        
}

#ifdef DEBUG_USENEWPOLYSTART
//==============================================================================
// findValidStart
//==============================================================================
bool BPather::findUnobstructedPoint(const BVector &vOrig, const BVector &vGoal, BObstructionNodePtrArray &alObs,
                                     BVector &vEscape)
{

   #ifdef DEBUG_GRAPH_UNOBSTRUCTEDPOINTS
   debugClearRenderLines("unobstructedpoints");
   #endif
   
   #ifdef DEBUG_GRAPH_UNOBSTRUCTEDPOINTS
   debugAddPoint("unobstructedpoints", vOrig, cColorPurple);
   #endif

   
   // Ok, we are on top on an obstruction (or obstructions).  We need to try to separate ourselves
   // before doing anything else.
   bool bEscapesFound = false;

   BVector point2, iPoint;
   float distSqr;
   static const long cEscapesAllowed=4;
   BVector vProjPoint;
   float fBestCost = cMaximumFloat;

   for( long h=0; h<alObs.getNumber(); h++)
   {
      // Get the expanded hull associated with the first obstructing unit.
      const BOPQuadHull *hullPtr = mPathParms.pObManager->getExpandedHull(alObs[h]);
      if(!hullPtr)
      {
         BASSERT(0);
         return(0);
      }

      #ifdef DEBUG_GRAPH_UNOBSTRUCTEDPOINTS
      debugAddHull("unobstructedpoints", hullPtr, cColorCyan);
      #endif

      for(long i=0; i<cNumEscapeDirections /*&& escapesFound<cEscapesAllowed*/; i++)
      {
         bool result=hullPtr->rayIntersects(vOrig, cDirections[i], iPoint, NULL, &distSqr);
         // If we didn't hit carry on around
         if(!result)
         {
            continue;
         }

         // If we are moving too far through the obstruction, skip
         // this point.      
         if((float)sqrt((double)distSqr)>3.0f*mPathParms.fRadius)
            continue;
         

         // Extend iPoint out past obstruction.
         point2 = iPoint+mPathParms.fRadius*cDirections[i];

         float fCost = 0.0f;
         float fDist1 = 0.0f, fDist2 = 0.0f;
         // Check the new point to see if it is on any obstructions.
         static BObstructionNodePtrArray testObs;
         mPathParms.pObManager->findObstructions(point2, false, false, testObs);
         if(testObs.getNumber() == 0)

         {
            #ifdef DEBUG_GRAPH_UNOBSTRUCTEDPOINTS
            debugAddPoint("unobstructedpoints", point2, cColorGreen);
            #endif
            // Compute the cost of this point, and return the point with the best cost.
            fDist1 = vOrig.xzDistanceSqr(point2);
            fDist2 = point2.xzDistanceSqr(vGoal);
            fCost = fDist1 + fDist2;
            if (fCost < fBestCost)
            {
               fBestCost = fCost;
               vEscape = point2;
               bEscapesFound = true;
            }
         }
         #ifdef DEBUG_GRAPH_UNOBSTRUCTEDPOINTS
         else
         {
            const BOPQuadHull *pHull = mPathParms.pObManager->getExpandedHull(testObs[0]);
            debugAddHull("unobstructedpoints", pHull, cColorYellow);
            debugAddPoint("unobstructedpoints", point2, cColorRed);
         }
         #endif

      }
   }
   return bEscapesFound;
}
#endif

//==============================================================================
// findOverlappingHulls
// Right now, we have a bonehead check to make sure we don't extend *too* 
// far in any one direction.  There's got to be a better heuristic we can
// use to get reasonable numbers of hulls.
// return true if we found *all* of the overlapping obstructions,
// false if we had to bail early.
//==============================================================================
bool BPather::findOverlappingObstructions(const BVector &vTestPoint, BDynamicSimArray<BOPObstructionNode*> &obs, float limitDistSqr)
{

   bool bRetVal = true;
   // New Non-recursive method.
   // Note:  We assume that alObs has one item in it, that being the
   // obstruction ID that we passed in. 
   long lExisting = 0;
   long lNew = 0;
   BVector vCenter;
   for (;;)
   {
      lNew = obs.getNumber();
      if (lNew == lExisting)
         break;

      for (long l = lExisting; l < lNew; l++)
      {
         const BOPQuadHull *testHull = mPathParms.pObManager->getExpandedHull(obs[l]);
         if(!testHull)
         {
            BASSERT(0);
            continue;
         }

         float distSqr=0.0f;
         testHull->findClosestPointOnHull(vTestPoint, NULL, &distSqr);
         if (distSqr > limitDistSqr)
            bRetVal = false;
         else
            mPathParms.pObManager->findObstructionsQuadHull(testHull, false, true, obs);
      }
      lExisting = lNew;
   }

   return bRetVal;
}


//==============================================================================
// BLowLevelPather::assemblePath
//==============================================================================
void BPather::assemblePath(const BPath *source, BPath *dest, long lPathResult)
{
   // Check for bogus params.
   if(!source || !dest || source->getNumberWaypoints() < 2)
   {
      BASSERT(0);
      return;
   }

   #ifdef DEBUG_ASSEMBLEPATH
   blog("Assemble path ----------------------------------------------------");
   #endif

   long numSourcePoints = source->getNumberWaypoints();

#ifdef DEBUG_NOBACKPATH
   long lBackSave = mPathParms.lBackPath;
   mPathParms.lBackPath = cBackPathOff;
#endif
   if (mPathParms.lBackPath == cBackPathOff || (mPathParms.lBackPath == cBackPathOnlyOnFull && lPathResult != BPath::cFull))
   {
      // Just copy the points backwards into the path.. 
      for (long l = 0; l < numSourcePoints; l++)
         dest->addWaypointAtStart(source->getWaypoint(l));
#ifdef DEBUG_NOBACKPATH
      mPathParms.lBackPath = lBackSave;
#endif
      return;
   }


   // Add start point to dest path.  Note that the source path is a BACKWARDS path,
   // so we are reversing its order as we go.
   long lastPointAdded = 0;
   dest->addWaypointAtStart(source->getWaypoint(lastPointAdded));

   #ifdef DEBUG_ASSEMBLEPATH
   blog("Adding starting index %d (%0.4f, %0.4f, %0.4f)", lastPointAdded, source->getWaypoint(lastPointAdded).x,
      source->getWaypoint(lastPointAdded).y, source->getWaypoint(lastPointAdded).z);
   #endif
   
   bool bDone = false;
   while (!bDone)
   {
      // Go through each of the source points.
      long i;
      for(i=numSourcePoints-1; i > lastPointAdded; i--)
      {
         #ifdef DEBUG_ASSEMBLEPATH
         blog("Checking %d to %d  -- (%0.4f, %0.4f, %0.4f) to (%0.4f, %0.4f, %0.4f)", lastPointAdded, i, 
            source->getWaypoint(lastPointAdded).x, source->getWaypoint(lastPointAdded).y, source->getWaypoint(lastPointAdded).z,
            source->getWaypoint(i).x, source->getWaypoint(i).y, source->getWaypoint(i).z);
         #endif

         // Check segment from last point added to this point.
         bool result = mPathParms.pObManager->segmentIntersects(source->getWaypoint(lastPointAdded), source->getWaypoint(i), true);
         //bool result=segIntersects(source->getWaypoint(lastPointAdded), source->getWaypoint(i), -0.01f);
         //bool result=segIntersects(source->getWaypoint(lastPointAdded), source->getWaypoint(i));

         // If we didn't hit anything, then we're done with this loop.
         if(!result)
         {
            #ifdef DEBUG_ASSEMBLEPATH
            blog("   Hit no obstructions.");
            #endif
            if (i == numSourcePoints-1)
               bDone = true;
            break;
         }
      }
      if (i == lastPointAdded)
      {
         lastPointAdded++;
      }
      else
         lastPointAdded = i;

      #ifdef DEBUG_ASSEMBLEPATH
      blog("Adding index %d (%0.4f, %0.4f, %0.4f)", lastPointAdded, source->getWaypoint(lastPointAdded).x,
         source->getWaypoint(lastPointAdded).y, source->getWaypoint(lastPointAdded).z);
      #endif
   
      dest->addWaypointAtStart(source->getWaypoint(lastPointAdded));
      if (lastPointAdded == numSourcePoints - 1)
         bDone = true;
   }

   #ifdef DEBUG_ASSEMBLEPATH
   blog("assemblePath (end) --------------------------------------------------");
   #endif

}


//==============================================================================
// BPather::findHighLevelPath2
//==============================================================================
long BPather::findHighLevelPath2(const BVector &start, 
   const BVector &goal, const float range, BPath *path)
{

   SCOPEDSAMPLE(BPather_findHighLevelPath2);
   long lResult = BPath::cFailed;
   path;

   #ifdef DEBUG_FINDHIGHLEVELPATH2
   debug("----> findHighLevelPath2");
   #endif


   #ifndef BUILD_FINAL
   BTimer localTimer;
   localTimer.start();
   mlHLCallsPerUpdate++;
   #endif

   // For long range pathing, we also ignore moveable objects, unless 
   // our client is doing some special mojo..
   // jce [4/19/2005] -- Save off old settings so we can put  
   long oldQuadtrees = mPathParms.pObManager->getTreesToScan();
   if (!mbSkipObBegin)
   {
      long quadtrees = oldQuadtrees & ~(BObstructionManager::cIsNewTypeCollidableMovingUnit | BObstructionManager::cIsNewTypeCollidableStationaryUnit | BObstructionManager::cIsNewTypeCollidableMovingSquad);
      mPathParms.pObManager->setTreesToScan(quadtrees);
   }

   BVector vNewStart = start;
   BVector vNewGoal = goal;

   //bool bResult = true;
   //bool bChanged = false;


   BLrpTree *pPathTree = NULL;

   if ((mPathParms.lPathClass == cLandPath) || (mPathParms.lPathClass == cSquadLandPath))
      pPathTree = mLrpTreeLand;
   else if (mPathParms.lPathClass == cFloodPath)
      pPathTree = mLrpTreeFlood;
   else if (mPathParms.lPathClass == cScarabPath)
      pPathTree = mLrpTreeScarab;
   else if (mPathParms.lPathClass == cHoverPath)
      pPathTree = mLrpTreeHover;

   if (pPathTree == NULL)
   {
      // jce [4/19/2005] -- Restore quad settings we stepped on. 
      mPathParms.pObManager->setTreesToScan(oldQuadtrees);
      #ifndef BUILD_FINAL
      localTimer.stop();
      long elapsed = localTimer.getElapsedMilliseconds();
      mHLAverageTimePerCall.addSample((float)elapsed);
      #endif
      return lResult;
   }

   if (!pPathTree->isInitialized())
   {
      // jce [4/19/2005] -- Restore quad settings we stepped on. 
      mPathParms.pObManager->setTreesToScan(oldQuadtrees);
      #ifndef BUILD_FINAL
      localTimer.stop();
      long elapsed = localTimer.getElapsedMilliseconds();
      mHLAverageTimePerCall.addSample((float)elapsed);
      #endif
      return lResult;      
   }
   
   #ifdef DEBUG_FINDHIGHLEVELPATH2
   debug("\tExecuting WaterPath...");
   #endif
   lResult = pPathTree->findPath(mPathParms.lEntityID, vNewStart, vNewGoal, range, path,
      mPathParms.lPlayerID, mPathParms.fRadius, mPathParms.fTargetRadius, mPathParms.lBackPath, mPathParms.canJump);

   #ifndef BUILD_FINAL
   mlHLIterations = pPathTree->getIterations();
   if (mlHLIterations > mlHLIterationMax)
      mlHLIterationMax = mlHLIterations;
   mlHLIterationSum += mlHLIterations;
   #endif
   

   #ifdef DEBUG_FINDHIGHLEVELPATH2
   debug("\tfindPath returned: %d", lResult);
   #endif

   #ifdef DEBUG_FINDHIGHLEVELPATH2
   debug("\tReturning with: %d", lResult);
   debug("<---- findHighLevelPath");
   #endif

   // jce [4/19/2005] -- Restore quad settings we stepped on. 
   mPathParms.pObManager->setTreesToScan(oldQuadtrees);

   #ifndef BUILD_FINAL
   localTimer.stop();
   long elapsed = localTimer.getElapsedMilliseconds();
   mHLAverageTimePerCall.addSample((float)elapsed);
   #endif

   return lResult;
}

//==============================================================================
// BPather::buildPathingQuads
//==============================================================================
void BPather::buildPathingQuads(BObstructionManager *pObManager)
{
   SCOPEDSAMPLE(BPather_buildPathingQuads)

   // Why are you building pathing quads without an obstruction manager??
   if (!pObManager)
   {
      BASSERT(0);
      return;
   }

   #ifndef BUILD_FINAL
//   DWORD startTime=timeGetTime();
   #endif

   long lLandOptions = BObstructionManager::cIsNewTypeBlockLandUnits | 
                       BObstructionManager::cIsNewTypeSolidNonMoveableUnits |
                       BObstructionManager::cIsNewTypeBlockAirMovement;

   long lFloodOptions = BObstructionManager::cIsNewTypeBlockFloodUnits | 
                        BObstructionManager::cIsNewTypeSolidNonMoveableUnits |
                        BObstructionManager::cIsNewTypeBlockAirMovement;

   long lScarabOptions = BObstructionManager::cIsNewTypeBlockScarabUnits | 
                         BObstructionManager::cIsNewTypeSolidNonMoveableUnits |
                         BObstructionManager::cIsNewTypeBlockAirMovement;

   // We no longer call buildPermOb's here.  This is done at the 
   // time the obstruction manager is created, in BWorld::creatObstructionManager.
   // All we do now is just use the 
   // Get Terrain Info
   // Look up terrain
/*   BTerrainBase *terrain = gWorld->getTerrain();
   if(!terrain)
   {
      BASSERT(0);
      return;
   }*/
   
   
   long lTerrainXTiles = gTerrainSimRep.getNumXDataTiles();
    long lTerrainZTiles = gTerrainSimRep.getNumXDataTiles();

   // We keep Terrain Tile Size around for some limit checks in the quad pather..
   mfTileSize = gTerrainSimRep.getDataTileScale();
   mfTileSizeSqr = mfTileSize * mfTileSize;

   float fMaxX = lTerrainXTiles * mfTileSize;
   float fMaxZ = lTerrainZTiles * mfTileSize;


   // Three quarter size quad's for now..
//   float fMinSize = mfTileSize * 0.75f;
   float fMinSize = mfTileSize * cfMinQuadFactor;

   // Get the obmgr going in "land" mode.  
   // dlm 5/8/02 -- Use half the min quad size as the expansion radius for obMgr checks.
   #ifdef DEBUG_UPDATEBASEQUAD3
   pObManager->begin(BObstructionManager::cBeginNone, fMinSize * 0.5f, lLandOptions, BObstructionManager::cObsNodeTypeAll & ~BObstructionManager::cObsNodeTypeEdgeofMap, 0, 
      0.0f, NULL, mPathParms.canJump);
   #else
   pObManager->begin(BObstructionManager::cBeginNone, 0.0f, lLandOptions, BObstructionManager::cObsNodeTypeAll & ~BObstructionManager::cObsNodeTypeEdgeofMap, 0, 
      0.0f, NULL);
   #endif

   resetTrees();
      
   //long memAtStart = gMemoryManager.getCurrentAllocationSize();
   
   if (mLrpTreeLand)
      delete mLrpTreeLand;

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

   if (!gConfig.isDefined(cConfigAlpha))
   {
      if (mLrpTreeFlood)
         delete mLrpTreeFlood;

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

      if (mLrpTreeScarab)
         delete mLrpTreeScarab;

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
   }

   //long memWhenDone = gMemoryManager.getCurrentAllocationSize();
   //long bytes = memWhenDone-memAtStart;

   pObManager->end();

}

//==============================================================================
// BPather::resetTrees
//==============================================================================
void BPather::resetTrees(void)
{
   if (mLrpTreeLand)
   {
      delete mLrpTreeLand;
      mLrpTreeLand = NULL;
   }
   
   if (mLrpTreeFlood)
   {
      delete mLrpTreeFlood;
      mLrpTreeFlood = NULL;
   }
   
   if (mLrpTreeScarab)
   {
      delete mLrpTreeScarab;
      mLrpTreeScarab = NULL;
   }

   if (mLrpTreeHover)
   {
      delete mLrpTreeHover;
      mLrpTreeHover = NULL;
   }

   if (mLrpTreeAir)
   {
      delete mLrpTreeAir;
      mLrpTreeAir = NULL;
   }
}

//==============================================================================
// BPather::updatePathingQuad
//==============================================================================
void BPather::updatePathingQuad(BObstructionManager *pObManager, BOPObstructionNode* theNode, bool AddNode)
{
   SCOPEDSAMPLE(BPather_updatePathingQuad);

   // Is updating currently enabled?
   if (!mbQuadUpdate /*|| gWorld->getPatherUpdating() == false*/)
      return;

   if (gConfig.isDefined(cConfigAlpha) && !mLrpTreeLand)
      return;
   else if (!(mLrpTreeLand && mLrpTreeFlood && mLrpTreeScarab))
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
#if 0
#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigPathingNoWaterUpdate))
      count = 1;
#endif
#endif

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
         pathOptions|=(BObstructionManager::cIsNewTypeSolidNonMoveableUnits| BObstructionManager::cIsNewTypeBlockAllMovement | BObstructionManager::cIsNewTypeBlockAirMovement);

      #ifdef DEBUG_UPDATEBASEQUAD3
      if (bInUse)
      {
		   newState = saveState;
		   newState.mSQuadTreestoScan = pathOptions;
		   newState.mSValidNodeTypes = BObstructionManager::cObsNodeTypeAllSolid & ~BObstructionManager::cObsNodeTypeEdgeofMap;
         newState.mSRadius = pPathTrees[idx]->getCellSize() * 0.5f;
		   pObManager->setSessionState(newState);
	   }
	   else
		   pObManager->begin(BObstructionManager::cBeginNone, pPathTrees[idx]->getCellSize() * 0.5f, pathOptions, BObstructionManager::cObsNodeTypeAllSolid & ~BObstructionManager::cObsNodeTypeEdgeofMap, theNode->mPlayerID, 0.0f, NULL, mPathParms.canJump);
      #else
      if (bInUse)
	   {
		   newState = saveState;
		   newState.mSQuadTreestoScan = pathOptions;
		   newState.mSValidNodeTypes = BObstructionManager::cObsNodeTypeAllSolid & ~BObstructionManager::cObsNodeTypeEdgeofMap;
		   pObManager->setSessionState(newState);
	   }
	   else
		   pObManager->begin(BObstructionManager::cBeginNone, 0.0f, pathOptions, BObstructionManager::cObsNodeTypeAllSolid & ~BObstructionManager::cObsNodeTypeEdgeofMap, 0, 0.0f, NULL);
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

   if (gConfig.isDefined(cConfigAlpha) && !mLrpTreeLand)
      return;
   else if (!(mLrpTreeLand && mLrpTreeFlood && mLrpTreeScarab))
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
#if 0
#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigPathingNoWaterUpdate))
      count = 1;
#endif
#endif

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
		   newState.mSValidNodeTypes = BObstructionManager::cObsNodeTypeAllSolid & ~BObstructionManager::cObsNodeTypeEdgeofMap;
         #ifdef DEBUG_UPDATEBASEQUAD3
         newState.mSRadius = pPathTrees[idx]->getCellSize() * 0.5f;
         #endif
		   pObManager->setSessionState(newState);
	   }
	   else
      {
         #ifdef DEBUG_UPDATEBASEQUAD3
		   pObManager->begin(BObstructionManager::cBeginNone, pPathTrees[idx]->getCellSize() * 0.5f, pathOptions, BObstructionManager::cObsNodeTypeAllSolid & ~BObstructionManager::cObsNodeTypeEdgeofMap, 0, 0.0f, NULL, mPathParms.canJump);
         #else
		   pObManager->begin(BObstructionManager::cBeginNone, 0.0f, pathOptions, BObstructionManager::cObsNodeTypeAllSolid & ~BObstructionManager::cObsNodeTypeEdgeofMap, 0, 0.0f, NULL);
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
// BPather::render
//==============================================================================
void BPather::render()
{

   // LRP tree visualization
   #ifndef BUILD_FINAL
   if(gGame.getRenderPathingQuad())
   {
      if (gConfig.isDefined(cConfigAlpha) && !mLrpTreeLand)
         return;
      else if (!(mLrpTreeLand && mLrpTreeFlood && mLrpTreeScarab))
         return;

      /*if (gConfig.isDefined(cConfigPathingRenderWater) && mLrpTreeFlood)
         mLrpTreeFlood->render();
      else*/
      switch (gGame.getRenderLRPTreeType())
      {
         case 0:
            mLrpTreeLand->render();
            break;
         case 1:
            mLrpTreeScarab->render();
            break;
         case 2:
            mLrpTreeFlood->render();
            break;
         default:
            mLrpTreeLand->render();
            break;
      }
//      mLrpTreeFlood->render();
//      mLrpTreeScarab->render();
   }
   if (gGame.getShowPaths())
   {
      // High Level Paths.. 
      if (gGame.getShowPaths() == 2)
         renderDebugLowLevelPath();
      if (gGame.getShowPaths() == 3)
         mLrpTreeLand->renderDebugPath();
   }
   #endif

}

#ifndef BUILD_FINAL
//==============================================================================
// BPather::renderDebugLowLevelPath
//==============================================================================
void BPather::renderDebugLowLevelPath()
{
   if (!mbInitialized)
      return;
   
   // Post Hulls in Blue.
   if (mPostHulls != NULL)
   {
      if (mPostHulls->getNumber() != 0)
      {
         for (long n = 0; n < mPostHulls->getNumber(); n++)
         {
            BNonconvexHull *pNch = &(*mPostHulls)[n];
            if (!pNch)
               continue;
            float fHeight = 2.0f;
            pNch->addDebugLines(BDebugPrimitives::cCategoryPathing, cDWORDBlue, &fHeight);
         }
      }
   }
   
   gTerrainSimRep.addDebugPointOverTerrain(mDebugPath.getWaypoint(0), 2.0f, cDWORDGreen, 2.0f, BDebugPrimitives::cCategoryPathing);
   for (long n = 0; n < mDebugPath.getNumberWaypoints() - 1; n++)
   {
      gTerrainSimRep.addDebugLineOverTerrain(mDebugPath.getWaypoint(n), mDebugPath.getWaypoint(n+1), cDWORDPurple, cDWORDPurple, 1.0f, BDebugPrimitives::cCategoryPathing);
      DWORD color = cDWORDPurple;
      if (n == mDebugPath.getNumberWaypoints()-1 )
         color = cDWORDRed;
      gTerrainSimRep.addDebugPointOverTerrain(mDebugPath.getWaypoint(n+1), 2.0f, color, 2.0f, BDebugPrimitives::cCategoryPathing);
   }
   return;
}
#endif

//==============================================================================
// BPather::debug
//==============================================================================
void BPather::debug(char* v, ... ) const
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

   if (lSpecificSquad == -1 || ((lSpecificSquad != -1) && (lSpecificSquad == mPathParms.lEntityID)))
      bMatch = true;

   if (!bMatch)
      return;
   
   static char out[BLogManager::cMaxLogTextLength];
   va_list va;
   va_start(va, v);
   bvsnprintf(out, sizeof(out), v, va);
   static char out2[BLogManager::cMaxLogTextLength*2];
   bsnprintf(out2, sizeof(out2), "ENTITY #%5d: %s", mPathParms.lEntityID, out);
   gConsole.output(cChannelSim, out2);
}

#ifndef BUILD_FINAL
//==============================================================================
// BPather::debugAddPoint
//==============================================================================
void BPather::debugAddPoint(int category, const BVector &start, DWORD color)
{
   // Key of of specific unit if need be
   long lSpecificSquad = -1;
   gConfig.get(cConfigDebugSpecificSquad, &lSpecificSquad);

   bool bMatch = false;

   if (lSpecificSquad == -1 || ((lSpecificSquad != -1) && (lSpecificSquad == mPathParms.lEntityID)))
      bMatch = true;

   if (!bMatch)
      return;
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
   gTerrainSimRep.addDebugLineOverTerrain(p1, p2, color, color, fDepth, category, 1000);
   p1.z = start.z + 0.25f;
   p2.z = start.z - 0.25f;
   gTerrainSimRep.addDebugLineOverTerrain(p1, p2, color, color, fDepth, category, 1000);
}

//==============================================================================
// BPather::debugAddHull
//==============================================================================
void BPather::debugAddHull(int category, const BConvexHull &hull, DWORD color)
{   
   // Key of of specific unit if need be
   long lSpecificSquad = -1;
   gConfig.get(cConfigDebugSpecificSquad, &lSpecificSquad);

   bool bMatch = false;

   if (lSpecificSquad == -1 || ((lSpecificSquad != -1) && (lSpecificSquad == mPathParms.lEntityID)))
      bMatch = true;

   if (!bMatch)
      return;

   static float fDepth = 0.2f;
   fDepth += 0.2f;
   if (fDepth > 1.4f)
      fDepth = 0.2f;
   long lIdx1 = hull.getPointCount() - 1;

   BVector vP1(cOriginVector);
   BVector vP2(cOriginVector);
   for (long lIdx2 = 0; lIdx2 < hull.getPointCount(); lIdx2++)
   {
      vP1 = hull.getPoint(lIdx1);
      vP2 = hull.getPoint(lIdx2);
      gTerrainSimRep.addDebugLineOverTerrain(vP1, vP2, color, color, fDepth, category, 10000);
      lIdx1 = lIdx2;
   }
}

//==============================================================================
// BPather::debugAddHull
//==============================================================================
void BPather::debugAddHull(int category, const BOPQuadHull *pHull, DWORD color)
{   
   // Key of of specific unit if need be
   long lSpecificSquad = -1;
   gConfig.get(cConfigDebugSpecificSquad, &lSpecificSquad);

   bool bMatch = false;

   if (lSpecificSquad == -1 || ((lSpecificSquad != -1) && (lSpecificSquad == mPathParms.lEntityID)))
      bMatch = true;

   if (!bMatch)
      return;

   static float fDepth = 0.2f;
   fDepth += 0.2f;
   if (fDepth > 1.4f)
      fDepth = 0.2f;
   long lIdx1 = 3;

   BVector vP1(cOriginVector);
   BVector vP2(cOriginVector);
   for (long lIdx2 = 0; lIdx2 < 4; lIdx2++)
   {
      vP1.x = pHull->mPoint[lIdx1].mX;
      vP1.z = pHull->mPoint[lIdx1].mZ;
      vP2.x = pHull->mPoint[lIdx2].mX;
      vP2.z = pHull->mPoint[lIdx2].mZ;
      gTerrainSimRep.addDebugLineOverTerrain(vP1, vP2, color, color, fDepth, category, 10000);
      lIdx1 = lIdx2;
   }
}

//==============================================================================
// BPather::debugAddNch
//==============================================================================
void BPather::debugAddNch(int category, const BNonconvexHull &hull, DWORD color, long group)
{   

   // Key of of specific unit if need be
   long lSpecificSquad = -1;
   gConfig.get(cConfigDebugSpecificSquad, &lSpecificSquad);
 
   bool bMatch = false;

   if (lSpecificSquad == -1 || ((lSpecificSquad != -1) && (lSpecificSquad == mPathParms.lEntityID)))
      bMatch = true;

   if (!bMatch)
      return;

   static float fDepth = 0.2f;
   fDepth += 0.2f;
   if (fDepth > 1.4f)
      fDepth = 0.2f;
   long lHullCount = hull.getHullCount();
   for (long n = 0; n < lHullCount; n++)
   {
      long lIdx1 = hull.getPointCount(n) - 1;

      BVector vP1(cOriginVector);
      BVector vP2(cOriginVector);
      DWORD dwColor;
      if (hull.getHole(n))
         dwColor = cColorInteriorNch;
      else
         dwColor = cColorPostNchs;
      for (long lIdx2 = 0; lIdx2 < hull.getPointCount(n); lIdx2++)
      {
         vP1 = hull.getPoint(lIdx1,n);
         vP2 = hull.getPoint(lIdx2,n);
         gTerrainSimRep.addDebugLineOverTerrain(vP1, vP2, dwColor, dwColor, fDepth, category, 10000);
         lIdx1 = lIdx2;
      }
   }
}

//==============================================================================
// BPather::debugAddHull
//==============================================================================
void BPather::debugClearRenderLines(int category)
{
   gpDebugPrimitives->clear(category);

   return;
}
#endif


#ifndef BUILD_FINAL
//==============================================================================
// BPather::initStats()
//==============================================================================
void BPather::initStats()
{
   // DLM 3/11/08 - Updated
   mlLLCallsPerUpdate = 0;
   mlHLCallsPerUpdate = 0;
   mLLAverageCallsPerUpdate.set(255);
   mLLAverageTimePerCall.set(255);
   mHLAverageCallsPerUpdate.set(255);
   mHLAverageTimePerCall.set(255);
   mHullAreaAverageTimePerCall.set(255);
   // End of stuff I've updated

   mStartStas = 100; // ignore first 100

   mlHLIterations = 0L;
   mlHLIterationSum = 0L;
   mlHLIterationMax = 0L;

   mLLFullCalls = 0;
   mLLExpansions = 0;

   mlPolyCalls = 0L;
   mfPolySum = 0.0f;
   mfPolyMax = 0.0f;
   mlPolyNewIntersects = 0L;
   mlPolyIterations = 0L;
   mlPolyIterationMax = 0L;
   mlPolyStartCalls = 0L;
   mlPolyEndCalls = 0L;

   mlNCCalls = 0L;
   mlNCFails = 0L;
   mfNCSum = 0.0f;
   mfNCMax = 0.0f;
   mlNCMaxObs = 0L;
   mlNCObsAtMax = 0L;

   mlOLCalls = 0L;
   mfOLSum = 0.0f;
   mfOLMax = 0.0f;

   mlPHCalls = 0L;
   mfPHSum = 0.0f;
   mfPHMax = 0.0f;

   if (mLrpTreeLand)
      mLrpTreeLand->initStats();
   if (mLrpTreeFlood)
      mLrpTreeFlood->initStats();
   if (mLrpTreeScarab)
      mLrpTreeScarab->initStats();         
   
   return;
   
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// BPather::dumpStats()
//==============================================================================
void BPather::dumpStats()
{
   gConsole.output(cChannelSim, "===============================================");
   gConsole.output(cChannelSim, "Pathing Stats:");
   gConsole.output(cChannelSim, "Short Range Pather:");
   gConsole.output(cChannelSim, "Avg Calls Per Update:         %8.3f", mLLAverageCallsPerUpdate.getAverage());
   gConsole.output(cChannelSim, "Max Calls Per Update:         %8.3f", mLLAverageCallsPerUpdate.getMaximum());
   gConsole.output(cChannelSim, "Avg Time of Call:             %8.3f", mLLAverageTimePerCall.getAverage());
   gConsole.output(cChannelSim, "Max Time of Call:             %8.3f", mLLAverageTimePerCall.getMaximum());
   gConsole.output(cChannelSim, "Total Number of Calls:        %8.3f", mLLAverageTimePerCall.getNumSamples());
   gConsole.output(cChannelSim, ".");
   gConsole.output(cChannelSim, "Calls to HullArea:            %8.3d", mHullAreaAverageTimePerCall.getNumSamples());
   gConsole.output(cChannelSim, "Avg Time of Call:             %8.3f", mHullAreaAverageTimePerCall.getAverage());
   gConsole.output(cChannelSim, "Max Time of Call:             %8.3f", mHullAreaAverageTimePerCall.getMaximum());
   gConsole.output(cChannelSim, ".");
   gConsole.output(cChannelSim, "Long Range Pather:");
   gConsole.output(cChannelSim, "Avg Calls Per Update:         %8.3f", mHLAverageCallsPerUpdate.getAverage());
   gConsole.output(cChannelSim, "Max Calls Per Update:         %8.3f", mHLAverageCallsPerUpdate.getMaximum());
   gConsole.output(cChannelSim, "Avg Time of Call:             %8.3f", mHLAverageTimePerCall.getAverage());
   gConsole.output(cChannelSim, "Max Time of Call:             %8.3f", mHLAverageTimePerCall.getMaximum());
   gConsole.output(cChannelSim, "Total Number of Calls:        %8ld",  mHLAverageTimePerCall.getNumSamples());
   gConsole.output(cChannelSim, ".");
   gConsole.output(cChannelSim, "=============================================");

   // Dump the Land Stats
   if (mLrpTreeLand)
      mLrpTreeLand->dumpStats();
   if (mLrpTreeFlood)
      mLrpTreeFlood->dumpStats();
   if (mLrpTreeScarab)
      mLrpTreeScarab->dumpStats();

   BNonconvexHull::dumpStats();
   
}
#endif

//==============================================================================
// BPather::findLowLevelPath2
//==============================================================================
long BPather::findLowLevelPath2(const BVector &vStart, 
   const BVector &vGoal, const float fRange, BPath *path)
{
   SCOPEDSAMPLE(BPather_findLowLevelPath2);

   long pathResult = BPath::cFailed;
   long lPathIdx = 0L;
   long lExpansionAttempts = 0L;
   bool bExpansionsAllowed = true;

   // Initialize the Pre and Post Hull Ptrs..
   for(long i=0; i<mHullArray1.getNumber(); i++)
      mHullArray1[i].reset();
   for(long i=0; i<mHullArray2.getNumber(); i++)
      mHullArray2[i].reset();
   mHullArray1.setNumber(0);
   mHullArray2.setNumber(0);
   mPreHulls = &mHullArray1;
   mPostHulls = &mHullArray2;

   BNonconvexHull::resetNonCached();

   #ifndef BUILD_FINAL
   BTimer localTimer;
   localTimer.start();
   mlLLCallsPerUpdate++;
   #endif


   if (!mbInitialized)
      return BPath::cError;

   mObArray.setNumber(0);

   #ifdef DEBUG_GRAPHICAL
      debugClearRenderLines(BDebugPrimitives::cCategoryPatherPostHullPather);
   #endif

   #ifdef DEBUGGRAPH_HULLEDAREA
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherHulledArea);
   #endif

   #ifdef DEBUGGRAPH_HULLEDITEMS
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherHulledItems);
   #endif

   #ifdef DEBUGGRAPH_POSTNCHS
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherPostNchs);
   #endif

   #ifdef DEBUGGRAPH_POSTHULLS
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherPostHulls);
   #endif

   #ifdef DEBUG_NONCONVEX_GRAPHICAL
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherNonConvex);
   #endif
   
   #ifdef DEBUG_GRAPHNEWPOLYSTART
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherNewPolyStart);
   #endif

   #ifdef DEBUG_FINDLOWLEVELPATH2
   debug("----> findLowLevelPath2");
   debug("\tInit Start, Goal: (%f, %f) (%f, %f)", vStart.x, vStart.z, vGoal.x, vGoal.z);
   #endif

   // Syncotron
   #ifdef DEBUG_PATHSYNC
   syncPathingCode("----> findLowLevelPath2");
   syncPathingData("Start.x", vStart.x);
   syncPathingData("Start.y", vStart.y);
   syncPathingData("Start.z", vStart.z);
   syncPathingData("Goal.x", vGoal.x);
   syncPathingData("Goal.y", vGoal.y);
   syncPathingData("Goal.z", vGoal.z);
   syncPathingData("Range", fRange);
   #endif

   #ifdef DEBUGGRAPH_STARTENDPOINTS
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherStartEndPoints);
   debugAddPoint(BDebugPrimitives::cCategoryPatherStartEndPoints, vStart, cColorOrigStartPnt);
   debugAddPoint(BDebugPrimitives::cCategoryPatherStartEndPoints, vGoal, cColorOrigGoalPnt);
   #endif

   #ifdef DEBUG_GRAPHPATHNODES
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherPathNodes);
   #endif

   BUnit *unit = NULL;
   if(mPathParms.lEntityType == BEntity::cClassTypeUnit)
   {
      // Look unit up.
      unit = gWorld->getUnit(mPathParms.lEntityID);
      
      /*// If he's banned, just fail.
      if(unit && unit->getFlag(BUnit::cBannedFromPathing))
         return(BPath::cFailed);*/
   }


   // Set up the limit factors for overlapping obstructions..
//   float fDistSqr = vStart.xzDistanceSqr(vGoal);
   //mfLimitDist = cShortRangeLimitDist * 0.5f ;
   mfLimitDist = 4.0f*4.0f;

   /* Don't do this any more.  We're not going to use mvLimitPos,
      but instead use the point we pass in for Limit Checks in findOverlappingObstructions
   */
/*   
   BVector vDir = vGoal - vStart;
   vDir.y = 0.0f;
   if (fDistSqr > cFloatCompareEpsilon)
   {
      vDir *= 0.5f;
      mvLimitPos = vStart + vDir;
   }
   else
      mvLimitPos = vStart;
*/   

   // DLM 6/17/08 - The only thing I can assume is that somewhere along the way we removed the limitCheck
   // from inside of pather, or we never actually had it here, but in older versions of the code, we always
   // checked the distance before calling the pather.  In any event, I'm sure this will break lots of things,
   // but if we're asking for an actual SRP longer than the limit distance, return failed.
   float fDistSqr = vStart.xzDistanceSqr(vGoal);
   if ((fDistSqr > (float)cShortRangeLimitDist) && !(mPathParms.lPathClass == cAirPath))
   {
      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("findLowLevelPath2 - return failed because distance squared of start to goal was: %f, which is greater than the limit of %f",
         fDistSqr, (float)cShortRangeLimitDist);
      #endif
      return BPath::cFailed;
   }
   // Do Start and Goal checks here, and clear them
   // from obstructions.
   BVector vNewStart(vStart);
   BVector vNewGoal(vGoal);
   BVector vAdjStart(vStart);
   BVector vAdjGoal(vGoal);

   // See if we need a new start.
   // Find obstructions I'm on.  Use expanded obstructions for test.
   bool bResult = true;

   mObArray.setNumber(0);
   mPathParms.pObManager->findObstructions(vStart, false, false, mObArray);

   // jce 11/15/2001 -- want to use relaxed check, but needs to be off until intersecting with
   // hull unit is stand on the edge of is fixed.
   //mPathParms.pObManager->findObstructions(vStart, true, false, mlObStack);

   long lNumObs1 = mObArray.getNumber();

   if (lNumObs1)
   {
      SCOPEDSAMPLE(BPather_findLowLevelPath2_lNumObs1);
      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("\tFinding new vStart..");
      #endif
      #ifdef DEBUG_PATHSYNC
      syncPathingCode("Finding new vStart");
      #endif

      #ifndef BUILD_FINAL
      ++mlPolyStartCalls;
      #endif

      #ifdef DEBUG_USENEWPOLYSTART
      bResult = findUnobstructedPoint(vStart, vGoal, mObArray, vAdjStart);
      #else
      bResult = findNewPolyStart(vStart, vAdjStart, mObArray, 0L, NULL, true);
      #endif

      #ifdef DEBUG_FINDLOWLEVELPATH2
      if (bResult)
      {
         debug("\tvAdjStart is now: (%f, %f)", vAdjStart.x, vAdjStart.z);
         if (_fabs(vStart.x - vAdjStart.x) < cFloatCompareEpsilon && _fabs(vStart.z - vAdjStart.z) < cFloatCompareEpsilon)
            debug("\tAdjusted Start was the *SAME* as original start.");
      }
      #endif
   }

   // If we were unable to find an unobstructed vStart position, just bail.
   if (!bResult)
   {
      SCOPEDSAMPLE(BPather_findLowLevelPath2_worst);
      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("\tUnable to find unobstructed vStart.  Returning failed.");
      #endif
      #ifdef DEBUG_PATHSYNC
      syncPathingCode("Unable to find unobstructed vStart.  Returning failed.");
      #endif

      #ifndef BUILD_FINAL
      localTimer.stop();
      long elapsed = localTimer.getElapsedMilliseconds();
      mLLAverageTimePerCall.addSample((float)elapsed);
      #endif
      
      // jce [8/15/2005] -- If a unit, ban it from pathing any more this update because it's so wedged in that it's
      // not going anywhere.
      /*if(unit)
      {
         unit->setFlag(BUnit::cBannedFromPathing, true);
         mBannedUnits.add(unit->getID());
      }*/
      
      return pathResult;
   }

   bool bAdjGoalInRange = true;

   mObArray.setNumber(0);
   // DLM 2/27/08 - don't use relaxed hulls to do check.  If the object
   // is on the edge of a nonrelaxed hull, we want to move it off. 
   mPathParms.pObManager->findObstructions(vGoal, false, false, mObArray);
   //mPathParms.pObManager->findObstructions(vGoal, true, false, mObArray);
   long lNumObs2 = mObArray.getNumber();
   if (lNumObs2)
   {
      SCOPEDSAMPLE(BPather_findLowLevelPath2_lNumObs2);
      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("\tFinding new vGoal...");
      #endif

      #ifdef DEBUG_PATHSYNC
      syncPathingCode("\tFinding new vGoal...");
      #endif

      #ifndef BUILD_FINAL
      ++mlPolyEndCalls;
      #endif

      // If we don't find on unobstructed vGoal, that's okay.  Try for a partial.
      #ifdef DEBUG_USENEWPOLYSTART
      bResult = findUnobstructedPoint(vGoal, vStart, mObArray, vAdjGoal);
      #else
      bResult = findNewPolyStart(vGoal, vAdjGoal, mObArray, 0L, &vAdjStart, false);
      #endif
      // If the distance between the new vGoal and the original vGoal is greater than 
      // the distance from me to the original vGoal, it's worthless
      if (bResult)
      {
         #ifdef DEBUG_FINDLOWLEVELPATH2
         if (bResult)
            debug("\tNew vGoal is now: (%f, %f)", vAdjGoal.x, vAdjGoal.z);
         #endif

         // Use the real start here, instead of the adjusted start.
         float fDist1 = vGoal.xzDistance(vAdjGoal);
         float fDist2 = vGoal.xzDistance(vStart);
         if (fDist1 > fDist2)
         {
            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("\tCouldn't find a new vGoal that was any closer than my current vGoal.  Returning InRangeAtStart.");
            debug("<---- findLowLevelPath");
            #endif

            #ifdef DEBUG_PATHSYNC
            syncPathingCode("Couldn't find a new vGoal that was any closer than my current vGoal.  Returning Failed.");
            #endif

            #ifndef BUILD_FINAL
            localTimer.stop();
            long elapsed = localTimer.getElapsedMilliseconds();
            mLLAverageTimePerCall.addSample((float)elapsed);
            #endif
            //pathResult = BPath::cInRangeAtStart;
            // jce [9/26/2008] -- this should not have been inRangeAtStart ... it's a failure case where the path is
            // not getting you any closer to the goal whatsoever.
            pathResult = BPath::cFailed;
            return pathResult;
         }

      }
      else
      {
         // If we could't even FIND an adjusted goal, then the adjusted goal will definately not be in range, and
         // set our goal back to the original goal.
         // jce [6/29/2005] -- since the adjusted goal now IS the original goal, it's in range.  We might not be
         // able to get to it though.
         bAdjGoalInRange = true;
         vAdjGoal = vGoal;
      }

   }

   // Determine if the new goal is "in range" of the original goal.  If it's not, and we need "Full Path",
   // then we can bail now.  dlm 8/19/02
   if (mPathParms.lTargetID != -1L)
   {
      BUnit *pTarget = gWorld->getUnit(mPathParms.lTargetID);
      if (pTarget)
      {  
         // Check range to target.
         if(mPathParms.pEntity->calculateXZDistance(vAdjGoal, pTarget) > fRange)
            bAdjGoalInRange = false;
      }
      else
      {
         // Check range to goal point.
         if(mPathParms.pEntity->calculateXZDistance(vAdjGoal, vGoal) > fRange)
            bAdjGoalInRange = false;
      }
   }
   else if(mPathParms.pEntity)
   {
      // Check range to goal point.
      if((mPathParms.pEntity->calculateXZDistance(vAdjGoal, vGoal) > fRange) && (mPathParms.lPathClass != cAirPath))
         bAdjGoalInRange = false;
   }
   else
   {
      // No entity so we check point to point.
      float dist = vAdjGoal.distance(vGoal);
      if(dist > fRange + mPathParms.fRadius)
         bAdjGoalInRange = false;
   }

   // jce [8/12/2002] -- if we're looking for full paths only, bail out now since we won't return 
   // a full path any time there's an adjusted goal.
   // But we need to at least see if we could get in range of the goal.. because then this *would* be
   // a full path, even though we adjusted the goal.
   if(mPathParms.fullPathOnly && !bAdjGoalInRange)
   {
      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("\tWe wanted full paths only, but we found found an obstructed goal and it's not in range, so bailing out now....");
      #endif

      #ifndef BUILD_FINAL
      localTimer.stop();
      long elapsed = localTimer.getElapsedMilliseconds();
      mLLAverageTimePerCall.addSample(elapsed);
      #endif
      return BPath::cFailed;
   }

  
   // Get min/max tiles.
   float tileSize = gTerrainSimRep.getDataTileScale();
   float fMinX = 0.0f, fMaxX = 0.0f, fMinZ = 0.0f, fMaxZ = 0.0f;

   fMaxX = gTerrainSimRep.getNumXDataTiles() * tileSize;
   fMaxZ = gTerrainSimRep.getNumXDataTiles() * tileSize;

   if (vAdjGoal.x < fMinX)
      vAdjGoal.x = fMinX;
   if (vAdjGoal.z < fMinZ)
      vAdjGoal.z = fMinZ;
   if (vAdjGoal.x >= fMaxX)
      vAdjGoal.x = fMaxX - 0.1f;
   if (vAdjGoal.z >= fMaxZ)
      vAdjGoal.z = fMaxZ - 0.1f;

   if (vAdjStart.x < fMinX)
      vAdjStart.x = fMinX;
   if (vAdjStart.z < fMinZ)
      vAdjStart.z = fMinZ;
   if (vAdjStart.x >= fMaxX)
      vAdjStart.x = fMaxX - 0.1f;
   if (vAdjStart.z >= fMaxZ)
      vAdjStart.z = fMaxZ - 0.1f;

   #ifdef DEBUGGRAPH_STARTENDPOINTS
   debugAddPoint(BDebugPrimitives::cCategoryPatherStartEndPoints, vAdjStart, cColorPathStartPnt);
   debugAddPoint(BDebugPrimitives::cCategoryPatherStartEndPoints, vAdjGoal, cColorPathGoalPnt);
   #endif

   #ifdef DEBUG_PATHSYNC
   syncPathingCode("Adjusted Start & Goal");
   syncPathingData("vAdjStart", vAdjStart);
   syncPathingData("vAdjGoal", vAdjGoal);
   #endif

   // Failsafe to see if Adj. Start & Adj. Goal have ended up being the same.  If so,
   // just return failed now.  
   if (_fabs(vAdjStart.x - vAdjGoal.x) < cFloatCompareEpsilon &&
       _fabs(vAdjStart.z - vAdjGoal.z) < cFloatCompareEpsilon)
   {
      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("\tAdjusted Start & Adjusted Goal ended up being the same.  Returning full path found.");
      debug("<---- findLowLevelPath");
      #endif
      #ifdef DEBUG_PATHSYNC
      syncPathingCode("Adjusted Start & Adjusted Goal ended up being the same.  Returning full path found.");
      #endif

      #ifndef BUILD_FINAL
      localTimer.stop();
      long elapsed = localTimer.getElapsedMilliseconds();
      mLLAverageTimePerCall.addSample((float)elapsed);
      #endif

      mTempPath.zeroWaypoints();
      mTempPath.addWaypointAtEnd(vAdjGoal);
      pathResult = BPath::cFull;
      bExpansionsAllowed = false;
   }


   // Houdini numbers.. subject to tweaking..
   // TRB 1/31/07:  Increase this to fit new, larger unit scale.  Try making it a function of unit
   // radius.  Hopefully these will remain consistent so we don't get wildly varying expansion values.
   const float fExpansionDelta = Math::Max(8.0f, (mPathParms.fRadius * 8.0f));
   float fExpansionBase =  Math::Max(8.0f, (mPathParms.fRadius * 8.0f));
   //const float fExpansionDelta = 8.0f;
   //float fExpansionBase =  8.0f;
   //float fExpansionDelta = 0.10f;
   //float fExpansionBase = 0.10f;

   #ifndef BUILD_FINAL
   mLLFullCalls++;
   #endif
   
   {
      SCOPEDSAMPLE(BPather_findLowLevelPath2_hullExpand);
   
   while (bExpansionsAllowed)
   {
      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("\tHulling Area...");
      #endif

      #ifdef DEBUG_PATYSYNC
      syncPathingCode("\tHulling Area...");
      #endif

      #ifdef DEBUG_NONCONVEX_GRAPHICAL
      debugClearRenderLines(BDebugPrimitives::cCategoryPatherNonConvex);
      #endif

      // This will be set to true if we really need to expand..
      bExpansionsAllowed = false;
      lPathIdx = 0L;

      // Reset Start & Goal to Adjusted Positions..
      vNewStart = vAdjStart;
      vNewGoal = vAdjGoal;

      // Build Hulled Area.

      BVector vMajorAxis = vNewGoal - vNewStart;
      vMajorAxis.y = 0.0f;
      float fLength = vMajorAxis.length();

      BVector vForward = vMajorAxis;
      vForward.normalize();

      BVector vRight(vForward.z, 0.0f, -vForward.x);

      float useLen = 0.5f*fLength + fExpansionBase;
      //if(useLen < cfMinHulledAreaLength)
        // useLen = cfMinHulledAreaLength;
      //vForward *= useLen;
      vRight *= useLen;

      sScratchPoints[0] = vNewStart - useLen*vForward + vRight;
      sScratchPoints[1] = vNewStart - useLen*vForward - vRight;
      //sScratchPoints[2] = vNewGoal + (useLen-fRange)*vForward - vRight;
      //sScratchPoints[3] = vNewGoal + (useLen-fRange)*vForward + vRight;
      sScratchPoints[2] = vNewGoal + useLen*vForward - vRight;
      sScratchPoints[3] = vNewGoal + useLen*vForward + vRight;
      mHulledArea.initialize(sScratchPoints, 4, true);

      #ifdef DEBUGGRAPH_HULLEDAREA
      debugAddHull(BDebugPrimitives::cCategoryPatherHulledArea, mHulledArea, cColorHulledArea);
      #endif

      // Do the Hulling..
      hullArea();

      #ifdef DEBUGGRAPH_POSTHULLS
      //debugClearRenderLines("posthulls");
      for (long l = 0; l < mPostHulls->getNumber(); l++)
      {
         BColor color(cColorPostHulls);
         color.g -= (float)lExpansionAttempts * .33f;
         color.b -= (float)lExpansionAttempts * .33f;
         debugAddNch(BDebugPrimitives::cCategoryPatherPostHulls, (*mPostHulls)[l], packRGB(color));
      }
      #endif

      // See if start is inside a hull..
      long lStartHullIdx = doInsideCheck(vNewStart, -1L);

      #ifdef DEBUGGRAPH_POSTNCHS
      debugClearRenderLines(BDebugPrimitives::cCategoryPatherPostNchs);
      #endif

      // Start by assuming this isn't and interior path.
      bool interior=false;

      bool onEdge = false;
      long edgeHull = -1;
      long edgeNchIndex = -1;
      long edgeSeg = -1;

      // If the start point is in the hull, break it up into nonconvexhulls.
      if(lStartHullIdx>=0)
      {

         #ifdef DEBUG_FINDLOWLEVELPATH2
         debug("\tStart was found within Convex hull %d, so we're breaking it up into nonconvex hulls...",
            lStartHullIdx);
         #endif

         // Replace the lStartHullIdx hull with a set of non-convex hulls.
         BVector vNchStart(0.0f);
         lStartHullIdx=doNchBuild(lStartHullIdx, vNewStart, onEdge, vNchStart, edgeSeg, edgeNchIndex);
         if (lStartHullIdx == -2)
         {
            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("NchBuild failed to build a nonconvex hull.  Returning (%d) now.", BPath::cFailed);
            #endif

            #ifndef BUILD_FINAL
            localTimer.stop();
            long elapsed = localTimer.getElapsedMilliseconds();;
            mLLAverageTimePerCall.addSample((float)elapsed);            
            #endif

            return BPath::cFailed;
         }
         // DLM 2/26/08 -So much for Grimm's Fairy Tails.  This code is still needed, even with the
         // revised NONCONVEX hull code.  
         if (onEdge)
         {
            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("Determined that start was found on edge of Nch.. setting new start to (%f, %f)",
               vNewStart.x, vNewStart.z);
            #endif
            vNewStart = vNchStart;
            edgeHull = lStartHullIdx;
            // If we determined that we were on the edge of an obstruction, we should also re-do the inside check.
            // dlm 9/3/02
            // But *don't* allow us to consider ourselves to be inside the hull that we just determined we're on the edge of -- 
            // exclude it.  If there are no other hulls, then this also will reset insideidx to -1.
            lStartHullIdx = doInsideCheck(vNewStart, lStartHullIdx);
            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("Re-performing inside check because we found a spot on the edge of the Nch..",
               vNewStart.x, vNewStart.z);
            #endif
         }

         #ifdef DEBUG_FINDLOWLEVELPATH2
         debug("Convex Hulls converted into Nonconvex Hulls..");
         #endif

         if(lStartHullIdx>=0)
         {
            // If we're going to be doing an interior pass.. we need to re-do our inside check,
            // to make sure that we're truly getting the innermost hull.  dlm 9/3/02
            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("Checking now to see if we're inside one of the nonconvex hulls..");
            #endif
            lStartHullIdx = doInsideCheck(vNewStart, -1L);
            // If we are, then see if it's a hole.. and mark our path as interior if so.
            if (lStartHullIdx >= 0)
            {

               // Remember that we're doing an interior path.
               interior=true;
               long ihull = 0;
               bool inhole = (*mPostHulls)[lStartHullIdx].insideHole( vNewStart, ihull );
               //
               // if we're !inside a hole, then we're just screwed.
               //
               if (!inhole)
               {
                  #ifdef DEBUG_FINDLOWLEVELPATH2
                  debug("even after breaking up nonconvex hull, we're still inside, and not in a hole, so returning failed..");
                  #endif
                  return BPath::cFailed;
               }
            }

         }
      }

      // If the goal point is in a hull, bust it up into non-convex hulls -- we don't need to bust
      // it up if it is already the result of a non-convex build.
      long lGoalHullIdx=doInsideCheck(vNewGoal, -1L);
      #ifdef DEBUG_FINDLOWLEVELPATH2
      if (lGoalHullIdx >= 0)
         debug("\tGoal is inside of hull %d", lGoalHullIdx);
      #endif
      if(lGoalHullIdx>=0 && (*mPostHulls)[lGoalHullIdx].isBuiltConvex())
      {
         // Replace the lGoalHullIdx hull with a set of non-convex hulls.
         // Goal is passed in as "start" because we want to find out what hull the goal is inside (if any).
         #ifdef DEBUG_FINDLOWLEVELPATH2
         debug("\tThis was a convex hull, so we're breaking it up into nonconvex hulls...");
         #endif
         
         bool tempOnEdge = false;
         long edgeSeg=-1;
         long edgeNchIndex=-1;
         BVector vNchStart(0.0f);
         lGoalHullIdx=doNchBuild(lGoalHullIdx, vNewGoal, tempOnEdge, vNchStart, edgeSeg, edgeNchIndex);
         if (lGoalHullIdx == -2)
         {

            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("NchBuild failed to build a nonconvex hull.  Returning (%d) now.", BPath::cFailed);
            #endif

            #ifndef BUILD_FINAL
            localTimer.stop();
            mLLAverageTimePerCall.addSample((float)localTimer.getElapsedMilliseconds());
            #endif

            return BPath::cFailed;
         }

         #ifdef DEBUG_FINDLOWLEVELPATH2
         debug("\tAdjusted Goal Hull Idx is now %d", lGoalHullIdx);
         #endif
      }

      // DLM 2/20/08 - ACTUALLY, we don't really need any of this anymore.  Look.. if we're inside a hole, and it's the same
      // hole as the start, then we're fine, and lets path as close as we can.  If we're inside a different hole, 
      // or if were just locked inside a hull and findNewPolyStart didn't get us out (which is odd), then just
      // path as close as we can. 
      /*
      if (lGoalHullIdx >= 0) 
      {
         #ifdef DEBUG_FINDLOWLEVELPATH2
         debug("\tGoal is inside of Hull %d, but it is already a nonconvex hull...", lGoalHullIdx);
         #endif

         long ghull = 0;
         if ((*mPostHulls)[lGoalHullIdx].inside( vNewGoal, ghull ))
         {
            // DLM 2/20/08 - basically do the same thing here that we did for the start. 
            // If we're inside a hole, and it's the same hole as the start, then then we're fine.  Run an interior path.
            // If we're not inside a hole, or it's a different hole than the start hole, meh, just go for the partial. 
            // outside the hull.  
            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("\tGoal is inside of Sub-Hull %d at postHullIdx %d.. checking to see if this is a hole.....", ghull, lGoalHullIdx);
            #endif
            long ihull = 0;
            bool inhole = (*mPostHulls)[lGoalHullIdx].insideHole( vNewGoal, ihull );
            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("\tinhole check returned: %d", inhole);
            #endif            
            if (!inhole)
            {
               #ifdef DEBUG_FINDLOWLEVELPATH2
                   debug("\tGoal is inside of Hull %d, but it is already a nonconvex hull...", lGoalHullIdx);
               #endif
               // adjust to closest point on hull to goal
               vNewGoal = (*mPostHulls)[lGoalHullIdx].findClosestPointOnHull( vNewGoal );
               // get a new goal, if we're still inside then bad news....
               // jce [6/29/2005] -- only consider hulls other than the one we just found a point on.  Numerical accuracy
               // can claim that this point on the edge is inside. 
               long newGoalHullIndex = doInsideCheck(vNewGoal, ghull);
               if (newGoalHullIndex >= 0 && (lGoalHullIdx != newGoalHullIndex))
               {
                  return BPath::cFailed;
               }
            }            
         }
         
      }
      */

      // Run the Path..
      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("\tRunning Path...");
      #endif

      #ifdef DEBUG_PATHSYNC
      syncPathingCode("\tRunning Path...");
      #endif

      if(onEdge)
      {
         pathResult = postHullPather(vNewStart, vNewGoal, vGoal, fRange, &mTempPath, 
            lStartHullIdx, lGoalHullIdx, interior, edgeHull, edgeSeg, edgeNchIndex);
      }
      else
      {
         pathResult = postHullPather(vNewStart, vNewGoal, vGoal, fRange, &mTempPath, 
            lStartHullIdx, lGoalHullIdx, interior, -1, -1, -1);
      }

      // If we got outside the hull, the expand and try again, if allowed.
      if (pathResult == BPath::cOutsideHulledAreaFailed)
      {
         if (++lExpansionAttempts <= clAllowedExpansions)
         {
            #ifdef DEBUG_FINDLOWLEVELPATH2
            debug("\tReturned cOutsideHulledAreaFailed.  Rehulling and trying again.");
            #endif
            bExpansionsAllowed = true;
            fExpansionBase += fExpansionDelta;
            
            #ifndef BUILD_FINAL
            mLLExpansions++;
            #endif
         }
      }
      
      //mTempPath.reverse():

   } // end of while expansions allowed..
   }

   // Fix up the back path.. 
   if (pathResult == BPath::cFull || pathResult == BPath::cInRangeAtStart || pathResult == BPath::cPartial || pathResult == BPath::cOutsideHulledAreaFailed)
   {

      if (lNumObs1)
         mTempPath.addWaypointAtEnd(vStart);

      #ifdef DEBUG_FINDLOWLEVELPATH2
      debug("\tFinal Stitched Path:");
      for (long k = 0; k < mTempPath.getNumberWaypoints(); k++)
      {
         BVector vPoint = mTempPath.getWaypoint(k);
         debug("Waypoint %d: (%f, %f)", k, vPoint.x, vPoint.z);
      }
      #endif

      #ifdef DEBUG_PATHSYNC
      // Syncotron
      syncPathingCode("Final Stitched Path:");
      for (long k1 = 0; k1 < mTempPath.getNumberWaypoints(); k1++)
      {
         BVector vPoint = mTempPath.getWaypoint(k1);
         syncPathingData("Waypoint", vPoint);
      }
      #endif

      if (pathResult != BPath::cOutsideHulledAreaFailed && (fabs(vNewGoal.x - vGoal.x) > cFloatCompareEpsilon ||
          fabs(vNewGoal.z - vGoal.z) > cFloatCompareEpsilon))          
      {
         // If we Adjusted the goal, and got a full path back, we should check to see if we want to
         // actually change the path from full to partial.
         // This fixes a problem with attacking units not getting within in range,
         // but we still want to say they got a complete attack.  We should probably
         // find a *better* way to do this, as this is too expsensive to do every path.         

         // This check is now done before the path is run, and the results are saved in bAdjGoalInRange.  We
         // can do this because the postHullPather no longer modifies the goal passed to it.  dlm 8/19/02
         #ifdef DEBUG_FINDLOWLEVELPATH
         debug("\tDetermining if adjusted path should be full or partial...");
         #endif

         #ifdef DEBUG_PATHSYNC
         syncPathingCode("Determining if adjusted path should be full or partial...");
         #endif

         if (!bAdjGoalInRange)
         {
            pathResult = BPath::cPartial;
            #ifdef DEBUG_FINDLOWLEVELPATH
            debug("\tSet Full Path to Partial...");
            #endif
         }
         // If the path we got back was partial, see if the final spot we found was within range of
         // our original goal.. and if so, upgrade to full. dlm 9/22/02
         if (pathResult == BPath::cPartial && mTempPath.getNumberWaypoints() > 1 && mPathParms.pEntity != NULL)
         {
            // Path hasn't been assembled yet, (ie., path is backwards) so goal point is first point in path.. 
            BVector vTestPos = mTempPath.getWaypoint(0);
            if (mPathParms.lTargetID != -1L)
            {
               BUnit *pTarget = gWorld->getUnit(mPathParms.lTargetID);
               if (pTarget)
               {  
                  // Check range to target.
                  if(mPathParms.pEntity->calculateXZDistance(vTestPos, pTarget) <= fRange)
                     pathResult = BPath::cFull;
               }
               else
               {
                  // Check range to goal point.
                  if(mPathParms.pEntity->calculateXZDistance(vTestPos, vGoal) <= fRange)
                     pathResult = BPath::cFull;
               }
            }
            else
            {
               // Check range to goal point.
               if(mPathParms.pEntity->calculateXZDistance(vTestPos, vGoal) <= fRange)
                  pathResult = BPath::cFull;
            }
            #ifdef DEBUG_FINDLOWLEVELPATH
            debug("\tUpgraded Partial Path to Full as final point found was within range of goal...");
            #endif
         }
      }

      if (mTempPath.getNumberWaypoints() > 1)
      {
         path->zeroWaypoints();
         assemblePath(&mTempPath, path, pathResult);
      }
      else
         pathResult = BPath::cFailed;
   }

   // drop the fully assembled path into the debug cache... 
#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigDebugPather))
   {
      mDebugPath.zeroWaypoints();
      for (int n = 0; n < path->getNumberWaypoints(); n++)
      {
         mDebugPath.addWaypointAtEnd(path->getWaypoint(n));
      }       
   }
#endif

   #ifndef BUILD_FINAL
   localTimer.stop();
   mLLAverageTimePerCall.addSample((float)localTimer.getElapsedMilliseconds());
   #endif

   #ifdef DEBUG_FINDLOWLEVELPATH2
   debug("\tFinal Start, Goal:(%f, %f) (%f, %f)", vAdjStart.x, vAdjStart.z, vAdjGoal.x, vAdjGoal.z);
   debug("\tPathing Successful. (%d)  We are done.", pathResult);
   debug("<---- findLowLevelPath\n");
   #endif

   #ifdef DEBUG_PATHSYNC
   syncPathingCode("<---- findLowLevelPath2");
   #endif

   return pathResult;
}

//==============================================================================
// BPather::hullArea
// used to hull-up the list of obstructions found within a specified area
//==============================================================================
void BPather::hullArea()
{
   #ifndef BUILD_FINAL
   BTimer localTimer;
   localTimer.start();
   #endif

   // jce FIXME: quick fix to the convex/non-convex problem... just start over from scratch on expansion.
   mObArray.setNumber(0);

   for(long i=0; i<mPreHulls->getNumber(); i++)
      (*mPreHulls)[i].reset();
   for(long i=0; i<mPostHulls->getNumber(); i++)
      (*mPostHulls)[i].reset();
   
   mPreHulls->setNumber(0);
   mPostHulls->setNumber(0);

   // Save off current number of obstructions.
   long startIndex=mObArray.getNumber();

   // Now find the new obstructions..
   mPathParms.pObManager->findObstructions(mHulledArea, false, true, mObArray);

   #ifdef DEBUGGRAPH_HULLEDITEMS
   {
      for (long l = startIndex; l < mObArray.getNumber(); l++)
      {
         const BOPQuadHull *pHull = mPathParms.pObManager->getExpandedHull(mObArray[l]);
         debugAddHull(BDebugPrimitives::cCategoryPatherHulledItems, pHull, cColorHulledObs);
      }
   }
   #endif


   // Set up the Arrays, and copy the hulls... 
   long lNumNewHulls = mObArray.getNumber()-startIndex;

   #ifdef DEBUG_HULLAREA
   debug("hullArea -- No. of Objects to Hull: %d", lNumNewHulls);
   #endif

   // If nothing new in the list, short-circuit out now.
   if(lNumNewHulls == 0)
      return;

   long lNumObs = 0L;
   long lNumPostObs = 0L;
   //long lNumPnts = 0L;

   // Note.. do *not* initialize mPostHulls->getNumber() here
   // or mPreHulls or mPostHulls, as this code needs
   // to be reentrant.
   //mPostHulls->getNumber() = 0;
   //mPreHulls = mHullArray1;
   //mPostHulls = mHullArray2;
   
   // Initialize the Pre and Post Hull Arrays..
   // If we already have post hulls, swap the pre/post hull pntrs 
   // so the preHull array will be ready to go..
   if (mPostHulls->getNumber())
      bswap(mPreHulls, mPostHulls);

   bool bPreSetToArray1 = (mPreHulls == &mHullArray1);

   // Reassign mPre & mPost Hull, as the resizing may have changed the addresses. 
   if (bPreSetToArray1)
   {
      mPreHulls = &mHullArray1;
      mPostHulls = &mHullArray2;
   }
   else
   {
      mPreHulls = &mHullArray2;
      mPostHulls = &mHullArray1;
   }

   // Set the size of pre array.
   long lNumHulls = mPreHulls->getNumber() + lNumNewHulls;
   mPreHulls->setNumber(lNumHulls);

   // jce [8/12/2002] -- TODO: consider doing the first iteration using the original BOPQuadHulls since their overlap
   // check will be quicker.

   long lPreHullIdx = mPostHulls->getNumber();
   for (long l = 0; l < lNumNewHulls; l++)
   {
      const class BOPQuadHull *pHull = mPathParms.pObManager->getExpandedHull(mObArray[l+startIndex]);
      (*mPreHulls)[lPreHullIdx+l].initializeQuadHull(pHull);

      (*mPreHulls)[lPreHullIdx+l].getObstructions().setNumber(0);

      (*mPreHulls)[lPreHullIdx+l].getObstructions().add(mObArray[l+startIndex]);

   }

   // Clear out the postHulls
   //for (l = 0; l < lNumHulls; l++)
      //(*mPostHulls)[l].getObstructions().setNumber(0);

   // *Now* reset the posthull count..
   for(long i=0; i<mPostHulls->getNumber(); i++)
      (*mPostHulls)[i].reset();
   mPostHulls->setNumber(0);

   bool bDone = false;
   while (!bDone)
   {  
      //This needs to be set to 1 before we start accessing the 0th element below.  MWC
      for(long i=0; i<mPostHulls->getNumber(); i++)
         (*mPostHulls)[i].reset();
      mPostHulls->setNumber(1);

      (*mPostHulls)[0].initializeConvex((*mPreHulls)[0].getPoints(0).getPtr(), (*mPreHulls)[0].getPointCount(0));
      (*mPostHulls)[0].getObstructions().setNumber(0);
      lNumObs = (*mPreHulls)[0].getObstructions().getNumber();
      for (long m=0; m < lNumObs; m++)
         (*mPostHulls)[0].getObstructions().add((*mPreHulls)[0].getObstructions()[m]);

      bool bHullsMerged = false;

      // For Each Hull in the prehull array, compare it to each hull in the post hull array..
      for (long l = 1; l < lNumHulls; l++)
      {
         long j;
         for (j = 0; j < mPostHulls->getNumber(); j++)
         {
            // jce [8/12/2002] -- took out "fast" checks since they were slowing things down.
            // Do the Comparison..
            //mAddPoints.setNumber(0);
            //long lInsideCount = 0L;
            
            // First, see how many of the points in the hull are in the target hull.
            /*
            lNumPnts = (*mPreHulls)[l].getPointCount();
            for (long n = 0; n < lNumPnts; n++)
            {
               if ((*mPostHulls)[j].inside((*mPreHulls)[l].getPoint(n)))
               {
                  ++lInsideCount;
               }
               else
                  mAddPoints.add((*mPreHulls)[l].getPoint(n));                  
            }
            // If all the points were inside the target hull, discard the hull, and just
            // add all of the hulls that were used in the source hull to the target hull
            if (lInsideCount == lNumPnts)
            {
               lNumObs = (*mPreHulls)[l].getObstructions().getNumber();
               lNumPostObs = (*mPostHulls)[j].getObstructions().getNumber();
               (*mPostHulls)[j].getObstructions().setNumber(lNumPostObs + lNumObs);
               memcpy(&(*mPostHulls)[j].getObstructions()[lNumPostObs], (*mPreHulls)[l].getObstructions(), sizeof(long) * lNumObs);
               break;
            }
            // If we had even one point inside the hull, we don't have to do the
            // overlap check, and we just add the valid points to the hull..
            if (lInsideCount > 0)
            {
               (*mPostHulls)[j].addPointsConvex(mAddPoints, mAddPoints.getNumber(), true);
               // And add the list of obs..
               lNumObs = (*mPreHulls)[l].getObstructions().getNumber();
               lNumPostObs = (*mPostHulls)[j].getObstructions().getNumber();
               (*mPostHulls)[j].getObstructions().setNumber(lNumPostObs + lNumObs);
               memcpy(&(*mPostHulls)[j].getObstructions()[lNumPostObs], (*mPreHulls)[l].getObstructions(), sizeof(long) * lNumObs);
               break;
            }
            */
            // If we're here.. we need to do the overlap hull check.  But we still
            // only have to add the valid points..
            const BNonconvexHull *preHull=&((*mPreHulls)[l]);
            if ((*mPostHulls)[j].overlapsHull(*preHull))
            {
               //(*mPostHulls)[j].addPointsConvex(mAddPoints, mAddPoints.getNumber(), true);
               // jce [8/12/2002] -- If not doing "fast" checks we just need to pass in all the points.
               (*mPostHulls)[j].addPointsConvex(preHull->getPoints(0).getPtr(), preHull->getPointCount(0));
               // And add the list of obs..
               lNumObs = (*mPreHulls)[l].getObstructions().getNumber();
               lNumPostObs = (*mPostHulls)[j].getObstructions().getNumber();
               (*mPostHulls)[j].getObstructions().setNumber(lNumPostObs + lNumObs);
               memcpy(&(*mPostHulls)[j].getObstructions()[lNumPostObs], (*mPreHulls)[l].getObstructions().getPtr(), sizeof(long) * lNumObs);
               break;               
            }

         } // end of for j..
         // If we got all the way through, and we didn't merge this hull with
         // any of the existing ones, then add it to the post hull array..
         // Otherwise.. 
         if (j == mPostHulls->getNumber())
         {
            long onNum = mPostHulls->getNumber();

            mPostHulls->setNumber(onNum+1);

            (*mPostHulls)[onNum].initializeConvex((*mPreHulls)[l].getPoints(0).getPtr(), (*mPreHulls)[l].getPointCount(0));
            lNumObs = (*mPreHulls)[l].getObstructions().getNumber();
            (*mPostHulls)[onNum].getObstructions().setNumber(lNumObs);
            memcpy((*mPostHulls)[onNum].getObstructions().getPtr(), (*mPreHulls)[l].getObstructions().getPtr(), sizeof(long) * lNumObs);
         }
         else
            bHullsMerged = true;
      } // end of for l

      // If we went all the way through without merging any hulls.. we're done.  Otherwise,
      // copy the finished hulls back to the start, and do it again.
      if (bHullsMerged)
      {
         lNumHulls = mPostHulls->getNumber();
         bswap(mPreHulls, mPostHulls);
      }
      else
         bDone = true;
   } // end of while !bDone
            
   #ifndef BUILD_FINAL
   localTimer.stop();
   long elapsed = localTimer.getElapsedMilliseconds();
   mHullAreaAverageTimePerCall.addSample((float)elapsed);
   #endif

}

//==============================================================================
// BPather::doInsideCheck
// Scans the list of postHulls (or postNch's and checks to see if the passed in point
// is inside any of them.  As the postHulls are guaranteed succinct, it
// returns on the first one it finds.  But for postNch's, you may have one
// inside of another.  In this case, we want the "innermost" one as the one
// returned.  
//==============================================================================
long BPather::doInsideCheck(const BVector &vPoint, long lExcludeIndex)
{


   // Sanity Check.
   long lBestInside = -1L;
   long lBestHull = 0;
   BVector vClosestPoint;
   float fDistSqr;
   long lSegment;

   #ifdef DEBUG_DOINSIDECHECK
   debug("---->doInsideCheck");
   debug("vPoint: (%f, %f)", vPoint.x, vPoint.z);
   #endif

   long lHullCount = mPostHulls->getNumber();
   if (!lHullCount)
   {
      BASSERT(0);
      return lBestInside;
   }

   #ifdef DEBUG_DOINSIDECHECK
   debug("\tChecking %d hulls...", lHullCount);
   #endif

   bool bRetVal = false;
   for (long l = 0; l < lHullCount; l++)
   {
      // Skip if this hull is excluded.
      // dlm 9/6/02
      if (l == lExcludeIndex)
         continue;

      long inHull = 0;
      bRetVal = (*mPostHulls)[l].inside(vPoint, inHull);
      if (bRetVal)
      {
         // Find the closest point on this hull from the interior point.
         // We're not doing this inside "is it really inside check" for now, but
         // just using the same inside that everyone else uses. dlm 4/17/02
         vClosestPoint = (*mPostHulls)[l].findClosestPointOnHull(inHull, vPoint, &lSegment, &fDistSqr);
         #ifdef DEBUG_DOINSIDECHECK
         debug("\tPoint found inside hull %d with a distSqr of %f to point (%f, %f)", l, fDistSqr,
            vClosestPoint.x, vClosestPoint.z);
         #endif

         #ifdef DEBUGGRAPH_STARTENDPOINTS
         debugAddPoint(BDebugPrimitives::cCategoryPatherStartEndPoints, vClosestPoint, cColorExaminedPoint);
         #endif
         //if (fDistSqr > cFloatCompareEpsilon)
         //if (fDistSqr > cOnSegmentEpsilon)
         //{
         
         if (lBestInside != -1L)
         {
            // If a point from *this* hull is inside our bestInside so far,
            // then this hull becomes the new bestInside.
            if ((*mPostHulls)[lBestInside].overlapsHull( (*mPostHulls)[l] ) )  //inside ((*mPostHulls)[l].getPoint(0, inHull), lBestHull))
            {
               lBestInside = l;
               lBestHull = inHull;
            }
         }
         else
         {
            lBestInside = l;
            lBestHull = inHull;
         }
         //}
         // Don't return here.. we really do need to find the best inside hull.. not the first one. 
         // dlm 4/17/02
         //return(l);
      }
   }

   #ifdef DEBUG_DOINSIDECHECK
   debug("\tBest Inside Idx was: %d", lBestInside);
   debug("<== doInsideCheck");
   #endif

   return lBestInside;

}

//==============================================================================
// BPather::doSegIntersectCheck
// Performs a segIntersect check against the set of post hulls, returning
// true if there in an intersection.  New version.. the results will be
// stored in the mIntersectResults array, as the pather may need to look
// at more than just the closest result. 
//==============================================================================
bool BPather::doSegIntersectCheck(const BVector &vStart, const BVector &vGoal,
   long lCurrHull, long lCurrIdx, long startInsideIndex, bool interior, bool &hitInterior)
{
   BVector vIPointCheck(cOriginVector);
   long lSegmentCheck = -1L;
   long lHullCheck = 0;
   float fDistSqrCheck = cMaximumFloat;
   bool bRetVal = false;
   hitInterior=false;
   if (mPostHulls->getNumber() == 0)
   {
      BASSERT(0);
      return bRetVal;
   }

   long lResultIdx = 0L;
   for (long l = 0; l < mPostHulls->getNumber(); l++)
   {
      long lIgnoreIdx = -1L;
      if (lCurrHull != -1L && lCurrHull == l)
         lIgnoreIdx = lCurrIdx;

      // DLM 2/20/08 - if we're running an interior path, then don't ignore holes
      bool bHit = false;
      if (interior)
         bHit = (*mPostHulls)[l].segmentIntersectsFast(vStart, vGoal, lIgnoreIdx, vIPointCheck,
            lSegmentCheck, lHullCheck, fDistSqrCheck, true, false, 0.01, false);
      else
         bHit = (*mPostHulls)[l].segmentIntersectsFast(vStart, vGoal, lIgnoreIdx, vIPointCheck,
            lSegmentCheck, lHullCheck, fDistSqrCheck, true, false);

     if (bHit)
      {
         if ((long)mIntersectResults.getSize() <= lResultIdx)
            mIntersectResults.resize(lResultIdx + 1);
         mIntersectResults[lResultIdx].fDistSqr = fDistSqrCheck;
         mIntersectResults[lResultIdx].lHullIdx = l;
         mIntersectResults[lResultIdx].lSegmentIdx = lSegmentCheck;
         mIntersectResults[lResultIdx].lNCHIdx = lHullCheck;
         mIntersectResults[lResultIdx].vIntersect = vIPointCheck;
         ++lResultIdx;
         bRetVal = true;

         //if((*mPostHulls)[l].getHole(lHullCheck))
         if(l == startInsideIndex)
            hitInterior=true;
      }
   }

   mIntersectResults.setNumber(lResultIdx);

   // Sort the results.. (SLOW.. look for speedup SOON.. dlm)
   BPathIntersectResult tempResult;
   for (long i = 0; i < lResultIdx; i++)
   {
      bool bSwapped = false;
      for (long j = 0; j < lResultIdx - 1; j++)
      {
         if (mIntersectResults[j].fDistSqr > mIntersectResults[j+1].fDistSqr)
         {
            tempResult = mIntersectResults[j];
            mIntersectResults[j] = mIntersectResults[j+1];
            mIntersectResults[j+1] = tempResult;
            bSwapped = true;
         }
      }
      if (!bSwapped)
         break;
   }

   return bRetVal;
}

#ifdef _PATH_3
//==============================================================================
// BPather::doSegIntersectCheck_3
// Performs a segIntersect check against the set of post hulls, returning
// true if there in an intersection.  New version.. the results will be
// stored in the mIntersectResults array, as the pather may need to look
// at more than just the closest result. 
//==============================================================================
bool BPather::doSegIntersectCheck_3(const BVector &vStart, const BVector &vGoal,
                                    long lCurrHull, long lCurrIdx, long startInsideIndex, 
                                    bool interior, bool &hitInterior)
{
   BVector vIPointCheck(cOriginVector);
   long lSegmentCheck = -1L;
   long lHullCheck = 0;
   float fDistSqrCheck = cMaximumFloat;
   bool bRetVal = false;

   hitInterior=false;
   bool bHit = false;

   if (interior)
      bHit = mMasterHull.segmentIntersectsFast_3(vStart, vGoal, lCurrHull, lCurrIdx, 
      vIPointCheck, lSegmentCheck, lHullCheck, fDistSqrCheck, true, false, 0.01, false);
   else
      bHit = mMasterHull.segmentIntersectsFast_3(vStart, vGoal, lCurrHull, lCurrIdx, 
      vIPointCheck, lSegmentCheck, lHullCheck, fDistSqrCheck, true, false);

   mIntersectResults.setNumber(0);
   if (bHit)
   {
      BPathIntersectResult result;
      result.fDistSqr = fDistSqrCheck;
      result.lHullIdx = lHullCheck;
      result.lSegmentIdx = lSegmentCheck;
      result.lNCHIdx = lHullCheck;
      result.vIntersect = vIPointCheck;
      bRetVal = true;

      mIntersectResults.add(result);
      if(lHullCheck == startInsideIndex)
         hitInterior=true;
   }
   return bRetVal;
}
#endif

//==============================================================================
// BPather::segIntersects
//==============================================================================
bool BPather::segIntersects(const BVector &vStart, const BVector &vGoal, float errorEpsilon)
{
   // Run through hulls and look for intersection.
   for (long l = 0; l < mPostHulls->getNumber(); l++)
   {
      if((*mPostHulls)[l].segmentIntersects(vStart, vGoal, errorEpsilon))
         return(true);
   }

   return(false);
}

//==============================================================================
// BPather::postHullPather
// Some parameter explanation might be in order.  We're using the postHullPather
// in two different ways.  The first, is to path around the convex post hulls
// created by prehulling.  The second, is to path around the nonconvex hulls
// that exist within a particular post hull, for the cases of getting into or
// out of one of the post hulls.  Ninety percent of the code is the same
// for both of these types of paths, the only real differnce is where we draw
// the nodes from -- the internal array of post hulls (mPostHulls), or the
// internal array of nonconvexhulls (mPostNchs).  Hence, the lNodeSource
// parm.  The remaining two parms have slightly different meaning based on
// the lNodeSource parm.  If lNodeSource is cUsePostHulls, then the bTerminateOnHull
// parm indicates that even though we have a goal, if we intersect a certain postHull,
// we should add that point of intersection and return immediately.  The lHullIdx
// is the PostHull to check against.  If lNodeSource is cUsePostNchs, then
// bTerminateOnHull means though we'e trying to get out of the PostHull indicated
// by lHullIdx, and as soon as we cross any segment of it, we should terminate
// with that point.  Clear?  Cool.
// The last two parms, lStartInsideIdx & lGoalInsideIdx are only meaningful if
// you're using PostNch's.  The tell the pather that the start is inside of
// a particular Nch, and/or the goal is inside a particular nch.
//==============================================================================
long BPather::postHullPather(const BVector &vStart, const BVector &vGoal, const BVector &vOrigGoal, float fRange, 
                             BPath *path, long lStartInsideIdx, long lGoalInsideIdx, 
                             bool interior, long edgeHull, long edgeSegment, long edgeNchIndex)
{

   // Sanity Check.
   if (mPostHulls == NULL)
   {
      BASSERT(0);
      return BPath::cError;
   }

   #ifdef DEBUG_POSTHULLPATHER
   debug("================");
   debug("----> postHullPather");
   debug("Start: (%f, %f)", vStart.x, vStart.z);
   debug("Goal:  (%f, %f)", vGoal.x, vGoal.z);
   debug("Range:  %f", fRange);
   debug("lGoalInsideIdx: %d", lGoalInsideIdx);
   #endif

   #ifdef DEBUG_PATHSYNC
   syncPathingCode("----> postHullPather");
   #endif

   #ifdef DEBUG_GRAPHICAL
   debugClearRenderLines(BDebugPrimitives::cCategoryPatherPostHullPather);
   #endif

   // Initialize
   BVector vIPoint(cOriginVector);
   long lSegment = -1L;
   long lHull = 0;
   long lIntersectHullIdx = -1L;
   float fBestDistSqr = vStart.xzDistanceSqr(vGoal);
   BPathNode2 *pnodeBest = NULL;
   bool bGotToGoal = false;
   bool bDone = false;
   long lIterations = 0L;
   bool bOutsideHulledArea = false;
   bool bReachedHull = false;
   
   // Init Queues
   mQueues->initialize(vStart, vGoal);

   // Init Path
   path->zeroWaypoints();

   // Put the Start Node on the Queue...
   BPathNode2 *pnodeCurr = NULL;
   
   if(edgeHull >= 0)
   {
      pnodeCurr = mQueues->addOpenNode(vStart, NULL, edgeHull, edgeSegment, false);
      if(!pnodeCurr)
      {
         BFAIL("Failed to create initial node");
         return BPath::cError;
      }
      pnodeCurr->mSpecialStart = true;
      pnodeCurr->mlDirFrom = edgeNchIndex;      // piggy-back on this variable
   }
   else
   {
      pnodeCurr = mQueues->addOpenNode(vStart, NULL, -1L, -1L, false);
      if(!pnodeCurr)
      {
         BFAIL("Failed to create initial node");
         return BPath::cError;
      }
   }

   #ifdef DEBUG_PATHSYNC
   syncPathingData("addOpenNode (vStart)", vStart);
   #endif

   #ifdef DEBUG_POSTHULLPATHER
   if (pnodeCurr)
      debug("\tAdded Start (%f, %f) to Open List.", vStart.x, vStart.z);
   else
   {
      debug("\tFailed to add Start (%f, %f) to Open List.", vStart.x, vStart.z);
      BASSERT(0);
   }
   #endif


   // Okay.. Path to the goal!
   #ifdef DEBUG_POSTHULLPATHER
   debug("\tBeginning Node Search..");
   #endif
   while(!bDone)
   {

      #ifdef DEBUG_POSTHULLPATHER
      debug(" ");
      debug("\t\tGetting next open node..");
      #endif

      pnodeCurr = mQueues->getOpenNode();
      if (!pnodeCurr)
      {
         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tNo open nodes in List.  Done searching.");
         #endif
         bDone = true;
         continue;
      }
      else
         // Put it on the closed list.
         mQueues->addClosedNode(pnodeCurr);

      #ifdef DEBUG_POSTHULLPATHER
      debug("\t\tNext Node -- Point (%f, %f) Cost: %f Estimate: %f Total: %f", pnodeCurr->mvPoint.x, pnodeCurr->mvPoint.z, pnodeCurr->mfCost, pnodeCurr->mfEstimate,
         pnodeCurr->mfCost + pnodeCurr->mfEstimate);
      debug("\t\tHull Index: %d  Point Index: %d  Intersect: %s", pnodeCurr->mlHullIndex, pnodeCurr->mlPointIndex,
         (pnodeCurr->mbIsIntersection?"true":"false"));
      #endif
   
      
      #ifdef DEBUG_GRAPHPATHNODES
      debugAddPoint("pathnodes", pnodeCurr->mvPoint, cColorExaminedPoint);
      #endif
      

      #ifdef DEBUG_GRAPHICAL
      if (getShowPaths() == 2)
      {
         if(pnodeCurr->mParent)
            gTerrainSimRep.addDebugLineOverTerrain(pnodeCurr->mParent->mvPoint, pnodeCurr->mvPoint, cColorNchPather, cColorNchPather, 0.75f, BDebugPrimitives::cCategoryPatherPostHullPather);
      }
      #endif
      
      if(pnodeCurr->mvPoint.x < gWorld->getSimBoundsMinX())
         continue;
      if(pnodeCurr->mvPoint.x > gWorld->getSimBoundsMaxX())
         continue;
      if(pnodeCurr->mvPoint.z < gWorld->getSimBoundsMinZ())
         continue;
      if(pnodeCurr->mvPoint.z > gWorld->getSimBoundsMaxZ())
         continue;

      // If the node we pulled off is outside the hulled area, then
      // we'll need to bail.  Do this check before best node check,
      // because we don't want to put points outside the hull on the
      // path.
      if (!mHulledArea.inside(pnodeCurr->mvPoint))
      {
         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tCurrent Node (%f, %f) was outside the Hulled Area.  Bailing..",
            pnodeCurr->mvPoint.x, pnodeCurr->mvPoint.z);
         #endif
         bOutsideHulledArea = true;
         break;
      }

      // If we have an interior hull, the point can't legally be outside the hull.
      /*
      if(interior && pnodeCurr->mlHullIndex<0)
      {
         bool inside=(*mPostHulls)[0].inside(pnodeCurr->mvPoint);
         if(!inside)
         {
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tWe're working on an interior hull and the current point is outside of it... skipping.");
            #endif

            continue;
         }
      }
      */

      // Are we in range of the goal?
      // Perform Range check against our current location.  
      float fDistToGoal = pnodeCurr->mvPoint.xzDistance(vOrigGoal);
      //fDistToGoal -= cSqrt2*mPathParms.fTargetRadius;
      //fDistToGoal -= cSqrt2*mPathParms.fRadius;
      if (fDistToGoal < fRange)
      {
         // We're within range of the goal, we're done. 
         bGotToGoal=true;
         pnodeBest = pnodeCurr;
         
         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tGot within range of goal. Done.");
         #endif

         // Bail on the whole loop
         break;
      }
      
      // See if this is our best path thus far..
      float fDistSqrTest = vGoal.xzDistanceSqr(pnodeCurr->mvPoint);
      #ifdef DEBUG_POSTHULLPATHER
      debug("\t\tDistSqr between curr and goal: %f", fDistSqrTest);
      #endif
      if(fDistSqrTest < fBestDistSqr)
      {
         fBestDistSqr = fDistSqrTest;
         pnodeBest = pnodeCurr;
         
         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tAssigning this to bestDistSqr, and this node to bestPath");
         #endif
      }

      // If this is an intersection node, then continue..
      if (pnodeCurr->mbIsIntersection)
      {
         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tThis is an intersection Node.  Continueing...");
         #endif
         continue;
      }

      // If we know the goal is inside a hull, and we're already on that hull,
      // don't even bother with a segIntersect check.  Just continue..
      // if this node is the best we've found so far.
      // DLM 2/28/08 - We often have the case where the edge start and
      // edge goal are on the same index.. if we get that.. let's not 
      // early out just yet, but give the special start a chance to work.
      if (lGoalInsideIdx != -1L && (pnodeCurr->mlHullIndex == lGoalInsideIdx) && !pnodeCurr->mSpecialStart)
      {
         if (interior)
         {
            // If the goal index is not the same as the start index, then continue as well.
            if (lGoalInsideIdx != lStartInsideIdx)
            {
               #ifdef DEBUG_POSTHULLPATHER
               debug("\t\tNot doing segIntersect check because we're on the hull that we know the goal is inside.  Continuing...");
               #endif
               continue;
            }
         }
         else
         {
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tNot doing segIntersect check because we're on the hull that we know the goal is inside.  Continuing...");
            #endif
            continue;
         }
      }

      // Returned passing of current hull and current index to segintersect checks, to
      // prevent multiple segintersect from being generated at the endpoints of 
      // segments.  DLM 11/19/01
      bool hitInterior=false;
      bool bHit;
      if(pnodeCurr->mSpecialStart)
      {
         // jce [2/25/2005] -- Crazy special start node which means we are starting on the edge of the hull
         // and thus need special processing.  TODO: clean this up, move if-check out of loop, etc.
         #ifdef DEBUG_POSTHULLPATHER
                  debug("\t\tThis is the crazy Special Start Node...");
         #endif
         mIntersectResults.setNumber(1);
         mIntersectResults[0].fDistSqr = 0.0f;
         mIntersectResults[0].lHullIdx = pnodeCurr->mlHullIndex;
         mIntersectResults[0].lNCHIdx = pnodeCurr->mlDirFrom;         // nch index in "dirFrom"
         mIntersectResults[0].lSegmentIdx = pnodeCurr->mlPointIndex;  // point index is really segment index in this case
         mIntersectResults[0].vIntersect = pnodeCurr->mvPoint;
         bHit=true;
      }
      else
      {
         bHit = doSegIntersectCheck(pnodeCurr->mvPoint, vGoal, pnodeCurr->mlHullIndex,
            pnodeCurr->mlPointIndex, lStartInsideIdx, interior, hitInterior);
      }

      // An interior shot to outside that doesn't hit the interior hull is invalid (probably actually right on the edge)
      if(interior && !hitInterior)
      {
         if (lGoalInsideIdx<0)
         {
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tStart is interior and segment didnt intersect interior hull, and goal is outside -- skipping.");
            #endif
            continue;
         }
         // If the goal index is not < 0, it's still only valid to move from this hull to
         // that hull if that hull is inside of my current one. 
         if (pnodeCurr->mlHullIndex != -1 && (pnodeCurr->mlHullIndex != lGoalInsideIdx) && (!(*mPostHulls)[pnodeCurr->mlHullIndex].isInside((*mPostHulls)[lGoalInsideIdx])))
         {
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tStart is interior and segment didnt intersect interior hull, and goal not inside our hull -- skipping.");
            #endif
            continue;
         }

         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tStart is interior and segment didnt intersect interior hull, BUT  goal hull is inside our hull -- using.");
         #endif

      }

      
      #ifdef DEBUG_POSTHULLPATHER
      debug("\t\tdoSegIntersectResults:");
      for (long q = 0; q < mIntersectResults.getNumber(); q++)
         debug("\t\t  %d\tfDistSqr: %8.5f lHullIdx: %03d lSegIdx: %03d vIPnt: (%8.5f, %8.5f)",
         q, mIntersectResults[q].fDistSqr, mIntersectResults[q].lHullIdx, mIntersectResults[q].lSegmentIdx,
         mIntersectResults[q].vIntersect.x, mIntersectResults[q].vIntersect.z);
      debug("\t.");
      #endif

      // Now, it's not enough to just see if we hit something or not.  If the distance to
      // our closest intersection is approx. 0 (that is, we're doing the test from a hull,
      // we need to excercise this bit of convoluted logic to determine if we actually
      // want to use this intersection.  Basically, the point of this is to 1.) Not allow
      // us to enter a hull.. 2.) Not allow us to cross through a hull.. and 3.) Allow
      // us to leave a hull. 
#if 0
      if (bHit && mIntersectResults[0].fDistSqr < cFloatCompareEpsilon)
      {
         bHit=false;

         /*
         // If we only have one intersection with the closest Hull...
         if (mIntersectResults[0].lNumIntersections == 1)
         {
            // If we're pathing around post hulls, and this is not the intersection node,
            // don't use the intersection.  IT means we're trying to leave a post hull.
            // If this is the intersection node, then use the new intersection, even if it's
            // zero distance, to keep us from moving into the hull.
            if (lStartInsideIdx == -1)
            {
               if (lGoalInsideIdx != mIntersectResults[0].lHullIdx)
               {
                  #ifdef DEBUG_POSTHULLPATHER
                  debug("\t\tUsing PostNCH's, lStartInsideIdx == -1, & lGoalInsideIdx != mIntersectResults[0].lHullIdx.  bUseIntersection = false");
                  #endif
                  bUseIntersection = false;
               }
            }
            else
            {
               if (lStartInsideIdx == mIntersectResults[0].lHullIdx && lGoalInsideIdx == mIntersectResults[0].lHullIdx)
               {
                  #ifdef DEBUG_POSTHULLPATHER
                  debug("\t\tUsing PostNCH's, lStartIdx == %d, & lGoalInsideIdx = %d & mIntersectResults[0].lHullIdx == %d.  They are the same, bUseIntersection = false",
                     lStartInsideIdx, lGoalInsideIdx, mIntersectResults[0].lHullIdx);
                  #endif
                  bUseIntersection = false;               
               }
            }
         }
         */
      }
#endif 

      // If we didn't get a hit.. or we got a single hit with a hull we don't care about, then
      // we are done.  
      if (!bHit)
      {
         // dlm 9-5-02 -- HACK.  Last ditch effort to avoid the case where even though we have no intersections returned, the path
         // takes us *through* the non convex hull.  Do a "real" seg intersect check between our current point and the goal
         // against relaxed obstructions.  If this fails, then we're not really done.
         bool bHitObs = false;
         bHitObs = mPathParms.pObManager->segmentIntersects(pnodeCurr->mvPoint, vGoal, true);
         if (!bHitObs)
         {
            bGotToGoal = true;
            pnodeBest = mQueues->newNode(vGoal, pnodeCurr);
            if (!pnodeBest)
            {
               BASSERT(0);
               break;
            }
         
            #ifdef DEBUG_GRAPHICAL
            if (getShowPaths() == 2)
            {
               gTerrainSimRep.addDebugLineOverTerrain(pnodeCurr->mvPoint, 
                  pnodeBest->mvPoint, cColorNchPather, cColorNchPather, 0.75f, BDebugPrimitives::cCategoryPatherPostHullPather);
            }
            #endif
         
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tNo Obstructions between curr and goal.  Done.");
            #endif
         
            // Bail on the whole loop
            break;
         }
         else
         {
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tdoSegIntersectCheck returned no obstructions, but obMgr reported that we had some, so we're just continueing..");
            #endif
            continue;
         }

      }
      
      // Check our Iteration count, and exit if we've exceeded our limit.
      ++lIterations;
      
      // At each iteration, if we have timing limits turned on, check them..
      #ifdef DEBUG_PATHING_LIMITS
      if (lIterations > mlBadPathThresholdLow)
      {
         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tBad Path Threshold Reached.  Done Searching. Iterations were: %ld", lIterations);
         #endif
         
         break;
      }
      #endif
      
    
      // Determine the correct Intersect point to check against..      
      lIntersectHullIdx = mIntersectResults[0].lHullIdx;
      vIPoint = mIntersectResults[0].vIntersect;
      lSegment = mIntersectResults[0].lSegmentIdx;
      lHull = mIntersectResults[0].lNCHIdx;
   
      // If the intersection point is the SAME as our current node point,
      // we need to handle it differently.
      bool bNewNode = true;

      //if (mIntersectResults[0].fDistSqr < cFloatCompareEpsilon)
      if (mIntersectResults[0].fDistSqr < 0.001f)
      {
         // If the intersection distance was zero, but we didn't discard the intersection,
         // don't attempt to add a new node at the same point, but just use the current one
         // instead.
         if (bHit)
         {
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tCurrent Node is same as Intersection.  Using Current Node as Parent of Hull nodes.");
            #endif
            bNewNode = false;
         }
         else
         {
            // If the closest intersection was set to zero, and we decided to
            // discard that intersection, then if we're here we must have another
            // intersection with another hull.  Use that as the intersection.
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tFirst intersection was zero distance.  Using next Intersection as valid one.");
            #endif
            // Sanity check, just the same.
            if (mIntersectResults.getNumber() < 2)
            {
               BASSERT(0);
               continue;
            }
            lIntersectHullIdx = mIntersectResults[1].lHullIdx;
            vIPoint = mIntersectResults[1].vIntersect;
            lSegment = mIntersectResults[1].lSegmentIdx;            
            lHull = mIntersectResults[1].lNCHIdx;
         }
      }

      // Before we actually create or assign the new intersect node,
      // if the intersection point was on the hull we're currently
      // pulling nodes off of, ignore it, and move on.
      if ((pnodeCurr->mlHullIndex == lIntersectHullIdx) && !pnodeCurr->mSpecialStart)
      {
         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tIntersected with the current Hull.  Continuing...");
         #endif
         continue;
      }

      // Now create (or assign) the intersect node.. and then create nodes
      // from all of the vertices of the (newly) intersected hull
      BPathNode2 *pnode = NULL;
      if (bNewNode)
      {
         pnode = mQueues->addOpenNode(vIPoint, pnodeCurr, lIntersectHullIdx, -1L, true);

         // If we failed to add, it means the point was already in the list.  We have already
         // rejected intersection points at this point, so use the current node as the parent for
         // the "around the hull" points.
         if(!pnode)
         {
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tIntersection point is very close to start... using start node instead.");
            #endif

            pnode=pnodeCurr;
         }

         #ifdef DEBUG_PATHSYNC
         syncPathingData("addOpenNode (vIPoint)", vIPoint);
         #endif
      }
      else
         pnode = pnodeCurr;
      if (pnode == NULL)
      {
         #ifdef DEBUG_POSTHULLPATHER
         debug("\t\tUnable to add intersection point.  Continuing..");
         #endif
         continue;
      }


      #ifdef DEBUG_POSTHULLPATHER
      debug("\t\tAdded (or assigned) Intersection Node (%f, %f)", pnode->mvPoint.x, pnode->mvPoint.z);
      #endif


      // Walk the hull and place the points on the hull...
      long lNumPoints = (*mPostHulls)[lIntersectHullIdx].getPointCount(lHull);
      // NumUsedPoints & NumPoints will most of the time be the same.. but if we
      // need to skip the first vertex, numUsedPoins is the number of actual points
      // we added to the open list, whereas numPoints will still be the number of 
      // points on the actual hull.
      long lNumUsedPoints = lNumPoints;
      long lHullIterations = lNumPoints >> 1;
      BPathNode2 *pnodeCw = NULL;
      BPathNode2 *pnodeCcw = NULL;
      BPathNode2 *pnodeCwParent = pnode;
      BPathNode2 *pnodeCcwParent = pnode;
      BPathNode2 *pnodeGoal = NULL;
      BVector vCwPoint(cOriginVector), vCcwPoint(cOriginVector);
      long lCwIdx = (lSegment == lNumPoints - 1)?0:lSegment + 1;
      long lCcwIdx = lSegment;

      // If we're about to add the points around the hull, check to see if the first
      // ccw point is the same as the intersection (which it would be if the intersection
      // occured at a vertex.  If so, then skip this point, and decrement the number of points.  
      vCcwPoint = (*mPostHulls)[lIntersectHullIdx].getPoint(lCcwIdx, lHull);

      if ((_fabs(pnode->mvPoint.x - vCcwPoint.x) < cFloatCompareEpsilon) &&
          (_fabs(pnode->mvPoint.z - vCcwPoint.z) < cFloatCompareEpsilon))
      {
         lCcwIdx = (lSegment == 0)?lNumPoints - 1:lSegment - 1;
         --lNumUsedPoints;
         lHullIterations = lNumUsedPoints >> 1;
      }

      // Check the first cw point as well..
      vCwPoint = (*mPostHulls)[lIntersectHullIdx].getPoint(lCwIdx, lHull);
      if ((_fabs(pnode->mvPoint.x - vCwPoint.x) < cFloatCompareEpsilon) &&
          (_fabs(pnode->mvPoint.z - vCwPoint.z) < cFloatCompareEpsilon))
      {
         ++lCwIdx;
         if (lCwIdx == lNumPoints)
            lCwIdx = 0;
         --lNumUsedPoints;
         lHullIterations = lNumUsedPoints >> 1;
      }

      // dlm 9/6/02 - If our goal is inside this hull, then find the closest point *to* the goal on the hull, and 
      // add that point. 
      bool bAddGoalPoint = false;
      BVector vClosestToGoalOnHull(0.0f);
      long lGoalSegment = -1;
      long lPrevGoalSegment = -1;
      if (lGoalInsideIdx != -1 && lGoalInsideIdx == lIntersectHullIdx)
      {
         vClosestToGoalOnHull = (*mPostHulls)[lIntersectHullIdx].findClosestPointOnHull(vGoal, &lGoalSegment, NULL);
         if (lGoalSegment != -1)
         {
            lPrevGoalSegment = (lGoalSegment == 0)?3:lGoalSegment + 1;
            // If the lGoalSegment is the same as the intersection segment.. then add the node immediately.
            if (lGoalSegment == lSegment)
            {
               pnodeGoal = mQueues->addOpenNode(vClosestToGoalOnHull, pnode, lIntersectHullIdx, -1, false);
               if (pnodeGoal)
               {
                  bAddGoalPoint = false;
                  pnode = pnodeGoal;
                  pnodeCwParent = pnodeGoal;
                  pnodeCcwParent = pnodeGoal;
               }                  
            }
            else
               bAddGoalPoint = true;
         }

      }

      // Add the hull points.
      for (long n = 0; n < lNumUsedPoints; n++)
      {
         if(pnodeCwParent)
         {
            // Add CW point
            vCwPoint = (*mPostHulls)[lIntersectHullIdx].getPoint(lCwIdx, lHull);

            pnodeCw = mQueues->addOpenNode(vCwPoint, pnodeCwParent, lIntersectHullIdx, lCwIdx, false);

            #ifdef DEBUG_PATHSYNC
            syncPathingData("addOpenNode (vCwPoint)", vCwPoint);
            #endif

            #ifdef DEBUG_POSTHULLPATHER
            if (pnodeCw == NULL)
               debug("\t\tFailed to add Clockwise Point %d of Hull %d (%f, %f)", lCwIdx, lIntersectHullIdx,
                  vCwPoint.x, vCwPoint.z);
            else
               debug("\t\tAdded Clockwise Point %d         of Hull %d (%f, %f)  cost=%0.1f, estimate=%0.1f, total=%0.1f", lCwIdx, lIntersectHullIdx,
                  vCwPoint.x, vCwPoint.z, pnodeCw->mfCost, pnodeCw->mfEstimate, pnodeCw->mfCost+pnodeCw->mfEstimate);
            #endif

            // Before we advance, see if we need to insert the on Goal Point.. And if so, then do so.
            if (bAddGoalPoint && pnodeCw && (lCwIdx == lGoalSegment))
            {
               pnodeGoal = mQueues->addOpenNode(vClosestToGoalOnHull, pnodeCw, lIntersectHullIdx, -1, false);
               if (pnodeGoal)
               {
                  pnodeCw = pnodeGoal;
               }
               // Whether we succeeded or not, we made the attempt.. so don't make it again.
               bAddGoalPoint = false;
            }
            
            pnodeCwParent = pnodeCw;                  

            // Next CW point.
            lCwIdx = (lCwIdx == lNumPoints - 1)?0:lCwIdx + 1;
         }


         if(pnodeCcwParent)
         {
            // Add CCW point.
            vCcwPoint = (*mPostHulls)[lIntersectHullIdx].getPoint(lCcwIdx, lHull);

            pnodeCcw = mQueues->addOpenNode(vCcwPoint, pnodeCcwParent, lIntersectHullIdx, lCcwIdx, false);

            #ifdef DEBUG_PATHSYNC
            syncPathingData("addOpenNode (vCcwPoint)", vCcwPoint);
            #endif
         
            #ifdef DEBUG_POSTHULLPATHER
            if (pnodeCcw == NULL)
               debug("\t\tFailed to add Counter-clockwise Point %d of Hull %d (%f, %f)", lCcwIdx, lIntersectHullIdx,
                  vCcwPoint.x, vCcwPoint.z);
            else
               debug("\t\tAdded Counter-Clockwise Point %d of Hull %d (%f, %f)  cost=%0.1f, estimate=%0.1f, total=%0.1f", lCcwIdx, lIntersectHullIdx,
                  vCcwPoint.x, vCcwPoint.z, pnodeCcw->mfCost, pnodeCcw->mfEstimate, pnodeCcw->mfCost+pnodeCcw->mfEstimate);
            #endif
            // Before we advance, see if we need to insert the on Goal Point.. And if so, then do so.
            if (bAddGoalPoint && pnodeCcw && (lCcwIdx == lPrevGoalSegment))
            {
               pnodeGoal = mQueues->addOpenNode(vClosestToGoalOnHull, pnodeCcw, lIntersectHullIdx, -1, false);
               if (pnodeGoal)
               {
                  pnodeCcw = pnodeGoal;
               }
               // Whether we succeeded or not, we made the attempt.. so don't make it again.
               bAddGoalPoint = false;
            }

            pnodeCcwParent = pnodeCcw;

            // Next CCW point.
            lCcwIdx = (lCcwIdx == 0)?lNumPoints - 1:lCcwIdx - 1;
         }
      }
     
   } // end of while !bDone

   // Okay.. we're done.  What kind of path did we get?
   #ifdef DEBUG_POSTHULLPATHER
   debug("\tExited Node Search.");
   #endif
   
   #ifndef BUILD_FINAL
   mlPolyIterations += lIterations;
   if (lIterations > mlPolyIterationMax)
      mlPolyIterationMax = lIterations;
   #endif
   
   // If we found a path of some sort, set it up for return.   
   if(pnodeBest /*&& pnodeBest->mParent */)
   {
      
      #ifdef DEBUG_POSTHULLPATHER
      debug("Path Returned:");
      long lWaypoint = 0;
      #endif
      
      pnodeCurr = pnodeBest;
      while(pnodeCurr)
      {
         #ifdef DEBUG_POSTHULLPATHER
         debug("Waypoint: %d (%f, %f)", lWaypoint, pnodeCurr->mvPoint.x, pnodeCurr->mvPoint.z);
         lWaypoint++;
         #endif

         path->addWaypointAtEnd(pnodeCurr->mvPoint);
         pnodeCurr = pnodeCurr->mParent;
      }

      // Release the nodes..
      mQueues->terminate();
      
      if(bGotToGoal)
      {
         #ifdef DEBUG_POSTHULLPATHER
         debug("<---- Returned with FULL from postHullPather");
         #endif
         
         return(BPath::cFull);
      }
      else
      {
         if (bOutsideHulledArea)
         {
            #ifdef DEBUG_POSTHULLPATHER
            debug("<---- Returned with OUTSIDEHULLEDAREAFAILED from postHullPather");
            #endif
            return(BPath::cOutsideHulledAreaFailed);
         }

         if (bReachedHull)
         {

            #ifdef DEBUG_POSTHULLPATHER
            debug("<---- Returned with REACHEDHULL from postHullPather");
            #endif
            return(BPath::cReachedHull);
         }
         /*
         #ifdef DEBUG_GRAPHSTARTENDPOINTS
         debugAddPoint("startendpoints", path->getStart(), cColorPartialBestPnt);
         #endif
         */
         
         #ifdef DEBUG_POSTHULLPATHER
         debug("<---- Returned with PARTIAL from postHullPather");
         #endif
         return(BPath::cPartial);
      }
   }
   
   // Release the nodes..
   mQueues->terminate();
   
   if (bOutsideHulledArea)
   {
      #ifdef DEBUG_POSTHULLPATHER
      debug("<---- Returned with OUTSIDEHULLEDAREAFAILED from postHullPather");
      #endif

      return BPath::cOutsideHulledAreaFailed;
   }
      
   #ifdef DEBUG_POSTHULLPATHER
   debug("<---- Returned with FAILED from postHullPather");
   #endif

   return BPath::cFailed;
   
}

//==============================================================================
// BPather::doNchBuild
// Take an index into the postHull array.  Obviously, for this to work,
// you must have already built the post hull array.  Builds the set of 
// nonconvexhull's determined my overlapping and non-overlapping obstructions
// found within the posthull specified by the index.
// Returns the hull that the start point is inside (or -1).
// If for whatever reason the NchBuild fails, return -2.
//==============================================================================
long BPather::doNchBuild(long lPostHullIdx, const BVector &start, bool &bOnEdge, BVector &vAdjStart, long &segIndex, long &nchIndex)
{
   #ifdef DEBUG_DONCHBUILD
   debug("---> doNchBuild");
   #endif

   if (!mbInitialized)
   {
      BASSERT(0);
      return(-1);
   }

   // Assume we aren't in any hull.
   long insideIndex=-1;

   bOnEdge = false;

   // Pull the old CONVEX hull off of the list and replaces it with non-convex hulls of the stuff contained in
   // the hull.

   // Figure out how many hulls we have.
   long lNumObs = (*mPostHulls)[lPostHullIdx].getObstructions().getNumber();

   #ifdef DEBUG_DONCHBUILD
   debug("Examining lPostHullIdx: %d", lPostHullIdx);
   debug("There are %d obs within it.", lNumObs);
   #endif

   // Need something to process
   if(lNumObs<1)
   {
      BASSERT(0);
      return(-1);
   }

   // Copy.. slow.. but we need to be destructive.  Think of
   // something better though.
   mPostObs.setNumber(lNumObs);

   memcpy((BOPObstructionNode **)mPostObs.getPtr(), (BOPObstructionNode **)(*mPostHulls)[lPostHullIdx].getObstructions().getPtr(), lNumObs * sizeof(long));

   // Put start and goal into special point array.  Not particularly brilliant, I'll admit.
   // We can do away with these for now, as they're not actually used. dlm 4/18/02
   /*
   BVector specialPoints[2];
   specialPoints[0]=start;
   specialPoints[1]=goal;
   */

   // Remember if this is the first one or not.
   bool firstOne=true;

   // While there are more hulls to process...
   while(mPostObs.getNumber())
   {
      // Poke the first one into the hull we are replacing and the subsequent ones at the end of the
      // post hull list.
      long index;
      if(firstOne)
      {
         index=lPostHullIdx;
         firstOne=false;
      }
      else
      {
         index=mPostHulls->getNumber();
         mPostHulls->setNumber(index+1);
      }
      
      // Clear current list.
      sCurrentList.setNumber(0);

      // Add a remaining obstruction to be processed.
      long lastIndex=mPostObs.getNumber()-1;
      sCurrentList.add(mPostObs[lastIndex]);

      // Init to first hull.
      //(*mPostHulls)[index].reset();
      //const BConvexHull *hull=mPathParms.pObManager->getExpandedHull(mPostObs[lastIndex]);
      //(*mPostHulls)[index].addHullNonconvex(mPostObs[lastIndex], *hull);

      // Shrink array.
      mPostObs.setNumber(lastIndex);

      // For each current list hull we haven't checked yet, check all the remaining hulls.
      long checkIndex=0;
      while(checkIndex<sCurrentList.getNumber())
      {
         // Look up current hull.
	      const BOPQuadHull *currentHull = mPathParms.pObManager->getExpandedHull(sCurrentList[checkIndex]);
         if(!currentHull)
         {
            BFAIL("Null expanded hull");
            continue;
         }

         // Run through remaining hulls.
         for(long i=0; i<mPostObs.getNumber(); i++)
         {
            // Look up hull.
            const BOPQuadHull *hull = mPathParms.pObManager->getExpandedHull(mPostObs[i]);
            if(!hull)
            {
               BFAIL("Null expanded hull");
               continue;
            }
               
            // Check if they overlap.
            if(hull->overlapsHull(currentHull))
            {
               // Add to current list.
               sCurrentList.add(mPostObs[i]);
               //(*mPostHulls)[index].addHullNonconvex(mPostObs[i], *hull);

               // Remove from to-be-processed list by copying the last element down and
               // shrinking the array.
               lastIndex=mPostObs.getNumber()-1;
               mPostObs[i]=mPostObs[lastIndex];
               mPostObs.setNumber(lastIndex);

               // Adjust index because we put a new thing in the slot.
               i--;
            }
         }

         // Next current hull.
         checkIndex++;
      }

      // Okay.. sCurrentList should have a pack of overlapping hulls.. 
      #ifdef DEBUG_DONCHBUILD
      debug("\tNow building NCH index.. %d", index);
      #endif

      if (!(*mPostHulls)[index].initialize(sCurrentList, mPathParms.pObManager, NULL, -1, NULL))
      {
         #ifdef DEBUG_DONCHBUILD
         debug("<---- Failed to build NCH.  Returning -2.");
         #endif
         BASSERT(0);
         return(-2);
      }

      #ifdef DEBUG_DONCHBUILD
      debug("\tNCH index %d successfully built and created %d Hulls.", index, (*mPostHulls)[index].getHullCount());
      for (long n = 0; n < (*mPostHulls)[index].getHullCount(); n++)
         debug("\tNCH index %d, Hull %d contains %d points", index, n, (*mPostHulls)[index].getPointCount(n));
      #endif

      #ifndef BUILD_FINAL
      if (mStartStas == 0)
      {
         ++mlNCCalls;
         float elapsed = (*mPostHulls)[index].getInitTime();
         mfNCSum += elapsed;
         if (elapsed > mfNCMax)
         {
            mfNCMax = elapsed;
            mlNCObsAtMax = lNumObs;
         }
         if (lNumObs > mlNCMaxObs)
            mlNCMaxObs = lNumObs;
      }
      else
      {
         mStartStas--;
      }
      #endif

      // Before we do the inside check.. see if we're on the edge of
      // this nch.  If we are, then set the bOnEdge and vAdjStartParms
      // accordingly.
      if (!bOnEdge)
      {
         float fDistSqr = cMaximumFloat;
         vAdjStart = (*mPostHulls)[index].findClosestPointOnHull(start, &segIndex, &nchIndex, &fDistSqr);

         if (fDistSqr < cOnSegmentEpsilon)
         {
            bOnEdge = true;
            // Set insideIndex.  This is the hull we were on the edge of.  Don't allow us to 
            // be considered inside this hull after we do the inside check. 
            insideIndex = index;
         }
      
         #ifdef DEBUG_DONCHBUILD
         debug("\tEdge Check.  start: (%f, %f) AdjStrt: (%f, %f), DstSqr: %f bOnEdge: %d",
            start.x, start.z, vAdjStart.x, vAdjStart.z, fDistSqr, bOnEdge);
         #endif
      }

      if(insideIndex<0 && !bOnEdge)
      {
         long lTempHull = 0;
         bool inside=(*mPostHulls)[index].inside(start, lTempHull);
         if(inside)
         {
            insideIndex=index;
            #ifdef DEBUG_DONCHBUILD
            debug("\tstart found inside hull (%d) of NCH Index: %d", lTempHull, insideIndex);
            #endif
         }
      }

      #ifdef DEBUGGRAPH_POSTNCHS
      debugAddNch(BDebugPrimitives::cCategoryPatherPostNchs, (*mPostHulls)[index], cColorPostNchs);
      #endif

   }


   #ifdef DEBUG_DONCHBUILD
   debug("<--- doNchBuild (returnValue: %d)", insideIndex);
   #endif

   return(insideIndex);
}


//==============================================================================
// BPather::tileSegmentIntersect
//==============================================================================
bool BPather::tileSegmentIntersect(const BVector &vStart, const BVector &vEnd)
{
   if(mLrpTreeLand)
      return mLrpTreeLand->segmentIntersect(0, vStart, vEnd);
   else
      return false;
}

//==============================================================================
// BPather::updateEntityInfo
//==============================================================================
void BPather::updateEntityInfo()
{
   if (!mbInitialized)
      return;

   switch(mPathParms.lPathSource)
   {
      case cUnitSource:
      {
         if (mPathParms.lEntityID != -1L)
            mPathParms.pEntity = gWorld->getUnit(mPathParms.lEntityID);
         else
            mPathParms.pEntity = NULL;
         break;
      }
      case cUnitGroupSource:
      {
         if (mPathParms.lEntityID != -1L)
            mPathParms.pEntity = gWorld->getSquad(mPathParms.lEntityID);
         else
            mPathParms.pEntity = NULL;
         break;
      }
      case cPlatoonSource:
      {
         if (mPathParms.lEntityID != -1L)
            mPathParms.pEntity = gWorld->getPlatoon(mPathParms.lEntityID);
         else
            mPathParms.pEntity = NULL;
         break;
      }
      case cProtoUnitSource:
      case cUndefinedSource:
         mPathParms.pEntity = NULL;
         break;
   }

   // And the target..
   if (mPathParms.lTargetID != -1L)
   {
      mPathParms.pTarget = gWorld->getUnit(mPathParms.lTargetID);
      if (!mPathParms.pTarget)
         mPathParms.fTargetRadius = 0.0f;
      else
         mPathParms.fTargetRadius = mPathParms.pTarget->getObstructionRadius();
   }
   else
   {
      mPathParms.pTarget = NULL;
      mPathParms.fTargetRadius = 0.0f;
   }
   return;
}

//==============================================================================
// BPather::findClosestPassableTile
//==============================================================================
bool BPather::findClosestPassableTile(const BVector &vReference, BVector &vResult, long lLandFloodScarab)
{
   if ((lLandFloodScarab == cLandPath) || (lLandFloodScarab == cSquadLandPath))
   {
      if (mLrpTreeLand)
         return mLrpTreeLand->findClosestPassableTile(vReference, vResult);
      else
         return false;
   }
   else if (lLandFloodScarab == cFloodPath)
   {
      if (mLrpTreeFlood)
         return mLrpTreeFlood->findClosestPassableTile(vReference, vResult);
      else
         return false;
   }
   else if (lLandFloodScarab == cScarabPath)
   {
      if (mLrpTreeScarab)
         return mLrpTreeScarab->findClosestPassableTile(vReference, vResult);
      else
         return false;
   }
   else if (lLandFloodScarab == cHoverPath)
   {
      if (mLrpTreeHover)
         return mLrpTreeHover->findClosestPassableTile(vReference, vResult);
      else
         return false;
   }

   return false;
}


//==============================================================================
// BPather::isObstructedTile
//==============================================================================
bool BPather::isObstructedTile(const BVector &vPos, long lLandFloodScarab)
{
   if ((lLandFloodScarab == cLandPath) || (lLandFloodScarab == cSquadLandPath))
   {
      if (mLrpTreeLand)
         return mLrpTreeLand->isObstructedTile(vPos);
      else
         return true;
   }
   else if (lLandFloodScarab == cFloodPath)
   {
      if (mLrpTreeFlood)
         return mLrpTreeFlood->isObstructedTile(vPos);
      else
         return true;
   }
   else if (lLandFloodScarab == cScarabPath)
   {
      if (mLrpTreeScarab)
         return mLrpTreeScarab->isObstructedTile(vPos);
      else
         return true;
   }
   else if (lLandFloodScarab == cHoverPath)
   {
      if (mLrpTreeHover)
         return mLrpTreeHover->isObstructedTile(vPos);
      else
         return true;
   }
   return true;

}

//==============================================================================
// BPather::findClosestPassableTileEx
//==============================================================================
bool BPather::findClosestPassableTileEx(const BVector &vReference, BVector &vResult, long lLandFloodScarab)
{
   if ((lLandFloodScarab == cLandPath) || (lLandFloodScarab == cSquadLandPath))
   {
      if (mLrpTreeLand)
         return mLrpTreeLand->findClosestPassableTileEx(vReference, vResult);
      else
         return false;
   }
   else if (lLandFloodScarab == cFloodPath)
   {
      if (mLrpTreeFlood)
         return mLrpTreeFlood->findClosestPassableTileEx(vReference, vResult);
      else
         return false;
   }
   else if (lLandFloodScarab == cScarabPath)
   {
      if (mLrpTreeScarab)
         return mLrpTreeScarab->findClosestPassableTileEx(vReference, vResult);
      else
         return false;
   }
   else if (lLandFloodScarab == cHoverPath)
   {
      if (mLrpTreeHover)
         return mLrpTreeHover->findClosestPassableTileEx(vReference, vResult);
      else
         return false;
   }
   return false;
   // DLMTODO: Add soft obstructions to this check.  6/10/02
}

//==============================================================================
// BPather::getLongRangeSyncing
//==============================================================================
bool BPather::getLongRangeSyncing(long lLandFloodScarab)
{
   if ((lLandFloodScarab == cLandPath) || (lLandFloodScarab == cSquadLandPath))
      return mLrpTreeLand->isSyncEnabled();
   else if (lLandFloodScarab == cFloodPath)
      return mLrpTreeFlood->isSyncEnabled();
   else if (lLandFloodScarab == cScarabPath)
      return mLrpTreeScarab->isSyncEnabled();
   else if (lLandFloodScarab == cHoverPath && mLrpTreeHover)
      return mLrpTreeHover->isSyncEnabled();
   return false;
}

//==============================================================================
// BPather::setLongRangeSyncing
//==============================================================================
void BPather::setLongRangeSyncing(bool bEnable, long lLandFloodScarab)
{
   if ((lLandFloodScarab == cLandPath) || (lLandFloodScarab == cSquadLandPath))
      mLrpTreeLand->enableSync(bEnable);
   else if (lLandFloodScarab == cFloodPath)
      mLrpTreeFlood->enableSync(bEnable);
   else if (lLandFloodScarab == cScarabPath)
      mLrpTreeScarab->enableSync(bEnable);
   else if (lLandFloodScarab == cHoverPath && mLrpTreeHover)
      mLrpTreeHover->enableSync(bEnable);
}

//==============================================================================
// BPather::installObstructions
//==============================================================================
bool BPather::installObstructions(BBitQuadtree* tree)
{
   long sx, sz, ex, ez;
   float mx, mz, rx, rz;
   BBitQuadIterator iter;

   mObArray.setNumber(0);
   long obsType;

   while(tree->getNext(iter, sx, sz, ex, ez))
   {
      mx = (ex + sx)*0.5f;
      mz = (ez + sz)*0.5f;
      rx = (ex - sx)*0.5f;
      rz = (ez - sz)*0.5f;

      BOPObstructionNode* node = mPathParms.pObManager->getNewObstructionNode();

      mPathParms.pObManager->fillOutNonRotatedPosition(node, mx, mz, rx, rz);

      node->mType = BObstructionManager::cObsNodeTypeTerrain;
//      obsType = BObstructionManager::cObsTypeBlockWaterMovement;
      obsType = BObstructionManager::cObsTypeUnknown;
		node->mProperties |=	BObstructionManager::cObsPropertyUpdatePatherQuadTree;

      mPathParms.pObManager->installObjectObstruction(node, obsType);

      mObArray.add(node);
   }
   
   return(true);
}

//==============================================================================
// BPather::removeObstructions
//==============================================================================
bool BPather::removeObstructions(BBitQuadtree* /*tree*/)
{
   long count = mObArray.getNumber();
   for (long idx=0; idx<count; idx++)
      mPathParms.pObManager->deleteObstruction(mObArray[idx]);

   mObArray.setNumber(0);
   return(true);
}


/*
//==============================================================================
// BPather::saveLRPTree
//==============================================================================
void BPather::saveLRPTree(const char* terrainName)
{
   mLrpTreeLand->saveECFLRP(terrainName);
}
*/

//==============================================================================
// BPather::loadLRPTree
//==============================================================================
bool BPather::loadLRPTree(const char* terrainName)
{
   SCOPEDSAMPLE(BPather_loadLRPTree)
   mbLRPTreeLoaded = true;
   
   BDEBUG_ASSERT(mLrpTreeLand);

   if (!mLrpTreeLand->loadECFLRP(cLandPassECFChunkID, terrainName))
      mbLRPTreeLoaded = false;

   if (!gConfig.isDefined(cConfigAlpha))
   {
      BDEBUG_ASSERT(mLrpTreeFlood);
      if (!mLrpTreeFlood->loadECFLRP(cFloodPassECFChunkID, terrainName))
         mbLRPTreeLoaded = false;

      BDEBUG_ASSERT(mLrpTreeScarab);
      if (!mLrpTreeScarab->loadECFLRP(cScarabPassECFChunkID, terrainName))
         mbLRPTreeLoaded = false;
   }

   return (mbLRPTreeLoaded);
}


//==============================================================================
// BPather::findUnObstructedPoint_3
// This routine assumed the vPoint is already obstructed.
// DLM - this routine is now public.  As such, it uses the publig gObsManager
// singleton, insead of mPathParms.pObManager (which should always be the same
// damn thing anyway).  It also ASSUMES though that the obstruction manager
// has been initialized.  If not, it returns false right away. 
//==============================================================================
bool BPather::findUnobstructedPoint_3(const BVector &vPoint, BVector &vAdjPoint)
{

   // Make sure the obstruction manager has been set up..
   if (!gObsManager.inUse())
      return false;

   // If I'm inside any obstructions, then gather up the obstructions around me within
   // an arbitray distance, create the nonconvex hull(s) from that set of obstructions,
   // and and find the closest point inside that hull, and move off of it.  
   sScratchPoints[0].set(vPoint.x - cFindUnobstructedLimit, 0, vPoint.z - cFindUnobstructedLimit);
   sScratchPoints[1].set(vPoint.x - cFindUnobstructedLimit, 0, vPoint.z + cFindUnobstructedLimit);
   sScratchPoints[2].set(vPoint.x + cFindUnobstructedLimit, 0, vPoint.z + cFindUnobstructedLimit);
   sScratchPoints[3].set(vPoint.x + cFindUnobstructedLimit, 0, vPoint.z - cFindUnobstructedLimit);

   BConvexHull tempHull;
   tempHull.initialize(sScratchPoints, 4, true);
   #ifdef DEBUGGRAPH_FINDUNOBSTRUCTED_3
   debugAddHull(BDebugPrimitives::cCategoryPathing, tempHull, cDWORDBlack);
   #endif   
   gObsManager.findObstructions(tempHull, false, false, mObArray);
   if (mObArray.getNumber() == 0)
      return false;

   BNonconvexHull tempNCH;
   tempNCH.initialize(mObArray, &gObsManager);
   long nSegIndex = -1;
   long nHullIndex = -1;
   float fDistSqr = 0.0f;
   BVector vTempPoint = tempNCH.findClosestPointOnHull(vPoint, &nSegIndex, &nHullIndex, &fDistSqr);
   BVector vDirection;
   // If we're on the hull, push off perp to segment.  If we're on vertex, we're probably screwed. 
   if (fDistSqr < cFloatCompareEpsilon)
   {
      if (tempNCH.isVertex(vPoint))
         tempNCH.computePerpAtPoint(nSegIndex, vDirection.x, vDirection.z, nHullIndex);
      else
         tempNCH.computePerpToSegment(nSegIndex, vDirection.x, vDirection.z, nHullIndex);
   }
   else
      // Push off a bit..
      vDirection = vTempPoint - vPoint;

   float fMag = vDirection.length();
   
   fMag += cFindUnobstructedNudge;
   vDirection.safeNormalize();
   vAdjPoint = vPoint + (vDirection * fMag);

   // Check this final point against obstructions.  If still blocked, return fail.
   gObsManager.findObstructions(vAdjPoint,false, false, mObArray);

   #ifdef DEBUGGRAPH_FINDUNOBSTRUCTED_3
   debugAddNch(BDebugPrimitives::cCategoryPathing, tempNCH, cDWORDRed);
   debugAddPoint(BDebugPrimitives::cCategoryPathing, vPoint, cDWORDGreen);
   debugAddPoint(BDebugPrimitives::cCategoryPathing, vAdjPoint, cDWORDBlue);
   #endif

   if (mObArray.getNumber())
      return false;

   return true;

}

#ifdef _PATH_3


//==============================================================================
// BPather::findLowLevelPath_3
// Hey it's a new decade and time for a Low Level Pather.
// findLowLevelPath3 makes more extensive use of BNonconvexhull's cabilities
// to tightly manage the collection of convex and nonconvex hulls we have to
// deal with.  Wee! 
//==============================================================================
long BPather::findLowLevelPath_3(const BVector &vStart, 
                                const BVector &vGoal, const float fRange, BPath *path)
{
   // If we're not initalized, we can return right away. 
   if (!mbInitialized)
      return BPath::cError;

   #ifdef DEBUG_TIMING_STATS
   ++mlLLCalls;
   BTimer localTimer;
   localTimer.start();
   #endif

   #ifdef DEBUG_FINDLOWLEVELPATH_3
   debug("Entering findLowLevelPath_3");
   debug("   Init Start, Goal: (%f, %f) (%f, %f)", vStart.x, vStart.z, vGoal.x, vGoal.z);
   #endif

   // Some of the old we want to keep..    
   long pathResult = BPath::cFailed;
   long lExpansionAttempts = 0L;
   bool bExpansionsAllowed = true;

   // Reset BNonconvexHull nonCached cache.. 
   BNonconvexHull::resetNonCached();

   #ifndef BUILD_FINAL
   debugClearRenderLines(BDebugPrimitives::cCategoryPathing);
   #endif

   // Clear the obArray
   mObArray.setNumber(0);

   // Do Start and Goal checks here, and clear them
   // from obstructions.
   BVector vAdjStart(vStart);
   BVector vAdjGoal(vGoal);
   bool bAdjGoalInRange = true;

   // See if we need a new start.
   // Find obstructions I'm on.  Use expanded obstructions for test.
   mObArray.setNumber(0);
   mPathParms.pObManager->findObstructions(vStart, false, false, mObArray);
   long lNumObs1 = mObArray.getNumber();
   if (lNumObs1)
   {
      #ifdef DEBUG_FINDLOWLEVELPATH_3
      debug("   Finding new start...");
      #endif

      bool bAdjusted = findUnobstructedPoint_3(vStart, vAdjStart);
      if (!bAdjusted)
      {
         #ifdef DEBUG_FINDLOWLEVELPATH_3
         debug("   Unable to find unobstructed start.");
         debug("Exiting findLowLevelPath_3");
         #endif
         #ifdef DEBUG_TIMING_STATS
         localTimer.stop();
         mfLLTime += localTimer.getElapsedMilliseconds();
         #endif
         return BPath::cFailed;
      }
      #ifdef DEBUG_FINDLOWLEVELPATH_3
      debug("   Adjusted Start:(%f, %f)", vAdjStart.x, vAdjStart.z);
      #endif
   }

   // Do it again for the goal..
   mObArray.setNumber(0);
   mPathParms.pObManager->findObstructions(vGoal, false, false, mObArray);
   lNumObs1 = mObArray.getNumber();
   if (lNumObs1)
   {
      #ifdef DEBUG_FINDLOWLEVELPATH_3
      debug("   Finding new goal...");
      #endif
      bool bAdjusted = findUnobstructedPoint_3(vGoal, vAdjGoal);
      if (bAdjusted)
      {
         #ifdef DEBUG_FINDLOWLEVELPATH_3
         if (bAdjusted)
            debug("   New vGoal is now: (%f, %f)", vAdjGoal.x, vAdjGoal.z);
         #endif

         // Use the real start here, instead of the adjusted start.
         float fDist1 = vGoal.xzDistance(vAdjGoal);
         float fDist2 = vGoal.xzDistance(vStart);
         if (fDist1 > fDist2)
         {
            #ifdef DEBUG_FINDLOWLEVELPATH_3
            debug("   Couldn't find a new vGoal that was any closer than my current vGoal.  Returning InRangeAtStart.");
            debug("Exiting findLowLevelPath");
            #endif
            #ifdef DEBUG_TIMING_STATS
            localTimer.stop();
            mfLLTime += localTimer.getElapsedMilliseconds();
            #endif
            return BPath::cInRangeAtStart;
         }
         // If we adjusted the goal, and the adjusted goal is out of range of the original goal
         // and we requested fullPath only, then we should return now. 
         // Determine if the new goal is "in range" of the original goal.  If it's not, and we need "Full Path",
         // then we can bail now.  dlm 8/19/02
         bAdjGoalInRange = true;
         if (mPathParms.lTargetID != -1L)
         {
            BUnit *pTarget = gWorld->getUnit(mPathParms.lTargetID);
            if (pTarget)
            {  
               // Check range to target.
               if(mPathParms.pEntity->calculateXZDistance(vAdjGoal, pTarget) > fRange)
                  bAdjGoalInRange = false;
            }
            else
            {
               // Check range to goal point.
               if(mPathParms.pEntity->calculateXZDistance(vAdjGoal, vGoal) > fRange)
                  bAdjGoalInRange = false;
            }
         }
         else
         {
            // Check range to goal point.
            if(mPathParms.pEntity->calculateXZDistance(vAdjGoal, vGoal) > fRange)
               bAdjGoalInRange = false;
         }
         if(mPathParms.fullPathOnly && !bAdjGoalInRange)
         {
            #ifdef DEBUG_FINDLOWLEVELPATH_3
            debug("   We wanted full paths only, but we found found an obstructed goal and it's not in range, so bailing out now....");
            debug("Exiting findLowLevelPath");
            #endif

            #ifdef DEBUG_TIMING_STATS
            localTimer.stop();
            mfLLTime += localTimer.getElapsedMilliseconds();
            #endif
            return BPath::cFailed;
         }
      }
      #ifdef DEBUG_FINDLOWLEVELPATH_3
      else
      {
         debug("   Unable to find unobstructed goal.");
      }
      #endif
   }

   // Get min/max tiles.
   float tileSize = gTerrainSimRep.getDataTileScale();
   float fMinX = 0.0f, fMaxX = 0.0f, fMinZ = 0.0f, fMaxZ = 0.0f;

   fMaxX = gTerrainSimRep.getNumXDataTiles() * tileSize;
   fMaxZ = gTerrainSimRep.getNumXDataTiles() * tileSize;

   if (vAdjGoal.x < fMinX)
      vAdjGoal.x = fMinX;
   if (vAdjGoal.z < fMinZ)
      vAdjGoal.z = fMinZ;
   if (vAdjGoal.x >= fMaxX)
      vAdjGoal.x = fMaxX - 0.1f;
   if (vAdjGoal.z >= fMaxZ)
      vAdjGoal.z = fMaxZ - 0.1f;

   if (vAdjStart.x < fMinX)
      vAdjStart.x = fMinX;
   if (vAdjStart.z < fMinZ)
      vAdjStart.z = fMinZ;
   if (vAdjStart.x >= fMaxX)
      vAdjStart.x = fMaxX - 0.1f;
   if (vAdjStart.z >= fMaxZ)
      vAdjStart.z = fMaxZ - 0.1f;

   // Failsafe to see if Adj. Start & Adj. Goal have ended up being the same.  If so,
   // just return failed now.  
   if (_fabs(vAdjStart.x - vAdjGoal.x) < cFloatCompareEpsilon &&
      _fabs(vAdjStart.z - vAdjGoal.z) < cFloatCompareEpsilon)
   {
      #ifdef DEBUG_FINDLOWLEVELPATH_3
      debug("   Adjusted Start & Adjusted Goal ended up being the same.  Returning full path found.");
      debug("Exiting findLowLevelPath");
      #endif
      #ifdef DEBUG_PATHSYNC
      syncPathingCode("Adjusted Start & Adjusted Goal ended up being the same.  Returning full path found.");
      #endif
      #ifdef DEBUG_TIMING_STATS
      localTimer.stop();
      mfLLTime += localTimer.getElapsedMilliseconds();
      #endif

      mTempPath.zeroWaypoints();
      mTempPath.addWaypointAtEnd(vAdjGoal);
      pathResult = BPath::cFull;
      bExpansionsAllowed = false;
   }

   const float fExpansionDelta = Math::Max(8.0f, (mPathParms.fRadius * 8.0f));
   float fExpansionBase =  Math::Max(8.0f, (mPathParms.fRadius * 8.0f));

   #ifndef BUILD_FINAL
   mLLFullCalls++;
   #endif

   while (bExpansionsAllowed)
   {
      #ifdef DEBUG_FINDLOWLEVELPATH_3
      debug("   Hulling Area Expansion #%d", lExpansionAttempts + 1);
      #endif

      #ifdef DEBUG_PATHSYNC
      syncPathingCode("Hulling Area...");
      #endif

      // This will be set to true if we really need to expand..
      bExpansionsAllowed = false;

      // Build Hulled Area.
      BVector vMajorAxis = vAdjGoal - vAdjStart;
      vMajorAxis.y = 0.0f;
      float fLength = vMajorAxis.length();

      BVector vForward = vMajorAxis;
      vForward.normalize();

      BVector vRight(vForward.z, 0.0f, -vForward.x);

      float useLen = 0.5f*fLength + fExpansionBase;
      vRight *= useLen;

      sScratchPoints[0] = vAdjStart - useLen*vForward + vRight;
      sScratchPoints[1] = vAdjStart - useLen*vForward - vRight;
      sScratchPoints[2] = vAdjGoal + useLen*vForward - vRight;
      sScratchPoints[3] = vAdjGoal + useLen*vForward + vRight;
      mHulledArea.initialize(sScratchPoints, 4, true);

      #ifdef DEBUGGRAPH_HULLEDAREA
      debugAddHull(BDebugPrimitives::cCategoryPathing, mHulledArea, cColorHulledArea);
      #endif

      // And here.. we begin to depart significantly from the old code.  
      // Get the list of obstructions inside the hulled area.. 
      mPathParms.pObManager->findObstructions(mHulledArea, false, false, mObArray);
      mMasterHull.initialize(mObArray, mPathParms.pObManager);

      // This is the code to convert all of the nch's to convexhulls.  It is currently not working correctly.  Turned off until we
      // get more time to work on this, or can get it worked arouned entirely in nonconvexhull2.cpp
      /*
      long nStartHull = -1;
      long nGoalHull = -1;
      mMasterHull.insideConvexHull(vAdjStart, nStartHull);
      mMasterHull.insideConvexHull(vAdjGoal, nGoalHull);
      for (long n = 0; n < mMasterHull.getHullCount(); n++)
      {
         if (n != nStartHull && n != nGoalHull)
            mMasterHull.convertHullToConvex(n);
      }
      */

      #ifdef DEBUGGRAPH_MASTERHULL_3
      debugAddNch(BDebugPrimitives::cCategoryPathing, mMasterHull, cColorPostNchs);
      #endif

      // Figure out if the start is inside of a HOLE.  If so, then we're doing an interior path..
      bool interior = false;
      long lStartIdx = -1;
      long lGoalIdx = -1;
      if (mMasterHull.insideHole(vAdjStart, lStartIdx))
         interior = true;      
      else
      {
         // This doesn't duplicate exactly what the old pather did.  Here, we're going to just use the nonconvex hulls already created,
         // and path around those.  To duplicate exactly the old pather's behaviour, we'd revert convert each individual nonconvex hull
         // into it's convex counterpart, *except* for the hulls who's convex counterpart contained either the start or the goal -- those
         // we'd leave as nonconvex.  Let's try the simpler thing first.

         // Neither the start or the goal should by now be inside any of the created nonconvexhull's.  So if one is, then return fail.
         if (mMasterHull.inside(vAdjStart, lStartIdx))
         {
            #ifdef DEBUG_FINDLOWLEVELPATH_3
            debug("   Even after finding an unobstructed start, we detected that the start was still inside of an NCH. Returning cFailed.");
            debug("Exiting findLowLevelPath");
            #endif
            #ifdef DEBUG_PATHSYNC
            syncPathingCode("   Even after finding an unobstructed start, we detected that the start was still inside of an NCH. Returning cFailed.");
            #endif
            #ifdef DEBUG_TIMING_STATS
            localTimer.stop();
            mfLLTime += localTimer.getElapsedMilliseconds();
            #endif
            return BPath::cFailed;
         }
      }
      // It's worth while to see if the goal is inside of a hole as well (though it too should be), as
      // the pather will use that information to determine if it should quit
      if (!mMasterHull.insideHole(vAdjGoal, lGoalIdx))
      {
         mMasterHull.inside(vAdjGoal, lGoalIdx);
      }

      // Run the path.. 
      pathResult = postHullPather_3(vAdjStart, vAdjGoal, vGoal, fRange, &mTempPath, 
         lStartIdx, lGoalIdx, interior);

      //pathResult = BPath::cOutsideHulledAreaFailed;
      // If we got outside the hull, the expand and try again, if allowed.
      if (pathResult == BPath::cOutsideHulledAreaFailed)
      {
         if (++lExpansionAttempts <= clAllowedExpansions)
         {
            #ifdef DEBUG_FINDLOWLEVELPATH_3
            debug("   Returned cOutsideHulledAreaFailed.  Rehulling and trying again.");
            #endif
            bExpansionsAllowed = true;
            fExpansionBase += fExpansionDelta;

            #ifndef BUILD_FINAL
            mLLExpansions++;
            #endif
         }
      }
   } // while expansionsallowed

   // Fix up the back path.. 
   if (pathResult == BPath::cFull || pathResult == BPath::cInRangeAtStart || pathResult == BPath::cPartial || pathResult == BPath::cOutsideHulledAreaFailed)
   {
      if (lNumObs1)
         mTempPath.addWaypointAtEnd(vStart);

      #ifdef DEBUG_FINDLOWLEVELPATH_3
      debug("   Final Stitched Path:");
      for (long k = 0; k < mTempPath.getNumberWaypoints(); k++)
      {
         BVector vPoint = mTempPath.getWaypoint(k);
         debug("   Waypoint %d: (%f, %f)", k, vPoint.x, vPoint.z);
      }
      #endif

      #ifdef DEBUG_PATHSYNC
      // Syncotron
      syncPathingCode("Final Stitched Path:");
      for (long k1 = 0; k1 < mTempPath.getNumberWaypoints(); k1++)
      {
         BVector vPoint = mTempPath.getWaypoint(k1);
         syncPathingData("Waypoint", vPoint);
      }
      #endif

      if (pathResult != BPath::cOutsideHulledAreaFailed && (fabs(vAdjGoal.x - vGoal.x) > cFloatCompareEpsilon ||
         fabs(vAdjGoal.z - vGoal.z) > cFloatCompareEpsilon))          
      {
         // If we Adjusted the goal, and got a full path back, we should check to see if we want to
         // actually change the path from full to partial.
         // This fixes a problem with attacking units not getting within in range,
         // but we still want to say they got a complete attack.  We should probably
         // find a *better* way to do this, as this is too expsensive to do every path.         

         // This check is now done before the path is run, and the results are saved in bAdjGoalInRange.  We
         // can do this because the postHullPather no longer modifies the goal passed to it.  dlm 8/19/02
         #ifdef DEBUG_FINDLOWLEVELPATH_3
         debug("   Determining if adjusted path should be full or partial...");
         #endif

         #ifdef DEBUG_PATHSYNC
         syncPathingCode("Determining if adjusted path should be full or partial...");
         #endif

         if (!bAdjGoalInRange)
         {
            pathResult = BPath::cPartial;
            #ifdef DEBUG_FINDLOWLEVELPATH_3
            debug("   Set Full Path to Partial...");
            #endif
         }
         // If the path we got back was partial, see if the final spot we found was within range of
         // our original goal.. and if so, upgrade to full. dlm 9/22/02
         if (pathResult == BPath::cPartial && mTempPath.getNumberWaypoints() > 1 && mPathParms.pEntity != NULL)
         {
            // Path hasn't been assembled yet, (ie., path is backwards) so goal point is first point in path.. 
            BVector vTestPos = mTempPath.getWaypoint(0);
            if (mPathParms.lTargetID != -1L)
            {
               BUnit *pTarget = gWorld->getUnit(mPathParms.lTargetID);
               if (pTarget)
               {  
                  // Check range to target.
                  if(mPathParms.pEntity->calculateXZDistance(vTestPos, pTarget) <= fRange)
                     pathResult = BPath::cFull;
               }
               else
               {
                  // Check range to goal point.
                  if(mPathParms.pEntity->calculateXZDistance(vTestPos, vGoal) <= fRange)
                     pathResult = BPath::cFull;
               }
            }
            else
            {
               // Check range to goal point.
               if(mPathParms.pEntity->calculateXZDistance(vTestPos, vGoal) <= fRange)
                  pathResult = BPath::cFull;
            }
            #ifdef DEBUG_FINDLOWLEVELPATH_3
            debug("   Upgraded Partial Path to Full as final point found was within range of goal...");
            #endif
         }
      }

      if (mTempPath.getNumberWaypoints() > 1)
      {
         path->zeroWaypoints();
         assemblePath(&mTempPath, path, pathResult);
      }
      else
         pathResult = BPath::cFailed;
   }

   // drop the fully assembled path into the debug cache... 
   #ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigDebugPather))
   {
      mDebugPath.zeroWaypoints();
      for (int n = 0; n < path->getNumberWaypoints(); n++)
      {
         mDebugPath.addWaypointAtEnd(path->getWaypoint(n));
      }       
   }
   #endif

   #ifdef DEBUG_TIMING_STATS
   localTimer.stop();
   mfLLTime += localTimer.getElapsedMilliseconds();
   #endif

   #ifdef DEBUG_FINDLOWLEVELPATH_3
   debug("   Final Start, Goal:(%f, %f) (%f, %f)", vAdjStart.x, vAdjStart.z, vAdjGoal.x, vAdjGoal.z);
   debug("   Pathing Complete Returning: %d", pathResult);
   debug("Exiting findLowLevelPath\n");
   #endif

   #ifdef DEBUG_PATHSYNC
   syncPathingCode("<---- findLowLevelPath2");
   #endif

   return pathResult;
}

//==============================================================================
// BPather::postHullPather_3
// The kinder, gentler posthullpather.
// More importantly, this one's designed to work by iterating over the hulls
// of a *single* BNonconvexHull - mMasterHull.
// We're taking out (for now) all the special "on-edge" special case handling.
// I'm sure I'll have to put some version of this back in.  But I want it to 
// be cleaned up anyway.
//==============================================================================
long BPather::postHullPather_3(const BVector &vStart, const BVector &vGoal, const BVector &vOrigGoal, float fRange, 
                             BPath *path, long lStartInsideIdx, long lGoalInsideIdx, bool interior)
{


   #ifdef DEBUG_POSTHULLPATHER_3
   debug("   ================");
   debug("   Entering postHullPather_3");
   debug("      Start: (%f, %f)", vStart.x, vStart.z);
   debug("      Goal:  (%f, %f)", vGoal.x, vGoal.z);
   debug("      Range:  %f", fRange);
   debug("      lStartInsideIdx: %d", lStartInsideIdx);
   debug("      lGoalInsideIdx:  %d", lGoalInsideIdx);
   debug("      interior = %d", interior);
   #endif

   #ifdef DEBUG_PATHSYNC
   syncPathingCode("----> postHullPather");
   #endif


   // Initialize
   BVector vIPoint(cOriginVector);
   long lSegment = -1L;
   long lHull = 0;
   long lIntersectHullIdx = -1L;
   float fBestDistSqr = vStart.xzDistanceSqr(vGoal);
   BPathNode2 *pnodeBest = NULL;
   bool bGotToGoal = false;
   bool bDone = false;
   long lIterations = 0L;
   bool bOutsideHulledArea = false;
   bool bReachedHull = false;

   // Init Queues
   mQueues->initialize(vStart, vGoal);

   // Init Path
   path->zeroWaypoints();

   // Put the Start Node on the Queue...
   BPathNode2 *pnodeCurr = NULL;

   pnodeCurr = mQueues->addOpenNode(vStart, NULL, -1L, -1L, false);
   if(!pnodeCurr)
   {
      BFAIL("Failed to create initial node");
      return BPath::cError;
   }

   #ifdef DEBUG_PATHSYNC
   syncPathingData("addOpenNode (vStart)", vStart);
   #endif

   #ifdef DEBUG_POSTHULLPATHER_3
   if (pnodeCurr)
      debug("      Added Start (%f, %f) to Open List.", vStart.x, vStart.z);
   else
   {
      debug("      Failed to add Start (%f, %f) to Open List.", vStart.x, vStart.z);
      BASSERT(0);
   }
   #endif


   // Okay.. Path to the goal!
   #ifdef DEBUG_POSTHULLPATHER_3
   debug("      Beginning Node Search..");
   #endif
   while(!bDone)
   {
      #ifdef DEBUG_POSTHULLPATHER_3
      debug(" ");
      debug("      Getting next open node..");
      #endif

      pnodeCurr = mQueues->getOpenNode();
      if (!pnodeCurr)
      {
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      No open nodes in List.  Done searching.");
         #endif
         bDone = true;
         continue;
      }
      else
         // Put it on the closed list.
         mQueues->addClosedNode(pnodeCurr);

      #ifdef DEBUG_POSTHULLPATHER_3
      debug("      Next Node -- Point (%f, %f) Cost: %f Estimate: %f Total: %f", pnodeCurr->mvPoint.x, pnodeCurr->mvPoint.z, pnodeCurr->mfCost, pnodeCurr->mfEstimate,
         pnodeCurr->mfCost + pnodeCurr->mfEstimate);
      debug("      Hull Index: %d  Point Index: %d  Intersect: %s", pnodeCurr->mlHullIndex, pnodeCurr->mlPointIndex,
         (pnodeCurr->mbIsIntersection?"true":"false"));
      #endif

      #ifdef DEBUGGRAPH_POSTHULLPATHER_3
      if(pnodeCurr->mParent)
         gTerrainSimRep.addDebugLineOverTerrain(pnodeCurr->mParent->mvPoint, pnodeCurr->mvPoint, cColorNchPather, cColorNchPather, 0.75f, BDebugPrimitives::cCategoryPathing);
      #endif

      // If the node we pulled off is outside the hulled area, then
      // we'll need to bail.  Do this check before best node check,
      // because we don't want to put points outside the hull on the
      // path.
      if (!mHulledArea.inside(pnodeCurr->mvPoint))
      {
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Current Node (%f, %f) was outside the Hulled Area.  Bailing..",
            pnodeCurr->mvPoint.x, pnodeCurr->mvPoint.z);
         #endif
         bOutsideHulledArea = true;
         break;
      }

      // Are we in range of the goal?
      // Perform Range check against our current location.  
      float fDistToGoal = pnodeCurr->mvPoint.xzDistance(vOrigGoal);
      if (fDistToGoal < fRange)
      {
         // We're within range of the goal, we're done. 
         bGotToGoal=true;
         pnodeBest = pnodeCurr;
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Got within range of goal. Done.");
         #endif
         // Bail on the whole loop
         break;
      }

      // See if this is our best path thus far..
      float fDistSqrTest = vGoal.xzDistanceSqr(pnodeCurr->mvPoint);
      #ifdef DEBUG_POSTHULLPATHER_3
      debug("      DistSqr between curr and goal: %f", fDistSqrTest);
      #endif
      if(fDistSqrTest < fBestDistSqr)
      {
         fBestDistSqr = fDistSqrTest;
         pnodeBest = pnodeCurr;
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Assigning this to bestDistSqr, and this node to bestPath");
         #endif
      }

      // If this is an intersection node, then continue..
      if (pnodeCurr->mbIsIntersection)
      {
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      This is an intersection Node.  Continuing...");
         #endif
         continue;
      }

      // If we know the goal is inside a hull, and we're already on that hull,
      // don't even bother with a segIntersect check.  Just continue..
      // if this node is the best we've found so far.
      if (lGoalInsideIdx != -1L && (pnodeCurr->mlHullIndex == lGoalInsideIdx))
      {
         if (interior)
         {
            // If the goal index is not the same as the start index, then continue as well.
            if (lGoalInsideIdx != lStartInsideIdx)
            {
               #ifdef DEBUG_POSTHULLPATHER_3
               debug("      Not doing segIntersect check because we're on the hull that we know the goal is inside.  Continuing...");
               #endif
               continue;
            }
         }
         else
         {
            #ifdef DEBUG_POSTHULLPATHER_3
            debug("      Not doing segIntersect check because we're on the hull that we know the goal is inside.  Continuing...");
            #endif
            continue;
         }
      }

      // Returned passing of current hull and current index to segintersect checks, to
      // prevent multiple segintersect from being generated at the endpoints of 
      // segments.  DLM 11/19/01
      bool hitInterior=false;
      bool bHit;
      bHit = doSegIntersectCheck_3(pnodeCurr->mvPoint, vGoal, pnodeCurr->mlHullIndex,
         pnodeCurr->mlPointIndex, lStartInsideIdx, interior, hitInterior);

      // An interior shot to outside that doesn't hit the interior hull is invalid (probably actually right on the edge)
      if(interior && !hitInterior)
      {
         if (lGoalInsideIdx<0)
         {
            #ifdef DEBUG_POSTHULLPATHER_3
            debug("      Start is interior and segment didnt intersect interior hull, and goal is outside -- skipping.");
            #endif
            continue;
         }
         // If the goal index is not < 0, it's still only valid to move from this hull to
         // that hull if that hull is inside of my current one. 
         if (pnodeCurr->mlHullIndex != 1)
         {
            BNonconvexHull goalHull;
            goalHull.initializeFromSubhull(mMasterHull, lGoalInsideIdx);
            BNonconvexHull currHull;
            currHull.initializeFromSubhull(mMasterHull, pnodeCurr->mlHullIndex);
            if ((pnodeCurr->mlHullIndex != lGoalInsideIdx) && (!(currHull.isInside(goalHull))))
            {
               #ifdef DEBUG_POSTHULLPATHER_3
               debug("      Start is interior and segment didn't intersect interior hull, and goal not inside our hull -- skipping.");
               #endif
               continue;
            }
         }

         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Start is interior and segment didnt intersect interior hull, BUT  goal hull is inside our hull -- using.");
         #endif

      }


      #ifdef DEBUG_POSTHULLPATHER_3
      debug("      doSegIntersectResults:");
      for (long q = 0; q < mIntersectResults.getNumber(); q++)
         debug("        %d\tfDistSqr: %8.5f lHullIdx: %03d lSegIdx: %03d vIPnt: (%8.5f, %8.5f)",
         q, mIntersectResults[q].fDistSqr, mIntersectResults[q].lHullIdx, mIntersectResults[q].lSegmentIdx,
         mIntersectResults[q].vIntersect.x, mIntersectResults[q].vIntersect.z);
      debug(".");
      #endif

      // If we didn't get a hit.. or we got a single hit with a hull we don't care about, then
      // we are done.  
      if (!bHit)
      {
         // dlm 9-5-02 -- HACK.  Last ditch effort to avoid the case where even though we have no intersections returned, the path
         // takes us *through* the non convex hull.  Do a "real" seg intersect check between our current point and the goal
         // against relaxed obstructions.  If this fails, then we're not really done.
         bool bHitObs = false;
         bHitObs = mPathParms.pObManager->segmentIntersects(pnodeCurr->mvPoint, vGoal, true);
         if (!bHitObs)
         {
            bGotToGoal = true;
            pnodeBest = mQueues->newNode(vGoal, pnodeCurr);
            if (!pnodeBest)
            {
               BASSERT(0);
               break;
            }
            #ifdef DEBUGGRAPH_POSTHULLPATHER_3
               gTerrainSimRep.addDebugLineOverTerrain(pnodeCurr->mvPoint, 
                  pnodeBest->mvPoint, cColorNchPather, cColorNchPather, 0.75f, BDebugPrimitives::cCategoryPathing);
            #endif

            #ifdef DEBUG_POSTHULLPATHER_3
            debug("      No Obstructions between curr and goal.  Done.");
            #endif

            // Bail on the whole loop
            break;
         }
         else
         {
            #ifdef DEBUG_POSTHULLPATHER_3
            debug("      doSegIntersectCheck returned no obstructions, but obMgr reported that we had some, so we're just continuing..");
            #endif
            continue;
         }

      }

      // Check our Iteration count, and exit if we've exceeded our limit.
      ++lIterations;

      // At each iteration, if we have timing limits turned on, check them..
      #ifdef DEBUG_PATHING_LIMITS
      if (lIterations > mlBadPathThresholdLow)
      {
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("\t\tBad Path Threshold Reached.  Done Searching. Iterations were: %ld", lIterations);
         #endif
         break;
      }
      #endif


      // Determine the correct Intersect point to check against..      
      lIntersectHullIdx = mIntersectResults[0].lHullIdx;
      vIPoint = mIntersectResults[0].vIntersect;
      lSegment = mIntersectResults[0].lSegmentIdx;
      lHull = mIntersectResults[0].lNCHIdx;

      // If the intersection point is the SAME as our current node point,
      // we need to handle it differently.
      bool bNewNode = true;

      //if (mIntersectResults[0].fDistSqr < cFloatCompareEpsilon)
      if (mIntersectResults[0].fDistSqr < 0.001f)
      {
         // If the intersection distance was zero, but we didn't discard the intersection,
         // don't attempt to add a new node at the same point, but just use the current one
         // instead.
         if (bHit)
         {
            #ifdef DEBUG_POSTHULLPATHER_3
            debug("   Current Node is same as Intersection.  Using Current Node as Parent of Hull nodes.");
            #endif
            bNewNode = false;
         }
         // Taking this special case logic out worries me the most of all.  This was in place
         // basically to allow us to move off of hulls we inadventently got an intersection
         // result on.  This special case might be the reason I have to actually go *back*
         // to collecting multiple intersection results instead of best and/or closest.  
         /*
         else
         {
            // If the closest intersection was set to zero, and we decided to
            // discard that intersection, then if we're here we must have another
            // intersection with another hull.  Use that as the intersection.
            #ifdef DEBUG_POSTHULLPATHER
            debug("\t\tFirst intersection was zero distance.  Using next Intersection as valid one.");
            #endif
            // Sanity check, just the same.
            if (mIntersectResults.getNumber() < 2)
            {
               BASSERT(0);
               continue;
            }
            lIntersectHullIdx = mIntersectResults[1].lHullIdx;
            vIPoint = mIntersectResults[1].vIntersect;
            lSegment = mIntersectResults[1].lSegmentIdx;            
            lHull = mIntersectResults[1].lNCHIdx;
         }
         */
      }

      // Before we actually create or assign the new intersect node,
      // if the intersection point was on the hull we're currently
      // pulling nodes off of, ignore it, and move on.
      if (pnodeCurr->mlHullIndex == lIntersectHullIdx)
      {
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Intersected with the current Hull.  Continuing...");
         #endif
         continue;
      }

      // Now create (or assign) the intersect node.. and then create nodes
      // from all of the vertices of the (newly) intersected hull
      BPathNode2 *pnode = NULL;
      if (bNewNode)
      {
         pnode = mQueues->addOpenNode(vIPoint, pnodeCurr, lIntersectHullIdx, -1L, true);

         // If we failed to add, it means the point was already in the list.  We have already
         // rejected intersection points at this point, so use the current node as the parent for
         // the "around the hull" points.
         if(!pnode)
         {
            #ifdef DEBUG_POSTHULLPATHER_3
            debug("      Intersection point is very close to start... using start node instead.");
            #endif

            pnode=pnodeCurr;
         }

         #ifdef DEBUG_PATHSYNC
         syncPathingData("addOpenNode (vIPoint)", vIPoint);
         #endif
      }
      else
         pnode = pnodeCurr;

      if (pnode == NULL)
      {
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Unable to add intersection point.  Continuing..");
         #endif
         continue;
      }


      #ifdef DEBUG_POSTHULLPATHER_3
      debug("      Added (or assigned) Intersection Node (%f, %f)", pnode->mvPoint.x, pnode->mvPoint.z);
      #endif


      // Walk the hull and place the points on the hull...
      long lNumPoints = mMasterHull.getPointCount(lIntersectHullIdx);
      // NumUsedPoints & NumPoints will most of the time be the same.. but if we
      // need to skip the first vertex, numUsedPoins is the number of actual points
      // we added to the open list, whereas numPoints will still be the number of 
      // points on the actual hull.
      long lNumUsedPoints = lNumPoints;
      long lHullIterations = lNumPoints >> 1;
      BPathNode2 *pnodeCw = NULL;
      BPathNode2 *pnodeCcw = NULL;
      BPathNode2 *pnodeCwParent = pnode;
      BPathNode2 *pnodeCcwParent = pnode;
      BPathNode2 *pnodeGoal = NULL;
      BVector vCwPoint(cOriginVector), vCcwPoint(cOriginVector);
      long lCwIdx = (lSegment == lNumPoints - 1)?0:lSegment + 1;
      long lCcwIdx = lSegment;

      // If we're about to add the points around the hull, check to see if the first
      // ccw point is the same as the intersection (which it would be if the intersection
      // occured at a vertex.  If so, then skip this point, and decrement the number of points.  
      vCcwPoint = mMasterHull.getPoint(lCcwIdx, lIntersectHullIdx);

      if ((_fabs(pnode->mvPoint.x - vCcwPoint.x) < cFloatCompareEpsilon) &&
         (_fabs(pnode->mvPoint.z - vCcwPoint.z) < cFloatCompareEpsilon))
      {
         lCcwIdx = (lSegment == 0)?lNumPoints - 1:lSegment - 1;
         --lNumUsedPoints;
         lHullIterations = lNumUsedPoints >> 1;
      }

      // Check the first cw point as well..
      vCwPoint = mMasterHull.getPoint(lCwIdx, lIntersectHullIdx);
      if ((_fabs(pnode->mvPoint.x - vCwPoint.x) < cFloatCompareEpsilon) &&
         (_fabs(pnode->mvPoint.z - vCwPoint.z) < cFloatCompareEpsilon))
      {
         ++lCwIdx;
         if (lCwIdx == lNumPoints)
            lCwIdx = 0;
         --lNumUsedPoints;
         lHullIterations = lNumUsedPoints >> 1;
      }

      // dlm 9/6/02 - If our goal is inside this hull, then find the closest point *to* the goal on the hull, and 
      // add that point. 
      bool bAddGoalPoint = false;
      BVector vClosestToGoalOnHull(0.0f);
      long lGoalSegment = -1;
      long lPrevGoalSegment = -1;
      if (lGoalInsideIdx != -1 && lGoalInsideIdx == lIntersectHullIdx)
      {
         BNonconvexHull intersectHull;
         intersectHull.initializeFromSubhull(mMasterHull, lIntersectHullIdx);
         vClosestToGoalOnHull = intersectHull.findClosestPointOnHull(vGoal, &lGoalSegment, NULL);
         if (lGoalSegment != -1)
         {
            lPrevGoalSegment = (lGoalSegment == 0)?3:lGoalSegment + 1;
            // If the lGoalSegment is the same as the intersection segment.. then add the node immediately.
            if (lGoalSegment == lSegment)
            {
               pnodeGoal = mQueues->addOpenNode(vClosestToGoalOnHull, pnode, lIntersectHullIdx, -1, false);
               if (pnodeGoal)
               {
                  bAddGoalPoint = false;
                  pnode = pnodeGoal;
                  pnodeCwParent = pnodeGoal;
                  pnodeCcwParent = pnodeGoal;
               }                  
            }
            else
               bAddGoalPoint = true;
         }
      }

      // Add the hull points.
      for (long n = 0; n < lNumUsedPoints; n++)
      {
         if(pnodeCwParent)
         {
            // Add CW point
            vCwPoint = mMasterHull.getPoint(lCwIdx, lIntersectHullIdx);
            pnodeCw = mQueues->addOpenNode(vCwPoint, pnodeCwParent, lIntersectHullIdx, lCwIdx, false);

            #ifdef DEBUG_PATHSYNC
            syncPathingData("addOpenNode (vCwPoint)", vCwPoint);
            #endif

            #ifdef DEBUG_POSTHULLPATHER_3
            if (pnodeCw == NULL)
               debug("      Failed to add Clockwise Point %d of Hull %d (%f, %f)", lCwIdx, lIntersectHullIdx,
                  vCwPoint.x, vCwPoint.z);
            else
               debug("      Added Clockwise Point %d         of Hull %d (%f, %f)  cost=%0.1f, estimate=%0.1f, total=%0.1f", lCwIdx, lIntersectHullIdx,
                  vCwPoint.x, vCwPoint.z, pnodeCw->mfCost, pnodeCw->mfEstimate, pnodeCw->mfCost+pnodeCw->mfEstimate);
            #endif

            // Before we advance, see if we need to insert the on Goal Point.. And if so, then do so.
            if (bAddGoalPoint && pnodeCw && (lCwIdx == lGoalSegment))
            {
               pnodeGoal = mQueues->addOpenNode(vClosestToGoalOnHull, pnodeCw, lIntersectHullIdx, -1, false);
               if (pnodeGoal)
               {
                  pnodeCw = pnodeGoal;
               }
               // Whether we succeeded or not, we made the attempt.. so don't make it again.
               bAddGoalPoint = false;
            }

            pnodeCwParent = pnodeCw;                  

            // Next CW point.
            lCwIdx = (lCwIdx == lNumPoints - 1)?0:lCwIdx + 1;
         }


         if(pnodeCcwParent)
         {
            // Add CCW point.
            vCcwPoint = mMasterHull.getPoint(lCcwIdx, lIntersectHullIdx);
            pnodeCcw = mQueues->addOpenNode(vCcwPoint, pnodeCcwParent, lIntersectHullIdx, lCcwIdx, false);

            #ifdef DEBUG_PATHSYNC
            syncPathingData("addOpenNode (vCcwPoint)", vCcwPoint);
            #endif

            #ifdef DEBUG_POSTHULLPATHER_3
            if (pnodeCcw == NULL)
               debug("      Failed to add Counter-clockwise Point %d of Hull %d (%f, %f)", lCcwIdx, lIntersectHullIdx,
                  vCcwPoint.x, vCcwPoint.z);
            else
               debug("      Added Counter-Clockwise Point %d of Hull %d (%f, %f)  cost=%0.1f, estimate=%0.1f, total=%0.1f", lCcwIdx, lIntersectHullIdx,
                  vCcwPoint.x, vCcwPoint.z, pnodeCcw->mfCost, pnodeCcw->mfEstimate, pnodeCcw->mfCost+pnodeCcw->mfEstimate);
            #endif
            // Before we advance, see if we need to insert the on Goal Point.. And if so, then do so.
            if (bAddGoalPoint && pnodeCcw && (lCcwIdx == lPrevGoalSegment))
            {
               pnodeGoal = mQueues->addOpenNode(vClosestToGoalOnHull, pnodeCcw, lIntersectHullIdx, -1, false);
               if (pnodeGoal)
               {
                  pnodeCcw = pnodeGoal;
               }
               // Whether we succeeded or not, we made the attempt.. so don't make it again.
               bAddGoalPoint = false;
            }

            pnodeCcwParent = pnodeCcw;

            // Next CCW point.
            lCcwIdx = (lCcwIdx == 0)?lNumPoints - 1:lCcwIdx - 1;
         }
      }

   } // end of while !bDone

   // Okay.. we're done.  What kind of path did we get?
   #ifdef DEBUG_POSTHULLPATHER_3
   debug("      Exited Node Search.");
   #endif

   #ifdef DEBUG_TIMING_STATS  
   mlPolyIterations += lIterations;
   if (lIterations > mlPolyIterationMax)
      mlPolyIterationMax = lIterations;
   #endif

   // If we found a path of some sort, set it up for return.   
   if(pnodeBest)
   {

      #ifdef DEBUG_POSTHULLPATHER_3
      debug("      Path Returned:");
      long lWaypoint = 0;
      #endif

      pnodeCurr = pnodeBest;
      while(pnodeCurr)
      {
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Waypoint: %d (%f, %f)", lWaypoint, pnodeCurr->mvPoint.x, pnodeCurr->mvPoint.z);
         lWaypoint++;
         #endif

         path->addWaypointAtEnd(pnodeCurr->mvPoint);
         pnodeCurr = pnodeCurr->mParent;
      }

      // Release the nodes..
      mQueues->terminate();

      if(bGotToGoal)
      {
         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Returned with FULL from postHullPather");
         #endif

         return(BPath::cFull);
      }
      else
      {
         if (bOutsideHulledArea)
         {
            #ifdef DEBUG_POSTHULLPATHER_3
            debug("      Returned with OUTSIDEHULLEDAREAFAILED from postHullPather");
            #endif
            return(BPath::cOutsideHulledAreaFailed);
         }

         if (bReachedHull)
         {

            #ifdef DEBUG_POSTHULLPATHER_3
            debug("      Returned with REACHEDHULL from postHullPather");
            #endif
            return(BPath::cReachedHull);
         }

         #ifdef DEBUG_POSTHULLPATHER_3
         debug("      Returned with PARTIAL from postHullPather");
         #endif
         return(BPath::cPartial);
      }
   }

   // Release the nodes..
   mQueues->terminate();

   if (bOutsideHulledArea)
   {
      #ifdef DEBUG_POSTHULLPATHER
      debug("      Returned with OUTSIDEHULLEDAREAFAILED from postHullPather");
      #endif

      return BPath::cOutsideHulledAreaFailed;
   }

   #ifdef DEBUG_POSTHULLPATHER
   debug("      Returned with FAILED from postHullPather");
   #endif

   return BPath::cFailed;

}


#endif // _PATH_3

//==============================================================================
// eof: pather.cpp
//==============================================================================

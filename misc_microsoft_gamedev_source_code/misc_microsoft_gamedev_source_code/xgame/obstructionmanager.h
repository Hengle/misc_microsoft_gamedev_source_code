//=============================================================================
// Copyright (c) 1999-2007 Ensemble Studios
//
// Obstruction manager
//=============================================================================
#pragma once 



//=============================================================================
#include "convexhull.h"
#include "typedid.h"
#include "bitarray.h"

//#include <stdarg.h>

#pragma warning(disable: 4201)

//=============================================================================
class BPather;
class BOPObNodeArrayManager;
class BOPObQuadNodeListManager;
class BOPHullArrayManager;
class BOPObstructionNode;
class BObstructionManager;

class BWorld;
class BUnit;
class BEntity;
class BDopple;
class BTerrainBase;
class BPlayer;

const float cDefaultRadiusSofteningFactor = 0.8f;
const float cfWaterDepthForObstructions = 1.0f;


typedef bool (*CHECKUNIT_CALLBACK)(const BEntity *unit, void *param1);

//=============================================================================
// initObstructionRotationTables
// 
// This initializes a table for the quantized rotations used in the obstruction
// manager.  This should be called at startup.
//=============================================================================
void initObstructionRotationTables(void);


//=============================================================================
// Data Structure used for storing up and passing around the
// results of a segIntersectCheck.
class BSegIntersectResult
{
   public:
      float                   fDistSqr;
      BOPObstructionNode      *pObNode;
      long                    lSegmentIdx;
      BVector                 vIntersect;
};

typedef BDynamicSimArray<BSegIntersectResult> BSegIntersectResultArray;



//=============================================================================
class BSaveObsManagerState
{
	public:
		float							mSRadius;								// Radius to Expand By
		float							mSRelaxedRadius;						// "releaxed" version of that radius
		long							mSMode;									// "Type" of Begin that was called

		long							mSQuadTreestoScan;
		long							mSValidNodeTypes;
		long							mSPlayerID;                      // changing this to plain player ID, which will get converted to the appropriate player mask.

      // jce [9/24/2008] -- Converted to instance to avoid saving stale passed-in pointers
		BEntityIDArray          mSEntityIgnoreList;					// BTypedID array of units to ignore
};

//=============================================================================
// Another struct for wall checks.
class BWallCheckResult
{
   public:
                              BWallCheckResult(void) {};
                              BWallCheckResult(const BVector &v, float dist, BOPObstructionNode *pOb, bool in) :
                                 mPoint(v), mDist(dist), mpObstruction(pOb), mIn(in) {};

   BVector                    mPoint;
   float                      mDist;
   BOPObstructionNode         *mpObstruction;
   bool                       mIn;
};
typedef BDynamicSimArray<BWallCheckResult> BWallCheckResultArray;


// =============================================================================
// =============================================================================
// =============================================================================
// =============================================================================
// =============================================================================

struct BOPPoint
{
   float    mX;
   float    mZ;
};


// =============================================================================

class BOPQuadHull
{

   public:
      bool                    segmentIntersects(const BVector &point1, const BVector &point2, const long ignorePoint,
                                 BVector &iPoint, long &segmentIndex, float &distanceSqr, long &lNumIntersections,
                                 bool checkBBox, bool bCheckInside = true, float ignoreDistSqr=0.0f) const;
      bool                    segmentIntersects(const BVector &point1, const BVector &point2, bool checkBBox) const;
      bool                    rayIntersects(const BVector &point, const BVector &vector, BVector &iPoint, long *segmentIndex, 
                                 float *distSqr) const;
      bool                    rayIntersectsFar(const BVector &point, const BVector &vector, BVector &iPoint, long *segmentIndex, 
                                 float *distSqr) const;
      BVector                 findClosestPointOnHull(const BVector &vStart, long *plSegmentIndex, float *pfClosestDistSqr) const;
      void                    computePerpToSegment(long segIndex, float &x, float &z) const;

      bool                    overlapsBox(const float minX, const float minZ, const float maxX, const float maxZ) const;
      bool                    overlapsHull(const BConvexHull &hull, float errorEpsilon=0.0f) const;
      bool                    overlapsHull(const BOPQuadHull *hull, float errorEpsilon=0.0f) const;
      bool                    checkSide(float dx, float dz, float vx, float vz, float errorEpsilon) const;

      bool                    inside(const BVector &point) const;

      long                    minMaxSegIntersect(const BVector &point1, const BVector &point2, 
                                 BVector &minPoint, BVector &maxPoint) const;

      float                   distance(const BVector &point) const;
      float                   distanceSqr(const BVector &point) const;
      float                   distance(const BOPQuadHull *hull) const;
      float                   distanceSqr(const BOPQuadHull *hull) const;
      float                   distance(const BConvexHull &hull) const;
      float                   distanceSqr(const BConvexHull &hull) const;

		bool							isRotated(void) const		{return ((mRotation & 0x03F) != 0); };

      void                    computeCenter(BVector &center) const;

      inline float            getMinX(void) const {return(mDirectVal[mIdxMinX]);}
      inline float            getMaxX(void) const {return(mDirectVal[mIdxMaxX]);}
      inline float            getMinZ(void) const {return(mDirectVal[mIdxMinZ]);}
      inline float            getMaxZ(void) const {return(mDirectVal[mIdxMaxZ]);}

      void                    expandFrom(const BOPQuadHull *hull, float expansionRadius);
      bool                    suggestPlacement(const BVector vLocation, float fTestRadiusX, float fTestRadiusZ, BVector &vSuggestion) const;

      bool                    createSimpleHull(const BVector* points);


		// 48 bytes of packed data goodness

	   BOPQuadHull*				mNextNode;					// Pointer for linking nodes in the free node list

		float							mRadius;						// Radius the hull has been expanded by

		BYTE							mUnused1;	
		BYTE							mRotation;					// Angle of rotation for the hull
		WORD							mSequence;					// Sequence # for recycling nodes....


		union
		{
			struct
			{
				BYTE             mIdxMinX;						// DirectVal[] index # of Minimum X Coordinate
				BYTE             mIdxMinZ;						// DirectVal[] index # of Minimum Z Coordinate
				BYTE             mIdxMaxX;						// DirectVal[] index # of Maximum X Coordinate
				BYTE             mIdxMaxZ;						// DirectVal[] index # of Maximum Z Coordinate
			};
			long						mBoundingIndicies;		// format to access all bounding indicies at once
		};


		// Storage for the actual obstruction data

      union
      {
         struct 
         {
            float             mDirectVal[8];          // Direct acces to float values
         };
         struct 
         {
            BOPPoint          mPoint[4];              // Indexed Point Access
         };
         struct 
         {
            float             mX1, mZ1;               // 1st Point
            float             mX2, mZ2;               // 2nd Point
            float             mX3, mZ3;               // 3rd Point
            float             mX4, mZ4;               // 4th Point
         };
      };




}; // BOPQuadHull 


// =============================================================================

class BOPObstructionNode     // Data only, no methods
{
   public:

      // 1st 16-byte block

      union
      {
         BEntity*					mObject;						// a 4-byte pointer to the Unit
		   BDopple*					mDopple;						// a 4-byte pointer to the Dopple
         BOPObstructionNode*  mNextNode;					// Pointer for linking nodes in the free node list
      };

      long                    mThisNodeIndex;			// Index of QuadTree Record that this record is listed in
      long                    mThisNodePointerIndex;	// List Index position in quadTree Record that points ot this record
	   long							mThisNodeQuadTree;		// Index # of QuadTree this record has been assigned to

      // 2nd 16-byte block

      BOPQuadHull*            mExpandedHull;				// Pointer to epxanded hull coordinates

	   long							mEntityID;					// ID of the Unit or Dopple

	   union
	   {
		   struct
		   {
			   BYTE              mType;						// Type of data node represents, used to decode unit/entity pointer
			   BYTE              mRotation;					// Rotation of Obstruction
			   BYTE              mPlayerID;					// ID of Player Owning this Obstruction
			   BYTE              mProperties;				// Bit Flag Array of Unit Properties
		   };
		   long						mNodeProperties;
	   };

	   union
	   {
		   struct					
		   {
			   BYTE					mIdxMinX;					// DirectVal[] index # of Minimum X Coordinate
			   BYTE					mIdxMinZ;					// DirectVal[] index # of Minimum Z Coordinate
			   BYTE					mIdxMaxX;					// DirectVal[] index # of Maximum X Coordinate
			   BYTE					mIdxMaxZ;					// DirectVal[] index # of Maximum Z Coordinate
		   };
		   long						mBoundingIndicies;		// format to access all bounding indicies at once
	   };

      // 3rd & 4th 16-byte block       (64 bytes total)
      // This is block that holds the actual obstruction data
      // We have three different ways of accessing it for eash of writing code

      union
      {
         struct
         {
            float             mDirectVal[8];          // Direct acces to float values
         }; 
         struct 
         {
            BOPPoint          mPoint[4];              // Indexed Point Access
         }; 
         struct 
         {
            float             mX1, mZ1;               // 1st Point
            float             mX2, mZ2;               // 2nd Point
            float             mX3, mZ3;               // 3rd Point
            float             mX4, mZ4;               // 4th Point
         };
      };

      const BOPQuadHull       *getHull(void) const 
      {
         // jce/dlm 2/5/2002 -- fixed this cast to point to the location of the mExpandedHull pointer in
         // the BOPObstructionNode pointer (instead of casting the pointer itself) which is the start of data 
         // that is mostly like BOPQuadHull.  Certain fields are not valid when you do this, but they should 
         // theoretically only be the control structures used to manage the expanded hull list.
         return((const BOPQuadHull*)(&mExpandedHull));
      }

};


typedef BDynamicSimArray<BOPObstructionNode*> BObstructionNodePtrArray;
typedef BDynamicSimArray<BOPObstructionNode> BObstructionNodeArray;


// =============================================================================

typedef struct BOPQuadTreeNode
{
	BOPObstructionNode*	*mObstructionList;		// Pointer to list of Pointers for all obstructions in node
	unsigned short			mNumObstructions;			// Number of valid entries in the Obstruction List
	short						mListType;					// Which type of list is present

} BOPQuadTreeNode;


// =============================================================================

typedef struct BOPQuadTreeLevelInfo
{
	long						mLevel;						// level # - Redundant?
	long						mShiftCount;				// Number of bits to shift right to scale (X, Z) to current Tree Level coordinates
	long						mScaleZShift;				// Number of bits to shift left to get (Z * MapHeight + X) for Array index
	long						mQTNodeIndex;				// Index # of 1st QuadTreeNode entry for this level of the tree
	long						mMetersPerNode;			// size of each cell in meters at this level
	float						mfMetersPerNode;			// float size of each cell in meters at this level
	BOPQuadTreeNode		*mQTNodes;					// Direct Pointer to QT Element #[mQTNodeIndex]
} BOPQuadTreeLevelInfo;


//=============================================================================

class BObBlackTileSearch                        // used for scanline searching of black tiles for wall placement.
{
   public:
      BObBlackTileSearch(){}
      ~BObBlackTileSearch(){}

      BVector                 segs[4];
      BVector                 direction;
      BVector                 point1;
      BVector                 point2;
      BVector                 normal;
      BTerrainBase*           pTerrain;
      BPlayer*                pPlayer;
      float                   radius;
      long                    insideCount;
      BWallCheckResultArray*  pResults;
      BObstructionManager*    pObMgr;
};

//=============================================================================
class BObstructionManager
{
   public:

		// == New Obstruction Manager Enumerations ==========================================================================================================

		// ============= Enumerated List of Virtual Obstruction Quad Trees

		enum
		{
         cObsTypeUnknown = -1,                  // default value for non-initialized

         cObsTypeNonCollidableUnit = 0,         // Units that don't actually obstruct anything
         cObsTypeCollidableMovingUnit,          // Unit that can Move and *IS* currently in motion
         cObsTypeCollidableStationaryUnit,      // Unit that can Move and *IS NOT* currently in motion
         cObsTypeCollidableNonMovableUnit,      // Unit that can't move
         cObsTypeBlockAirMovement,              // Unit that blocks Air movement  
         cObsTypeDoppleganger,                  // Dopplegangers
         cObsTypeBlockLandMovement,             // Terrain that blocks Land-based movement
         cObsTypeBlockFloodMovement,            // Terrain that blocks Flood movement
         cObsTypeBlockScarabMovement,           // Terrain that blocks Scarab movement
         cObsTypeBlockLandAndFloodMovement,     // Terrain that blocks Normal Land and Flood movement
         cObsTypeBlockLandAndScarabMovement,    // Terrain that blocks Normal Land and Scarab movement
         cObsTypeBlockFloodAndScarabMovement,   // Terrain that blocks Flood and Scarab movement
         cObsTypeBlockAllMovement,              // Terrain that blocks all types of movement, Now includes the edge of the map
         cObsTypeCollidableMovingSquad,         // Squad that can Move and *IS* currently in motion
         cObsTypeCollidableStationarySquad,     // Squad that can Move and *IS NOT* currently in motion
         cObsTypeNonCollidableSquad,            // Noncollidable squads.  Garrisoned squads become noncollidable.  Or should, at least.

         cNumQuadTrees				// ** DO NOT CHANGE ** This is the number of different quad trees that will be maintained.
		};


		// ============= Bit Fields constants for Virtual Quadtrees

		enum
		{
         cIsNewTypeNonCollidableUnit            = 0x0001,      // Units that don't actually obstruct anything                                
         cIsNewTypeCollidableMovingUnit         = 0x0002,      // Unit that can Move and *IS* currently in motion                            
         cIsNewTypeCollidableStationaryUnit     = 0x0004,      // Unit that can Move and *IS NOT* currently in motion                        
         cIsNewTypeCollidableNonMovableUnit     = 0x0008,      // Unit that can't move                                                       
         cIsNewTypeBlockAirMovement             = 0x0010,      // Unit that blocks Air movement.
                                                            // These may be either units (farms, foundations) or terrain (ice).
         cIsNewTypeDoppleganger                 = 0x0020,      // Dopplegangers                                                              
         cIsNewTypeBlockLandMovement            = 0x0040,      // Terrain that blocks Normal Land-based movement
         cIsNewTypeBlockFloodMovement           = 0x0080,      // Terrain that blocks Flood movement
         cIsNewTypeBlockScarabMovement          = 0x0100,      // Terrain that blocks Scarab movement
         cIsNewTypeBlockLandAndFloodMovement    = 0x0200,      // Terrain that blocks Normal Land and Flood movement
         cIsNewTypeBlockLandAndScarabMovement   = 0x0400,      // Terrain that blocks Normal Land and Scarab movement
         cIsNewTypeBlockFloodAndScarabMovement  = 0x0800,      // Terrain that blocks Flood and Scarab movement
         cIsNewTypeBlockAllMovement             = 0x1000,      // Terrain that blocks all types of movement, Now includes the edge of the map
         cIsNewTypeCollidableMovingSquad        = 0x2000,      // Squad that can Move and *IS* currently in motion                            
         cIsNewTypeCollidableStationarySquad    = 0x4000,      // Squad that can Move and *IS NOT* currently in motion                        
         cIsNewTypeNonCollidableSquad           = 0x8000,      // Noncollidable squads.  Yes we have them. 

         // All these a "combo" constant that select a group of obstruction sets (quadtrees)
         cIsNewTypeAnyUnit                      = 0x001F,      // All obstructions from BUnit's
         cIsNewTypeSolidNonMoveableUnits        = 0x0008,      // All BUnits that are solid and non-moveable
         cIsNewTypeBlockLandUnits               = 0x1640,      // All terrain that blocks land movement
         cIsNewTypeBlockFloodUnits              = 0x1A80,      // All terrain that blocks flood movement
         cIsNewTypeBlockScarabUnits             = 0x1D00,      // All terrain that blocks scarab movement
         cIsNewTypeAllCollidableUnits           = 0x001E,      // All BUnits that are collidable (movable and non-movable)
         cIsNewTypeAllCollidableSquads          = 0x6000,      // All BSquads that are collidable (movable and non-movable)

			cRenderNodesMode1                      = cIsNewTypeAnyUnit,                // Used by Diagnostic obstruction Render (debug) display
			cRenderNodesMode2                      = cIsNewTypeAllCollidableSquads | cIsNewTypeNonCollidableSquad,
			cRenderNodesMode3                      = cIsNewTypeBlockLandUnits | cIsNewTypeBlockAllMovement,
			cRenderNodesMode4					         = cRenderNodesMode1 | cRenderNodesMode2
		};

		// mType Enums to determine what type contents the node points to/represents

		enum	ObsNodeType			
		{
			cObsNodeTypeUnknown        = 0x00,
			cObsNodeTypeAll            = 0x7f,
			cObsNodeTypeAllSolid       = 0x1d,

			cObsNodeTypeUnit				= 0x01,					// This node contains data from a unit
			cObsNodeTypeDoppleganger	= 0x02,					// This node contains data from a doppleganger
			cObsNodeTypeTerrain			= 0x04,					// This node contains data from a terrain obstruction
			cObsNodeTypeEdgeofMap		= 0x08,					// This node contains data from the edge of map boundaries
         cObsNodeTypeMesh           = 0x10,              // This node contains data from an obstructed mesh (i.e. train track)
         cObsNodeTypeSquad          = 0x20,					// This node contains data from a squad
         cObsNodeTypeCorpse         = 0x40					// This node contains data from a corpse
		};


		// mProperties Bit Flag Values

		enum
		{
			cObsPropertyMovableUnit					=  0x01,				// Unit moves, can change virtual quadtree
			cObsPropertyMultiPlayerOwned			=  0x02,				// Unit is 'owned' by more than one player, do a lookup
			cObsPropertyUpdatePatherQuadTree		=  0x04,				// When Unit updates, copy the pather
			cObsPropertyInIgnoreList				=  0x08,				// When the unit is in the Ignore List
         cObsPropertyDeletedNode             =  0x10           // This bit gets set when a node is freed, and cleared when the node is alloated.  dlm 7/24/02
		};

		// ============= Begin() processing Modes

		enum ObsBeginType			
		{
			cBeginNone = 0,
			cBeginUnit,
			cBeginProtoUnit,
			cBeginEntity
		};

		// ============= Segment Intersetcion Checking Modes

		enum ObsSegIntersectMode
		{
			cGetAnyIntersect,
			cGetNearestIntersect,
			cGetAllIntersections
		};

		// ============= Misc Enumerations

		enum
		{
			cMaxQuadTreeSizeinCells = 256,									// this * cMetersPerSmallestQuadCellSide = Max Size of World Map in Meters
																						// i.e. 2048 Meters		
			cNoPlayerOwnedIgnores = 0,
			cNoRotation = 64,														// Value for unrotated objects
			cIsObstructionRotated = 0x003F

		};

		// Terrain obstruction generation type ENnms

		enum
		{
			cNOObstruction = 0,
			cObstructs_Land_Movement,
			cObstructsWaterMovement,
			cObstructsAllMovement,//land_flood_scarab
			cObstructsAirMovement,

         cObstructs_Flood_Movement,
         cObstructs_Scarab_Movement,

         cObstructs_Land_Flood_Movement,
         cObstructs_Land_Scarab_Movement,

         cObstructs_Flood_Scarab_Movement,
		
		};


		// == New Obstruction Manager Public Functions ==========================================================================================================

		// Constructor(s) & Destructor

                              BObstructionManager(void);
                              ~BObstructionManager(void);

		// Public Init/DeInit access

      bool                    initialize(void); 
      void                    deinitialize(void);

		// Public operation control

		void							disablePathingUpdates(void);
		void							enablePathingUpdatesAndRebuild(void);

		// Terrain & Edge of World Object Generation

		void							generateWorldBoundries(void);

		void							generateTerrainObstructions(BYTE *obstructionMap);
		void							generateTerrainObstructions(BYTE *obstructionMap, long startX, long startZ, long endX, long endZ, bool clearExisting = true);
		void							regenerateTerrainObstructions(BYTE *obstructionMap, long startX, long startZ, long endX, long endZ);

		// Obstruction Data Node Manipulation

		void							resetObstructionNode(BOPObstructionNode* theNode);
		void							setAlignedObstruction(BOPObstructionNode *theNode, float x1, float z1, float x2, float z2);


		
		// Obstruction Creation/Update/Deletion for Units/Dopples/etc

		BOPObstructionNode*		getNewObstructionNode(void);

		void							fillOutRotatedPosition(BOPObstructionNode* theNode, float x, float z, float radiusX, float radiusZ, float rotationX, float rotationZ);
		void							fillOutNonRotatedPosition(BOPObstructionNode* theNode, float x, float z, float radiusX, float radiusZ);


		void							installObjectObstruction(BOPObstructionNode* theNode, long obsType);
		bool							updateObstructionLocation(BOPObstructionNode* theNode, long obsType, bool bForce = false);
		void							deleteObstruction(BOPObstructionNode *theNode);

		// More Obstruction update stuff

		void							updatePathingInformation(BOPObstructionNode *theNode);

		// ================= Begin <---> End blocking control =======================

		// For regular Begin
		bool							begin(ObsBeginType mode, float radius, long QuadTreeToScan, long validNodeTypes, long lPlayerID, 
																	float ExpansionScale, const BEntityIDArray* ignoreList, bool canJump);

		// For begin Unit/ProtoUnit/Entity
		bool							begin(ObsBeginType mode, long modeval1, long modeval2, long QuadTreeToScan, long validNodeTypes, long lPlayerID, 
																	float ExpansionScale, const BEntityIDArray* ignoreList, bool canJump);


		bool							inUse() const							{return (mInUse);};

		bool							end();

		// Push & Pop  Begin <---> End State support

		bool							getSessionState(BSaveObsManagerState &theData);
		void							setSessionState(BSaveObsManagerState &theData);

		// =================== Obstruction Find/Search Methods ===========================

      // Use these outside of begin/end session
      bool							testObstructions(float x1, float z1, float x2, float z2, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID);
      bool                    testObstructions( const BVector cPoint, const float cTestRadius, const float expansionRadius, long quadTreesToScan, long nodeTypes, long playerID );

      bool                    testObstructionsInCachedList(float x1, float z1, float x2, float z2, float expansionRadius, BObstructionNodePtrArray &cachedObstructions);

      void							findObstructions(float x1, float z1, float x2, float z2, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID, bool ignoreCurrentList, BObstructionNodePtrArray &theList);
		void							findObstructions(const BConvexHull &theHull, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID, bool ignoreCurrentList, BObstructionNodePtrArray &theList, CHECKUNIT_CALLBACK checkUnitCB=NULL, void *checkUnitParam=NULL);
      bool							findObstructionsOnPoint(const BVector thePoint, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID, bool ignoreCurrentList, BObstructionNodePtrArray& theList);
		void							findObstructionsQuadHull(const BOPQuadHull *pHull, float expansionRadius, long QuadTreesToScan, long nodeTypes, long lPlayerID, bool ignoreCurrentList, BObstructionNodePtrArray &theList);      

		void							findEntityObstructions(const BConvexHull &theHull, bool includeNonCollidableUnitObs, bool isSquadCheck = false, CHECKUNIT_CALLBACK checkUnitCB=NULL, void *checkUnitParam=NULL);
		void							findEntityObstructions(const float minX, const float minZ, const float maxX, const float maxZ, bool includeNonCollidableUnitObs, bool isSquadCheck = false);
      void                    findEntityObstructions(const BVector p0, const BVector p1, bool includeNonCollidableUnitObs, bool isSquadCheck = false);
      void                    findEntityObstructions(const BVector point, const float radius, bool includeNonCollidableUnitObs, bool isSquadCheck = false);
		void							findEntityObstructionsQuadHull(const BOPQuadHull *pHull, bool includeNonCollidableUnitObs, bool isSquadCheck = false);

      // Use these inside of begin/end session
      void                    findObstructions(const BVector &point, const bool relaxed, const bool add, BObstructionNodePtrArray &obstructions);
		void                    findObstructions(const BConvexHull &hull, const bool relaxed, const bool add, BObstructionNodePtrArray &obstructions);
      void                    findObstructionsQuadHull(const BOPQuadHull *pHull, const bool relaxed, const bool add, BObstructionNodePtrArray &obstructions);
      void							findObstructionsNoExpansion(const BConvexHull &theHull, bool ignoreCurrentList, BObstructionNodePtrArray &obstructions);
      void							findObstructionsNoExpansionQuadHull(const BOPQuadHull *pHull, bool ignoreCurrentList, BObstructionNodePtrArray &obstructions);


		// ==================== Segment Intersects Replacment & wrapper functions

		// These function should be called inside of begin/end session.
      bool							getObjectsIntersections(const ObsSegIntersectMode mode, const BVector &point1, const BVector &point2, const bool useRelaxed,
                                                       BVector &iPoint, BObstructionNodePtrArray &obstructions);
      bool							getObjectsIntersections(const ObsSegIntersectMode mode, const BVector &point1, const BVector &point2, const bool useRelaxed,
                                                       BSegIntersectResultArray &results);
      bool                    segmentIntersects(const BVector &point1, const BVector &point2, const bool relaxed, BVector &iPoint);
      bool                    segmentIntersects(const BVector &point1, const BVector &point2, const bool relaxed);
	
		// ==================== Wall Check replacment

		long							performNewWallCheck( const BVector &point1, const BVector &point2, long ignoreWallForPlayerID, BWallCheckResultArray &results, bool ignoreWalls=true);

		//  =================== Default Find Results List access & Control ==================

      // These work in conjunction with findEntityObstructions.
      BObstructionNodePtrArray& getFoundObstructionResults(void)				{return (mFoundObstructionResults);};
		void							resetFoundObstructionResults(void)				{mFoundObstructionResults.setNumber(0);};


      // Right now calling any of these loses the results of a previous call to a function in this set
      // since they all recycle the same cache slot.
      const BOPQuadHull       *getExpandedHull(const BOPObstructionNode *obs);
      const BOPQuadHull       *getExpandedHull(const BOPObstructionNode *obs, float extraRadius);
      const BOPQuadHull       *getRelaxedExpandedHull(const BOPObstructionNode *obs);

		// Unit Ignore status setting

      void                    addUnitIgnore(BEntityID id);
      void                    addUnitIgnoreList(const BEntityIDArray& IDList);
      void                    clearUnitIgnore(BEntityID id);
      void                    clearUnitIgnoreList(const BEntityIDArray& IDList);
      const BEntityIDArray&   getEntityIgnoreList() {return(mEntityIgnoreList);}

      // =========== "Safe" Unit access ==============
      BEntity             *getObject(const BOPObstructionNode *obs) const
      {
         if (obs->mProperties & cObsPropertyDeletedNode)
         {
            BASSERT(0);
            return NULL;
         }
         else
            return obs->mObject;
      }

		// Player Owned Obstruction Control

		bool							updatePlayerOwnedMasks(void);

		void							rebuildPlayerRelationMasks(void);
		void							dontUpdatePlayerRelationMasks(void) 			{mUpdatePlayerRelationMasks = false;};


		// general info accessor functions

		long							getNumQuadTreeNodes(void)							{return (mNumQuadTreeNodes);};
		BOPQuadTreeNode*			getQuadTreeNodeList(const long treeNum)		{return (mQuadTreeNodes[treeNum]);};

		void							setTreesToScan(long treestoScan)					{mQuadTreesToScan = treestoScan;};
		long							getTreesToScan(void)									{return (mQuadTreesToScan);};

      float                   getRadius(void)                 {return (mRadius);};

      static float				getWaterPathingDepth();
      bool                    getAllInHeightRange(const BConvexHull &hull, float &minHeight, float &maxHeight);

		void							releaseExpandedHulls(void);

      // saving/loading
      bool                    saveOrdering(BChunkWriter* chunkWriter);
      bool                    loadOrdering(BChunkReader* chunkReader);


		//  ======================= Debugging Functions =======================

      void                    render( long mode );
		void							renderObstructions(long QuadTreesToRender, long clipHint, long level, long x, long z, float delta);


		// Public Static data


		static long					cDefaultQuadTreesToScanforLand;
      static long					cDefaultQuadTreesToScanforFlood;
      static long					cDefaultQuadTreesToScanforScarab;
      static long             cDefaultQuadTreesToScanforHover;



	protected:

		// Init & Setup

		// Virtual Quad Tree Control

      void                    setupEntityObstructionData(long& quadTrees, long& nodeTypes, bool includeNonCollidableUnitObs, bool isSquadCheck);

		void							emptyVirtualQuadtree(const long obsType);
		void							emptyVirtualQuadtreeofDataType(const long obsType, const long nodeType);
		void							emptyVirtualQuadtreeNodes(const long obsType, float x1, float z1, float x2, float z2, const long nodeType);

		// Node Point update

		void							addNodeToList(BOPQuadTreeNode* theQuadNode, BOPObstructionNode* theNode);
		void							removeNodeFromList(BOPQuadTreeNode* theQuadNode, long index);

		// Terrain Obstruction Generation

		void							expandSquare(BYTE* *Matrix, long StartX, long StartZ, long &width, long &height, long maxSize, BYTE obsType);
		void							expandWidth(BYTE* *Matrix, long StartX, long StartZ, long &width, long &height, long maxSize, BYTE obsType);
		void							expandHeight(BYTE* *Matrix, long StartX, long StartZ, long &width, long &height, long maxSize, BYTE obsType);

		void							fillOutMinAndMax(BOPObstructionNode* theNode);

		// Ignore Obstruction Control

		void							markObstructionsToBeIgnored(BObstructionNodePtrArray &theList, const long numItems);
		void							unmarkObstructionsToBeIgnored(BObstructionNodePtrArray &theList, const long numItems);

		// Internal Expanded Hull Functions

		BOPQuadHull*				getExpandedHullPoints(BOPObstructionNode *theNode, const float expansionRadius);

		// Wall check support


		long							singleWallCheck(const BOPQuadHull *hull, const BVector &point1, const BVector &point2, const BVector &dir, const BVector &normal, float width,  
																const BVector segs[4], BWallCheckResultArray &results, BOPObstructionNode *pNode);


      void                    debugAddPoint(const char *szName, const BVector &start, DWORD color);
      static void CALLBACK    findBlackTiles(long x, long z, void* pParam);
      
      void                    makeValidX(long &x);
      void                    makeValidZ(long &z);

      void                    computeVertex(long x, long z, BVector &vertex) const;

		// Control Values for Begin <--> End Cycle

		float							mRadius;									// Radius to Expand By
		float							mRelaxedRadius;						// "releaxed" version of that radius
		long							mMode;									// "Type" of Begin that was called

		long						   mQuadTreesToScan;					// Bit flags of the quadtrees to be scanned, used by external process
		long							mValidNodeTypes;
		long							mlPlayerID;

      bool                    mbCanJump;                      //Is the entity we are pathing for a warthog, with special jump pathing?

		// Unit Ignore Lists

      // jce [9/24/2008] -- making this a real array since saving a pointer to some passed in parameter can easily result
      // in holding a stale pointer
		BEntityIDArray          mEntityIgnoreList;									// EntityID array of units to ignore

      BObstructionNodePtrArray	mSessionUnitIgnoreList;								// Dynamically allocated list of units to ignore

		// Private constants that control quad node sizes, etc...

		static const float		cWorldObstructionDensityFactor;		// factor to estimate # of objects in world based on number of tiles
		static const long			cMetersPerSmallestQuadCellSide;		// Size of smallest quad node in meters (must be ^ of 2?)
		static const long			cTilesPerSmallestQuad;					// Size of the smallest quad node in Tiles
		static const long			cMetersToQuadCellShift;					// This the N in 2^N = cMetersPerSmallestQuadCellSide; 
		static const long			cMetersToQuadCellULANDMask;			// Mask to round down a coord in meters to the Upper/Left edge of a quad cell
		static const long			cMetersToQuadCellLRORMask;			   // Mask to round up a coord in meters to the Lower/Right edge of a quad cell

		static const float		cRoundUpFactor;							// Used to round up floats to next highest integer
		static const float		cEdgeOfWorldThickness;					// Thickness of Edge of World Obstructions
		static const float		cNodeDropDownFactor;						// Adjustment to make tile aligned obstructions "drop" to a lower node


		// ========================== New Obstruction Manager member Vars ===========================

		long								mTileWidth, mTileHeight;		// The size of the world in tiles (Integer version)
		float								mfTileWidth, mfTileHeight;		// The size of the world in tiles (float version)
											
		long								mWidth, mHeight;					// The size of the world in meters (Integer version)
		float								mfWidth, mfHeight;				// The size of the world in meters (float version)
		
		float								mfTileSize;							// Size of a tile in meters (float Version)
		float								mfConvertToTiles;					// 1/Size of tile -> to convert meter coordinates to tiles


		long								mNumQuadTreeLevels;				// Number of 'levels' in the QuadTree
		long								mNumQuadTreeNodes;				// Number of nodes in each whole tree

		BOPObNodeArrayManager		*mNodeManager;						// Our obstruction Node memory pool manager

		BOPObQuadNodeListManager	*mListManager;						// Our QuadTree obstruction List Manager

		BOPHullArrayManager			*mHullManager;						// Our Expanded Hull Data Manager

		BOPQuadTreeLevelInfo*		mQTLevelInfo[cNumQuadTrees];	// Array of arrays of quadTree Level Info

		BOPQuadTreeNode*				mQuadTreeNodes[cNumQuadTrees];	// Array of arrays of QuadTree Nodes

		long								mNodeLevelLookup[cMaxQuadTreeSizeinCells];		// Quick Lookup for QuadTree level
		


      //BPather*							mThePather;							// The obmgr keeps a ptr to the pather so that it can talk to it

		BVector							mTempVectors[4];					// Temporaty vectors to build Hull with
		

		BObstructionNodePtrArray	mFoundObstructionResults;		// Array of pointers
		BObstructionNodePtrArray	mSegmentIntersectResults;		// Array of pointers


		BObstructionNodePtrArray	mExpandCheckoutList;				// List of obstructions with expanded Hulls

      // DLM - We no longer just create 4 obstructions along the edge of the world.  This makes the pather VERY sad.
      // as we not longer create just four, not going to save room for four node pointers either.
      // BOPObstructionNode*        mpEdgeOb[4];

		bool								mUpdatePlayerRelationMasks;	//  flag to allow player relation masks to be built
		bool							   mInUse;									// Indicates a begin->Process->End cycle is active


		// == END Replaced Functions ============================================================================================================
};


// ===================================== External Static Data ==============================

extern long ObsSystemPlayerMasks[];

extern BObstructionManager gObsManager;
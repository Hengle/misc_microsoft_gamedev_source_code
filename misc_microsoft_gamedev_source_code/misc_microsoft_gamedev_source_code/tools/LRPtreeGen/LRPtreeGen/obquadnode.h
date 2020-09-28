//==============================================================================
// obquadnode.h
//
// Copyright (c) 1999-2007, Ensemble Studios
// Optimizations started by MPritchard 11/2001
//==============================================================================
#pragma once


//==============================================================================

//#include "obstructionmanager.h"
class BOPObstructionNode;
class BOPQuadHull;

/*
struct BOPTypeID
{
   long     mType;
   long     mID;
};

struct BOPPoint
{
   float    mX;
   float    mZ;
};

struct BOPLine
{
   BOPPoint       mPoint1;  
   BOPPoint       mPoint2;  
};
*/



class BOPObNodeArrayManager
{
   // Convention notes: a node is allocated if it 'is use' by the game, and free if not
   //

   public:
      // ENums

      enum {
         cFreeNodePoolEmpty = -1
      };

      // Ctors & Dtor

      BOPObNodeArrayManager(const long initialSize, const long newZoneSize);
      ~BOPObNodeArrayManager(void);

      // Node Allocation Functions

      BOPObstructionNode*     getNode(void);
      void                    freeNode(BOPObstructionNode *theNode);
      void                    reclaimAllNodes(void);

      // Accessor Functions allowed to outsiders

      void                    setNewZoneSize(const long size)        {mDefaultNodeGrowSize = size;};
      long                    getNewSizeSize(void)                   {return (mDefaultNodeGrowSize);};

      long                    getTotalNodeCount(void)                {return (mTotalNodesAllocted);};
      long                    getAllocatedNodeCount(void)            {return (mTotalNodesAllocted-mFreeNodes);};
      long                    getFreeNodeCount(void)                 {return (mFreeNodes);};

      // Profiling functions
   
   #ifdef _PROFILE_NODES
      void                    resetPerfStats(void);

      long                    getAllocationCount(void)               {return (mAllocCount);};
      long                    getDeallocationCount(void)             {return (mDeallocCount);};
      long                    getHighAllocationCount(void)           {return (mHighAllocationCount);}:
      long                    getNewZoneCount(void)                  {return (mNewZoneCount);}:
      long                    getResetCount(void)                    {return (mResetCount);}:
   #endif   


   private:
      // Private methods

      void                    resizeZoneList(const long newSize);
      void                    allocateZone(const long numNodes);



      // Data about the memory zones themselves

      BOPObstructionNode*     *mZonePtrs;                // Array of allocated "zones"
      long                    *mZoneSizes;               // Size of each allocated "zone"

      long                    mUsedZones;                // # of Zones with elements in use or space allocated
      long                    mMaxZones;                 // size of mZonePtrs array, maximum # of zones

      // Data about the unused node elements
   
      BOPObstructionNode      *mFreeListHead;            // Head of threaded free element list
      
      long                    mFreeNodePoolZone;         // Zone # where the free node pool start
      long                    mFreeNodePoolIndex;        // Index # in the FreeNodePoolZone of the starting element.
   
      // Data about node allocaions

      long                    mTotalNodesAllocted;       // Total # of nodes from all zones
      long                    mFreeNodes;                // # of unallocated nodes available

      long                    mDefaultNodeGrowSize;      // # of nodes to allocate in each new zone

      // Performance Profiling Stuff
      
   #ifdef _PROFILE_NODES
      long                    mAllocCount;               // # of allocations performed
      long                    mDeallocCount;             // # of deallocations performed
      long                    mNewZoneCount;             // # of Zone Allocations
      long                    mResetCount;               // # of times the Obstructions are rebults
      long                    mHighAllocationCount;      // peak # of nodes in use
   #endif

};




class BOPHullArrayManager
{

   public:
      // ENums

      enum {
         cFreeNodePoolEmpty = -1
      };

      // Ctors & Dtor

      BOPHullArrayManager(const long initialSize, const long newZoneSize);
      ~BOPHullArrayManager(void);

      // Node Allocation Functions

      BOPQuadHull*				getNode(void);
      void                    freeNode(BOPQuadHull *theNode);
      void                    reclaimAllNodes(void);

      // Accessor Functions allowed to outsiders

      void                    setNewZoneSize(const long size)        {mDefaultNodeGrowSize = size;};
      long                    getNewSizeSize(void)                   {return (mDefaultNodeGrowSize);};

      long                    getTotalNodeCount(void)                {return (mTotalNodesAllocted);};
      long                    getAllocatedNodeCount(void)            {return (mTotalNodesAllocted-mFreeNodes);};
      long                    getFreeNodeCount(void)                 {return (mFreeNodes);};

      // Profiling functions
   
   #ifdef _PROFILE_NODES
      void                    resetPerfStats(void);

      long                    getAllocationCount(void)               {return (mAllocCount);};
      long                    getDeallocationCount(void)             {return (mDeallocCount);};
      long                    getHighAllocationCount(void)           {return (mHighAllocationCount);}:
      long                    getNewZoneCount(void)                  {return (mNewZoneCount);}:
      long                    getResetCount(void)                    {return (mResetCount);}:
   #endif   


   private:
      // Private methods

      void                    resizeZoneList(const long newSize);
      void                    allocateZone(const long numNodes);



      // Data about the memory zones themselves

      BOPQuadHull*				*mZonePtrs;                // Array of allocated "zones"
      long                    *mZoneSizes;               // Size of each allocated "zone"

      long                    mUsedZones;                // # of Zones with elements in use or space allocated
      long                    mMaxZones;                 // size of mZonePtrs array, maximum # of zones

      // Data about the unused node elements
   
      BOPQuadHull					*mFreeListHead;            // Head of threaded free element list
      
      long                    mFreeNodePoolZone;         // Zone # where the free node pool start
      long                    mFreeNodePoolIndex;        // Index # in the FreeNodePoolZone of the starting element.
   
      // Data about node allocaions

      long                    mTotalNodesAllocted;       // Total # of nodes from all zones
      long                    mFreeNodes;                // # of unallocated nodes available

      long                    mDefaultNodeGrowSize;      // # of nodes to allocate in each new zone

      // Performance Profiling Stuff
      
   #ifdef _PROFILE_NODES
      long                    mAllocCount;               // # of allocations performed
      long                    mDeallocCount;             // # of deallocations performed
      long                    mNewZoneCount;             // # of Zone Allocations
      long                    mResetCount;               // # of times the Obstructions are rebults
      long                    mHighAllocationCount;      // peak # of nodes in use
   #endif

};




class BOPQuadNodeListPoolInfo
{
	public:
		BOPObstructionNode**		*mListPoolPointers;			// List of Pointers to pooled pointer lists (got that?)
		long							*mListPoolSizes;				// Number of pointer lists in each pool
		long							mNumPoolsActive;				// Number of entries in mListPoolPointers actually used
		long							mMaxNumPoolsAvailable;		// Number of entries in mListPolPointers allocated

		BOPObstructionNode*		*mFirstFreeList;				// LIFO list (chain) of recycled pointer lists
		long							mFreeListPoolNumber;			// Pool number of first free list not in the LIFO chain (high water mark)		
		long							mFreeListIndexNumber;		// Index number of first free list not in the LIFO chain

		long							mTotalLists;					// Total number of pointer lists (total of all active pools)
		long							mFreeLists;						// number of pointer lists not in use by app
		long							mNewPoolSize;					// size to make each new list pool

		long							mNodeTypeID;					// Index Number
		long							mListElementSize;				// number of pointers in each list


   #ifdef _PROFILE_LISTS
      long                    mAllocCount;               // # of allocations performed
      long                    mDeallocCount;             // # of deallocations performed
      long                    mNewZoneCount;             // # of pool Allocations
      long                    mResetCount;               // # of times the lists are disposed of
      long                    mHighAllocationCount;      // peak # of lists in use
   #endif

};


class BOPObQuadNodeListManager
{
   // Manages the pooled Pointer Lists, aka "Pools" of "Lists"
   //

   public:

      enum {
         cFreeListPoolEmpty = -1,
			cInvalidNodeListType = -1,
			cMaxNumNodeTypes = 6,
			cNumPoolsToExpandBy = 16
      };

      // Ctors & Dtor

      BOPObQuadNodeListManager(const long numUsedLowLevelNodes);
      ~BOPObQuadNodeListManager(void);

      // Node Allocation Functions

      BOPObstructionNode**    getList(const long nodeType);
      BOPObstructionNode**    getList(const long numNodes, long &listType);

      void                    freeList(BOPObstructionNode **theList, const long nodeType);
      void                    reclaimAllLists(long nodeType);

		long							getNodeListType(long numNodes);	

      // Accessor Functions allowed to outsiders
	
		// Individual list size specific accessors

      void                    setNewPoolSize(const long nodeType, const long size)        {mListPoolInfoArray[nodeType].mNewPoolSize = size;};
      long                    getNewPoolSize(const long nodeType)									{return (mListPoolInfoArray[nodeType].mNewPoolSize);};

      long                    getTotalListCount(const long nodeType)								{return (mListPoolInfoArray[nodeType].mTotalLists);};
      long                    getAllocatedListCount(const long nodeType)						{return (mListPoolInfoArray[nodeType].mTotalLists - mListPoolInfoArray[nodeType].mFreeLists);};
      long                    getFreeListCount(const long nodeType)								{return (mListPoolInfoArray[nodeType].mFreeLists);};

		// Global (total) specific accessors


      // Profiling functions
   
   #ifdef _PROFILE_LISTS
      void                    resetPerfStats(void);
      void                    resetPerfStats(const long nodeType);

      long                    getAllocationCount(const long nodeType)               {return (mListPoolInfoArray[nodeType].mAllocCount);};
      long                    getDeallocationCount(const long nodeType)             {return (mListPoolInfoArray[nodeType].mDeallocCount);};
      long                    getHighAllocationCount(const long nodeType)           {return (mListPoolInfoArray[nodeType].mHighAllocationCount);}:
      long                    getNewZoneCount(const long nodeType)                  {return (mListPoolInfoArray[nodeType].mNewZoneCount);}:
      long                    getResetCount(const long nodeType)                    {return (mListPoolInfoArray[nodeType].mResetCount);}:
   #endif   

		long								mListSizes[cMaxNumNodeTypes];				// number of pointers a list of each type holds
		long								mExpandSize[cMaxNumNodeTypes];			// size at which a list is switched to the next larger size
		long								mContractSize[cMaxNumNodeTypes];			// size at which a list is switched to the next smaller size

   private:
      // Private methods

		void							setListSizes(const long nodeType, const long listSize, const long expandSize, const long contractSize, const long initialPoolSize, const long extendPoolSize);

      void                    allocateListPool(BOPQuadNodeListPoolInfo &theList, const long numLists);

      // Data about the memory zones themselves

		BOPQuadNodeListPoolInfo		mListPoolInfoArray[cMaxNumNodeTypes];	// Array of info for each pool;

};

//==============================================================================
// obquadnode.cpp
//
// Copyright (c) 1999, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "obquadnode.h"
#include "obstructionmanager.h"

#include "segintersect.h"
#include "game.h"
#include "world.h"
#include "unit.h"
#include "config.h"
//#include "aconfigenums.h"
#include "pointin.h"
//#include "terrainbase.h"

//==============================================================================
// Defines


//==============================================================================
// BOPObNodeArrayManager:: constructor
//==============================================================================
BOPObNodeArrayManager::BOPObNodeArrayManager(const long initialSize, const long newZoneSize) :
   mZonePtrs(NULL),
   mZoneSizes(0),
   mUsedZones(0),
   mMaxZones(0),
   mFreeListHead(NULL),
   mFreeNodePoolZone(cFreeNodePoolEmpty),
   mFreeNodePoolIndex(0),
   mTotalNodesAllocted(0),
   mFreeNodes(0),
   mDefaultNodeGrowSize(0)
#ifdef _PROFILE_NODES
   ,
   mAllocCount(0),
   mDeallocCount(0),
   mNewZoneCount(0),
   mResetCount(0),
   mHighAllocationCount(0)
#endif
    
{
   BASSERT(initialSize > 0);
   BASSERT(newZoneSize > 0);

   // Create the 1st node pool, and setup for more.
   
   resizeZoneList(8);
   allocateZone(initialSize);
   
   setNewZoneSize(newZoneSize);

}


//==============================================================================
// BOPObNodeArrayManager:: destructor
//==============================================================================
BOPObNodeArrayManager::~BOPObNodeArrayManager(void)
{
   if (mZonePtrs != NULL && mZoneSizes != NULL && mMaxZones > 0)
   {
      for (long n = 0; n < mMaxZones; n++)
      {
         if (mZonePtrs[n] != NULL)
            HEAP_DELETE_ARRAY(mZonePtrs[n], gSimHeap);
      }

      HEAP_DELETE_ARRAY(mZonePtrs, gSimHeap);
      HEAP_DELETE_ARRAY(mZoneSizes, gSimHeap);
   }

}

//==============================================================================
// BOPObNodeArrayManager::resizeZoneList
//==============================================================================
void BOPObNodeArrayManager::resizeZoneList(const long newSize)
{
   BASSERT(newSize > 0);

	long n;

   if (newSize < mMaxZones)
   {
      BASSERT(0);
      return;
   }

   // Allocate new list

   BOPObstructionNode** ptrs= HEAP_NEW_ARRAY(BOPObstructionNode*, newSize, gSimHeap);
   long *size = HEAP_NEW_ARRAY(long, newSize, gSimHeap);

   // copy over any existing lists

   if (mZonePtrs != NULL && mZoneSizes != NULL)
   {
      for (n = 0; n < mMaxZones; n++)
      {
         ptrs[n] = mZonePtrs[n];
         size[n] = mZoneSizes[n];
      }

      // Delete old copy
      HEAP_DELETE_ARRAY(mZonePtrs, gSimHeap);
      HEAP_DELETE_ARRAY(mZoneSizes, gSimHeap);

	}

   for (n = mMaxZones; n < newSize; n++)
   {
      ptrs[n] = NULL;
      size[n] = 0;
   }

   // and assign new pointers

   mZonePtrs = ptrs;
   mZoneSizes = size;

   mMaxZones = newSize;
}


//==============================================================================
// BOPObNodeArrayManager::allocateZone
//==============================================================================
void BOPObNodeArrayManager::allocateZone(const long numNodes)
{
   // Do we need more zone pools?  If so allocate a block

   if (mUsedZones >= mMaxZones)
   {
      resizeZoneList(mMaxZones + 8);
   }

   // Allocate new pool of nodes

   mZonePtrs[mUsedZones]  = HEAP_NEW_ARRAY(BOPObstructionNode, numNodes, gSimHeap);
   mZoneSizes[mUsedZones] = numNodes;

   // Update Stats

   mTotalNodesAllocted+= numNodes;
   mFreeNodes+= numNodes;

   // Is the free node pool empty?  if so reset it

   if (mFreeNodePoolZone == cFreeNodePoolEmpty)
   {
      mFreeNodePoolZone = mUsedZones;
      mFreeNodePoolIndex = 0;
   }

   mUsedZones++;

   // Performance Profiling Stuff

#ifdef _PROFILE_NODES
   mNewZoneCount++;
#endif

}


//==============================================================================
// BOPObNodeArrayManager::getNode
//==============================================================================
BOPObstructionNode* BOPObNodeArrayManager::getNode(void)
{
	BOPObstructionNode   *retVal;

	// No free nodes anywhere?  Then create a new heap

	if (mFreeNodes == 0)
	{
		allocateZone(mDefaultNodeGrowSize);
	}

#ifdef _PROFILE_NODES
	mAllocCount++;
   mHighAllocationCount = max(mHighAllocationCount, mTotalNodesAllocted-mFreeNodes+1);
#endif

	// First, we reuse any Nodes from the free (previously discarded) list

	if (mFreeListHead)
	{
		retVal = mFreeListHead;
		mFreeListHead = mFreeListHead->mNextNode;
		mFreeNodes--;

      // Turn off the "deleted bit" dlm 7/24/02
      retVal->mProperties &= ~(BObstructionManager::cObsPropertyDeletedNode);

		return(retVal);
	}

	// Otherwise we take the first element from the free node pool

	retVal = mZonePtrs[mFreeNodePoolZone] + mFreeNodePoolIndex;

	mFreeNodes--;

	// Now we advance the free node pointers
	// And if we hit the end of this zone, we advance to the next zone
	// if we are totally empty, a new heap will be allocated next time

	mFreeNodePoolIndex++;

	if (mFreeNodePoolIndex == mZoneSizes[mFreeNodePoolZone])
	{
		mFreeNodePoolZone++;
		mFreeNodePoolIndex = 0;
		
		if (mFreeNodePoolZone == mUsedZones)
		{
			mFreeNodePoolZone = cFreeNodePoolEmpty;
			BASSERT(mFreeNodes == 0);
		}
	}

   // Turn off the "deleted bit" dlm 7/24/02
   retVal->mProperties &= ~(BObstructionManager::cObsPropertyDeletedNode);

	return(retVal);

}


//==============================================================================
// BOPObNodeArrayManager::freeNode
//==============================================================================
void BOPObNodeArrayManager::freeNode(BOPObstructionNode *theNode)
{
	// All discarded nodes are put into a FIFO list

   // Set the deleted node bit on this bad boy.. dlm 7/24/02
   theNode->mProperties |= BObstructionManager::cObsPropertyDeletedNode;

	theNode->mNextNode = mFreeListHead;
	mFreeListHead = theNode;

	mFreeNodes++;

#ifdef _PROFILE_NODES
	mDeallocCount++;
#endif

}



//==============================================================================
// BOPObNodeArrayManager::reclaimAllNodes
//==============================================================================
void BOPObNodeArrayManager::reclaimAllNodes(void)
{
	mFreeListHead = NULL;

	mFreeNodePoolZone = 0;
	mFreeNodePoolIndex = 0;

	mFreeNodes = mTotalNodesAllocted;
	   
#ifdef _PROFILE_NODES
   mResetCount++;
#endif
	
}


#ifdef _PROFILE_NODES
//==============================================================================
// BOPObNodeArrayManager::resetPerfStats
//==============================================================================
void BOPObNodeArrayManager::resetPerfStats(void)
{
    mAllocCount++;
    mDeallocCount++;
    mNewZoneCount++;
    mResetCount++;
    mHighAllocationCount++;
}
#endif














//==============================================================================
// BOPHullArrayManager:: constructor
//==============================================================================
BOPHullArrayManager::BOPHullArrayManager(const long initialSize, const long newZoneSize) :
   mZonePtrs(NULL),
   mZoneSizes(0),
   mUsedZones(0),
   mMaxZones(0),
   mFreeListHead(NULL),
   mFreeNodePoolZone(cFreeNodePoolEmpty),
   mFreeNodePoolIndex(0),
   mTotalNodesAllocted(0),
   mFreeNodes(0),
   mDefaultNodeGrowSize(0)
#ifdef _PROFILE_NODES
   ,
   mAllocCount(0),
   mDeallocCount(0),
   mNewZoneCount(0),
   mResetCount(0),
   mHighAllocationCount(0)
#endif
    
{
   BASSERT(initialSize > 0);
   BASSERT(newZoneSize > 0);

   // Create the 1st node pool, and setup for more.
   
   resizeZoneList(8);
   allocateZone(initialSize);
   
   setNewZoneSize(newZoneSize);

}


//==============================================================================
// BOPHullArrayManager:: destructor
//==============================================================================
BOPHullArrayManager::~BOPHullArrayManager(void)
{
   if (mZonePtrs != NULL && mZoneSizes != NULL && mMaxZones > 0)
   {
      for (long n = 0; n < mMaxZones; n++)
      {
         if (mZonePtrs[n] != NULL)
            HEAP_DELETE_ARRAY(mZonePtrs[n], gSimHeap);
      }

      HEAP_DELETE_ARRAY(mZonePtrs, gSimHeap);
      HEAP_DELETE_ARRAY(mZoneSizes, gSimHeap);
   }

}

//==============================================================================
// BOPHullArrayManager::resizeZoneList
//==============================================================================
void BOPHullArrayManager::resizeZoneList(const long newSize)
{
   BASSERT(newSize > 0);

	long n;

   if (newSize < mMaxZones)
   {
      BASSERT(0);
      return;
   }

   // Allocate new list

   BOPQuadHull** ptrs= HEAP_NEW_ARRAY(BOPQuadHull*, newSize, gSimHeap);
   long *size = HEAP_NEW_ARRAY(long, newSize, gSimHeap);

   // copy over any existing lists

   if (mZonePtrs != NULL && mZoneSizes != NULL)
   {
      for (n = 0; n < mMaxZones; n++)
      {
         ptrs[n] = mZonePtrs[n];
         size[n] = mZoneSizes[n];
      }

      // Delete old copy
      HEAP_DELETE_ARRAY(mZonePtrs, gSimHeap);
      HEAP_DELETE_ARRAY(mZoneSizes, gSimHeap);
	}

	// Clear undefined items

   for (n = mMaxZones; n < newSize; n++)
   {
      ptrs[n] = NULL;
      size[n] = 0;
   }

   // and assign new pointers

   mZonePtrs = ptrs;
   mZoneSizes = size;

   mMaxZones = newSize;
}


//==============================================================================
// BOPHullArrayManager::allocateZone
//==============================================================================
void BOPHullArrayManager::allocateZone(const long numNodes)
{
   // Do we need more zone pools?  If so allocate a block

   if (mUsedZones >= mMaxZones)
   {
      resizeZoneList(mMaxZones + 8);
   }

   // Allocate new pool of nodes

   mZonePtrs[mUsedZones]  = HEAP_NEW_ARRAY(BOPQuadHull, numNodes, gSimHeap);
   mZoneSizes[mUsedZones] = numNodes;

   // Update Stats

   mTotalNodesAllocted+= numNodes;
   mFreeNodes+= numNodes;

   // Is the free node pool empty?  if so reset it

   if (mFreeNodePoolZone == cFreeNodePoolEmpty)
   {
      mFreeNodePoolZone = mUsedZones;
      mFreeNodePoolIndex = 0;
   }

   mUsedZones++;

   // Performance Profiling Stuff

#ifdef _PROFILE_NODES
   mNewZoneCount++;
#endif

}


//==============================================================================
// BOPHullArrayManager::getNode
//==============================================================================
BOPQuadHull* BOPHullArrayManager::getNode(void)
{
	BOPQuadHull   *retVal;

	// No free nodes anywhere?  Then create a new heap

	if (mFreeNodes == 0)
	{
		allocateZone(mDefaultNodeGrowSize);
	}

#ifdef _PROFILE_NODES
	mAllocCount++;
   mHighAllocationCount = max(mHighAllocationCount, mTotalNodesAllocted-mFreeNodes+1);
#endif

	// First, we reuse any Nodes from the free (previously discarded) list

	if (mFreeListHead)
	{
		retVal = mFreeListHead;
		mFreeListHead = mFreeListHead->mNextNode;
		mFreeNodes--;

		return(retVal);
	}

	// Otherwise we take the first element from the free node pool

	retVal = mZonePtrs[mFreeNodePoolZone] + mFreeNodePoolIndex;

	mFreeNodes--;

	// Now we advance the free node pointers
	// And if we hit the end of this zone, we advance to the next zone
	// if we are totally empty, a new heap will be allocated next time

	mFreeNodePoolIndex++;

	if (mFreeNodePoolIndex == mZoneSizes[mFreeNodePoolZone])
	{
		mFreeNodePoolZone++;
		mFreeNodePoolIndex = 0;
		
		if (mFreeNodePoolZone == mUsedZones)
		{
			mFreeNodePoolZone = cFreeNodePoolEmpty;
			BASSERT(mFreeNodes == 0);
		}
	}

	return(retVal);

}


//==============================================================================
// BOPHullArrayManager::freeNode
//==============================================================================
void BOPHullArrayManager::freeNode(BOPQuadHull *theNode)
{
	// All discarded nodes are put into a FIFO list
	theNode->mNextNode = mFreeListHead;
	mFreeListHead = theNode;

	mFreeNodes++;

#ifdef _PROFILE_NODES
	mDeallocCount++;
#endif

}



//==============================================================================
// BOPHullArrayManager::reclaimAllNodes
//==============================================================================
void BOPHullArrayManager::reclaimAllNodes(void)
{
	mFreeListHead = NULL;

	mFreeNodePoolZone = 0;
	mFreeNodePoolIndex = 0;

	mFreeNodes = mTotalNodesAllocted;
	   
#ifdef _PROFILE_NODES
   mResetCount++;
#endif
	
}


#ifdef _PROFILE_NODES
//==============================================================================
// BOPHullArrayManager::resetPerfStats
//==============================================================================
void BOPHullArrayManager::resetPerfStats(void)
{
    mAllocCount++;
    mDeallocCount++;
    mNewZoneCount++;
    mResetCount++;
    mHighAllocationCount++;
}
#endif


































//==============================================================================
// BOPObQuadNodeListManager::constructor
//==============================================================================
BOPObQuadNodeListManager::BOPObQuadNodeListManager(const long numUsedLowLevelNodes)
{

	long numToAllocate, extendSize;

	// Level 0, 4 entry lists

	numToAllocate = (long) (numUsedLowLevelNodes * 1.25f);
	extendSize = (long) (numToAllocate * 0.25f);
		
	setListSizes(0, 4, 4, -1,  numToAllocate, extendSize);

	// Level 1, 16 entry lists

	numToAllocate = (long) (numUsedLowLevelNodes * 0.25f);
	extendSize = (long) (numToAllocate * 0.25f);

	setListSizes(1, 16, 16, 2, numToAllocate, extendSize);

	// Level 2, 64 entry lists

	numToAllocate = max ((long) (numUsedLowLevelNodes * 0.0625f), 16);
	extendSize = numToAllocate;

	setListSizes(2, 64, 64, 12, numToAllocate, extendSize);

	// Level 3, 256 entry lists

	numToAllocate = max ((long) (numUsedLowLevelNodes * 0.02f), 4);
	extendSize = numToAllocate;

	setListSizes(3, 256, 256, 56, numToAllocate, extendSize);

	// Level 4, 1024 entry lists

	numToAllocate = max ((long) (numUsedLowLevelNodes * 0.01f), 2);
	extendSize = numToAllocate;

	setListSizes(4, 1024, 1024, 240, numToAllocate, extendSize);

	// Level 5, 4096 entry lists

	numToAllocate = max ((long) (numUsedLowLevelNodes * 0.005f), 1);
	extendSize = numToAllocate;

	setListSizes(5, 4096, 4096, 1008, numToAllocate, extendSize);


}

//==============================================================================
// BOPObQuadNodeListManager::destructor
//==============================================================================
BOPObQuadNodeListManager::~BOPObQuadNodeListManager(void)
{

	for (long i = 0; i < cMaxNumNodeTypes; i++)
	{

		BOPQuadNodeListPoolInfo &ListInfo = mListPoolInfoArray[i];

		// Remove all allocated pools

		if (ListInfo.mNumPoolsActive > 0 && ListInfo.mListPoolPointers != NULL && ListInfo.mListPoolSizes != NULL)
		{
			for (long n = 0; n < ListInfo.mNumPoolsActive; n++)
			{
				if (ListInfo.mListPoolPointers[n] != NULL)
					HEAP_DELETE_ARRAY(ListInfo.mListPoolPointers[n], gSimHeap);
			}
			HEAP_DELETE_ARRAY(ListInfo.mListPoolPointers, gSimHeap);
			HEAP_DELETE_ARRAY(ListInfo.mListPoolSizes, gSimHeap);
		}
	}


}


//==============================================================================
// BOPObQuadNodeListManager::setListSizes
//==============================================================================
void BOPObQuadNodeListManager::setListSizes(const long nodeType, const long listSize, const long expandSize, 
                                            const long contractSize, const long initialPoolSize, const long extendPoolSize)
{
	BASSERT (nodeType >= 0 && nodeType < cMaxNumNodeTypes);


	// Setup the list properties (for when manipulated)

	mListSizes[nodeType]		= listSize;
	mExpandSize[nodeType]	= expandSize;
	mContractSize[nodeType]	= contractSize;

	// Reset the list pool information to initial state

	mListPoolInfoArray[nodeType].mListPoolPointers		= NULL;
	mListPoolInfoArray[nodeType].mListPoolSizes			= NULL;
	mListPoolInfoArray[nodeType].mNumPoolsActive			= 0;
	mListPoolInfoArray[nodeType].mMaxNumPoolsAvailable	= 0;

	mListPoolInfoArray[nodeType].mFirstFreeList			= NULL;
	mListPoolInfoArray[nodeType].mFreeListPoolNumber	= cFreeListPoolEmpty;
	mListPoolInfoArray[nodeType].mFreeListIndexNumber	= cFreeListPoolEmpty;

	mListPoolInfoArray[nodeType].mTotalLists			= 0;
	mListPoolInfoArray[nodeType].mFreeLists			= 0;
	mListPoolInfoArray[nodeType].mNewPoolSize			= extendPoolSize;

	mListPoolInfoArray[nodeType].mNodeTypeID        = nodeType;
	mListPoolInfoArray[nodeType].mListElementSize   = listSize;

#ifdef _PROFILE_LISTS
	mListPoolInfoArray[nodeType].mAllocCount			= 0; 
	mListPoolInfoArray[nodeType].mDeallocCount		= 0;
	mListPoolInfoArray[nodeType].mNewZoneCount		= 0;
	mListPoolInfoArray[nodeType].mResetCount			= 0;
	mListPoolInfoArray[nodeType].mHighAllocationCount = 0;
#endif

	// Allocate the first pool

   allocateListPool(mListPoolInfoArray[nodeType], initialPoolSize);

}

//==============================================================================
// BOPObQuadNodeListManager::
//==============================================================================
long BOPObQuadNodeListManager::getNodeListType(long numNodes)
{
	BASSERT(numNodes >= 0);

	for (long n = 0; n < cMaxNumNodeTypes ; n++)
	{
		if (numNodes <= mListSizes[n])
			return(n);
	}

	BASSERT(false);			// Oh boy, they want a list bigger than we support

	return(cInvalidNodeListType);


}


//==============================================================================
// BOPObQuadNodeListManager::allocateListPool
//==============================================================================
void BOPObQuadNodeListManager::allocateListPool(BOPQuadNodeListPoolInfo &theList, const long numLists)
{

	// is there room for a new pool?  If not, then expand the list

	if (theList.mNumPoolsActive >= theList.mMaxNumPoolsAvailable)
	{
		long newListSize = theList.mMaxNumPoolsAvailable + cNumPoolsToExpandBy;			// Add 16 at a time

		// Allocate new lists

		BOPObstructionNode*** newList = HEAP_NEW_ARRAY(BOPObstructionNode**, newListSize, gSimHeap);
		long * newSizes = HEAP_NEW_ARRAY(long, newListSize, gSimHeap);

		memset(newList, 0, sizeof(BOPObstructionNode*) * newListSize);
		memset(newSizes, 0, sizeof(long) * newListSize);

		// copy data

		if (theList.mNumPoolsActive > 0)
		{
			memcpy(newList, theList.mListPoolPointers, sizeof(BOPObstructionNode**) * theList.mNumPoolsActive);
			memcpy(newSizes, theList.mListPoolSizes, sizeof(long) * theList.mNumPoolsActive);
		}

		// delete current lists

		if (theList.mListPoolPointers != NULL)
			HEAP_DELETE_ARRAY(theList.mListPoolPointers, gSimHeap);

		if (theList.mListPoolSizes != NULL)
			HEAP_DELETE_ARRAY(theList.mListPoolSizes, gSimHeap);

		// Assign new lists

		theList.mListPoolPointers = newList;
		theList.mListPoolSizes = newSizes;

		theList.mMaxNumPoolsAvailable = newListSize;
	}

	// Determine size of new pool

	long numPointers = theList.mListElementSize * numLists;
	long index = theList.mNumPoolsActive;

	// Allocate new pool

	BASSERT (theList.mListPoolPointers[index] == NULL);

	theList.mListPoolPointers[index] = HEAP_NEW_ARRAY(BOPObstructionNode*, numPointers, gSimHeap);
	theList.mListPoolSizes[index] = numLists;

	// And update all the support info...
		
	theList.mNumPoolsActive++;
		
	if (theList.mFreeListPoolNumber	== cFreeListPoolEmpty)
	{
		theList.mFreeListPoolNumber = index;
		theList.mFreeListIndexNumber = 0;
	}

	theList.mTotalLists+= numLists;
	theList.mFreeLists+= numLists;


#ifdef _PROFILE_LISTS
	theList.mNewZoneCount++;
#endif

}

//==============================================================================
// BOPObQuadNodeListManager::getList
//==============================================================================
BOPObstructionNode** BOPObQuadNodeListManager::getList(const long numNodes, long &listType)
{
	BASSERT(numNodes > 0);

	listType = getNodeListType(numNodes);

	return(getList(listType));

}


//==============================================================================
// BOPObQuadNodeListManager::getList
//==============================================================================
BOPObstructionNode** BOPObQuadNodeListManager::getList(const long nodeType)
{
	BOPObstructionNode** newList;

	BOPQuadNodeListPoolInfo &ListInfo = mListPoolInfoArray[nodeType];

	// are we totally out of lists?

	if (ListInfo.mFreeLists == 0)
	{
		allocateListPool(ListInfo, ListInfo.mNewPoolSize);
	}

	// Update Stats
	
	ListInfo.mFreeLists--;

#ifdef _PROFILE_LISTS
	ListInfo.mAllocCount++;
	ListInfo.mHighAllocationCount = max(ListInfo.mHighAllocationCount, ListInfo.mTotalLists - ListInfo.mFreeLists);
#endif

	// Is there a recycled list available?

	if (ListInfo.mFirstFreeList)
	{
		newList = ListInfo.mFirstFreeList;
		ListInfo.mFirstFreeList = (BOPObstructionNode**) ListInfo.mFirstFreeList[0];

		return(newList);
	}

	// Otherwise, take the first list from the pool.

	newList = ListInfo.mListPoolPointers[ListInfo.mFreeListPoolNumber] + (ListInfo.mListElementSize * ListInfo.mFreeListIndexNumber);

	// Now we advance our free pool start to make that one allocated

	ListInfo.mFreeListIndexNumber++;
	
	if (ListInfo.mFreeListIndexNumber == ListInfo.mListPoolSizes[ListInfo.mFreeListPoolNumber])
	{
		ListInfo.mFreeListPoolNumber++;
		ListInfo.mFreeListIndexNumber = 0;

		if (ListInfo.mFreeListPoolNumber == ListInfo.mMaxNumPoolsAvailable )
		{
			ListInfo.mFreeListPoolNumber =	cFreeListPoolEmpty;
			BASSERT(ListInfo.mFreeLists == 0);
		}
	}

	// And return the new pointer list

	return(newList);
}



//==============================================================================
// BOPObQuadNodeListManager::freeList
//==============================================================================
void BOPObQuadNodeListManager::freeList(BOPObstructionNode **theList, const long nodeType)
{

	BOPQuadNodeListPoolInfo &ListInfo = mListPoolInfoArray[nodeType];

	theList[0] = (BOPObstructionNode*) ListInfo.mFirstFreeList;
	ListInfo.mFirstFreeList = theList;

	ListInfo.mFreeLists++;

#ifdef _PROFILE_LISTS
	ListInfo.mDeallocCount++;
#endif


}


//==============================================================================
// BOPObQuadNodeListManager::reclaimAllLists
//==============================================================================
void BOPObQuadNodeListManager::reclaimAllLists(long nodeType)
{

	BOPQuadNodeListPoolInfo &ListInfo = mListPoolInfoArray[nodeType];

	ListInfo.mFirstFreeList = NULL;
	
	ListInfo.mFreeListPoolNumber = 0;
	ListInfo.mFreeListIndexNumber = 0;

	ListInfo.mFreeLists = ListInfo.mTotalLists;

#ifdef _PROFILE_LISTS
   ListInfo.mResetCount++;
#endif

}


#ifdef _PROFILE_LISTS
//==============================================================================
// BOPObQuadNodeListManager::resetPerfStats
//==============================================================================
void BOPObQuadNodeListManager::resetPerfStats(void)
{
	for (long n = 0; n < cMaxNumNodeTypes; n++)
	{
		resetPerfStats(n);
	}

}


//==============================================================================
// BOPObQuadNodeListManager::resetPerfStats
//==============================================================================
void BOPObQuadNodeListManager::resetPerfStats(const long nodeType)
{
	mListPoolInfoArray[nodeType].mAllocCount				= 0; 
	mListPoolInfoArray[nodeType].mDeallocCount			= 0;
	mListPoolInfoArray[nodeType].mNewZoneCount			= 0;
	mListPoolInfoArray[nodeType].mResetCount				= 0;
	mListPoolInfoArray[nodeType].mHighAllocationCount	= 0;

}



#endif







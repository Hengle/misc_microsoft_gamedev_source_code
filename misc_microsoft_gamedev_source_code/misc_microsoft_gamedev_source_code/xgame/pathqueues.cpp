//==============================================================================
// pathqueues.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "pathqueues.h"

//#include <time.h>

//==============================================================================
// Defines
//#define DEBUG_HEAPINFO
//#define DEBUG_HASHSTATS
//#define DEBUG_GETOPENNODE
//#define DEBUG_ADDOPENNODE
//#define DEBUG_ADDCLOSEDNODE
//#define DEBUG_REMOVENODE
//#define DEBUG_ADDNODE
//#define DEBUG_STARTEND

const long clHashSize = 2048;            // Room for 2048 nodes
const float cfPointCompareEpsilon = 0.000001f;
const float cfCostCompareEpsilon  = 0.0001f;

// Some Q&D hash macros
#define _hashsize(n)       ((DWORD)1 << (n))
#define _hashmask(n)       (_hashsize(n) - 1)
#define _hashmix(a,b,c) \
{ \
      a -= b; a -= c; a ^= (c >> 13); \
      b -= c; b -= a; b ^= (a <<  8); \
      c -= a; c -= b; c ^= (b >> 13); \
      a -= b; a -= c; a ^= (c >> 12); \
      b -= c; b -= a; b ^= (a << 16); \
      c -= a; c -= b; c ^= (b >>  5); \
      a -= b; a -= c; a ^= (c >>  3); \
      b -= c; b -= a; b ^= (a << 10); \
      c -= a; c -= b; c ^= (b >> 15); \
   }

// Static Members
//BPathNode2PtrArray  BPathQueues::mOpenHash;
//BPathNode2PtrArray  BPathQueues::mClosedHash;
//BPathNode2PtrArray  BPathQueues::mAllocations;
//BPathHeap           BPathQueues::mOpenHeap;


//==============================================================================
// BPathQueues::BPathQueues
//==============================================================================
BPathQueues::BPathQueues(void)
{
   mpNodeFreeList = new BPathNode2FreeList;
   if (!mpNodeFreeList)
   {
      BASSERT(0);
      return;
   }

   // Perform Memory Allocations
   mOpenHash.setNumber(clHashSize);
   memset((BPathNode2 **)mOpenHash.getPtr(), '\0', sizeof(BPathNode2 *) * clHashSize);

   mClosedHash.setNumber(clHashSize);
   memset((BPathNode2 **)mClosedHash.getPtr(), '\0', sizeof(BPathNode2 *) * clHashSize);

   mAllocations.setNumber(clHashSize);
   mAllocations.setNumber(0);

   mOpenHeap.setNumber(clHashSize);
   mOpenHeap.setNumber(0);

} // BPathQueues::BPathQueues

//==============================================================================
// BPathQueues::~BPathQueues
//==============================================================================
BPathQueues::~BPathQueues(void)
{
   terminate();
   //releaseHashes();
   if (mpNodeFreeList)
      delete mpNodeFreeList;
} // BPathQueues::~BPathQueues

//==============================================================================
// BPathQueues::initialize
//==============================================================================
void BPathQueues::initialize(const BVector& vStart, const BVector& vGoal)
{
   mvStart = vStart;
   mvGoal = vGoal;

   // If we have Allocations already, it means someone didn't terminate
   // us.  Bassert. 
   if (mAllocations.getNumber())
   {
      BASSERT(0);
      return;
   }

   //releaseHashes();
   mOpenHeap.setNumber(0);
   memset((BPathNode2 **)mOpenHash.getPtr(), '\0', sizeof(BPathNode2 *) * clHashSize);
   memset((BPathNode2 **)mClosedHash.getPtr(), '\0', sizeof(BPathNode2 *) * clHashSize);

   mlLastIndex = 256L;
   
   #ifdef DEBUG_HASHSTATS
   mlNodesAdded = 0L;
   mlCollisions = 0L;
   mlMaxCollisionDepth = 0L;
   mlSumCollisionDepth = 0L;
   #endif

   #ifdef DEBUG_STARTEND
   blog("============================");
   blog("BPathQueues Initialized!!");
   #endif
   

} // BPathQueues::~BPathQueues

//==============================================================================
// BPathQueues::addOpenNode
//==============================================================================
BPathNode2 *BPathQueues::addOpenNode(const BVector &vPoint, BPathNode2 *pParent, long lHullIndex,
                                 long lPointIndex, bool bIsIntersection, long lDirFrom)
{

   // Calculate new cost
   BPathNode2 *pnode = NULL;

   float fNewCost = 0.0f;
   float fOldTotalCost = 0.0f;
   float fModifier = 0.0f;

   if (pParent)
      fNewCost = pParent->mfCost + pParent->mvPoint.xzDistance(vPoint);

   // Get the index for this point.
   unsigned long lIndex = hash(vPoint.x, vPoint.z, mlLastIndex);

   // Only need 11 bits (0 - 2047)
   lIndex = (lIndex & _hashmask(11));
   //mlLastIndex = lIndex;

   BPathNode2 *pnodeOpen = getNode(lIndex, vPoint, &mOpenHash);
   if (pnodeOpen)
   {
      if (pnodeOpen->mfCost <= fNewCost)
         return NULL;

      fOldTotalCost = pnodeOpen->mfCost + pnodeOpen->mfEstimate + pnodeOpen->mfModifier;
      pnode = pnodeOpen;
   }
   else
   {
      BPathNode2 *pnodeClosed = getNode(lIndex, vPoint, &mClosedHash);
      if (pnodeClosed)
      {
         if (pnodeClosed->mfCost <= fNewCost)
            return NULL;
         pnode = pnodeClosed;
         removeNode(lIndex, pnodeClosed, &mClosedHash);
      }
   }

   // If we didn't find the node in either the open or closed lists,
   // create a new one.
   if (pnode == NULL)
   {
      pnode = mpNodeFreeList->acquire();
      if (!pnode)
      {
         BASSERT(pnode);
         return NULL;
      }
      memset(pnode, '\0', sizeof(BPathNode2));
      mAllocations.add(pnode);
   }

   // Assign Parameters
   pnode->mvPoint = vPoint;
   pnode->mfCost = fNewCost;
   pnode->mfEstimate = vPoint.xzDistance(mvGoal);
   pnode->mfModifier = fModifier;
   pnode->mlHullIndex = lHullIndex;
   pnode->mlPointIndex = lPointIndex;
   pnode->mbIsIntersection = bIsIntersection;
   pnode->mlDirFrom = lDirFrom;
   pnode->mParent = pParent;
   pnode->mSpecialStart = false;
   // DO NOT SET pNext & pPrev to null here,
   // or you may VIOLATE an existing node, you idiot!

   // Add/Replace the item to the openheap
   BPathHeapElement item;
   item.fCost = pnode->mfCost + pnode->mfEstimate + pnode->mfModifier;
   item.lHashIndex = lIndex;

   #ifdef DEBUG_ADDOPENNODE
   blog("BPathQueues::addOpenNode");
   blog("\tAdding Node with Index: %d", lIndex);
   #endif

   if (pnodeOpen)
   {
      #ifdef DEBUG_ADDOPENNODE
      blog("\tNode Existed Already, Adjusting Heap.");
      #endif
      changeInHeap(lIndex, fOldTotalCost, item.fCost);
   }
   else
   {
      // Add the node to the open hash
      addNode(lIndex, pnode, &mOpenHash);
      // Add the new heap element to the open heap
      addToHeap(item);
      #ifdef DEBUG_ADDOPENNODE
      blog("\tNew Node Added to Open Hash & Heap..");
      #endif
   }
   return pnode;
}

//==============================================================================
// BPathQueues::addOpenNode
// A more generic version of addOpenNode, it doesn't include any parms
// for hullindex, direction from, etc, and allows you to optionally specify
// your own values for cost, estimate, and modifier.  If bUseHeauristics is
// true, then it uses the next three parms to determine the total cost of the node.
// if bUseHeurstics is false (the default), then it ignores the next three parms, and
// just use distance from parent node to this one as the cost, distance from
// this node to the goal as the estimate, and a modifier of 0.0.                                 
//==============================================================================
BPathNode2 *BPathQueues::addOpenNode(const BVector &vPoint, BPathNode2 *pParent, bool bUseHeuristics,
                                 float fCost, float fEstimate, float fModifier)
{

   // Calculate new cost
   BPathNode2 *pnode = NULL;

   float fNewCost = 0.0f;
   float fOldTotalCost = 0.0f;

   if (pParent)
   {
      if (bUseHeuristics)
         fNewCost = pParent->mfCost + fCost;
      else
         fNewCost = pParent->mfCost + pParent->mvPoint.xzDistance(vPoint);
   }

   // Get the index for this point.
   unsigned long lIndex = hash(vPoint.x, vPoint.z, mlLastIndex);

   // Only need 11 bits (0 - 2047)
   lIndex = (lIndex & _hashmask(11));
   //mlLastIndex = lIndex;

   BPathNode2 *pnodeOpen = getNode(lIndex, vPoint, &mOpenHash);
   if (pnodeOpen)
   {
      if (pnodeOpen->mfCost <= fNewCost)
         return NULL;

      fOldTotalCost = pnodeOpen->mfCost + pnodeOpen->mfEstimate + pnodeOpen->mfModifier;
      pnode = pnodeOpen;
   }
   else
   {
      BPathNode2 *pnodeClosed = getNode(lIndex, vPoint, &mClosedHash);
      if (pnodeClosed)
      {
         if (pnodeClosed->mfCost <= fNewCost)
            return NULL;
         pnode = pnodeClosed;
         removeNode(lIndex, pnodeClosed, &mClosedHash);
      }
   }

   // If we didn't find the node in either the open or closed lists,
   // create a new one.
   if (pnode == NULL)
   {
      pnode = mpNodeFreeList->acquire();
      if (!pnode)
      {
         BASSERT(pnode);
         return NULL;
      }
      memset(pnode, '\0', sizeof(BPathNode2));
      mAllocations.add(pnode);
   }

   // Assign Parameters
   pnode->mvPoint = vPoint;
   pnode->mfCost = fNewCost;
   if (bUseHeuristics)
   {
      pnode->mfEstimate = fEstimate;
      pnode->mfModifier = fModifier;
   }
   else
   {
      pnode->mfEstimate = vPoint.xzDistance(mvGoal);
      pnode->mfModifier = 0.0f;
   }
   pnode->mlHullIndex = -1L;
   pnode->mlPointIndex = -1L;
   pnode->mbIsIntersection = false;
   pnode->mlDirFrom = -1L;
   pnode->mParent = pParent;
   // DO NOT SET pNext & pPrev to null here,
   // or you may VIOLATE an existing node, you idiot!

   // Add/Replace the item to the openheap
   BPathHeapElement item;
   item.fCost = pnode->mfCost + pnode->mfEstimate + pnode->mfModifier;
   item.lHashIndex = lIndex;

   #ifdef DEBUG_ADDOPENNODE
   blog("BPathQueues::addOpenNode");
   blog("\tAdding Node with Index: %d", lIndex);
   #endif

   if (pnodeOpen)
   {
      #ifdef DEBUG_ADDOPENNODE
      blog("\tNode Existed Already, Adjusting Heap.");
      #endif
      changeInHeap(lIndex, fOldTotalCost, item.fCost);
   }
   else
   {
      // Add the node to the open hash
      addNode(lIndex, pnode, &mOpenHash);
      // Add the new heap element to the open heap
      addToHeap(item);
      #ifdef DEBUG_ADDOPENNODE
      blog("\tNew Node Added to Open Hash & Heap..");
      #endif
   }
   return pnode;
}

//==============================================================================
// BPathQueues::getOpenNode
//==============================================================================
BPathNode2 *BPathQueues::getOpenNode()
{
   #ifdef DEBUG_GETOPENNODE
   blog("----> getOpenNode");
   #endif

   BPathHeapElement itemTop;

   if (!removeTopFromHeap(itemTop))
   {
      #ifdef DEBUG_GETOPENNODE
      blog("\tTop of Heap returned false");
      dumpHeap();
      #endif
      return NULL;
   }

   BPathNode2 *pnode = mOpenHash[itemTop.lHashIndex];
   if (!pnode)
   {
      BASSERT(0);
      return NULL;
   }
   // If there's more than one, find the one that matches our total cost.
   if (pnode->mNext == NULL)
   {
      removeNode(itemTop.lHashIndex, pnode, &mOpenHash);
      pnode->mNext = pnode->mPrev = NULL;
      #ifdef DEBUG_GETOPENNODE
      blog("\tReturning with single entry.");
      blog("<---- getOpenNode");
      #endif
      return pnode;
   }

   while (pnode)
   {
      float fTotal = pnode->mfCost + pnode->mfEstimate + pnode->mfModifier;
      #ifdef DEBUG_GETOPENNODE
      blog("Checking Index %ld with a cost of %f for match..", itemTop.lHashIndex, fTotal);
      #endif
      if (_fabs(fTotal - itemTop.fCost) < cfCostCompareEpsilon)
      {
         removeNode(itemTop.lHashIndex, pnode, &mOpenHash);
         pnode->mNext = pnode->mPrev = NULL;
         #ifdef DEBUG_GETOPENNODE
         blog("\tReturning with collision entry.");
         blog("<---- getOpenNode");
         #endif
         return pnode;
      }
      else
         pnode = pnode->mNext;
   }

   #ifdef DEBUG_GETOPENNODE
   blog("\tUnable to find matching node, returning false");
   blog("\tIndex: %d", itemTop.lHashIndex);
   blog("\tCost: %f", itemTop.fCost);
   blog("<---- getOpenNode");
   #endif
   return NULL;
}

//==============================================================================
// BPathQueues::addClosedNode
//==============================================================================
void BPathQueues::addClosedNode(BPathNode2 *pnode)
{
   // Calculate the hash index
   unsigned long lIndex = hash(pnode->mvPoint.x, pnode->mvPoint.z, mlLastIndex);

   // Only need 11 bits (0 - 2047)
   lIndex = (lIndex & _hashmask(11));
//   mlLastIndex = lIndex;
   #ifdef DEBUG_ADDCLOSEDNODE
   blog("BPathQueues::addClosedNode");
   blog("Adding Node with Index: %d", lIndex);
   #endif
   
   addNode(lIndex, pnode, &mClosedHash);

   return;
}

//==============================================================================
// BPathQueues::newNode
// Use this function if you need an arbitrary node with a specified
// point value and parent.
//==============================================================================
BPathNode2 *BPathQueues::newNode(const BVector &vPoint, BPathNode2 *pParent)
{
   // Get a Node from the pool.
   BPathNode2 *pnode = mpNodeFreeList->acquire();
   if (!pnode)
   {
      BASSERT(0);
      return NULL;
   }

   memset(pnode, '\0', sizeof(BPathNode2));
   mAllocations.add(pnode);

   // Assign Parameters
   pnode->mvPoint = vPoint;
   pnode->mfCost = 0.0f;
   pnode->mfEstimate = 0.0f;
   pnode->mfModifier = 0.0f;
   pnode->mlHullIndex = -1L;
   pnode->mlPointIndex = -1L;
   pnode->mbIsIntersection = false;
   pnode->mParent = pParent;
   pnode->mNext = pnode->mPrev = NULL;

   return pnode;

}

/*
//==============================================================================
// BPathQueues::releaseNodes
// Give a path back to this function, and it will release the 
// nodes back to the pool.
//==============================================================================
void BPathQueues::releaseNodes(BPathNode2 *pNode)
{
   BPathNode2 *pNext = NULL;
   while (pNode)
   {
      pNext = pNode->mParent;
      mpNodeFreeList->release(pNode);
      pNode = pNext;
   }
   return;
}
*/

//==============================================================================
// DWORD BPathQueues::hash
// This hash function is from Bob Jenkins, whom published it in Dr. Dobbs,
// in 1996.  It's fast, and produces very few duplicates.  It's modified slightly
// to assume an 8 byte key.
//==============================================================================
DWORD BPathQueues::hash(float fX, float fZ, register unsigned long initval)
{
   register unsigned char *k0;
   register unsigned char *k1;
   register unsigned long a, b, c;

   k0 = (unsigned char *)&fX;
   k1 = (unsigned char *)&fZ;

   a = b = 0x923779b9;      // Golden ratio.  Pretty much arbitrary value

   c = initval + 8;         // Assume 8 byte keys

   b += ((unsigned long)k1[3] << 24);
   b += ((unsigned long)k1[2] << 16);
   b += ((unsigned long)k1[1] << 8);
   b += k1[0];
   a += ((unsigned long)k0[3] << 24);
   a += ((unsigned long)k0[2] << 16);
   a += ((unsigned long)k0[1] << 8);
   a += k0[0];

   _hashmix(a, b, c);

   return c;

}


//==============================================================================
// BPathNode2 *getNode(lIndex, vPoint, hash)
// Checkes to see if a given node exists in the appropriate has table.  If bOpen
// is true, it looks in the open hash, otherwise it looks in the closed hash.
// It uses lIndex to go to the appropriate location
//==============================================================================
BPathNode2 *BPathQueues::getNode(unsigned long lIndex, const BVector &vPoint, BPathNode2PtrArray *pHash)
{

   BASSERT(pHash);

   BPathNode2 *pNode = (*pHash)[lIndex];

   if (pNode == NULL)
      return NULL;

   while (pNode)
   {
      if ((_fabs(vPoint.x - pNode->mvPoint.x) < cfPointCompareEpsilon) &&
         (_fabs(vPoint.z - pNode->mvPoint.z) < cfPointCompareEpsilon))
      {
         return pNode;
      }
      pNode = pNode->mNext;
   }
   return pNode;
}

//==============================================================================
// void removeNode(lIndex, pnodeRemove, hash)
//==============================================================================
void BPathQueues::removeNode(unsigned long lIndex, BPathNode2 *pNodeRemove, BPathNode2PtrArray *pHash)
{

   BASSERT(pNodeRemove);
   BASSERT(pHash);

   BPathNode2 *pNode = (*pHash)[lIndex];
   if (!pNode)
   {
      BASSERT(0);
      return;
   }

   #ifdef DEBUG_REMOVENODE
   blog("BPathQueues::removeNode");
   blog("\tRemoving Node :%ld", lIndex);
   if (*pHash == mOpenHash)
   {
      blog("\tFrom Open Hash");
   }
   else
      blog("\tFrom Closed Hash");

   #endif

   // If this is the first one, move the others up into the spot, if they exist.
   if (pNode == pNodeRemove)
   {
      if (pNode->mNext)
      {
         (*pHash)[lIndex] = pNode->mNext;
         ((*pHash)[lIndex])->mPrev = NULL;
         #ifdef DEBUG_REMOVENODE
         blog("\tRemoved top of Node Chain.");
         #endif
      }
      else
      {
         (*pHash)[lIndex] = NULL;
         #ifdef DEBUG_REMOVENODE
         blog("\tRemoved Single Node.");
         #endif
      }

      pNodeRemove->mNext = NULL;
      pNodeRemove->mPrev = NULL;


   }
   else
   {
      pNode = pNode->mNext;
      while (pNode)
      {
         if (pNode == pNodeRemove)
         {
            if (pNode->mPrev)
               (pNode->mPrev)->mNext = pNode->mNext;
            if (pNode->mNext)
               (pNode->mNext)->mPrev = pNode->mPrev;         

            pNodeRemove->mNext = NULL;
            pNodeRemove->mPrev = NULL;

            #ifdef DEBUG_REMOVENODE
            blog("\tNode in the middle of the chain.");
            #endif

            break;

         }
         else
            pNode = pNode->mNext;
      }
   }

   #ifdef DEBUG_REMOVENODE
   blog("\tIndex is now:");
   pNode = (*pHash)[lIndex];
   while (pNode)
   {
      blog("\t\t%f", pNode->mfCost + pNode->mfEstimate + pNode->mfModifier);
      pNode = pNode->mNext;
   }
   blog("\tEnd of Index");
   #endif
   return;
}


//==============================================================================
// void addNode(lIndex, pnodeAdd, bUseOpen)
//==============================================================================
void BPathQueues::addNode(unsigned long lIndex, BPathNode2 *pNodeAdd, BPathNode2PtrArray *pHash)
{
   BASSERT(pNodeAdd);
   BASSERT(pHash);

   #ifdef DEBUG_ADDNODE
   blog("BPathQueues::addNode");
   blog("\tAdding Node :%ld", lIndex);
   if (*pHash == mOpenHash)
      blog("\tto Open Hash");
   else
      blog("\tto Closed Hash");
   #endif

   BPathNode2 *pNode = (*pHash)[lIndex];
   #ifdef DEBUG_HASHSTATS
   long lDepth = 0L;
   ++mlNodesAdded;
   #endif

   if (pNode == NULL)
   {
      (*pHash)[lIndex] = pNodeAdd;
      #ifdef DEBUG_ADDNODE
      blog("\tNo Collision Detected.  Node Added.");
      #endif

   }
   else
   {
      #ifdef DEBUG_HASHSTATS
      ++mlCollisions;
      #endif
      while (pNode->mNext)
      {
         pNode = pNode->mNext;
         #ifdef DEBUG_HASHSTATS
         ++lDepth;
         #endif;
      }

      #ifdef DEBUG_HASHSTATS
      if (lDepth > mlMaxCollisionDepth)
         mlMaxCollisionDepth = lDepth;
      mlSumCollisionDepth += lDepth;
      #endif

      pNode->mNext = pNodeAdd;
      pNodeAdd->mPrev = pNode;
      pNodeAdd->mNext = NULL;

      #ifdef DEBUG_ADDNODE
      blog("\tCollision Detected.  Node Added at End.");
      #endif

   }
   #ifdef DEBUG_ADDNODE
   blog("\tIndex is now:");
   pNode = (*pHash)[lIndex];
   while (pNode)
   {
      blog("\t\t%f", pNode->mfCost + pNode->mfEstimate + pNode->mfModifier);
      pNode = pNode->mNext;
   }
   blog("\tEnd of Index");
   #endif
   return;
}

//==============================================================================
// void addToHeap(BPathHeapElement &item)
//==============================================================================
void BPathQueues::addToHeap(BPathHeapElement &heapItem)
{

   #ifdef DEBUG_HEAPINFO
   blog(".");
   blog("----> addToHeap Cost: %8.3f Index: %ld",
      heapItem.fCost, heapItem.lHashIndex);
   #endif
   long n = mOpenHeap.getNumber();
   BPathHeapElement itemCurrent;
   if (n < 1)
   {
      itemCurrent.fCost = -cMaximumFloat;
      itemCurrent.lHashIndex = -1L;
      mOpenHeap.add(itemCurrent);
      n = 1;
   }

   mOpenHeap.setNumber(n+1);
   mOpenHeap[n] = heapItem;

   // UpHeap
   itemCurrent = mOpenHeap[n];
   while (mOpenHeap[n>>1].fCost >= itemCurrent.fCost)
   {
      mOpenHeap[n] = mOpenHeap[n>>1];
      n = n >> 1;
   }
   mOpenHeap[n] = itemCurrent;

   #ifdef DEBUG_HEAPINFO
   blog("\tHeap after add:");
   dumpHeap();
   blog("<---- addToHeap");
   #endif
}

//==============================================================================
// void removeTopFromHeap(BPathHeapElement &item)
// true if an item was successfully removed, false if the queue is empty.
//==============================================================================
bool BPathQueues::removeTopFromHeap(BPathHeapElement &heapItem)
{
   #ifdef DEBUG_HEAPINFO
   blog(".");
   blog("----> removeTopFromHeap");
   #endif

   long n = mOpenHeap.getNumber();
   if (n <= 1)
      return false;

   heapItem = mOpenHeap[1];

   #ifdef DEBUG_HEAPINFO
   blog("\tCost %8.3f Index: %ld", heapItem.fCost, heapItem.lHashIndex);
   #endif

   mOpenHeap[1] = mOpenHeap[--n];
   
   BPathHeapElement itemCurrent;

   // DownHeap
   long k = 1;
   long j = 0;
   long limit = n >> 1;
   itemCurrent = mOpenHeap[k];
   while (k <= limit)
   {
      j = k + k;     // j is the first child of k, j+1 is the second child of k.
      // We want to compare the smaller of the two children
      if (j < n && mOpenHeap[j].fCost > mOpenHeap[j+1].fCost)
         ++j;

      // If we're already smaller than the smallest of our two children, we're done.
      if (itemCurrent.fCost <= mOpenHeap[j].fCost)
         break;

      // Otherise, Exchange me with the smallest of my two children, and continue.
      mOpenHeap[k] = mOpenHeap[j];
      k = j;
   }
   mOpenHeap[k] = itemCurrent;
   mOpenHeap.setNumber(n);

   #ifdef DEBUG_HEAPINFO
   blog("\tHeap after remove:");
   dumpHeap();
   blog("<---- removeTopFromHeap");
   #endif

   return true;

}


//==============================================================================
// changeInHeap
// Find the existing element in the heap with the existing index and oldcost,
// change it's cost to the new cost, and fix up the heap.
//==============================================================================
void BPathQueues::changeInHeap(long lIndex, float fOldCost, float fNewCost)
{

   #ifdef DEBUG_HEAPINFO
   blog(".");
   blog("----> changeInHeap Index: %ld OldCost: %8.3f NewCost %8.3f",
      lIndex, fOldCost, fNewCost);
   #endif

   long n = mOpenHeap.getNumber();
   BPathHeapElement itemCurrent;

   long k = 0;
   long j = 0;
   long limit = n >> 1;
   while (k <= limit)
   {
      j = k + k;
      // check child 1
      if (mOpenHeap[j].lHashIndex == lIndex &&
         (_fabs(mOpenHeap[j].fCost - fOldCost) < cfCostCompareEpsilon))
         break;
      j++;
      // check child 2
      if (mOpenHeap[j].lHashIndex == lIndex &&
         (_fabs(mOpenHeap[j].fCost - fOldCost) < cfCostCompareEpsilon))
         break;
      k++;
   }
   // If we didn't find it to change, then bail.
   if (k > limit)
      return;

   mOpenHeap[j].fCost = fNewCost;

   // UpHeap
   itemCurrent = mOpenHeap[j];
   while (mOpenHeap[j>>1].fCost >= itemCurrent.fCost)
   {
      mOpenHeap[j] = mOpenHeap[j>>1];
      j = j >> 1;
   }
   mOpenHeap[j] = itemCurrent;

   #ifdef DEBUG_HEAPINFO
   blog("\tHeap after change:");
   dumpHeap();
   blog("<---- changeInHeap");
   #endif
}

//==============================================================================
// dumpHeap
// Debug routine to dump the contents of the open heap.
// 
//==============================================================================
void BPathQueues::dumpHeap()
{
   long n = mOpenHeap.getNumber();
   for (long l = 0; l < n; l++)
   {
      blog("\t[%02d]\t%8.3f\t%05d",
         l, mOpenHeap[l].fCost, mOpenHeap[l].lHashIndex);
   }
   return;
}

//==============================================================================
// hashStats
// Debug routine to give us some hash info
//==============================================================================
void BPathQueues::hashStats()
{
   blog("\tHash Stats:");
   blog("\tNodes Added:    %ld", mlNodesAdded);
   blog("\tCollisions:     %ld", mlCollisions);
   blog("\tMax Coll Depth: %ld", mlMaxCollisionDepth);
   blog("\tAvg Coll Depth: %ld", (mlCollisions)?mlSumCollisionDepth/mlCollisions:0L);

}

//==============================================================================
// BPathQueues::terminate()
// Releases the ptrs back to the pool.  
// NOTE:  DO NOT CALL THIS IF YOU are still using the pointers.. DUH. 
//==============================================================================
void BPathQueues::terminate()
{
   long lNum = mAllocations.getNumber();
   for (long l = 0; l < lNum; l++)
   {
      mpNodeFreeList->release(mAllocations[l]);
   }
   mAllocations.setNumber(0);

   #ifdef DEBUG_STARTEND
   blog("BPathQueues TERMINATED");
   blog("============================");
   #endif

   return;
}


   





//==============================================================================
// eof: pathqueues.cpp
//==============================================================================

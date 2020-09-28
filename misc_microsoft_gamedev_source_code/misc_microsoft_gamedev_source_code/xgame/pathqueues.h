//==============================================================================
// pathqueues.h
//
// BPathQeueues is designed to basically be your one stop shopping for
// your A* pathing datastructure needs.  It's job is to maintain an Open list
// a closed list, and to provide methods for inserting nodes into the list and
// retrieving the lowest cost nodes from the list.
// NOTE: We use the term "lists" here strictly in the general sense, as the
// underlying datastructures will most likely be priority queues of some sort.
// 
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BPathNode2;
typedef BDynamicSimArray<BPathNode2 *> BPathNode2PtrArray;

//==============================================================================
// Const declarations


//==============================================================================

class BPathNode2
{
   public:
                              BPathNode2()
                              { mNext = mPrev = NULL; }
//      void                    clearChildren(void) {mChildren.setNumber(0);}
//      void                    addChild(BPathNode2 *child) {mChildren.add(child);}

      BVector                 mvPoint;

      long                    mlHullIndex;
      long                    mlPointIndex;

      float                   mfCost;
      float                   mfEstimate;
      float                   mfModifier;

//      long                    mlHeapIndex;                  // Index into Priority Q
      long                    mlDirFrom;                    // Directin we took to get to this queue.

      BPathNode2              *mParent;
//      BPathNode2PtrArray      mChildren;
      BPathNode2              *mNext, *mPrev;

      bool                    mbIsIntersection;
      bool                    mSpecialStart;
};


class BPathHeapElement
{
   public:
      float                   fCost;
      long                    lHashIndex;
};

typedef BDynamicSimArray<BPathHeapElement> BPathHeap;
typedef BFreeList<BPathNode2, 11> BPathNode2FreeList;

class BPathQueues
{
   public:
      // Constructors
      BPathQueues( void );

      // Destructors
      ~BPathQueues( void );

      // Functions
      void                    initialize(const BVector& vStart, const BVector& vGoal); // Resets the queues, and saves the start and
                                                                                       // goal locations.
      void                    terminate();                                             // Releases all the pointers back to the pool.
         
      BPathNode2              *addOpenNode(const BVector &vPoint, BPathNode2 *pParent,         // Insert a node into the open list.
                                 long lHullIndex, long lPointIndex, bool bIsIntersection, 
                                 long lDirFrom = -1L);

      BPathNode2              *addOpenNode(const BVector &vPoint, BPathNode2 *pParent,        // More generic addOpenNode, with ability
                              bool bUseHeuristics = false, float fCost = 0.0f,         // to specify you're own values for cost, est,
                              float fEstimate = 0.0f, float fModifier = 0.0f);         // and modifier.

      void                    addClosedNode(BPathNode2 *pnode);                        // Move an exiting node to the closed list

      BPathNode2              *getOpenNode();                                          // Get the next (lowest cost) open node
      BPathNode2              *newNode(const BVector &vPoint, BPathNode2 *pParent);          // Create a new node with point & parent

      // void                    releaseNodes(BPathNode2 *pNode);                         // Release a Node Chain back to the pool.
      // void                    releaseHashes();
      void                    hashStats();                                             // Ask for hash stats
      // Variables

   protected:

      // Functions

      DWORD                   hash(float fX, float fZ, register unsigned long initval);// Fetch the hash index

      // Hash Access Functions
      BPathNode2              *getNode(unsigned long lIndex, const BVector &vPoint, 
                                 BPathNode2PtrArray *pHash);
      void                    removeNode(unsigned long lIndex, BPathNode2 *pNodeRemove,
                                 BPathNode2PtrArray *pHash);
      void                    addNode(unsigned long lIndex, BPathNode2 *pNodeAdd,
                                 BPathNode2PtrArray *pHash);


      // Heap Manipulation functions
      void                    addToHeap(BPathHeapElement &heapItem);
      bool                    removeTopFromHeap(BPathHeapElement &heapItem);
      void                    changeInHeap(long lIndex, float fOldCost, float fNewCost);
      void                    dumpHeap();

      // Variables
      BVector                 mvStart;
      BVector                 mvGoal;

       BPathHeap        mOpenHeap;                                                // Open Queue
      
       BPathNode2PtrArray  mOpenHash;
       BPathNode2PtrArray  mClosedHash;
       BPathNode2PtrArray  mAllocations;

      long                    mlLastIndex;

      BPathNode2FreeList      *mpNodeFreeList;

      // Hash Statistics
      long                    mlNodesAdded;
      long                    mlCollisions;
      long                    mlMaxCollisionDepth;
      long                    mlSumCollisionDepth;

      //long                    mFoo;

   private:

      // Functions

      // Variables

}; // BPathQueues
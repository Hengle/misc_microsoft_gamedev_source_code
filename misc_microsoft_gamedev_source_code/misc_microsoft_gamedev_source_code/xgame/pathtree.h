//==============================================================================
// pathtree.h
// This class makes use of BPathNode to create pathing trees.  All of the stand-
// alone c functions that were in the lowlevelpather module have been encapsulated
// within this class.  The intention is (1) provide a little more object oriented
// approach to the path tree, and (2) reduce the content of lowlevelpather to a
// more manageable level.
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


//==============================================================================
// BPathNode - The things BPathTree keeps track of.  
//==============================================================================
class BPathNode;
typedef BDynamicSimArray<BPathNode*> BPathNodePtrArray;
class BPathNode
{
   public:
      void                    clearChildren(void) {mChildren.setNumber(0);}
      void                    addChild(BPathNode *child) {mChildren.add(child);}

      BVector                 mvPoint;

      long                    mlHullIndex;
      long                    mlPointIndex;

      float                   mfCost;
      float                   mfEstimate;

      bool                    mbIsIntersection;

      // Used for pathing within a hull.
      long                    mArrivalDirection;
      long                    mObstructionID;
      long                    mSegmentIndex;

      BPathNode               *mParent;
      BPathNodePtrArray       mChildren;
      BPathNode               *mNext, *mPrev;
};


//==============================================================================
class BPathTree
{
   public:
      // Constructors
      BPathTree( void );

      // Destructors
      ~BPathTree( void );

      // Functions
      void                    addNodeSorted(BPathNode *node);
      void                    addNode(BPathNode *node);
      static void             removeNode(BPathNode *node);
      BPathNode               *getNode(bool allocate);
      BPathNode               *findMatchingNode(long hullIndex, long pointIndex);
      BPathNode               *findMatchingNode(const BVector &point, float errorTolerance, BPathNode *parentNode);
      BPathNode               *findLowerCostMatchingNode(long hullIndex, long pointIndex, const float cost);
      BPathNode               *findLowerCostMatchingNode(const BVector &point, const float cost, 
                                 const float errorTolerance, BPathNode *parentNode);
      void                    moveAllNodes(BPathTree &fromList);
      void                    deleteList();
      void                    moveAllChildren(BPathNode *node);

      BPathNode               *addNewOpen(BPathTree &closedList, BPathTree &unusedList, BPathNode *parent,
                                 const BVector &point, const long hullIndex, const long pointIndex, const BVector *goal, 
                                 const bool intersectionPoint, const long arrivalDirection, const long obstructionID, 
                                 const long segmentIndex, const bool exactPointCheck, const float errorTolerance);

      // Variables

   protected:

      // Functions

      // Variables
      //long                    mFoo;
      BPathNode               mHead;

   private:

      // Functions

      // Variables

}; // BPathTree





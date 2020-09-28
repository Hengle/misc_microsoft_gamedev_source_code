//==============================================================================
// pathtree.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "pathtree.h"

//==============================================================================
// Defines


//==============================================================================
// BPathTree::BPathTree
//==============================================================================
BPathTree::BPathTree(void)
{
   mHead.mNext = NULL;
   mHead.mPrev = NULL;
   mHead.mParent = NULL;
   mHead.clearChildren();

} // BPathTree::BPathTree

//==============================================================================
// BPathTree::~BPathTree
//==============================================================================
BPathTree::~BPathTree(void)
{
   deleteList();

} // BPathTree::~BPathTree

//==============================================================================
// BPathTree::
//==============================================================================


//==============================================================================
// BPathTree::addNodeSorted
//==============================================================================
void BPathTree::addNodeSorted(BPathNode *node)
{
   if(!node)
   {
      BASSERT(0);
      return;
   }

   // Look for the right spot.
   float costPlusEstimate = node->mfCost + node->mfEstimate;
   BPathNode *curr = &mHead;
   while(curr->mNext && (curr->mNext->mfCost+curr->mNext->mfEstimate <= costPlusEstimate))
   {
      // Sanity check to make sure we don't add the same node twice.
      if (curr == node)
      {
         BASSERT(0);
         return;
      }
      curr = curr->mNext;
   }

   // Insert.
   node->mNext = curr->mNext;
   curr->mNext = node;
   node->mPrev = curr;
   if(node->mNext)
      node->mNext->mPrev = node;

   #ifdef DEBUG_ADDNODESORTED
   blog("addNodeSorted (node: costPlusEstimate=%0.4f)--------------------------------------------------------", 
      node->mfCost + node->mfEstimate);
   curr=mHead.mNext;
   while(curr)
   {
      blog("costPlusEstimate: %0.4f  Point (%0.2f, %0.2f, %0.2f)  hullIndex: %d  pointIndex: %d",
         curr->mfCost+curr->mfEstimate, curr->mvPoint.x, curr->mvPoint.y, curr->mvPoint.z, curr->mlHullIndex, curr->mlPointIndex);
      if(curr->mParent)
         blog("       parent -> costPlusEstimate: %0.4f  Point (%0.2f, %0.2f, %0.2f)  hullIndex: %d  pointIndex: %d",
            curr->mParent->mfCost+curr->mParent->mfEstimate, curr->mParent->mvPoint.x, curr->mParent->mvPoint.y, curr->mParent->mvPoint.z, curr->mParent->mlHullIndex, curr->mParent->mlPointIndex);
   
      curr=curr->mNext;
   }
   blog("addNodeSorted (end)--------------------------------------------------------");
   #endif DEBUG_ADDNODESORTED
}


//==============================================================================
// BPathTree::addNode
//==============================================================================
void BPathTree::addNode(BPathNode *node)
{
   if(!node)
   {
      BASSERT(0);
      return;
   }
   if (node == mHead.mNext)
   {
      BASSERT(0);
      return;
   }
   node->mNext = mHead.mNext;
   mHead.mNext = node;
   node->mPrev = &mHead;
   if(node->mNext)
      node->mNext->mPrev = node;
}


//==============================================================================
// BPathTree::removeNode
//==============================================================================
void BPathTree::removeNode(BPathNode *node)
{
   if(!node || !node->mPrev)
   {
      BASSERT(0);
      return;
   }

   node->mPrev->mNext = node->mNext;
   if(node->mNext)
      node->mNext->mPrev = node->mPrev;
   node->mNext = node->mPrev = NULL;
}


//==============================================================================
// BPathTree::getNode
//==============================================================================
BPathNode *BPathTree::getNode(bool allocate)
{
   BPathNode *node = NULL;

   // If there is anything in the given list, pull the first node from it.
   if(mHead.mNext)
   {
      node = mHead.mNext;
      mHead.mNext = mHead.mNext->mNext;
      if(mHead.mNext)
         mHead.mNext->mPrev = &mHead;
   }
   else if(allocate)
   {
      // Nothing in the given mHead, so we must allocate a new node.
      node = new BPathNode;
      BASSERT(node);
   }

   return(node);
}


//==============================================================================
// BPathTree::findMatchingNode
//==============================================================================
BPathNode *BPathTree::findMatchingNode(long hullIndex, long pointIndex)
{
   BPathNode *curr = mHead.mNext;
   while(curr)
   {
      // Return the pointer if we have a match.
      if(curr->mlHullIndex==hullIndex && curr->mlPointIndex==pointIndex)
         return(curr);

      curr=curr->mNext;
   }

   return(NULL);
}


//==============================================================================
// BPathTree::findMatchingNode
//==============================================================================
BPathNode *BPathTree::findMatchingNode(const BVector &point, float errorTolerance, BPathNode *parentNode)
{
   parentNode;

   BPathNode *curr = mHead.mNext;
   while(curr)
   {
/*
      // Return the pointer if we have a match.
      if(curr == parentNode)
      {
         if(_fabs(curr->mvPoint.x-point.x) < cFloatCompareEpsilon)
         {
            if(_fabs(curr->mvPoint.z-point.z) < cFloatCompareEpsilon)
               return(curr);
         }
      }
      else
      {
*/
         if(_fabs(curr->mvPoint.x-point.x) < errorTolerance)
         {
            if(_fabs(curr->mvPoint.z-point.z) < errorTolerance)
               return(curr);
         }
//      }

      curr=curr->mNext;
   }

   return(NULL);
}


//==============================================================================
// findLowerCostMatchingNode
//==============================================================================
BPathNode *BPathTree::findLowerCostMatchingNode(long hullIndex, long pointIndex, const float cost)
{
   BPathNode *curr = mHead.mNext;
   while(curr)
   {
      // Return the pointer if we have a match.
      if(curr->mlHullIndex==hullIndex && curr->mlPointIndex==pointIndex && curr->mfCost<=cost)
         return(curr);

      curr=curr->mNext;
   }

   return(NULL);
}


//==============================================================================
// BPathTree::findLowerCostMatchingNode
//==============================================================================
BPathNode *BPathTree::findLowerCostMatchingNode(const BVector &point, const float cost, 
   const float errorTolerance, BPathNode *parentNode)
{
   parentNode;

   BPathNode *curr = mHead.mNext;
/*
   while (curr)
   {
      if (curr->mfCost < cost &&
          _fabs(curr->mvPoint.x - point.x) < errorTolerance &&
          _fabs(curr->mvPoint.z - point.z) < errorTolerance)
         return curr;

      curr = curr->mNext;
   }
*/


   while(curr)
   {
      // Return the pointer if we have a match.
      if(curr->mfCost<=cost)
      {
/*
         if(curr==parentNode)
         {
            if(_fabs(curr->mvPoint.x-point.x) < cFloatCompareEpsilon)
            {
               if(_fabs(curr->mvPoint.z-point.z) < cFloatCompareEpsilon)
                  return(curr);
            }
         }
         else
         {
*/
            if(_fabs(curr->mvPoint.x-point.x) < errorTolerance)
            {
               if(_fabs(curr->mvPoint.z-point.z) < errorTolerance)
                  return(curr);
            }
//         }
      }

      curr=curr->mNext;
   }

   return NULL;
}


//==============================================================================
// moveAllNodes
// Move All Nodes from the list passed in to this list
//==============================================================================
void BPathTree::moveAllNodes(BPathTree &fromList)
{
   // If there is nothing in the from list, there is nothing to do.
   if(!fromList.mHead.mNext)
      return;

   // Find the end of the fromList.
   BPathNode *end = fromList.mHead.mNext;
   while(end->mNext)
   {
      // Little circular pointer checking..
      if (end->mNext == end)
      {
         BASSERT(0);
         return;
      }
      end = end->mNext;
   }

   // Hook fromList to front of toList.
   end->mNext = mHead.mNext;
   if(mHead.mNext)
      mHead.mNext->mPrev = end;
   mHead.mNext = fromList.mHead.mNext;
   fromList.mHead.mNext = NULL;
}


//==============================================================================
// BPathTree::deleteList
//==============================================================================
void BPathTree::deleteList()
{
   BPathNode *curr = mHead.mNext;
   while(curr)
   {
      BPathNode *next = curr->mNext;
      delete curr;
      curr=next;
   }
}


//==============================================================================
// BPathTree::moveAllChildren
//==============================================================================
void BPathTree::moveAllChildren(BPathNode *node)
{
   // Bail on bad param.
   if(!node)
      return;

   // Bail if no children.
   long num = node->mChildren.getNumber();
   if(num <= 0)
      return;

   // Go through and move children.
   for(long i=0; i<num; i++)
   {
      BPathNode *childNode = node->mChildren[i];
      if(childNode)
      {
         // Move this node to the given list.
         removeNode(childNode);
         addNode(childNode);

         // Move its children.
         moveAllChildren(childNode);
      }
   }
}


//==============================================================================
// addNewOpen
// A fairly complex routine that creates and adds a node to the open list.
// It's assumed *we* are the open list. 
//==============================================================================
BPathNode *BPathTree::addNewOpen(BPathTree &closedList, BPathTree &unusedList, BPathNode *parent,
   const BVector &point, const long hullIndex, const long pointIndex, const BVector *goal, 
   const bool intersectionPoint, const long arrivalDirection, const long obstructionID, 
   const long segmentIndex, const bool exactPointCheck, const float errorTolerance)
{
   BPathNode *node = unusedList.getNode(true);

#ifdef _DEBUG
   if(!node)
   {
      BASSERT(0);
      return(NULL);
   }
#endif

   node->mvPoint = point;
   node->mlHullIndex = hullIndex;
   node->mlPointIndex = pointIndex;
   if(goal)
      node->mfEstimate = goal->xzDistance(node->mvPoint);
   else
      node->mfEstimate = 0.0f;
   node->mParent = parent;
   node->mbIsIntersection = intersectionPoint;
   node->mArrivalDirection = arrivalDirection;
   node->mObstructionID = obstructionID;
   node->mSegmentIndex = segmentIndex;
   node->clearChildren();

   if(parent)
      node->mfCost = parent->mfCost + node->mvPoint.xzDistance(parent->mvPoint);
   else
      node->mfCost = 0.0f;

   // See if there is a node for this point already on the open list.
   BPathNode *existingNode;
   if(exactPointCheck)
      existingNode = findMatchingNode(point, errorTolerance, parent);
   else
      existingNode = findMatchingNode(hullIndex, pointIndex);
   if(existingNode)
   {
      // If the existing node has a lower cost, just kill this one because
      // we don't need it.
      if(existingNode->mfCost < node->mfCost)
      {
         unusedList.addNode(node);
         return(NULL);
      }
      else
      {
         // The existing node has a higher cost, so we need to replace the existing node.

         // First remove the existing node from the open list.
         removeNode(existingNode);
         closedList.addNode(existingNode);

         // Then move all of its children to the closed list.
         closedList.moveAllChildren(existingNode);
      }
   }
   else
   {
      if(exactPointCheck)
         existingNode = closedList.findLowerCostMatchingNode(point, node->mfCost, errorTolerance, parent);
      else
         existingNode = closedList.findLowerCostMatchingNode(hullIndex, pointIndex, node->mfCost);
      if(existingNode)
      {
         unusedList.addNode(node);
         return(NULL);
      }
   }

   // Add the node to the open list.
   if(parent)
      parent->addChild(node);

   addNodeSorted(node);

   return(node);
}




//==============================================================================
// eof: pathtree.cpp
//==============================================================================

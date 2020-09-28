//==============================================================================
// pathquad.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "pathquad.h"
#include "obstructionmanager.h"
#include "game.h"
#include "world.h"
//#include "terrainbase.h"

//==============================================================================
// Defines


//==============================================================================
// BPathQuadNode::BPathQuadNode
//==============================================================================
BPathQuadNode::BPathQuadNode(void)
{
   mfMinX = 0.0f;
   mfMinZ = 0.0f;
   mfMaxX = 0.0f;
   mfMaxZ = 0.0f;
   mlObstructionMask = 0x0000;
   mpParent = NULL;
   mpChild[0] = mpChild[1] = mpChild[2] = mpChild[3] = NULL;
   mColor = cDWORDGrey;
} // BPathQuadNode::BPathQuadNode

//==============================================================================
// BPathQuadNode::~BPathQuadNode
//==============================================================================
BPathQuadNode::~BPathQuadNode(void)
{
   for (long l=0; l < 4; l++)
   {
      if (mpChild[l])
      {
         delete mpChild[l];
         mpChild[l] = NULL;
      }
   }


} // BPathQuadNode::~BPathQuadNode

//==============================================================================
// BPathQuadNode::setParms
//==============================================================================
void BPathQuadNode::setParms(float fMinX, float fMinZ, float fMaxX, float fMaxZ,
                             BPathQuadNode *pParent, BPathQuadTree *pTree,
                             long lChildIndex)
{
   mfMinX = fMinX;
   mfMaxX = fMaxX;
   mfMinZ = fMinZ;
   mfMaxZ = fMaxZ;
   mpParent = pParent;
   mpTree = pTree;
   mlChildIndex = lChildIndex;
}

//==============================================================================
// BPathQuadNode::buildTree
//==============================================================================
void BPathQuadNode::buildTree(float fMinX, float fMinZ, float fMaxX, float fMaxZ,
                              float fStopX, float fStopZ, BPathQuadNode *pNodeParent,
                              BPathQuadTree *pTree, long lChildIndex, BObstructionManager *pObManager)
{

	fMinX;
	fMinZ;
	fMaxX, fMaxZ; fStopX; fStopZ; pNodeParent; pTree; lChildIndex; pObManager;
/*
   // Establish Dims
   mfMinX = fMinX;
   mfMinZ = fMinZ;
   mfMaxX = fMaxX;
   mfMaxZ = fMaxZ;
   mpParent = pNodeParent;
   mpTree = pTree;

   mlChildIndex = lChildIndex;

   float fHalfX = (fMaxX - fMinX) / 2.0f;
   float fHalfZ = (fMaxZ - fMinZ) / 2.0f;

   // Any Obstructions at all?   
   if (pObManager->anyObstructions(fMinX, fMinZ, fMaxX, fMaxZ))
   {
      // TODO:  Determine type of obstructions, and set flags appropriately
      // For now, just mark and return (dlm)
      mlObstructionMask = 0x0001;

      // Smallest size?
      if (fMaxX - fMinX > fStopX && fMaxZ - fMinZ > fStopZ)
      {
         // Create Children, and continue
         mpChild[NW_CHILD] = new BPathQuadNode;
         if (!mpChild[NW_CHILD])
         {
            BASSERT(0);
            return;
         }
         mpChild[NW_CHILD]->buildTree(fMinX, fMinZ+fHalfZ, fMinX+fHalfX, fMaxZ,
            fStopX, fStopZ, this, mpTree, NW_CHILD, pObManager);


         mpChild[NE_CHILD] = new BPathQuadNode;
         if (!mpChild[NE_CHILD])
         {
            BASSERT(0);
            return;
         }
         mpChild[NE_CHILD]->buildTree(fMinX+fHalfX, fMinZ+fHalfZ, fMaxX, fMaxZ,
            fStopX, fStopZ, this, mpTree, NE_CHILD, pObManager);


         mpChild[SW_CHILD] = new BPathQuadNode;
         if (!mpChild[SW_CHILD])
         {
            BASSERT(0);
            return;
         }
         mpChild[SW_CHILD]->buildTree(fMinX, fMinZ, fMinX+fHalfX, fMinZ+fHalfZ,
            fStopX, fStopZ, this, mpTree, SW_CHILD, pObManager);


         mpChild[SE_CHILD] = new BPathQuadNode;
         if (!mpChild[SE_CHILD])
         {
            BASSERT(0);
            return;
         }
         mpChild[SE_CHILD]->buildTree(fMinX+fHalfX, fMinZ, fMaxX, fMinZ+fHalfZ,
            fStopX, fStopZ, this, mpTree, SE_CHILD, pObManager);

      }
   } // end of if obstructions
   else
   {
      mlObstructionMask = 0x0000;
   }

*/
   return;
}

//==============================================================================
// BPathQuadNode::obstructedBy(long lMask)
//==============================================================================
bool BPathQuadNode::obstructedBy(unsigned long lMask)
{
   // Eventually this will 'and' the passed in mask against
   // the obstruction mask, and return an appropiate value (dlm)
   if (lMask & mlObstructionMask)
      return true;
   return false;
}


//==============================================================================
// BPathQuadNode::addNodeDebugLines
//==============================================================================
void BPathQuadNode::addNodeDebugLines(const char *name)
{
#if 0
   game->getWorld()->getTerrain()->drawDebugLineOverTerrain(BVector(mfMinX, 0.1f, mfMinZ), BVector(mfMaxX, 0.1f, mfMinZ), mColor, mColor);
   game->getWorld()->getTerrain()->drawDebugLineOverTerrain(BVector(mfMaxX, 0.1f, mfMaxZ), BVector(mfMaxX, 0.1f, mfMinZ), mColor, mColor);
   game->getWorld()->getTerrain()->drawDebugLineOverTerrain(BVector(mfMaxX, 0.1f, mfMaxZ), BVector(mfMinX, 0.1f, mfMaxZ), mColor, mColor);
   game->getWorld()->getTerrain()->drawDebugLineOverTerrain(BVector(mfMinX, 0.1f, mfMinZ), BVector(mfMinX, 0.1f, mfMaxZ), mColor, mColor);

   for (long l=0; l < 4; l++)
   {
      if (mpChild[l])
         mpChild[l]->addNodeDebugLines(name);
   }
#endif
   return;

}

//==============================================================================
// BPathQuadNode::contains
// Returns true of the designated point lies within the boundary limits
// of this quad.
//==============================================================================
bool BPathQuadNode::contains(const BVector &vPoint)
{
   if ((vPoint.x >= mfMinX && vPoint.x < mfMaxX) &&
      (vPoint.z >= mfMinZ && vPoint.z < mfMaxZ))
      return true;

   return false;
}

//==============================================================================
// BPathQuadNode::getNode(const BVector &vPoint)
//==============================================================================
BPathQuadNode *BPathQuadNode::getNode(const BVector &vPoint)
{
   // Do I contain the point?
   if (contains(vPoint))
   {
      float fMidX = mfMinX + ((mfMaxX - mfMinX) / 2.0f);
      float fMidZ = mfMinZ + ((mfMaxZ - mfMinZ) / 2.0f);

      if (mpChild[NW_CHILD] &&
          vPoint.x >= mfMinX && vPoint.x < fMidX &&
          vPoint.z >= fMidZ && vPoint.z < mfMaxZ)
          return mpChild[NW_CHILD]->getNode(vPoint);

      if (mpChild[NE_CHILD] &&
          vPoint.x >= fMidX && vPoint.x < mfMaxX &&
          vPoint.z >= fMidZ && vPoint.z < mfMaxZ)
          return mpChild[NE_CHILD]->getNode(vPoint);

      if (mpChild[SW_CHILD] &&
          vPoint.x >= mfMinX && vPoint.x < fMidX &&
          vPoint.z >= mfMinZ && vPoint.z < fMidZ)
          return mpChild[SW_CHILD]->getNode(vPoint);

      if (mpChild[SE_CHILD] &&
          vPoint.x >= fMidX && vPoint.x < mfMaxX &&
          vPoint.z >= mfMinZ && vPoint.z < fMidZ)
          return mpChild[SE_CHILD]->getNode(vPoint);

      return this;      
   }
   else
      return NULL;

}

   
//==============================================================================
// BPathQuadNode::top()
//==============================================================================
BPathQuadNode *BPathQuadNode::top()
{
   // The node that is defined as on top of me is dependant upon
   // which child I am.
   BPathQuadNode *pnodeTop = NULL;
   BPathQuadNode *pnodeRet = NULL;

   
   // If I'm at the top, there is no one above me.
   if (fabs(mfMaxZ - mpTree->getMaxZ()) < cFloatCompareEpsilon)
      return NULL;

   if (!mpParent)
      return NULL;
   
   switch(mlChildIndex)
   {
      case NW_CHILD:
         pnodeTop = mpParent->top();
         if (pnodeTop)
         {
            pnodeRet = pnodeTop->getChild(SW_CHILD);
            if (!pnodeRet)
               pnodeRet = pnodeTop;
         }
         break;
      case NE_CHILD:
         pnodeTop = mpParent->top();
         if (pnodeTop)
         {
            pnodeRet = pnodeTop->getChild(SE_CHILD);
            if (!pnodeRet)
               pnodeRet = pnodeTop;
         }
         break;
      case SW_CHILD:
         pnodeRet = mpParent->getChild(NW_CHILD);
         break;
      case SE_CHILD:
         pnodeRet = mpParent->getChild(NE_CHILD);         
         break;
   }

   return pnodeRet;
}

//==============================================================================
// BPathQuadNode::upperRight()
//==============================================================================
BPathQuadNode *BPathQuadNode::upperRight()
{
   // The node that is defined as on top of me is dependant upon
   // which child I am.
   BPathQuadNode *pnodeTop = NULL;
   BPathQuadNode *pnodeRight = NULL;
   BPathQuadNode *pnodeRet = NULL;
   
   // If I'm at the top, or at the right, there is no one above me.
   if (_fabs(mfMaxZ - mpTree->getMaxZ()) < cFloatCompareEpsilon || 
       _fabs(mfMaxX - mpTree->getMaxX()) < cFloatCompareEpsilon)
      return NULL;

   if (!mpParent)
      return NULL;
   
   switch(mlChildIndex)
   {
      case NW_CHILD:
         pnodeTop = mpParent->top();
         if (pnodeTop)
         {
            pnodeRight = pnodeTop->getChild(SE_CHILD);
            if (pnodeRight)
            {
               while (pnodeRight->getChild(SW_CHILD) != NULL)
                  pnodeRight = pnodeRight->getChild(SW_CHILD);
               pnodeRet = pnodeRight;
            }
         }
         break;
      case NE_CHILD:
         pnodeTop = mpParent->top();
         if (pnodeTop)
         {
            pnodeRight = pnodeTop->right();
            if (pnodeRight)
            {
               while (pnodeRight->getChild(SW_CHILD) != NULL)
                  pnodeRight = pnodeRight->getChild(SW_CHILD);
               pnodeRet = pnodeRight;
            }
         }
         break;
      case SW_CHILD:
         pnodeRight = mpParent->getChild(NE_CHILD);
         while (pnodeRight->getChild(SW_CHILD) != NULL)
            pnodeRight = pnodeRight->getChild(SW_CHILD);
         pnodeRet = pnodeRight;
         break;
      case SE_CHILD:
         pnodeRight = mpParent->right();
         if (pnodeRight)
         {
            pnodeTop = pnodeRight->getChild(NW_CHILD);
            if (pnodeTop)
            {
               while (pnodeTop->getChild(SW_CHILD) != NULL)
                  pnodeTop = pnodeTop->getChild(SW_CHILD);
               pnodeRet = pnodeTop;
            }
         }
         break;
   }

   return pnodeRet;
}

//==============================================================================
// BPathQuadNode::upperLeft()
//==============================================================================
BPathQuadNode *BPathQuadNode::upperLeft()
{
   // The node that is defined as on top of me is dependant upon
   // which child I am.
   BPathQuadNode *pnodeTop = NULL;
   BPathQuadNode *pnodeLeft = NULL;
   BPathQuadNode *pnodeRet = NULL;
   
   // If I'm at the top, or at the left, there is no one to return.
   if (_fabs(mfMaxZ - mpTree->getMaxZ()) < cFloatCompareEpsilon || 
       _fabs(mfMinX - mpTree->getMinX()) < cFloatCompareEpsilon)
      return NULL;

   if (!mpParent)
      return NULL;
   
   switch(mlChildIndex)
   {
      case NW_CHILD:
         pnodeTop = mpParent->top();
         if (pnodeTop)
         {
            pnodeLeft = pnodeTop->left();
            if (pnodeLeft)
            {
               while (pnodeLeft->getChild(SE_CHILD) != NULL)
                  pnodeLeft = pnodeLeft->getChild(SE_CHILD);
               pnodeRet = pnodeLeft;
            }
         }
         break;
      case NE_CHILD:
         pnodeTop = mpParent->top();
         if (pnodeTop)
         {
            pnodeLeft = pnodeTop->getChild(SW_CHILD);
            if (pnodeLeft)
            {
               while (pnodeLeft->getChild(SE_CHILD) != NULL)
                  pnodeLeft = pnodeLeft->getChild(SE_CHILD);
               pnodeRet = pnodeLeft;
            }
         }
         break;
      case SW_CHILD:
         pnodeLeft = mpParent->left();
         if (pnodeLeft)
         {
            pnodeTop = pnodeLeft->getChild(NE_CHILD);
            if (pnodeTop)
            {
               while (pnodeTop->getChild(SE_CHILD) != NULL)
                  pnodeTop = pnodeTop->getChild(SE_CHILD);
               pnodeRet = pnodeTop;
            }
         }
         break;
      case SE_CHILD:
         pnodeLeft = mpParent->getChild(NW_CHILD);
         while (pnodeLeft->getChild(SE_CHILD) != NULL)
            pnodeLeft = pnodeLeft->getChild(SE_CHILD);
         pnodeRet = pnodeLeft;
         break;
   }

   return pnodeRet;
}

//==============================================================================
// BPathQuadNode::bottom()
//==============================================================================
BPathQuadNode *BPathQuadNode::bottom()
{
   BPathQuadNode *pnodeBottom = NULL;
   BPathQuadNode *pnodeRet = NULL;
   
   // If I'm at the bottom, there is no one below me.
   if (fabs(mfMinZ - mpTree->getMinZ()) < cFloatCompareEpsilon)
      return NULL;

   if (!mpParent)
      return NULL;
   
   switch(mlChildIndex)
   {
      case NW_CHILD:
         pnodeRet = mpParent->getChild(SW_CHILD);
         break;
      case NE_CHILD:
         pnodeRet = mpParent->getChild(SE_CHILD);         
         break;
      case SW_CHILD:
         pnodeBottom = mpParent->bottom();
         if (pnodeBottom)
         {
            pnodeRet = pnodeBottom->getChild(NW_CHILD);
            if (!pnodeRet)
               pnodeRet = pnodeBottom;
         }
         break;
      case SE_CHILD:
         pnodeBottom = mpParent->bottom();
         if (pnodeBottom)
         {
            pnodeRet = pnodeBottom->getChild(NE_CHILD);
            if (!pnodeRet)
               pnodeRet = pnodeBottom;
         }
         break;
   }

   return pnodeRet;
}

//==============================================================================
// BPathQuadNode::left()
//==============================================================================
BPathQuadNode *BPathQuadNode::left()
{
   BPathQuadNode *pnodeLeft = NULL;
   BPathQuadNode *pnodeRet = NULL;
   
   // If I'm at the left, there is no one to the left of me.
   if (_fabs(mfMinX - mpTree->getMinX()) < cFloatCompareEpsilon)
      return NULL;

   if (!mpParent)
      return NULL;
   
   switch(mlChildIndex)
   {
      case NW_CHILD:
         pnodeLeft = mpParent->left();
         if (pnodeLeft)
         {
            pnodeRet = pnodeLeft->getChild(NE_CHILD);
            if (!pnodeRet)
               pnodeRet = pnodeLeft;
         }
         break;
      case NE_CHILD:
         pnodeRet = mpParent->getChild(NW_CHILD);         
         break;
      case SW_CHILD:
         pnodeLeft = mpParent->left();
         if (pnodeLeft)
         {
            pnodeRet = pnodeLeft->getChild(SE_CHILD);
            if (!pnodeRet)
               pnodeRet = pnodeLeft;
         }
         break;
      case SE_CHILD:
         pnodeRet = mpParent->getChild(SW_CHILD);
         break;
   }

   return pnodeRet;
}

//==============================================================================
// BPathQuadNode::lowerRight()
//==============================================================================
BPathQuadNode *BPathQuadNode::lowerRight()
{
   // The node that is defined as on top of me is dependant upon
   // which child I am.
   BPathQuadNode *pnodeBottom = NULL;
   BPathQuadNode *pnodeRight = NULL;
   BPathQuadNode *pnodeRet = NULL;
   
   // If I'm at the bottom, or at the right, there is no one to return.
   if (_fabs(mfMinZ - mpTree->getMinZ()) < cFloatCompareEpsilon || 
       _fabs(mfMaxX - mpTree->getMaxX()) < cFloatCompareEpsilon)
      return NULL;

   if (!mpParent)
      return NULL;
   
   switch(mlChildIndex)
   {
      case NW_CHILD:
         pnodeRight = mpParent->getChild(SE_CHILD);
         while (pnodeRight->getChild(NW_CHILD) != NULL)
            pnodeRight = pnodeRight->getChild(NW_CHILD);
         pnodeRet = pnodeRight;
         break;
      case NE_CHILD:
         pnodeRight = mpParent->right();
         if (pnodeRight)
         {
            pnodeBottom = pnodeRight->getChild(SW_CHILD);
            if (pnodeBottom)
            {
               while (pnodeBottom->getChild(NW_CHILD) != NULL)
                  pnodeBottom = pnodeBottom->getChild(NW_CHILD);
               pnodeRet = pnodeBottom;
            }
         }
         break;
      case SW_CHILD:
         pnodeBottom = mpParent->bottom();
         if (pnodeBottom)
         {
            pnodeRight = pnodeBottom->getChild(NE_CHILD);
            if (pnodeRight)
            {
               while (pnodeRight->getChild(NW_CHILD) != NULL)
                  pnodeRight = pnodeRight->getChild(NW_CHILD);
               pnodeRet = pnodeRight;
            }
         }
         break;
      case SE_CHILD:
         pnodeBottom = mpParent->bottom();
         if (pnodeBottom)
         {
            pnodeRight = pnodeBottom->right();
            if (pnodeRight)
            {
               while (pnodeRight->getChild(NW_CHILD) != NULL)
                  pnodeRight = pnodeRight->getChild(NW_CHILD);
               pnodeRet = pnodeRight;
            }
         }
         break;
   }

   return pnodeRet;
}

//==============================================================================
// BPathQuadNode::right()
//==============================================================================
BPathQuadNode *BPathQuadNode::right()
{
   BPathQuadNode *pnodeRight = NULL;
   BPathQuadNode *pnodeRet = NULL;
   
   // If I'm at the right, there is no one to the right of me.
   if (fabs(mfMaxX - mpTree->getMaxX()) < cFloatCompareEpsilon)
      return NULL;

   if (!mpParent)
      return NULL;
   
   switch(mlChildIndex)
   {
      case NW_CHILD:
         pnodeRet = mpParent->getChild(NE_CHILD);
         break;
      case NE_CHILD:
         pnodeRight = mpParent->right();
         if (pnodeRight)
         {
            pnodeRet = pnodeRight->getChild(NW_CHILD);
            if (!pnodeRet)
               pnodeRet = pnodeRight;
         }
         break;
      case SW_CHILD:
         pnodeRet = mpParent->getChild(SE_CHILD);
         break;
      case SE_CHILD:
         pnodeRight = mpParent->right();
         if (pnodeRight)
         {
            pnodeRet = pnodeRight->getChild(SW_CHILD);
            if (!pnodeRet)
               pnodeRet = pnodeRight;
         }
         break;
   }

   return pnodeRet;
}

//==============================================================================
// BPathQuadNode::lowerLeft()
//==============================================================================
BPathQuadNode *BPathQuadNode::lowerLeft()
{
   BPathQuadNode *pnodeBottom = NULL;
   BPathQuadNode *pnodeLeft = NULL;
   BPathQuadNode *pnodeRet = NULL;
   
   // If I'm at the bottom, or at the right, there is no one to return.
   if (_fabs(mfMinZ - mpTree->getMinZ()) < cFloatCompareEpsilon || 
       _fabs(mfMaxX - mpTree->getMaxX()) < cFloatCompareEpsilon)
      return NULL;

   if (!mpParent)
      return NULL;
   
   switch(mlChildIndex)
   {
      case NW_CHILD:
         pnodeLeft = mpParent->left();
         if (pnodeLeft)
         {
            pnodeBottom = pnodeLeft->getChild(SE_CHILD);
            if (pnodeBottom)
            {
               while (pnodeBottom->getChild(NE_CHILD) != NULL)
                  pnodeBottom = pnodeBottom->getChild(NE_CHILD);
               pnodeRet = pnodeBottom;
            }
         }
         break;
      case NE_CHILD:
         pnodeLeft = mpParent->getChild(SW_CHILD);
         while (pnodeLeft->getChild(NE_CHILD) != NULL)
            pnodeLeft = pnodeLeft->getChild(NE_CHILD);
         pnodeRet = pnodeLeft;
         break;
      case SW_CHILD:
         pnodeBottom = mpParent->bottom();
         if (pnodeBottom)
         {
            pnodeLeft = pnodeBottom->left();
            if (pnodeLeft)
            {
               while (pnodeLeft->getChild(NE_CHILD) != NULL)
                  pnodeLeft = pnodeLeft->getChild(NE_CHILD);
               pnodeRet = pnodeLeft;
            }
         }
         break;
      case SE_CHILD:
         pnodeBottom = mpParent->bottom();
         if (pnodeBottom)
         {
            pnodeLeft = pnodeBottom->getChild(NW_CHILD);
            if (pnodeLeft)
            {
               while (pnodeLeft->getChild(NE_CHILD) != NULL)
                  pnodeLeft = pnodeLeft->getChild(NE_CHILD);
               pnodeRet = pnodeLeft;
            }
         }
         break;
   }

   return pnodeRet;
}


//==============================================================================
// BPathQuadNode::getMidpoint()
//==============================================================================
BVector BPathQuadNode::getMidpoint()
{
   BVector vMidPoint(0.0f, 0.0f, 0.0f);

   vMidPoint.x = mfMinX + ((mfMaxX - mfMinX) * 0.5f);
   vMidPoint.y = 0.1f;
   vMidPoint.z = mfMinZ + ((mfMaxZ - mfMinZ) * 0.5f);

   return vMidPoint;
}


//==============================================================================
// BPathQuadNode::getPassableList(unsigned long lDir, unsigned long lMask,
//                                BSimpleQuadPtrArray &quadList)
// lDir = the direction I traveled to get here, not the direction I came from. Got it?
//==============================================================================
void BPathQuadNode::getPassableList(unsigned long lDir, unsigned long lMask,
                                BPathQuadPtrArray &quadList)
{
   // Am I passable?  If so, then add me to the list, and return.  No need
   // to check my children.
   if (!obstructedBy(lMask))
   {
      quadList.add(this);
      return;
   }

   // Remember, the presence of one child implies the presence of them all
   if (mpChild[NW_CHILD] == NULL)
      return;

   // Okay, check two children, based on direction.
   switch (lDir)
   {
      case NORTH_DIR:
         mpChild[SW_CHILD]->getPassableList(lDir, lMask, quadList);
         mpChild[SE_CHILD]->getPassableList(lDir, lMask, quadList);
         break;
      case EAST_DIR:
         mpChild[NW_CHILD]->getPassableList(lDir, lMask, quadList);
         mpChild[SW_CHILD]->getPassableList(lDir, lMask, quadList);
         break;
      case SOUTH_DIR:
         mpChild[NW_CHILD]->getPassableList(lDir, lMask, quadList);
         mpChild[NE_CHILD]->getPassableList(lDir, lMask, quadList);
         break;
      case WEST_DIR:
         mpChild[NE_CHILD]->getPassableList(lDir, lMask, quadList);
         mpChild[SE_CHILD]->getPassableList(lDir, lMask, quadList);
         break;
   }
   return;
}

//==============================================================================
// BPathQuadNode::addObstruction
//==============================================================================
void BPathQuadNode::addObstruction(const BConvexHull &hull, float fMinQuadX, float fMinQuadZ,
                                   BPathQuadTree *pTree)
{
   // Does the hull overlap our box?
   if (hull.overlapsBox(mfMinX, mfMinZ, mfMaxX, mfMaxZ))
   {
      
      // TODO:  Determine type of obstructions, and set flags appropriately
      // For now, just mark and return (dlm)
      mlObstructionMask |= 0x0001;

      // Are we done?
      float fWidth = mfMaxX - mfMinX;
      float fHeight = mfMaxZ - mfMinZ;
      if (fWidth <= fMinQuadX || fHeight <= fMinQuadZ)
         return;

      float fMidX = mfMinX + (fWidth * 0.5f);
      float fMidZ = mfMinZ + (fHeight * 0.5f);

      // Create and update the children
      if (!mpChild[NW_CHILD])
      {
         mpChild[NW_CHILD] = new BPathQuadNode;
         BASSERT(mpChild[NW_CHILD]);
         mpChild[NW_CHILD]->setParms(mfMinX, fMidZ, fMidX, mfMaxZ, this, pTree, NW_CHILD);
      }
      if (!mpChild[NE_CHILD])
      {
         mpChild[NE_CHILD] = new BPathQuadNode;
         BASSERT(mpChild[NE_CHILD]);
         mpChild[NE_CHILD]->setParms(fMidX, fMidZ, mfMaxX, mfMaxZ, this, pTree, NE_CHILD);
      }
      if (!mpChild[SW_CHILD])
      {
         mpChild[SW_CHILD] = new BPathQuadNode;
         BASSERT(mpChild[SW_CHILD]);
         mpChild[SW_CHILD]->setParms(mfMinX, mfMinZ, fMidX, fMidZ, this, pTree, SW_CHILD);
      }
      if (!mpChild[SE_CHILD])
      {
         mpChild[SE_CHILD] = new BPathQuadNode;
         BASSERT(mpChild[SE_CHILD]);
         mpChild[SE_CHILD]->setParms(fMidX, mfMinZ, mfMaxX, fMidZ, this, pTree, SE_CHILD);
      }


      // Create and/or descend into children as appropriate.
      for (long l = 0; l < 4; l++)
      {
         mpChild[l]->addObstruction(hull, fMinQuadX, fMinQuadZ, pTree);
      }
   }
   return;
}

//==============================================================================
// BPathQuadNode::clearObstruction
//==============================================================================
bool BPathQuadNode::clearObstruction(const BConvexHull &hull, BObstructionManager *pObManager)
{
	hull; pObManager;
/*
   // Do I overlap the area to be cleared?
   if (hull.overlapsBox(mfMinX, mfMinZ, mfMaxX, mfMaxZ))
   {
      // Do I have Children?
      if (mpChild[0])
      {
         bool bNWClear = mpChild[NW_CHILD]->clearObstruction(hull, pObManager);
         bool bNEClear = mpChild[NE_CHILD]->clearObstruction(hull, pObManager);
         bool bSWClear = mpChild[SW_CHILD]->clearObstruction(hull, pObManager);
         bool bSEClear = mpChild[SE_CHILD]->clearObstruction(hull, pObManager);
         // Are all my children clear?
         if (bNWClear && bNEClear && bSWClear && bSEClear)
         {
            // By definition, if all my children are clear, than I am too.
            for (long l = 0; l < 4; l++)
            {
               delete mpChild[l];
               mpChild[l] = NULL;
            }
            // Clear the Obstruction Mask
            mlObstructionMask = 0x0000;
            return true;
         }
         return false;
      }
      else
      {
         // Do I have other obstructions?
         if (pObManager->anyObstructions(mfMinX, mfMinZ, mfMaxX, mfMaxZ))
            return false;
         // Clear the mask
         mlObstructionMask = 0x0000;
         return true;
      }
   }

   // Do I have other obstructions?
   if (pObManager->anyObstructions(mfMinX, mfMinZ, mfMaxX, mfMaxZ))
      return false;

   // Clear the mask.
   mlObstructionMask = 0x0000;
*/
   return true;
}






      




//==============================================================================
// BPathQuadTree::BPathQuadTree
//==============================================================================
BPathQuadTree::BPathQuadTree(void)
{
   mpRoot = NULL;
   mbInitialized = false;

} // BPathQuadTree::BPathQuadTree

//==============================================================================
// BPathQuadTree::~BPathQuadTree
//==============================================================================
BPathQuadTree::~BPathQuadTree(void)
{
   clear();
} // BPathQuadTree::~BPathQuadTree

//==============================================================================
// BPathQuadTree::Init
// ObManager used for obstruction checking
// fSmallestX/Z are the mininum sizes of a quad
// fLargestX/Z are the sizes of the root quad.
//==============================================================================
bool BPathQuadTree::init(BObstructionManager *pObManager, float fSmallestX, 
              float fSmallestZ, float fLargestX, float fLargestZ)
{
   if (!pObManager)
   {
      BASSERT(0);
      return false;
   }

   // Build the tree
   if (mpRoot)
   {
      delete mpRoot;
      mpRoot = NULL;
   }

   mpRoot = new BPathQuadNode;
   if (!mpRoot)
   {
      BASSERT(0);
      return false;
   }

   mfMinX = 0.0f;
   mfMinZ = 0.0f;
   mfMaxX = fLargestX;
   mfMaxZ = fLargestZ;
   mfMinQuadSizeX = fSmallestX;
   mfMinQuadSizeZ = fSmallestZ;

   mpRoot->buildTree(mfMinX, mfMinZ, mfMaxX, mfMaxZ, mfMinQuadSizeX, mfMinQuadSizeZ, NULL, this,
      -1, pObManager);

   mbInitialized = true;
   return true;
}


//==============================================================================
// BPathQuadTree::clear
//==============================================================================
void BPathQuadTree::clear(void)
{
   if (mpRoot)
   {
      delete mpRoot;
      mpRoot = NULL;
   }
   mbInitialized = false;
   return;
}

//==============================================================================
// BPathQuadTree::render
//==============================================================================
void BPathQuadTree::render(void)
{
   if (mpRoot)
      mpRoot->addNodeDebugLines("pathquad");
   return;
}

//==============================================================================
// BPathQuadTree::getNode
// Retrieve the smallest possible node 
//==============================================================================
BPathQuadNode *BPathQuadTree::getNode(const BVector &vPoint)
{
   // Descend to the lowest part of the tree that contains this point, and return
   // it.
   if (mpRoot)
      return mpRoot->getNode(vPoint);
   
   return NULL;
}

//==============================================================================
// BPathQuadTree::addObstruction
//==============================================================================
void BPathQuadTree::addObstruction(const BConvexHull &hull)
{
   if (mpRoot)
   {
      mpRoot->addObstruction(hull, mfMinQuadSizeX, mfMinQuadSizeZ, this);
   }
}

void BPathQuadTree::clearObstruction(const BConvexHull &hull, BObstructionManager *pObManager)
{
   if (mpRoot)
      mpRoot->clearObstruction(hull, pObManager);
}


//==============================================================================
// eof: pathquad.cpp
//==============================================================================

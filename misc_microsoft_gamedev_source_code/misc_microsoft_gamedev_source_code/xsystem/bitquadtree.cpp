//=============================================================================
// Copyright (c) 2001 Ensemble Studios
//
// Random map quadtree.  The nodes use BBitVector64s to store their data, so
// they are 8x8 nodes.
//=============================================================================


//=============================================================================
// Includes
#include "xsystem.h"
#include "bitquadtree.h"
#include "convexhull.h"
#include "chunker.h"

static const long cLeafSize=8;

//==============================================================================
// Static variables
const DWORD BBitQuadtree::msSaveVersion=0;


//=============================================================================
// distSqrToBox
//=============================================================================
float distSqrToBox(long x, long z, long startX, long startZ, long endX, long endZ)
{
   float xDist;
   if(x<startX)
      xDist=float(startX-x);
   else if(x>endX)
      xDist=float(x-endX);
   else
      xDist=0.0f;

   float zDist;
   if(z<startZ)
      zDist=float(startZ-z);
   else if(z>endZ)
      zDist=float(z-endZ);
   else
      zDist=0.0f;

   return(xDist*xDist + zDist*zDist);
}


//=============================================================================
// BBitQuadnode::BBitQuadnode
//=============================================================================
BBitQuadnode::BBitQuadnode(long startX, long startZ, long size) :
   mStartX(startX),
   mStartZ(startZ),
   mTileCount(0)
{
   // Clear bit array.
   mTiles.zero();

   // Clear children.
   for(long i=0; i<4; i++)
   {
      mChild[i]=NULL;
      mChildFull[i]=false;
   }

   // Leaf nodes are size 8.  Don't allow anything smaller.
   if(size<cLeafSize)
   {
      BASSERT(0);
      size=cLeafSize;   
   }

   // Also, the size should be a multiple of 8.
   if(size%cLeafSize)
   {
      BASSERT(0);
      size=(size/cLeafSize)*cLeafSize;
   }
   
   // Set up leaf status... which indicates whether this node contains tile data or not.
   if(size>cLeafSize)
      mLeaf=false;
   else
      mLeaf=true;

   // Get end tiles.
   mEndX=mStartX+size-1;
   mEndZ=mStartZ+size-1;   
}


//=============================================================================
// BBitQuadnode::~BBitQuadnode
//=============================================================================
BBitQuadnode::~BBitQuadnode(void)
{
   // Clean up children.
   for(long i=0; i<4; i++)
   {
      if(mChild[i])
      {
         delete mChild[i];
         mChild[i]=NULL;
      }
   }
}


//=============================================================================
// BBitQuadnode::addTile
//=============================================================================
bool BBitQuadnode::addTile(long x, long z)
{
   // If tile is not inside this node, return false.
   if(x<mStartX || x>mEndX || z<mStartZ || z>mEndZ)
      return(false);

   // If a leaf, check the actual bitvector.
   if(mLeaf)
   {
      // Compute offset into our bitvector.
      long xOffset=x-mStartX;
      long zOffset=z-mStartZ;
      long offset=xOffset*cLeafSize+zOffset;

      // If bit not already set, set it and increment count.
      if(!mTiles.checkAndSet(offset))
      {
         // Increment tile count.
         mTileCount++;

         // Added it.
         return(true);
      }
      
      // Didn't add anything.
      return(false);
   }

   // Figure out which child this belongs in.
   long childIndex=computeChildIndex(x, z);
   if(childIndex<0)
   {
      BASSERT(0);
      return(false);
   }

   // If this child is already full, no need to do anything.
   if(mChildFull[childIndex])
      return(false);

   // If we don't have this child, we need to allocate it.
   if(!mChild[childIndex])
   {
      long childSize=getSize()/2;
      long startX=(x/childSize)*childSize;
      long startZ=(z/childSize)*childSize;
      mChild[childIndex]=new BBitQuadnode(startX, startZ, childSize);
      if(!mChild[childIndex])
      {
         BASSERT(0);
         return(false);
      }
   }

   // Add it.
   bool ok=mChild[childIndex]->addTile(x, z);

   // Check if this child is now full.
   if(mChild[childIndex]->isFull())
   {
      // Nuke it.
      delete mChild[childIndex];
      mChild[childIndex]=NULL;

      // Mark it full.
      mChildFull[childIndex]=true;
   }

   // If we added something, increment count.
   if(ok)
      mTileCount++;

   return(ok);
}


//=============================================================================
// BBitQuadnode::removeTile
//=============================================================================
bool BBitQuadnode::removeTile(long x, long z)
{
   // If tile is not inside this node, return false.
   if(x<mStartX || x>mEndX || z<mStartZ || z>mEndZ)
      return(false);

   // If a leaf, turn off the required bit.
   if(mLeaf)
   {
      // Compute offset into our bitvector.
      long xOffset=x-mStartX;
      long zOffset=z-mStartZ;
      long offset=xOffset*cLeafSize+zOffset;

      // If it's not already set, then indicate that the tile was already
      // not present in the tree.
      if(!mTiles.isSet(offset))
         return(false);

      // Clear the bit.  TODO: create new method on bitvector that does checkAndUnset for speed.
      mTiles.unset(offset);

      // Decrement the tile count in this node.
      mTileCount--;

      // Indicate that we actually removed the tile.
      return(true);
   }

   // Not a leaf, so descend into child nodes.
   // Figure out which child this belongs in.
   long childIndex=computeChildIndex(x, z);
   if(childIndex<0)
   {
      BASSERT(0);
      return(false);
   }

   // If the node is full, we need to break it down.
   if(mChildFull[childIndex])
   {
      // Mark child as not full.
      mChildFull[childIndex]=false;

      // Create a new child (with bogus info to start since we'll overwrite it)
      mChild[childIndex] = new BBitQuadnode(0, 0, cLeafSize);
      if(!mChild[childIndex])
      {
         BFAIL("Could not allocate BBitQuadnode.");
         return(false);
      }

      // Get our size and midpoints.
      long size=getSize();

      // Get child's size.
      long childSize=size/2;

      // Set tile count of the new child to childSize^2 (since node is initially full)
      mChild[childIndex]->mTileCount = childSize*childSize;

      // Set bounding box.
      getChildCoordinates(childIndex, mChild[childIndex]->mStartX, mChild[childIndex]->mStartZ, mChild[childIndex]->mEndX, mChild[childIndex]->mEndZ);

      // Child a leaf?
      if(childSize == cLeafSize)
      {
         // Set leaf flag.
         mChild[childIndex]->mLeaf=true;

         // Set all tile bits on.
         mChild[childIndex]->mTiles.setToOnes();
      }
      else
      {
         // Not a leaf.
         mChild[childIndex]->mLeaf=false;

         // Mark all children as full.
         for(long i=0; i<4; i++)
            mChild[childIndex]->mChildFull[i]=true;
      }
   }

   // If the node doesn't exist, it is empty... so the tile is not present in the tree.
   if(!mChild[childIndex])
      return(false);

   // Ask child to remove.
   bool removed=mChild[childIndex]->removeTile(x, z);

   // If the tile was in the tree and now removed, we need to do some bookkeeping to
   // remove unneeded nodes.
   if(removed)
   {
      // Decrease our tile count.
      mTileCount--;

      // If the child node is now empty, we can safely kill it off.
      if(mChild[childIndex]->isEmpty())
      {
         delete mChild[childIndex];
         mChild[childIndex]=NULL;
      }

      // Let the caller know that the tile was actually removed.
      return(true);
   }

   // Tile was not in tree.
   return(false);
}


//=============================================================================
// BBitQuadnode::containsTile
//=============================================================================
bool BBitQuadnode::containsTile(long x, long z) const
{
   // If tile is not inside this node, return false.
   if(x<mStartX || x>mEndX || z<mStartZ || z>mEndZ)
      return(false);

   // If a leaf, check the actual bitvector.
   if(mLeaf)
   {
      // Compute offset into our bitvector.
      long xOffset=x-mStartX;
      long zOffset=z-mStartZ;
      long offset=xOffset*cLeafSize+zOffset;

      // Check the status.
      if(mTiles.isSet(offset))
         return(true);
      return(false);
   }

   // Figure out which child this belongs in.
   long childIndex=computeChildIndex(x, z);
   if(childIndex<0)
   {
      BASSERT(0);
      return(false);
   }

   // If this child is full, the tile is contained within.
   if(mChildFull[childIndex])
      return(true);

   // Node doesn't exist means empty, then.
   if(!mChild[childIndex])
      return(false);

   // Otherwise we need to check.
   bool contained=mChild[childIndex]->containsTile(x, z);
   return(contained);
}


//=============================================================================
// BBitQuadnode::canContainTile
//=============================================================================
bool BBitQuadnode::canContainTile(long x, long z) const
{
   // If tile is not inside this node, return false.
   if(x<mStartX || x>mEndX || z<mStartZ || z>mEndZ)
      return(false);

   // Otherwise, true.
   return(true);
}

//=============================================================================
// BBitQuadnode::addChild
//=============================================================================
bool BBitQuadnode::addChild(BBitQuadnode *node)
{
   // Check for bogus params.
   if(!node)
   {
      BASSERT(0);
      return(false);
   }

   // Make sure this fits.
   if(node->mStartX<mStartX || node->mEndZ>mEndZ || node->mStartZ<mStartZ || node->mEndZ>mEndZ)
   {
      // We shouldn't be adding this here if it is not within this node.
      BASSERT(0);
      return(false);
   }

   // Which child is this?
   long childIndex=computeChildIndex(node->mStartX, node->mStartZ);
   if(childIndex<0)
   {
      // We shouldn't be adding this here if it is not within this node.
      BASSERT(0);
      return(false);
   }

   // Do we already have this child?
   if(mChildFull[childIndex] || mChild[childIndex])
   {
      BASSERT(0);
      return(false);
   }

   // Grab tile count.
   mTileCount+=node->getTileCount();

   // Check if this child is now full.  If so, we can just nuke it and mark it full in
   // the parent.
   if(node->isFull())
   {
      // Nuke it.
      delete node;
      node=NULL;

      // Mark it full.
      mChildFull[childIndex]=true;
   }
   else
   {
      // Plug it in.
      mChild[childIndex]=node;
   }

   // Success.
   return(true);
}


//=============================================================================
// BBitQuadnode::computeChildIndex
//=============================================================================
long BBitQuadnode::computeChildIndex(long x, long z) const
{
   // Children's layout:
   // 1 2
   // 0 3

   // Get mid point.
   long size=getSize();
   long midX=mStartX+(size/2);
   long midZ=mStartZ+(size/2);

   // Left half.
   if(x>=mStartX && x<midX)
   {
      if(z>=mStartZ && z<midZ)
         return(0);
      else if(z>=midZ && z<=mEndZ)
         return(1);
   }
   // Right half.
   else if(x>=midX && x<=mEndX)
   {
      if(z>=mStartZ && z<midZ)
         return(3);
      else if(z>=midZ && z<=mEndZ)
         return(2);
   }

   // Node wasn't inside.  Badness.
   BASSERT(0);
   return(-1);
}


//=============================================================================
// BBitQuadnode::isFull
//=============================================================================
bool BBitQuadnode::isFull(void) const
{
   // How many tiles do we need to be full?
   long size=getSize();
   long fullTiles=size*size;

   // Do we have that many?
   if(mTileCount==fullTiles)
      return(true);
   else
      return(false);
}


//=============================================================================
// BBitQuadnode::isChildFull
//=============================================================================
bool BBitQuadnode::isChildFull(long childIndex) const
{
   // Check incoming index.
   if(childIndex<0 || childIndex>3)
   {
      BASSERT(0);
      return(false);
   }

   // Return full-ness status.
   return(mChildFull[childIndex]);
}


//=============================================================================
// BBitQuadnode::getChild
//=============================================================================
BBitQuadnode *BBitQuadnode::getChild(long childIndex) const
{
   // Check incoming index.
   if(childIndex<0 || childIndex>3)
   {
      BASSERT(0);
      return(NULL);
   }

   // Return full-ness status.
   return(mChild[childIndex]);
}


//=============================================================================
// BBitQuadnode::getChildCoordinates
//=============================================================================
void BBitQuadnode::getChildCoordinates(long childIndex, long &startX, long &startZ, long &endX, long &endZ) const
{
   // Get mid point.
   long size=getSize();
   long midX=mStartX+(size/2);
   long midZ=mStartZ+(size/2);

   switch(childIndex)
   {
      case 0:
         startX=mStartX;
         startZ=mStartZ;
         endX=midX-1;
         endZ=midZ-1;
         break;

      case 1:
         startX=mStartX;
         startZ=midZ;
         endX=midX-1;
         endZ=mEndZ;
         break;

      case 2:
         startX=midX;
         startZ=midZ;
         endX=mEndX;
         endZ=mEndZ;
         break;

      case 3:
         startX=midX;
         startZ=mStartZ;
         endX=mEndX;
         endZ=midZ-1;
         break;

      default:
         BASSERT(0);
         startX=-1;
         startZ=-1;
         endX=-1;
         endZ=-1;
         break;
   }
}


//=============================================================================
// BBitQuadnode::distSqrToTile
//=============================================================================
bool BBitQuadnode::distSqrToTile(long x, long z, float maxDist, float &distSqr) const
{
   // Check distance to box.
   float boxDistSqr=distSqrToBox(x, z);

   // If the distance to the box is further than any distance so far, don't bother
   // checking any further because we can't possible find a closer tile.
   if(boxDistSqr>distSqr)
      return(false);

   // If the distance to the box is bigger than the biggest distance we care about,
   // bail out now.
   if(boxDistSqr>maxDist*maxDist)
      return(false);

   // Nothing yet.
   bool gotAnything=false;

   // If a leaf, check distance to tiles.
   if(mLeaf)
   {
      // Run through the individual tiles.
      DWORD mask = 1;
      const DWORD *bits = (const DWORD*)mTiles.getRawValue();
      for(long tx=mStartX; tx<=mEndX; tx++)
      {
         float dx2=float(x-tx);
         dx2*=dx2;
         for(long tz=mStartZ; tz<=mEndZ; tz++)
         {
            // If the tile is present, check distance.
            if(*bits & mask)
            {
               float dz=float(z-tz);
               float tempDistSqr=dx2+dz*dz;
               if(tempDistSqr<distSqr)
               {
                  gotAnything=true;
                  distSqr=tempDistSqr;
               }
            }
            
            // Shift mask
            mask <<= 1;
            
            // Next DWORD if needed.
            if(mask == 0)
            {
               mask = 1;
               bits++;
            }
         }
      }
   }
   else
   {
      // Descend into children.
      for(long i=0; i<4; i++)
      {
         // If the child is full, do a check to the box.
         if(mChildFull[i])
         {
            // jce 10/5/2001 -- make this fastAR.
            long startX, startZ, endX, endZ;
            getChildCoordinates(i, startX, startZ, endX, endZ);
            float tempDistSqr=::distSqrToBox(x, z, startX, startZ, endX, endZ);
            if(tempDistSqr<distSqr)
            {
               gotAnything=true;
               distSqr=tempDistSqr;
            }
         }
         else if(mChild[i])
         {
            // Check child, record it if distance is closer than any distance so far.
            if(mChild[i]->distSqrToTile(x, z, maxDist, distSqr))
               gotAnything=true;
         }
      }
   }

   return(gotAnything);
}


//=============================================================================
// BBitQuadnode::closestTileToTile
//=============================================================================
bool BBitQuadnode::closestTileToTile(long x, long z, long &rX, long &rZ, float &rDistanceSqr, TILE_VALIDITY_CALLBACK validCallback, void *param) const
{
   //I'm sure this can be faster.

   //Nothing yet.
   bool gotAnything=false;

   // If a leaf, check distance to tiles.
   if(mLeaf)
   {
      // Run through the individual tiles.
      long index=0;
      for(long tx=mStartX; tx<=mEndX; tx++)
      {
         for(long tz=mStartZ; tz<=mEndZ; tz++, index++)
         {
            // If the tile is present, check distance.
            if(mTiles.isSet(index))
            {
               float dx=float(x-tx);
               float dz=float(z-tz);
               float tempDistSqr=dx*dx+dz*dz;
               if (tempDistSqr < rDistanceSqr)
               {
                  // Check validity callback if we have one.
                  if(!validCallback || validCallback(tx, tz, param))
                  {
                     gotAnything=true;
                     rDistanceSqr=tempDistSqr;
                     rX=tx;
                     rZ=tz;
                  }
               }
            }
         }
      }
   }
   else
   {
      // Descend into children.
      for(long i=0; i<4; i++)
      {
         if(mChild[i])
         {
            // Check child, record it if distance is closer than any distance so far.
            if(mChild[i]->closestTileToTile(x, z, rX, rZ, rDistanceSqr, validCallback, param))
               gotAnything=true;
         }
      }
   }

   return(gotAnything);
}


//=============================================================================
// BBitQuadnode::distSqrToBox
//=============================================================================
float BBitQuadnode::distSqrToBox(long x, long z) const
{
   return(::distSqrToBox(x, z, mStartX, mStartZ, mEndX, mEndZ));
}


//=============================================================================
// BBitQuadnode::reparent
//=============================================================================
BBitQuadnode *BBitQuadnode::reparent(void)
{
   // Run through the possible children.
   long count=0;
   long childIndex=-1;
   for(long i=0; i<4; i++)
   {
      // If we have a child or a "stubbed out" full child, increment the count.
      if(mChild[i] || mChildFull[i])
      {
         // Remember this child index in case it's the only one.
         childIndex=i;
         count++;
      }
   }

   // If we have only one node we can be reparented.
   if(count==1)
   {
      // Grab the node pointer for our only child node.
      BBitQuadnode *newParent=mChild[childIndex];

      // If it's there, then we just hand back that pointer and kill this node.
      if(newParent)
      {
         // Null out the link to the child that we be the new parent so it is not
         // deleted by the destructor.
         mChild[childIndex] = NULL;

         delete this;
         return(newParent);
      }

      // If there is not a pointer then our only child is completely full, so we just alter ourselves to match.
      long size=getSize();
      long midX=mStartX+(size/2);
      long midZ=mStartZ+(size/2);
      switch(childIndex)
      {
         // Convert to lower-left.
         case 0:
            mEndX = midX;
            mEndZ = midZ;
            break;

         // Convert to upper-left.
         case 1:
            mEndX = midX;
            mStartZ = midZ;
            break;

         // Convert to upper-right.
         case 2:
            mStartX = midX;
            mStartZ = midZ;
            break;

         // Conver to lower-right
         case 3:
            mStartX = midX;
            mEndZ = midZ;
            break;
      }

      // Are we now a leaf?
      if(size==cLeafSize)
      {
         // Mark as leaf.
         mLeaf=true;

         // Make sure bitarray is full.
         mTiles.setToOnes();
      }

      // We altered ourselves, so just return NULL to indicate caller doesn't need to do anything.
      return(NULL);
   }

   // We can't reparent.
   return(NULL);
}


//=============================================================================
// end of BBitQuadnode
//=============================================================================






//=============================================================================
// BBitQuadtree::BBitQuadtree
//=============================================================================
BBitQuadtree::BBitQuadtree(void) :
   mRoot(NULL),
   mMinX(cMaximumLong),
   mMinZ(cMaximumLong),
   mMaxX(cMinimumLong),
   mMaxZ(cMinimumLong)
{
}


//=============================================================================
// BBitQuadtree::~BBitQuadtree
//=============================================================================
BBitQuadtree::~BBitQuadtree(void)
{
   clear();
}


//=============================================================================
// BBitQuadtree::addTile
//=============================================================================
bool BBitQuadtree::addTile(long x, long z)
{
   // We don't support negative values (for now, not tested).
   if(x<0 || z<0)
   {
      BFAIL("Negative coordinates not supported!");
      return(false);
   }

   // Update bounds.
   if(x<mMinX)
      mMinX=x;
   if(z<mMinZ)
      mMinZ=z;
   if(x>mMaxX)
      mMaxX=x;
   if(z>mMaxZ)
      mMaxZ=z;

   // If the tree can't already contain this tile, we need to expand the tree.
   // Special case for empty tree.
   if(!mRoot)
   {
      // Compute starting location.
      long startX=(x/cLeafSize)*cLeafSize;
      long startZ=(z/cLeafSize)*cLeafSize;

      mRoot=new BBitQuadnode(startX, startZ, cLeafSize);
      if(!mRoot)
      {
         BASSERT(0);
         return(false);
      }
   }
   // Case where we have a tree, but it needs to grow to contain this tile.
   else if(!mRoot->canContainTile(x, z))
   {
      // Keep adding parents until we contain the tile.
      BBitQuadnode *child=mRoot;
      do
      {
         // Parent size is twich that of the child.
         long size=child->getSize()*2;

         // Get starting tile for parent.
         long startX=(child->getStartX()/size)*size;
         long startZ=(child->getStartZ()/size)*size;

         // Allocate parent.
         BBitQuadnode *parent=new BBitQuadnode(startX, startZ, size);
         if(!parent)
         {
            BASSERT(0);
            break;
         }
      
         // Add child to parent.
         bool ok=parent->addChild(child);
         if(!ok)
         {
            BASSERT(0);
            break;
         }

         // Reset for next pass.
         child=parent;
      } while(!child->canContainTile(x, z));

      // New root is last allocated parent (now confusingly stored in child)
      mRoot=child;
   }

   // Add the tile.
   bool ok=mRoot->addTile(x, z);
   return(ok);
}


//=============================================================================
// BBitQuadtree::containsTile
//=============================================================================
bool BBitQuadtree::containsTile(long x, long z) const
{
   // If the tree is completely empty, the tile is obviously not contained.
   if(!mRoot)
      return(false);

   // Check the tree.
   bool contained=mRoot->containsTile(x, z);
   return(contained);
}



//=============================================================================
// BBitQuadtree::getTileCount
//=============================================================================
long BBitQuadtree::getTileCount(void) const
{
   // No root == no tiles.
   if(!mRoot)
      return(0);

   // Return the count from the top of the tree.
   return(mRoot->getTileCount());
}


//=============================================================================
// BBitQuadtree::containsAdjacentTile
//=============================================================================
bool BBitQuadtree::containsAdjacentTile(long x, long z, bool checkDiagonals=false) const
{
   // Non-diagonals.
   if(containsTile(x+1, z))
      return(true);
   if(containsTile(x, z+1))
      return(true);
   if(containsTile(x, z-1))
      return(true);
   if(containsTile(x-1, z))
      return(true);

   // Diagonals.
   if(checkDiagonals)
   {
      if(containsTile(x-1, z+1))
         return(true);
      if(containsTile(x+1, z+1))
         return(true);
      if(containsTile(x+1, z-1))
         return(true);
      if(containsTile(x-1, z-1))
         return(true);
   }

   // Nothing found.
   return(false);
}

//=============================================================================
// BBitQuadtree::containsAllAdjacentTiles
//=============================================================================
bool BBitQuadtree::containsAllAdjacentTiles(long x, long z) const
{
   if(!containsTile(x+1, z))
      return(false);
   if(!containsTile(x, z+1))
      return(false);
   if(!containsTile(x, z-1))
      return(false);
   if(!containsTile(x-1, z))
      return(false);
   if(!containsTile(x-1, z+1))
      return(false);
   if(!containsTile(x+1, z+1))
      return(false);
   if(!containsTile(x+1, z-1))
      return(false);
   if(!containsTile(x-1, z-1))
      return(false);

   return(true);
}

//=============================================================================
// BRMQuadtree::countAdjacentTiles
//=============================================================================
long BBitQuadtree::countAdjacentTiles(long x, long z) const
{
   long count = 0;
   if(containsTile(x+1, z))
      count++;
   if(containsTile(x, z+1))
      count++;
   if(containsTile(x, z-1))
      count++;
   if(containsTile(x-1, z))
      count++;
   if(containsTile(x-1, z+1))
      count++;
   if(containsTile(x+1, z+1))
      count++;
   if(containsTile(x+1, z-1))
      count++;
   if(containsTile(x-1, z-1))
      count++;

   return(count);
}

//=============================================================================
// BBitQuadtree::getNext
//=============================================================================
bool BBitQuadtree::getNext(BBitQuadIterator &iterator, long &startX, long &startZ, long &endX, long &endZ) const
{
   // Clear out parameters for good measure.
   startX=startZ=endX=endZ=-1;

   // Temp variable.
   BBitQuadIteratorStackInfo curr;

   // Are we starting fresh?
   if(iterator.isEmpty())
   {
      // If no tree, we're done already.
      if(!mRoot)
         return(false);

      // Start at root.
      curr.mNode=mRoot;
      curr.mIndex=0;
      curr.mX=0;
      curr.mZ=0;

      // Push onto stack.
      iterator.push(curr);
   }

   // While there are still nodes on the stack, process.
   while(iterator.pop(curr))
   {
      // Make sure node is really there.
      if(!curr.mNode)
      {
         BASSERT(0);
         return(false);
      }

      // If a leaf, walk the individual tiles.
      if(curr.mNode->isLeaf())
      {
         for(; curr.mX<cLeafSize; curr.mX++)
         {
            for(; curr.mZ<cLeafSize; curr.mZ++, curr.mIndex++)
            {
               bool on=curr.mNode->checkIndex(curr.mIndex);
               if(on && startZ<0)
                  startZ=curr.mZ;
               else if(!on && startZ>=0)
               {
                  // Set up extents.
                  startX=endX=curr.mNode->getStartX()+curr.mX;
                  startZ+=curr.mNode->getStartZ();
                  endZ=curr.mNode->getStartZ()+curr.mZ-1;

                  // Inc counters for next time.
                  curr.mIndex++;
                  curr.mZ++;
                  if(curr.mZ>=cLeafSize)
                  {
                     curr.mZ=0;
                     curr.mX++;
                  }

                  // Push back on stack.
                  iterator.push(curr);

                  // Pass it back.
                  return(true);
               }
            }

            // Reset Z for next pass.
            curr.mZ=0;

            // Get last strip if needed.
            if(startZ>=0)
            {
               // Set up extents.
               startX=endX=curr.mNode->getStartX()+curr.mX;
               startZ+=curr.mNode->getStartZ();
               endZ=curr.mNode->getEndZ();

               // Inc X for next pass.
               curr.mX++;

               // Push back on stack.
               iterator.push(curr);

               // Pass it back.
               return(true);
            }

         }

         // Nothing left to check, pop next thing off stack.
         continue;
      }

      // If a non-leaf, walk children.
      for(; curr.mIndex<4; curr.mIndex++)
      {
         // If the child is full, report it back.
         if(curr.mNode->isChildFull(curr.mIndex))
         {
            // Fill in coordinates.
            curr.mNode->getChildCoordinates(curr.mIndex, startX, startZ, endX, endZ);

            // Push onto stack for next time.
            curr.mIndex++;
            iterator.push(curr);

            // Back out for now.
            return(true);
         }

         // If child exists, descend.
         BBitQuadnode *child=curr.mNode->getChild(curr.mIndex);
         if(child)
         {
            // Push parent onto stack.
            curr.mIndex++;
            iterator.push(curr);

            // Set up child.
            curr.mNode=child;
            curr.mIndex=0;
            curr.mX=0;
            curr.mZ=0;

            // Push that on the stack too.
            iterator.push(curr);

            // Pop out one level.
            break;
         }
      }
   }

   // Done.
   return(false);
}


//=============================================================================
// BBitQuadtree::addTiles
//=============================================================================
bool BBitQuadtree::addTiles(long startX, long startZ, long endX, long endZ)
{
   // Supa dumb hack version for now.
   for(long x=startX; x<=endX; x++)
   {
      for(long z=startZ; z<=endZ; z++)
      {
         addTile(x, z);
      }
   }
   return(true);
}


//=============================================================================
// BBitQuadtree::distanceToTile
//=============================================================================
float BBitQuadtree::distanceToTile(long x, long z) const
{
   // No tree?  Return "infinity".
   if(!mRoot)
      return(cMaximumFloat);

   // Init to max float.
   float distSqr=cMaximumFloat;
   bool anyDist=mRoot->distSqrToTile(x, z, cMaximumFloat, distSqr);

   // Sqrt to get distance if we found anything.
   if(anyDist)
      distSqr=float(sqrt(distSqr));

   // Give back (now non-squared) distance.
   return(distSqr);
}

//=============================================================================
// BBitQuadtree::buildConvexHull
//=============================================================================
void BBitQuadtree::buildConvexHull(BConvexHull& hull, float tileSize)
{
   if(tileSize < cFloatCompareEpsilon)
   {
      BASSERT(0);
      return;
   }

   BVector points[4];
   hull.clear();

   BBitQuadIterator iterator;
   long startX, startZ, endX, endZ;
   while(getNext(iterator, startX, startZ, endX, endZ))
   {
      points[0].x=(float)startX*tileSize;
      points[0].y=0.0f;
      points[0].z=(float)startZ*tileSize;

      points[1].x=points[0].x;
      points[1].y=points[0].y;
      points[1].z=(float)endZ*tileSize;

      points[2].x=(float)endX*tileSize;
      points[2].y=points[0].y;
      points[2].z=points[1].z;

      points[3].x=points[2].x;
      points[3].y=points[0].y;
      points[3].z=points[0].z;

      hull.addPoints(points, 4);
   }
}

//=============================================================================
// BBitQuadtree::distSqrToTile
//=============================================================================
bool BBitQuadtree::distSqrToTile(long x, long z, float maxDist, float &distSqr) const
{
   // No tree?  Can't be any closer.
   if(!mRoot)
      return(false);

   // Update if closer.
   return(mRoot->distSqrToTile(x, z, maxDist, distSqr));
}

//=============================================================================
// BBitQuadtree::closestTileToTile
//=============================================================================
bool BBitQuadtree::closestTileToTile(long x, long z, long &rX, long &rZ, TILE_VALIDITY_CALLBACK validCallback, void *param) const
{
   //Bomb check.
   if (mRoot == NULL)
      return(false);

   //Init to max float.
   rX=-1;
   rZ=-1;
   float rDistanceSqr=cMaximumFloat;
   bool anyDist=mRoot->closestTileToTile(x, z, rX, rZ, rDistanceSqr, validCallback, param);
   return(anyDist);
}

//=============================================================================
// BBitQuadtree::clear
//=============================================================================
void BBitQuadtree::clear(void)
{
   // Nuke the tree.
   if(mRoot)
   {
      delete mRoot;
      mRoot=NULL;
   }
}


//=============================================================================
// BBitQuadtree::tileDistSqrToBox
//=============================================================================
float BBitQuadtree::tileDistSqrToBox(long x, long z) const
{
   // If nothing in this, the tile is very far away.
   if(!mRoot)
      return(cMaximumFloat);

   // Get distance to root box.
   return(mRoot->distSqrToBox(x, z));
}


//=============================================================================
// BBitQuadtree::getBounds
//=============================================================================
bool BBitQuadtree::getBounds(long &minX, long &minZ, long &maxX, long &maxZ) const
{
   // If tree is empty, can't return bounds.
   if(!mRoot)
      return(false);

   // Fill in bounds.
   minX=mMinX;
   minZ=mMinZ;
   maxX=mMaxX;
   maxZ=mMaxZ;

   // Success.
   return(true);
}

//=============================================================================
// BBitQuadtree::save
//=============================================================================
bool BBitQuadtree::save( BChunkWriter* chunkWriter )
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("QT"), mainHandle);
   if (!result)
   {
      {setBlogError(4006); blogerror("BBitQuadtree::save -- error writing tag.");}
      return(false);
   }

   //Version.
   result=chunkWriter->writeDWORD(msSaveVersion);
   if (!result)
   {
      {setBlogError(4007); blogerror("BBitQuadtree::save -- error writing version.");}
      return(false);
   }

   long tileCount = getTileCount();
   CHUNKWRITESAFE(chunkWriter, Long, tileCount);

   long iterCount = 0;
   if(tileCount > 0)
   {
      BBitQuadIterator iterator;
      long startX, startZ, endX, endZ;
      bool badness = false;
      while(getNext(iterator, startX, startZ, endX, endZ) && !badness)
      {
         for(long x = startX; x <= endX && !badness; x++)
         {
            for(long z = startZ; z <= endZ && !badness; z++)
            {
               iterCount++;
               if(iterCount > tileCount)
               {
                  badness = true;
                  BFAIL("tileCount is greater than iterCount!  goto :hell");
               }
               else
               {
                  CHUNKWRITESAFE(chunkWriter, Long, x);
                  CHUNKWRITESAFE(chunkWriter, Long, z);
               }
            }
         }
      }
   }

   //Finish chunk.
   result=chunkWriter->writeSize(mainHandle);
   if (!result)
   {
      {setBlogError(4008); blogerror("BBitQuadtree::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BBitQuadtree::load
//=============================================================================
long BBitQuadtree::load( BChunkReader* chunkReader )
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(0);
   }

   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("QT"));
   if (!result)
   {
      {setBlogError(4009); blogerror("BBitQuadtree::load -- error reading tag.");}
      return(0);
   }

   //Version.
   DWORD version;
   result=chunkReader->readDWORD(&version);
   if (!result)
   {
      {setBlogError(4010); blogerror("BBitQuadtree::load -- error reading version.");}
      return(0);
   }

   long tileCount = 0;
   CHUNKREADSAFE(chunkReader, Long, tileCount);

   if(tileCount > 0)
   {
      long x,z;
      for(long i=0; i < tileCount; i++)
      {
         CHUNKREADSAFE(chunkReader, Long, x);
         CHUNKREADSAFE(chunkReader, Long, z);
         addTile(x,z);
      }
   }

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("QT"));
   if (!result)
   {
      {setBlogError(4011); blogerror("BBitQuadtree::load -- did not read chunk properly!");}
      return(0);
   }

   return(1);
}


//=============================================================================
// BBitQuadtree::removeTile
//=============================================================================
bool BBitQuadtree::removeTile(long x, long z)
{
   // If the tree is empty, the tile is not in the tree.
   if(!mRoot)
      return(false);

   // If this tile is out of bounds, it is not in the tree.
   if(x<mMinX || x>mMaxX || z<mMinZ || z>mMaxZ)
      return(false);

   // Descend the tree doing the remove.
   bool removed=mRoot->removeTile(x, z);

   // If we actually removed something, we need to recalc our bounding box.
   if(removed)
   {
      // See if the root node is now empty.
      if(mRoot->isEmpty())
      {
         // Remove root node.
         delete mRoot;
         mRoot=NULL;

         // Reset bounding box.
         mMinX=cMaximumLong;
         mMinZ=cMaximumLong;
         mMaxX=cMinimumLong;
         mMaxZ=cMinimumLong;
      }
      else
      {
         // It might now be possible to reparent the tree with a smaller parent.
         // As long as the reparent function gives us a non-null result, assign that as the
         // new root pointer.
         BBitQuadnode *newParent=mRoot->reparent();
         while(newParent)
         {
            mRoot=newParent;
            newParent=mRoot->reparent();
         }

         // This isn't perfect since it can be off by up to 7 tiles.
         mMinX=mRoot->getStartX();
         mMaxX=mRoot->getEndX();
         mMinZ=mRoot->getStartZ();
         mMaxZ=mRoot->getEndZ();
      }
   }

   // Return whether the tile was actually removed or not.  If not, it means that it was not in the 
   // tree to begin with.
   return(removed);
}


//=============================================================================
// BBitQuadtree::removeTiles
//=============================================================================
bool BBitQuadtree::removeTiles(long startX, long startZ, long endX, long endZ)
{
   // Supa dumb hack version for now.
   for(long x=startX; x<=endX; x++)
   {
      for(long z=startZ; z<=endZ; z++)
      {
         removeTile(x, z);
      }
   }
   return(true);
}

//=============================================================================
// BBitQuadtree::calcEdgeTiles
//=============================================================================
bool BBitQuadtree::calcEdgeTiles(BBitQuadtree& results, long maxX, long maxZ)
{
   if (!mRoot)
      return(false);

   results.clear();

   // First find a tile that is on the left edge of the area.
   BBitQuadIterator iterator;
   long startX,startZ,endX,endZ;
   if(getNext(iterator, startX, startZ, endX, endZ))
   {
      while(containsTile(startX,startZ))
         startX--;
      startX++;

      BTileCoordinates tile;
      tile.set(startX,startZ);
      results.addTile(startX, startZ);

      BTileCoordinates offsets[4];
      offsets[0].set(-1, 0);
      offsets[1].set( 0, 1);
      offsets[2].set( 1, 0);
      offsets[3].set( 0,-1);

      BTileCoordinates offsets2[8];
      offsets2[0].set(-1, 0);
      offsets2[1].set(-1, 1);
      offsets2[2].set( 0, 1);
      offsets2[3].set( 1, 1);
      offsets2[4].set( 1, 0);
      offsets2[5].set( 1, -1);
      offsets2[6].set( 0,-1);
      offsets2[7].set( -1,-1);

      // Now find the end tile so we'll be able to determine when we've finished going around the area.
      for(long i=3; i>=0; i--)
      {
         tile.x=startX+offsets[i].x;
         tile.z=startZ+offsets[i].z;
         if(containsTile(tile.x,tile.z))
         {
            endX=tile.x;
            endZ=tile.z;
            break;
         }
      }

      if(endX!=-1)
      {
         // Go around the area clockwise building the outline.
         bool done=false;
         long oi=1;
         long x=startX;
         long z=startZ;

         while(!done)
         {
            long i;
            for(i=0; i<4; i++)
            {
               tile.x=x+offsets[oi].x;
               tile.z=z+offsets[oi].z;
               if(containsTile(tile.x,tile.z))
               {
                  if(x==endX && z==endZ && tile.x==startX && tile.z==startZ)
                  {
                     done=true;
                     break;
                  }

                  // Don't add tiles that are on the edge of the map and completely inside the area.
                  bool add;
                  if(x==0 || z==0 || x==maxX || z==maxZ)
                  {
                     add=false;
                     for(long j=0; j<8; j++)
                     {
                        long x2=x+offsets2[j].x;
                        long z2=z+offsets2[j].z;
                        if(x2<0 || z2<0 || x2>maxX || z2>maxZ)
                           ;
                        else if(!containsTile(x2, z2))
                        {
                           add=true;
                           break;
                        }
                     }
                  }
                  else
                     add=true;

                  if(add)
                     results.addTile(tile.x, tile.z);

                  oi+=3;
                  if(oi>3)
                     oi-=4;
                  x=tile.x;
                  z=tile.z;
                  break;
               }
               oi++;
               if(oi==4)
                  oi=0;
            }
            if(i==4)
               done=true;
         }
      }
   }

   return(true);
}

//=============================================================================
// BBitQuadtree::calcEdgePoints
//=============================================================================
bool BBitQuadtree::calcEdgePoints(BCopyList<BTileCoordinates> &edgeList)
{
   if (!mRoot)
      return(false);
   
   BBitQuadIterator iterator;
   long startX,startZ,endX,endZ;
   edgeList.empty();

   if(getNext(iterator, startX, startZ, endX, endZ))
   {
      // First find a tile that is on the left edge of the area.
      for(long x=startX; x>=0; x--)
      {
         if(containsTile(x,startZ))
            startX=x;
      }

      BTileCoordinates offsets[8];
      offsets[0].set(-1, 0);
      offsets[1].set(-1, 1);
      offsets[2].set( 0, 1);
      offsets[3].set( 1, 1);
      offsets[4].set( 1, 0);
      offsets[5].set( 1, -1);
      offsets[6].set( 0,-1);
      offsets[7].set( -1,-1);

      BTileCoordinates corners[4];
      corners[0].set(0, 0);
      corners[1].set(0, 1);
      corners[2].set(1, 1);
      corners[3].set(1, 0);

      edgeList.empty();

      // Add the first point (bottom left corner of the starting tile).
      BTileCoordinates tile;
      tile.set(startX,startZ);
      edgeList.addToTail(tile);
      long point=0;

      // Now find the end tile so we'll be able to determine when we've finished going around the area.
      for(long i=7; i>=0; i--)
      {
         tile.x=startX+offsets[i].x;
         tile.z=startZ+offsets[i].z;
         if(containsTile(tile.x,tile.z))
         {
            endX=tile.x;
            endZ=tile.z;
            break;
         }
      }

      if(endX!=-1)
      {
         // Go around the area clockwise building the edge.
         bool done=false;
         long oi=1;
         long x=startX;
         long z=startZ;

         BTileCoordinates cornerTile;

         while(!done)
         {
            long i;
            for(i=0; i<8; i++)
            {
               tile.x=x+offsets[oi].x;
               tile.z=z+offsets[oi].z;
               if(containsTile(tile.x,tile.z))
               {
                  if(x==endX && z==endZ && tile.x==startX && tile.z==startZ)
                     done=true;

                  for(long j=0; j<4; j++)
                  {
                     if(point==3)
                        point=0;
                     else
                        point++;
                     cornerTile.x=x+corners[point].x;
                     cornerTile.z=z+corners[point].z;
                     
                     long k;
                     for(k=0; k<4; k++)
                     {
                        if(cornerTile.x==tile.x+corners[k].x && cornerTile.z==tile.z+corners[k].z)
                        {
                           point=k;
                           break;
                        }
                     }
                     if(!done || k!=0)
                        edgeList.addToTail(cornerTile);
                     if(k<4)
                        break;
                  }

                  if(done)
                     break;

                  oi+=5;
                  if(oi>7)
                     oi-=8;
                  x=tile.x;
                  z=tile.z;
                  break;
               }
               oi++;
               if(oi==8)
                  oi=0;
            }
            if(i==8)
               done=true;
         }
      }
   }

   return(true);
}

//=============================================================================
// BBitQuadtree::calcNeighborTiles
//=============================================================================
bool BBitQuadtree::calcNeighborTiles(BBitQuadtree& results, long maxX, long maxZ)
{
   if (!mRoot)
      return(false);

   results.clear();

   // This function gets all the tiles within 1 tile this bbitquadtree that are
   // not in this bbitquadtree

   // The implementation does the very dumb thing of going through each tile, looking
   // at all 8 of its neighbor tiles and adding it if it is not in the bbitquadtree


   BTileCoordinates tile;

   BTileCoordinates offsets[8];
   offsets[0].set(-1, 0);
   offsets[1].set(-1, 1);
   offsets[2].set( 0, 1);
   offsets[3].set( 1, 1);
   offsets[4].set( 1, 0);
   offsets[5].set( 1, -1);
   offsets[6].set( 0,-1);
   offsets[7].set( -1,-1);

   BBitQuadIterator iterator;
   long startX,startZ,endX,endZ;
   while(getNext(iterator, startX, startZ, endX, endZ))
   {
      for(long x = startX; x <= endX; x++)
      {
         for(long z = startZ; z <= endZ; z++)
         {
            // Check all 8 neighbors
            for (long oi = 0; oi < 8; oi++)
            {
               tile.x=x+offsets[oi].x;
               tile.z=z+offsets[oi].z;
               if (!containsTile(tile.x, tile.z))
               {
                  if ((tile.x >= 0) && (tile.z >= 0) && (tile.x <= maxX) && (tile.z <= maxZ))
                  {
                     results.addTile(tile.x, tile.z);
                  }
               }
            }
         }
      }
   }

   return(true);
}

//==============================================================================
// getTileDelta
//==============================================================================
void getTileDelta(const BTileCoordinates *p1, const BTileCoordinates *p2, long &dx, long &dz)
{
   if(p1->x>p2->x)
      dx=-1;
   else if(p1->x<p2->x)
      dx=1;
   else
      dx=0;

   if(p1->z>p2->z)
      dz=-1;
   else if(p1->z<p2->z)
      dz=1;
   else
      dz=0;
}


//==============================================================================
// smoothEdge
//==============================================================================
void smoothEdge(BCopyList<BTileCoordinates> &edgeList, long smoothDistance, long maxX, long maxZ)
{
   maxX, maxZ;

   // Bail on zero distance.
   if(smoothDistance<=0)
      return;

   // Copy edge.
   BCopyList<BTileCoordinates> testEdge;
   BHandle p1Handle;
   BTileCoordinates * p1=edgeList.getHead(p1Handle);
   while(p1)
   {
      testEdge.addToTail(*p1);

      // Add middle points if necessary
      BHandle p2Handle=p1Handle;
      const BTileCoordinates * const p2=edgeList.getNextWithWrap(p2Handle);
      long dx=p2->x-p1->x;
      long dz=p2->z-p1->z;
      if(dx!=0)
      {
         if(dx>0)
            p1->x++;
         else
            p1->x--;
         while(p2->x!=p1->x)
         {
            testEdge.addToTail(*p1);
            if(dx>0)
               p1->x++;
            else
               p1->x--;
         }
      }
      else
      {
         if(dz>0)
            p1->z++;
         else
            p1->z--;
         while(p2->z!=p1->z)
         {
            testEdge.addToTail(*p1);
            if(dz>0)
               p1->z++;
            else
               p1->z--;
         }
      }

      // Next.
      p1=edgeList.getNext(p1Handle);
   }

   // Clear edgeList.
   edgeList.empty();

   // Walk along edge.
   BTileCoordinates p;
   p1=testEdge.getHead(p1Handle);
   while(p1)
   {
      // MPB [2/16/2005] - Removed the special case of not smoothing points along the map
      // edge.  This was probably put here for a reason, but I don't know what that would be
      // now.  Since it causes some problem areas (especially for cliffs) along the map edge, 
      // I'm removing it.

      // Just keep the point if it is along the edge of the map.
      /*
      if (p1->x==0 || p1->z==0 || p1->x>=maxX || p1->z>=maxZ)
      {
         edgeList.addToTail(*p1);
      }
      else
      {
      */
         // Average with n points forward and n points backward.
         float tx=float(p1->x);
         float tz=float(p1->z);

         BHandle fwd=p1Handle;
         BHandle back=p1Handle;
         for(long i=0; i<smoothDistance; i++)
         {
            const BTileCoordinates * const fPt = testEdge.getNextWithWrap(fwd);
            const BTileCoordinates * const bPt=testEdge.getPrevWithWrap(back);
            if(!fPt || !bPt)
            {
               BASSERT(0);
               break;
            }

            tx+=fPt->x;
            tz+=fPt->z;
            tx+=bPt->x;
            tz+=bPt->z;
         }

         p.x=long(tx/(1.0f+2.0f*smoothDistance)+0.5f);
         p.z=long(tz/(1.0f+2.0f*smoothDistance)+0.5f);
         edgeList.addToTail(p);
      //}

      // Next.
      p1=testEdge.getNext(p1Handle);
   }
}


//==============================================================================
// cleanEdge
//==============================================================================
void cleanEdge(BCopyList<BTileCoordinates> &edgeList)
{
   // Replace diagonals with zig-zags.
   BHandle p1Handle;
   BHandle p2Handle;
   BTileCoordinates *p1=edgeList.getHead(p1Handle);
   BTileCoordinates *p2=edgeList.getTail(p2Handle);
   while(p1 && p2)
   {
      // Get p2 to p1 direction.
      long dx, dz;
      getTileDelta(p2, p1, dx, dz);

      // For now, cheezeball version.
      BTileCoordinates temp;

      // Up and right
      if(dx>0 && dz>0)
      {
         temp.x=p1->x;
         temp.z=p2->z;
         edgeList.addAfter(temp, p2Handle);
      }
      // Down and right
      else if(dx>0 && dz<0)
      {
         temp.x=p2->x;
         temp.z=p1->z;
         edgeList.addAfter(temp, p2Handle);
      }
      // Up and left
      else if(dx<0 && dz>0)
      {
         temp.x=p1->x;
         temp.z=p2->z;
         edgeList.addAfter(temp, p2Handle);
      }
      // Down and left.
      else if(dx<0 && dz<0)
      {
         temp.x=p2->x;
         temp.z=p1->z;
         edgeList.addAfter(temp, p2Handle);
      }

      // Next.
      p2=p1;
      p2Handle=p1Handle;
      p1=edgeList.getNext(p1Handle);
   }

   // Remove dupes.
   p1=edgeList.getHead(p1Handle);
   p2=edgeList.getTail(p2Handle);
   while(p1 && p2)
   {
      // Get p2 to p1 direction.
      long dx, dz;
      getTileDelta(p2, p1, dx, dz);

      // If a dupe point, nuke-o-tron.
      if(dx==0 && dz==0)
      {
         // Set up for next.
         BHandle tempHandle=p1Handle;
         p1=edgeList.getNext(tempHandle);

         // Remove extra one.
         edgeList.remove(p1Handle);

         // Reprocess from same starting point.
         p1Handle=tempHandle;
         continue;
      }

      // Next.
      p2=p1;
      p2Handle=p1Handle;
      p1=edgeList.getNext(p1Handle);
   }


   // Remove unnecessary points from edge.
   p1=edgeList.getHead(p1Handle);
   while(p1)
   {
      // Can we skip the next point?
      // Get next point.
      BHandle p2HandleB=p1Handle;
      BTileCoordinates *p2=edgeList.getNext(p2HandleB);
      if(p2HandleB)
      {
         // Get point after that.
         BHandle p3Handle=p2HandleB;
         BTileCoordinates *p3=edgeList.getNext(p3Handle);

         if(p3Handle)
         {
            // Get p1 to p2 direction.
            long dx1, dz1;
            getTileDelta(p1, p2, dx1, dz1);

            // Get p2 to p3 direction.
            long dx2, dz2;
            getTileDelta(p2, p3, dx2, dz2);

            // If they match, remove middle point (p2). (abs also prevents double-backs)
            if(abs(dx1)==abs(dx2) && abs(dz1)==abs(dz2))
            {
               edgeList.remove(p2HandleB);

               // Reprocess from same starting point.
               continue;
            }
         }
      }

      // Next.
      p1=edgeList.getNext(p1Handle);
   }

   // Make sure we don't need to remove the starting point (head of the list)
   p1=edgeList.getTail(p1Handle);
   if(p1Handle)
   {
      BHandle p2HandleC;
      BTileCoordinates *p2=edgeList.getHead(p2HandleC);
      if(p2HandleC)
      {
         BHandle p3Handle=p2HandleC;
         BTileCoordinates *p3=edgeList.getNext(p3Handle);
         if(p3Handle)
         {
            // Get p1 to p2 direction.
            long dx1, dz1;
            getTileDelta(p1, p2, dx1, dz1);

            // Get p2 to p3 direction.
            long dx2, dz2;
            getTileDelta(p2, p3, dx2, dz2);

            // If they match, remove middle point (p2).
            if(dx1==dx2 && dz1==dz2)
            {
               edgeList.remove(p2HandleC);
            }
         }
      }
   }
}


//=============================================================================
// BBitQuadtree::addTiles
//=============================================================================
bool BBitQuadtree::addTiles(const BBitQuadtree &tiles)
{
   // Walk tiles and add them.
   BBitQuadIterator iter;
   long startX, startZ, endX, endZ;
   bool result=true;
   while(tiles.getNext(iter, startX, startZ, endX, endZ))
   {
      bool ok = addTiles(startX, startZ, endX, endZ);
      if(!ok)
         result = false;
   }

   // Success.
   return(result);
}


//=============================================================================
// BBitQuadtree::simpleCalcEdgeTiles
//=============================================================================
void BBitQuadtree::simpleCalcEdgeTiles(BBitQuadtree &edge) const
{
   static const long sDeltaX[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
   static const long sDeltaZ[8] = {0, 1, 1, 1, 0, -1, -1, -1};

   // Clear any extraneous data.
   edge.clear();
   
   // Bail if no tiles.
   if(getTileCount() == 0)
      return;
   
   // Get left-most tile, with lower-most breaking ties.
   long beginX = cMaximumLong;
   long beginZ = cMaximumLong;
   BBitQuadIterator iter;
   long x1, z1, x2, z2;
   while(getNext(iter, x1, z1, x2, z2))
   {
      if((x1<beginX) || (x1==beginX) && (z1<beginZ))
      {
         beginX = x1;
         beginZ = z1;
      }
   }
  
   // Start at left/lower point we just found.
   long x = beginX;
   long z = beginZ;
   // Assume we moved right to get to this spot, so we'll check up first.
   long currDir = 4;
   do 
   {
      // Add to edge list.
      edge.addTile(x, z);
      
      // We'll generally try to make left turns to follow the edge.  Try "as left as possible" first.
      long startDir = currDir-2;
      if(startDir<0)
         startDir = 8 + startDir;
         
      // Try each direction.
      currDir = startDir;
      do
      {
         // Do we have a tile in this direction?
         long newX = x+sDeltaX[currDir];
         long newZ = z+sDeltaZ[currDir];
         if(containsTile(newX, newZ))
         {
            // This is the new tile, so start processing from there now.
            x = newX;
            z = newZ;
            break;
         }
         
         // Next direction.
         currDir++;
         if(currDir>7)
            currDir = 0;
      } while(currDir != startDir);
     
      // We keep doing until we get back to where we started.
   } while(x!=beginX || z!=beginZ);
}


//=============================================================================
// BBitQuadtree::shrinkInto
//=============================================================================
void BBitQuadtree::shrinkInto(BBitQuadtree &results) const
{
   // Sanity.
   if(&results == this)
   {
      BFAIL("Can't shrink into self.");
      return;
   }
   
   // Clear exist garbage.
   results.clear();
   
   // There's probably a more clever way to do this...
   // Get edge tiles.
   BBitQuadtree edge;
   simpleCalcEdgeTiles(edge);
   
   BBitQuadIterator iter;
   long x1, z1, x2, z2;
   while(getNext(iter, x1, z1, x2, z2))
   {
      for(long x=x1; x<=x2; x++)
      {
         for(long z=z1; z<=z2; z++)
         {
            // If not part of edge, add it.
            if(!edge.containsTile(x, z))
               results.addTile(x, z);
         }
      }
   }
}


//=============================================================================
// BBitQuadtree::growInto
//=============================================================================
void BBitQuadtree::growInto(BBitQuadtree &results) const
{
   // Sanity.
   if(&results == this)
   {
      BFAIL("Can't grow into self.");
      return;
   }

   BBitQuadIterator iter;
   long x1, z1, x2, z2;
   while(getNext(iter, x1, z1, x2, z2))
   {
      results.addTiles(max(0, x1-1), max(0, z1-1), x2+1, z2+1);
   }
}


//=============================================================================
// eof: bitquadtree.cpp
//=============================================================================

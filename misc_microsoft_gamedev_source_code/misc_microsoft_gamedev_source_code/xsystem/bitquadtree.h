//=============================================================================
// Copyright (c) 2001-2003 Ensemble Studios
//
// Bit quadtree.
//=============================================================================

#ifndef _BITQUADTREE_
#define _BITQUADTREE_

#include "bitvector.h"
#include "tilecoordinates.h"

class BConvexHull;
class BChunkWriter;
class BChunkReader;


//=============================================================================
// Callback definition.
typedef bool (*TILE_VALIDITY_CALLBACK)(long x, long z, void *param);



//=============================================================================
// class BBitQuadnode
//=============================================================================
class BBitQuadnode
{
   public:
                              BBitQuadnode(long startX, long startZ, long size);
                              ~BBitQuadnode(void);

      bool                    addTile(long x, long z);
      bool                    removeTile(long x, long z);
      bool                    containsTile(long x, long z) const;
      bool                    canContainTile(long x, long z) const;
      bool                    addChild(BBitQuadnode *node);

      long                    getStartX(void) const {return(mStartX);}
      long                    getStartZ(void) const {return(mStartZ);}
      long                    getEndX(void) const {return(mEndX);}
      long                    getEndZ(void) const {return(mEndZ);}
      long                    getSize(void) const {return(mEndX-mStartX+1);}
      
      bool                    distSqrToTile(long x, long z, float maxDist, float &distSqr) const;
      bool                    closestTileToTile(long x, long z, long &rX, long &rZ, float &rDistanceSqr, TILE_VALIDITY_CALLBACK validCallback, void *param) const;
      float                   distSqrToBox(long x, long z) const;  // inside the box is considered 0 distance

      bool                    isFull(void) const;
      bool                    isEmpty(void) const {return(mTileCount==0);}
      long                    getTileCount(void) const {return(mTileCount);}


      // Only intended to be used for walking tree externally in BBitQuadtree.
      BBitQuadnode             *getChild(long childIndex) const;
      void                    getChildCoordinates(long childIndex, long &startX, long &startZ, long &endX, long &endZ) const;
      bool                    isLeaf(void) const {return(mLeaf);}
      bool                    isChildFull(long childIndex) const;
      bool                    checkIndex(long index) const {if(mTiles.isSet(index)) return(true); return(false);}
      BBitQuadnode             *reparent(void);

   protected:
      long                    computeChildIndex(long x, long z) const;

      // Geography.
      long                    mStartX;
      long                    mStartZ;
      long                    mEndX;
      long                    mEndZ;

      // Contents.
      long                    mTileCount;
      BBitVector64            mTiles;
      bool                    mLeaf;

      // Children.
      BBitQuadnode             *mChild[4];
      bool                    mChildFull[4];
};


//=============================================================================
// class BBitQuadIteratorStackInfo
//=============================================================================
class BBitQuadIteratorStackInfo
{
   public:
                              BBitQuadIteratorStackInfo(void) :
                                 mNode(NULL),
                                 mIndex(0),
                                 mX(0),
                                 mZ(0) 
                              {}

      BBitQuadnode             *mNode;
      long                    mIndex;
      long                    mX;
      long                    mZ;
};


//=============================================================================
// class BBitQuadIterator
//=============================================================================
class BBitQuadIterator
{
   public:
                              BBitQuadIterator(void) {reset();}

      void                    reset(void) {mStack.setNumber(0);}


      // Stuff that should only be called from BBitQuadtree.
      bool                    isEmpty(void) const {if(mStack.getNumber()>0) return(false); else return(true);}
      void                    push(const BBitQuadIteratorStackInfo &stackInfo) {mStack.add(stackInfo);}
      bool                    pop(BBitQuadIteratorStackInfo &stackInfo)
                              {
                                 long lastIndex=mStack.getNumber()-1;
                                 if(lastIndex>=0)
                                 {
                                    stackInfo=mStack[lastIndex];
                                    mStack.setNumber(lastIndex);
                                    return(true);
                                 }
                                 return(false);
                              };

   protected:
      BDynamicSimArray<BBitQuadIteratorStackInfo> mStack;
};


//=============================================================================
// class BBitQuadtree
//=============================================================================
class BBitQuadtree
{
   public:
                              BBitQuadtree(void);
                              ~BBitQuadtree(void);

      bool                    addTile(long x, long z);
      bool                    addTiles(long startX, long startZ, long endX, long endZ);
      bool                    addTiles(const BBitQuadtree &tiles);

      bool                    removeTile(long x, long z);
      bool                    removeTiles(long startX, long startZ, long endX, long endZ);

      void                    clear(void);

      bool                    containsTile(long x, long z) const;
      bool                    containsAdjacentTile(long x, long z, bool checkDiagonals) const;
      bool                    containsAllAdjacentTiles(long x, long z) const;
      long                    countAdjacentTiles(long x, long z) const;

      long                    getTileCount(void) const;

      float                   distanceToTile(long x, long z) const;

      void                    buildConvexHull(BConvexHull& hull, float tileSize);
      // distSqr IN is closest distance so far (or cMaximumFloat) and will be filled in will new closest squared distance.
      bool                    distSqrToTile(long x, long z, float maxDist, float &distSqr) const;
      bool                    closestTileToTile(long x, long z, long &rX, long &rZ, TILE_VALIDITY_CALLBACK validCallback=NULL, void *param=NULL) const;

      float                   tileDistSqrToBox(long x, long z) const;

      // Functions to walk the tree.
      bool                    getNext(BBitQuadIterator &iterator, long &startX, long &startZ, long &endX, long &endZ) const;

      bool                    getBounds(long &minX, long &minZ, long &maxX, long &maxZ) const;

      bool                    save( BChunkWriter* chunkWriter );
      long                    load( BChunkReader* chunkReader );
      
      bool                    calcEdgeTiles(BBitQuadtree& results, long maxX, long maxZ);
      bool                    calcEdgePoints(BCopyList<BTileCoordinates> &edgeList);
      bool                    calcNeighborTiles(BBitQuadtree& results, long maxX, long maxZ);

      // jce [5/24/2004] -- I was confused by the extra logic in calcEdgeTiles, so here's a very simplistic obvious version.
      // If there are multiple disconnected areas, this will currently only grab the edge of one (the leftmost/lowermost).
      void                    simpleCalcEdgeTiles(BBitQuadtree &edge) const;

      // Shrinks by one tile.  Also not hip to multiple disconnected areas.
      void                    shrinkInto(BBitQuadtree &results) const;

      // Grows by one tile.
      void                    growInto(BBitQuadtree &results) const;

   protected:
      BBitQuadnode            *mRoot;
      long                    mMinX;
      long                    mMinZ;
      long                    mMaxX;
      long                    mMaxZ;

      static const DWORD      msSaveVersion;
};


//=============================================================================
// Helper c functions for edges.
//=============================================================================
void smoothEdge(BCopyList<BTileCoordinates> &edgeList, long smoothDistance, long maxX, long maxZ);
void cleanEdge(BCopyList<BTileCoordinates> &edgeList);

#endif


//=============================================================================
// eof: bitquadtree.h
//=============================================================================

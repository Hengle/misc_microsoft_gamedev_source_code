//==============================================================================
// pather.h
//
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes

//XCORE
#include "xcore.h"
#include "xcorelib.h"
#include "math\vector.h"

//#include "path.h"
//#include "typedid.h"
//#include "nonconvexhull2.h"
//#include "pathquad.h"
//#include "convexhull.h"

//==============================================================================
// Forward declarations
class BObstructionManager;
//class BConvexHull;
//class BUnit;
//class BPathQueues;
//class BPathQuadMgr;
//class BPathNode2;
class BLrpTree;
//class BLrpNode;
//class BBitQuadtree;
//class BEntity;

		// Forward Declarations to support the new obstruction manager

class BOPObstructionNode;


//==============================================================================
class BPather
{
   public:

      // Constructors
      BPather( void );

      // Destructors
      ~BPather( void );      
      
      // Quad Tree Updating
      void                    buildPathingQuads(BObstructionManager *pObManager);
      inline void             enableQuadUpdate(bool bUpdate = true) { mbQuadUpdate = bUpdate; }

      // Functions to support the new obstruction manager
		void							updatePathingQuad(BObstructionManager *pObManager, BOPObstructionNode* theNode, bool AddNode);
		void							updatePathingQuad(BObstructionManager *pObManager, const BVector &vMin, const BVector &vMax);
      bool                    saveLRPTree(const char* fileNameLRP);

   protected:
         
      // Variables
      float                   mfTileSize;             // Terrain Tile Size, we use it for some limit checks in the quad pather
      float                   mfTileSizeSqr;          // Guess..

      // New Parms for Lrp Pather
      BLrpTree                *mLrpTreeLand;
      BLrpTree                *mLrpTreeFlood;
      BLrpTree                *mLrpTreeScarab;
      BLrpTree                *mLrpTreeHover;
      BLrpTree                *mLrpTreeAir;

      bool                    mbQuadUpdate;           // Enable/Disable quad updating

   private:

      // Functions

      // Variables

}; // BPather

extern BPather gPather;

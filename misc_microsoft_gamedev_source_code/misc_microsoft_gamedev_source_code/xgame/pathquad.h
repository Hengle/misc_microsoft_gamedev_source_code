//==============================================================================
// pathquad.h
//
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BObstructionManager;
class BPathQuadNode;
class BPathQuadTree;
class BConvexHull;

//==============================================================================
// Const declarations

typedef BDynamicSimArray<BPathQuadNode> BPathQuadArray;
typedef BDynamicSimArray<BPathQuadNode *> BPathQuadPtrArray;

//==============================================================================
class BPathQuadNode
{
   public:
      // Children Indices
      enum 
      {
         NW_CHILD,
         NE_CHILD,
         SW_CHILD,
         SE_CHILD
      };

      // Direction Indices
      enum
      {
         NORTH_DIR,
         EAST_DIR,
         SOUTH_DIR,
         WEST_DIR
      };

      // Constructors
      BPathQuadNode( void );

      // Destructors
      ~BPathQuadNode( void );

      // Functions
      void                       setParms(float fMinX, float fMinZ, float fMaxX, float fMaxZ,
                                    BPathQuadNode *pParent, BPathQuadTree *pTree,
                                    long lChildIndex);
      void                       buildTree(float fMinX, float fMinZ, float fMaxX, float fMaxZ,
                                    float fStopX, float fStopZ, BPathQuadNode *pNodeParent,
                                    BPathQuadTree *mtree, long lChildIndex, BObstructionManager *pObManager);

      void                       getPassableList(unsigned long lDir, unsigned long lMask,
                                    BPathQuadPtrArray &quadList);

      void                       addObstruction(const BConvexHull &hull, float fMinQuadX, float fMinQuadZ,
                                    BPathQuadTree *pTree);

      bool                       clearObstruction(const BConvexHull &hull,
                                    BObstructionManager *pObManager);  // true if quad is cleared, false if not

      void                       addNodeDebugLines(const char *name);

      inline BPathQuadNode       *getChild(long lIndex)
                                 { return mpChild[lIndex]; }
      inline BPathQuadNode       *getParent()
                                 { return mpParent; }

      BPathQuadNode              *getNode(const BVector &vPoint);

      bool                        obstructedBy(unsigned long lMask);

      bool                       contains(const BVector &vPoint);
      
      BPathQuadNode              *top();                 // Get the node above me at the same or next higher level.
      BPathQuadNode              *bottom();              // Get the node below me at the same or next higher level.
      BPathQuadNode              *left();                // Get the node to the left of me at the same or next higher level.
      BPathQuadNode              *right();               // Get the node to the right of me at the same or next higher level.

      BPathQuadNode              *upperRight();          // Get the lowest node that's above me and to the right
      BPathQuadNode              *upperLeft();           // Get the lowest node that's above me and to the left
      BPathQuadNode              *lowerRight();          // Get the lowest node that's below me and to the right
      BPathQuadNode              *lowerLeft();           // Get the lowest nod that's below me and to the left


      BVector                    getMidpoint();

      void                       setColor(DWORD color)
                                 { mColor = color; }

      // Variables

   protected:

      // Functions

      // Variables
      float                      mfMinX;
      float                      mfMinZ;
      float                      mfMaxX;
      float                      mfMaxZ;

//      long                       mlIndex;                // Master Index
      long                       mlChildIndex;           // Index of which child I am

      BPathQuadTree              *mpTree;                // Ptr back to the tree
      BPathQuadNode              *mpParent;              // Parent
      BPathQuadNode              *mpChild[4];            // 0=NW 1=NE 2=SW 3=SE

      unsigned long              mlObstructionMask;      // What we're obstructed with

      DWORD                      mColor;                 // Debug color
      
   private:

      // Functions

      // Variables

}; // BPathQuadNode


//==============================================================================
// Class BPathQuadTree
// NOTE: Right now I'm implementing this in a "data structure/data element" model,
// where we have a class that represents the data structure itself, (the Quad Tree),
// and a separate class tha represents the elements of the data structure (Quad Nodes).
// I tend to prefer this paradigm, as I beleive that I will eventually need many 
// utility type functions that will operate on the tree itself, as opposed to just
// operating on individual nodes.  However, if after some usage we find that the
// tree class is superflous, it's a trivial thing to dispense with it. (dlm)
//==============================================================================
class BPathQuadTree
{
   public:
      // Constructors
      BPathQuadTree( void );

      // Destructors
      ~BPathQuadTree( void );

      // Functions
      bool                       init(BObstructionManager *pObManager, 
                                 float fSmallestX, float fSmallestZ, 
                                 float fLargestX, float fLargestZ);

      void                       clear();

      void                       render();

      bool                       initialized()
                                 { return mbInitialized; }

      void                       addObstruction(const BConvexHull &hull);
      void                       clearObstruction(const BConvexHull &hull, 
                                    BObstructionManager *pObManager);

      BPathQuadNode              *getNode(const BVector &vPoint);

      inline float               getMinX()
                                 { return mfMinX; }
      inline float               getMinZ()
                                 { return mfMinZ; }
      inline float               getMaxX()
                                 { return mfMaxX; }
      inline float               getMaxZ()
                                 { return mfMaxZ; }

      inline float               getMinQuadSizeX()
                                 { return mfMinQuadSizeX; }
      inline float               getMinQuadSizeZ()
                                 { return mfMinQuadSizeZ; }

   protected:
      BPathQuadNode              *mpRoot;
      bool                       mbInitialized;

      float                      mfMinX;
      float                      mfMaxX;
      float                      mfMinZ;
      float                      mfMaxZ;

      float                      mfMinQuadSizeX;
      float                      mfMinQuadSizeZ;

   private:

};
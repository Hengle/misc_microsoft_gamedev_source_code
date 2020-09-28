//==============================================================================
// nonconvexhull2.h
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
//#include "common.h"
#include "pathhull.h"
//#include "threading\win32semaphore.h"

#include "PropertyDefs.h"
//==============================================================================
// Forward declarations
class BObstructionManager;
class BOPObstructionNode;
class BOPQuadHull;
class BGrannyModel;

//==============================================================================
// Const declarations

class BNCHVector
{
public:
   float                   x, z;

   void                    set(const float ix, const float iz);
   
   BVector                 getBVector() const;

   bool                    compare( const BNCHVector& b, const float epsilon ) const;

   void                    zero();
};

typedef struct
{
   BDynamicArray<BNCHVector> vertex;     // Vertex array pointer
   bool                             hole;          
} BNCHVertexList;

class BNCHPolygon
{
public:
   BNCHPolygon         ();
   ~BNCHPolygon        ();
   BNCHPolygon         &operator =  (const BNCHPolygon &h);

   void                reset        ();
   void                dealloc      ();

   long                 mAlloc;
   DWORD                mAge;

   BDynamicArray<BNCHVertexList>    hull;        // Hull array pointer
};

typedef float NCHfloat;
const NCHfloat NCHEpsilon = FLT_EPSILON;

typedef enum                        // Edge bundle state                 
{
   UNBUNDLED,                        // Isolated edge not within a bundle 
   BUNDLE_HEAD,                      // Bundle head node                  
   BUNDLE_TAIL                       // Passive bundle tail node          
} bundle_state;

typedef struct v_shape              // Internal vertex list datatype     
{
   NCHfloat            x;            // X coordinate component            
   NCHfloat            z;            // Z coordinate component            
   struct v_shape     *next;         // Pointer to next vertex in list    
} vertex_node;

typedef struct p_shape              // Internal hull / tristrip type  
{
   int                active;        // Active flag / vertex count        
   bool               hole;          // Hole / external hull flag      
   vertex_node        *v[2];         // Left and right vertex list ptrs   
   struct p_shape     *next;         // Pointer to next polygon hull   
   struct p_shape     *proxy;        // Pointer to actual structure used  
} polygon_node;

typedef enum
{
   EPREV = 0,
   ENEXT,
   EPRED,
   ESUCC,
   ENEXTBOUND
} ptrStates;

struct edge_node_RarelyAccessed {
	BNCHVector vertex[3];
    polygon_node       *outp[2];        // Output polygon / tristrip pointer 
};
typedef struct edge_shape
{
	edge_shape() :m_pRarelyAccessedData(new edge_node_RarelyAccessed) {} //AMG
	~edge_shape() {delete m_pRarelyAccessedData;} //AMG
   int                 type;           // Clip / subject edge flag          

   SPLIT_ARRAY(BNCHVector,vertex);         // Piggy-backed hull vertex data  
   NCHfloat            xb;             // Scanbeam bottom x coordinate      
   NCHfloat            xt;             // Scanbeam top x coordinate         
   NCHfloat            dx;             // Change in x for a unit z increase 
   SPLIT_ARRAY(polygon_node*,outp);        // Output polygon / tristrip pointer 
   unsigned char scratch[4][2];  // 0 = bundle l/r, 1 = bundle edge above, 2 = bundle edge below, 3.0 = edge bundle state above, 3.1 = bstate below
   struct edge_shape  *ptrs[5];
   edge_node_RarelyAccessed* m_pRarelyAccessedData;
} edge_node;

typedef struct lmt_shape            // Local minima table                
{
   NCHfloat            z;            // Z coordinate at local minimum     
   edge_node          *first_bound;  // Pointer to bound list             
   struct lmt_shape   *next;         // Pointer to next local minimum     
} lmt_node;

typedef struct sbt_t_shape          // Scanbeam tree                     
{
   NCHfloat            z;            // Scanbeam node Z value             
   struct sbt_t_shape *less;         // Pointer to nodes with lower z     
   struct sbt_t_shape *more;         // Pointer to nodes with higher z    
} sb_tree;

typedef struct it_shape             // Intersection table                
{
   edge_node          *ie[2];        // Intersecting edge (bundle) pair   
   BNCHVector          point;        // Point of intersection             
   struct it_shape    *next;         // The next intersection table node  
} it_node;

typedef struct st_shape             // Sorted edge table                 
{
   edge_node          *edge;         // Pointer to AET edge               
   NCHfloat            xb;           // Scanbeam bottom x coordinate      
   NCHfloat            xt;           // Scanbeam top x coordinate         
   NCHfloat            dx;           // Change in x for a unit y increase 
   struct st_shape    *prev;         // Previous edge in sorted list      
} st_node;


//==============================================================================
class BNCHBuilder
{
   public:
      void                       polygonMerge( const BNCHPolygon& subj, const BNCHPolygon& clip, BNCHPolygon& result);

  
   protected:
      void                       resetIT(it_node **it);
      void                       resetLMT(lmt_node **lmt);
      void                       resetSBT(sb_tree **sbtree);
      void                       resetST();

      void                       insertBound(edge_node **b, edge_node *e);
      edge_node **               boundList(lmt_node **lmt, NCHfloat y);
      void                       addToSBT(int *entries, sb_tree **sbtree, NCHfloat y);
      void                       buildLMT(lmt_node **lmt, sb_tree **sbtree, int *sbt_entries, const BNCHPolygon& p, int type);
      void                       buildSBT(int *entries, NCHfloat *sbt, sb_tree *sbtree);
      void                       addEdgeToAET(edge_node **aet, edge_node *edge, edge_node *prev);
      void                       addIntersection(it_node **it, edge_node *edge0, edge_node *edge1, NCHfloat x, NCHfloat z);
      void                       addSTEdge(st_node **st, it_node **it, edge_node *edge, NCHfloat dz);
      void                       buildIntersectionTable(it_node **it, edge_node *aet, NCHfloat dz);
      int                        countHulls(polygon_node *polygon);
      void                       addLeft(polygon_node *p, NCHfloat x, NCHfloat z);
      void                       mergeLeft(polygon_node *p, polygon_node *q, polygon_node *list);
      void                       addRight(polygon_node *p, NCHfloat x, NCHfloat z);
      void                       mergeRight(polygon_node *p, polygon_node *q, polygon_node *list);
      void                       addLocalMin(polygon_node **p, edge_node *edge, NCHfloat x, NCHfloat z);
      void                       addVertex(vertex_node **t, NCHfloat x, NCHfloat z);

      enum
      {
         cMaxENodes = 2000,
         cMaxITNodes = 2000,
         cMaxSTNodes = 2000,
         cMaxLNodes = 2000,
         cMaxSBNodes = 2000,
         cMaxVNodes = 2000,
         cMaxPNodes = 2000,
      };
      
      edge_node                  mENodes[cMaxENodes];
      long                       mOnENode;

      it_node                    mITNodes[cMaxITNodes];
      long                       mOnITNode;

      st_node                    mSTNodes[cMaxSTNodes];
      long                       mOnSTNode;

      lmt_node                   mLNodes[cMaxLNodes];
      long                       mOnLNode;

      sb_tree                    mSBNodes[cMaxSBNodes];
      long                       mOnSBTree;

      vertex_node                mVNodes[cMaxVNodes];
      long                       mOnVNode;

      polygon_node               mPNodes[cMaxPNodes];
      long                       mOnPNode;

      BDynamicArray<NCHfloat>    mSBTBuffer;
      NCHfloat                   mLastZ;
};


//#define NCH_DEBUG
#ifdef NCH_DEBUG
class BNonconvexHullOld;
#endif

#ifdef SPLIT_DATA
class BNonconvexHull_RarelyAccessedData {

	BNonconvexHull_RarelyAccessedData(){}
	~BNonconvexHull_RarelyAccessedData(){}
	friend class BNonconvexHull;

	  bool mbDynAllocPolygon;
      bool mIsConvex;

      BNCHPolygon* mPolygon;
      
      BVector mBoundingMin;
      BVector mBoundingMax;

      //
      float mfInitTime;
};
#endif

//==============================================================================
class BNonconvexHull
{
public:

      // Constructors
      BNonconvexHull( void );
      BNonconvexHull(const BNonconvexHull &h);

      // Destructors
      ~BNonconvexHull( void );

      // Functions
      BNonconvexHull            &operator =(const BNonconvexHull &h);

      bool                       initialize(BDynamicSimArray<BOPObstructionNode*> &obs, BObstructionManager *obManager,
                                    BOPObstructionNode *startOb = NULL, long lStartSegmentIdx = -1L, const BVector *pvStartPoint = NULL);
      bool                       initialize(BGrannyModel *model);

      bool                       initializeConvex(const BNCHVector *points, const long num);
      bool                       initializeConvex(const BVector *points, const long num);
      bool                       initializeQuadHull(const BOPQuadHull *quadhull);
      bool                       addPointsConvex(const BNCHVector *points, const long num);
      bool                       addTris(long numTris, BYTE *indexArray, long indexSize, BYTE *vertexArray, long vertexSize);
      bool                       initializeNonConvex(const BVector *points, const long num);
      bool                       initializeNonConvex(const BNCHVector *points, const long num);
      bool                       initializeFromSubhull(const BNonconvexHull &masterHull, long nHullNum);

      void                       reset();

      static void                resetNonCached();

      const long                 getPointCount(long hull) const;
      const BVector              &getPoint(long index, long hull) const;
      const BDynamicArray<BNCHVector>
                                 &getPoints(long hull) const;
      const bool                 isVertex(const BVector &point, long *segIndex = NULL, long *hullIndex = NULL) const;

      const long                 getHullCount() const;

      float                      getInitTime();

      void                       addDebugLines(int category, DWORD color, float *height = NULL) const;
      void                       debugAddHull(int category, const BOPQuadHull *pHull, DWORD color);

      BVector                    findClosestPointOnHull(const long hull, const BVector &vStart, long *plSegmentIndex = NULL, float *pfClosestDistSqr = NULL);
      BVector                    findClosestPointOnHull(const BVector &vStart, long *plSegmentIndex = NULL, long *plHullIndex = NULL, float *pfClosestDistSqr = NULL);
      
      // Should be used only by HC pather now.
      const bool                 segmentIntersects(const BVector &point1, const BVector &point2, const long ignorePoint,
                                    BVector &iPoint, long &segmentIndex, long &hullIndex, float &distanceSqr, 
                                    long &lNumIndersections, bool checkBBox, bool bCheckInside = true, float ignoreDistSqr=0.0f, float errorEpsilon=0.01f, bool ignoreHoles = true) const;

      // This version is faster than above.
      const bool                 segmentIntersectsFast(const BVector &point1, const BVector &point2, const long ignorePoint,
                                    BVector &iPoint, long &segmentIndex, long &hullIndex, float &distanceSqr, 
                                    bool checkBBox, bool bCheckInside = true, float errorEpsilon=0.01f, bool ignoreHoles = true) const;
      const bool                 segmentIntersectsFast_3(const BVector &point1, const BVector &point2, const long ignoreHull, const long ignorePoint,
                                    BVector &iPoint, long &segmentIndex, long &hullIndex, float &distanceSqr,
                                    bool checkBBox, bool bCheckInside = true, float errorEpsilon=0.01f, bool ignoreHoles=true) const;


      BDynamicSimArray<BOPObstructionNode*> &getObstructions();
      const BOPObstructionNode*          getObstruction(long lIndex) const;

      const long                 getObstructionCount();

      const bool                 inside(const BVector &point, long &hull) const;
      const bool                 insideConvexHull(const BVector &point, long &hull) const;
      const bool                 getHole(const long h) const;
      const bool                 insideHole(const BVector &point, long &ihull) const;
      void                       convertHoleToHull(const long hull);
      void                       convertHullToConvex(long hull);

                                 // Return true if the passed in hull is inside of me
      const bool                 isInside(const BNonconvexHull &hull) const;

      const bool                 overlapsHull(const BNonconvexHull &hull) const;
      const bool                 overlapsBox(const float minX, const float minZ, const float maxX, const float maxZ) const;
      const bool                 segmentIntersects(const BVector &point1, const BVector &point2, float errorEpsilon=0.01f, bool ignoreHoles = true) const;

      const bool                 isBuiltConvex(void) const;
      void                       computePerpToSegment(long segIndex, float &x, float &z, long h);
      void                       computePerpAtPoint(long segIndex, float &x, float &z, long h);

      const BVector&             getBoundingMin() const { return(mBoundingMin); }
      const BVector&             getBoundingMax() const { return(mBoundingMax); }

      static void                dumpStats(void);
      static void                startup(void);
      static void                clearCache(void);
      static void                shutdown(void);

      static void                polygonMergeCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
      static void* _cdecl        buildNCHWorker(void* pVal);

private:
      void                       computeBoundingBox(void);
      bool                       checkConvexSide(const long h, const float dx, const float dz, const BNCHVector &v) const;
      void                       localBuildNCH(BDynamicSimArray<BOPObstructionNode*> &obs, long startIndex, long endIndex, BNCHPolygon &result);
      void                       recursiveBuildNCH(BDynamicSimArray<BOPObstructionNode*> &obs, long startIndex, long endIndex, BNCHPolygon &result);

      static inline void         copyTriIntoPolygon(BNCHPolygon &poly, BYTE *indexArray, long indexSize, BYTE *vertexArray, long vertexSize);
      static void                shellsortNCH(BNCHVector *a, long num);
      static void                shellsortNCH2(BNCHVector *a, long num);
      static bool                leftTurnNCH(const BNCHVector &v1, const BNCHVector &v2, const BNCHVector &v3);
      static bool                rightTurnNCH(const BNCHVector &v1, const BNCHVector &v2, const BNCHVector &v3);
      static long                leftScanHullNCH(BNCHVector *points, long n);
      static long                rightScanHullNCH(BNCHVector *points, long n);


      long                       oldestPolygon() const;
      long                       getNoncachePolygon() const;

      //static BThread                      mWorkerThread;
      //static BWin32Semaphore              mWorkerSemaphore;
      //static BCountDownEvent              mWorkerCountdownEvent;

#ifndef SPLIT_DATA
      bool                                mbDynAllocPolygon;
      bool                                mIsConvex;

      BNCHPolygon                         *mPolygon;
      BDynamicSimArray<BOPObstructionNode*>   mObstructionList;

      BVector                             mBoundingMin;
      BVector                             mBoundingMax;

      //
      float                               mfInitTime;

#else
	  SPLIT_FIELD(bool,mbDynAllocPolygon);
      SPLIT_FIELD(bool,mIsConvex);

      SPLIT_FIELD(BNCHPolygon*,mPolygon);
      BDynamicSimArray<BOPObstructionNode*>   mObstructionList;

      SPLIT_FIELD(BVector,mBoundingMin);
      SPLIT_FIELD(BVector,mBoundingMax);

      //
      SPLIT_FIELD(float,mfInitTime);

	  BNonconvexHull_RarelyAccessedData* m_pRarelyAccessedData;
#endif
      // stats
      static long                         moverlapsHull;
      static long                         misInside;
      static long                         mInsideConvexHull;
      static long                         minside;
      static long                         msegmentIntersects;
      static long                         mfindclosestpointonhull;
      static long                         minitialize;

#ifdef NCH_DEBUG
      BNonconvexHullOld                   *debugHull;
#endif
}; // BNonconvexHull

typedef BDynamicSimArray<BNonconvexHull, ALIGN_OF(BNonconvexHull), BDynamicArrayDefaultConstructorOptions>   BNonconvexArray;
typedef BDynamicSimArray<BNonconvexHull*> BSimpleNonconvexPtrArray;



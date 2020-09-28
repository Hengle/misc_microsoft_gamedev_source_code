//-------------------------------------------------------------------------------------------------   
// File: aabb_tree_builder.h
//-------------------------------------------------------------------------------------------------   
#pragma once

#include <map>
#include <set>

#include "aabbTree.h"

#define AABB_TREE_BUILDER_MESSAGES 1
#define AABB_TREE_BUILDER_DEBUG 1
#define DEBUG_INTERVAL_TRACKER 1
      
//-------------------------------------------------------------------------------------------------   
// template<typename T> int BAABBTreeBuilderGetObjIndex
//-------------------------------------------------------------------------------------------------   
template<typename T> 
class BAABBTreeBuilderGetObjIndex
{
public:
   static int get(const T& obj) 
   { 
      return obj.index(); 
   }
};

//-------------------------------------------------------------------------------------------------   
// template<typename T> int BAABBTreeBuilderGetNullIndex
//-------------------------------------------------------------------------------------------------      
template<typename T>
class BAABBTreeBuilderGetNullIndex
{
public:
   static int get(const T& obj) 
   { 
      return 0; 
   }
};
   
//-------------------------------------------------------------------------------------------------   
// template<typename ObjCont> class BAABBTreeBuilder
// ObjCont = object container
// Object should implement bounds() and index()
// Container should look like a BDynamicArray (random access)
// GetObjIndexPolicy should be BAABBTreeBuilderGetObjIndex or BAABBTreeBuilderGetNullIndex (yes this is EVIL)
//-------------------------------------------------------------------------------------------------
template<typename ObjCont, typename GetObjIndexPolicy = BAABBTreeBuilderGetObjIndex<ObjCont::valueType> >
class BAABBTreeBuilder
{  
public:
   typedef typename ObjCont::valueType    ObjType;
   typedef ObjCont                        ObjContType;

   //-------------------------------------------------------------------------------------------------      
   // struct BParams
   //-------------------------------------------------------------------------------------------------      
   struct BParams
   {
      // largest axis 
      float axisSizeWeight;

      // spatial balance 
      float spatialBalanceWeight;

      // face balance 
      float faceBalanceWeight;

      // num faces split 
      float facesSplitWeight;

      // volume overlap 
      float boundsOverlapWeight;

      // how close the total # of unique indices matches the total # of unique indices
      // minimizes the total number of unique sections created during a split
      float uniqueIndicesWeight;

      // weight for small groups of objects with the same index (material)
      //float smallGroupWeight;

      // nodes with dimensions larger than this will be split                       
      float maxNodeDim;

      // min # of objects for a node to be splittable
      int minNodeObjects;

      // if node exceeds this # of objects, it will always be split even if it's small
      int maxNodeObjects;

      // objects with the same index that are in groups less than smallObjIndexGroupThresh will be 
      // segmented into a different part of the tree
      int smallObjIndexGroupThresh;

      // controls the min # of objects penalty
      int minSplitObjects;

      // penalize the creation of tiny nodes         
      float minSplitObjectWeight;

      // if there are less than this # of objs in a node, it may be combined with another node in the same index group in a postprocess
      int reassignThresh;

      // maximum surface area expansion ratio allowed when reassinging a node to another during the tree optimization phase
      float maxNodeExpansionRatio;
      
      // if true, optimize the tree by moving tiny groups of objects using the same index (i.e. minimize small rendering sections)
      bool optimizeForRendering;
      
      // maximum dimension of an expanded node (only used if optimizeForRendering is true)
      float maxExpandedNodeDim;
      
      int maxMergeNodeObjects;
      
      bool splitLargestAxis;
      
      bool splitAxisHints;
      
      bool buildBSP;
      
      float BSPSpatialCutObjThresh;
      float BSPMaxFacesSplitThresh;
      float BSPScoreThresh;
               
      BParams() :
         axisSizeWeight(7.0f),
         spatialBalanceWeight(3.0f),
         faceBalanceWeight(.5f),
         facesSplitWeight(1.0f),
         boundsOverlapWeight(5.0f),
         uniqueIndicesWeight(1.0f),
         minSplitObjectWeight(5.0f),
         reassignThresh(200),       // if there are less than this # of objs in a node, it may be combined with another node in the same index group in a postprocess
         maxNodeExpansionRatio(1.5f),
         optimizeForRendering(true),
         splitLargestAxis(false),
         splitAxisHints(false),
         buildBSP(false),
         
         BSPSpatialCutObjThresh(.33f),
         BSPMaxFacesSplitThresh(.99f),
         BSPScoreThresh(.98f),
         
#if 0
         maxNodeDim(200.0f),        // nodes with dimensions larger than this will be split     
         minNodeObjects(1500),      // min # of objects for a node to be splittable
         maxNodeObjects(2000),      // if node exceeds this # of objects, it will always be split even if it's small
         smallObjIndexGroupThresh(500), // tris in index groups smaller than this will be forced to a separate left tree
         minSplitObjects(100),     // controls the min # of objects penalty
         maxMergeNodeObjects(5000),
         maxExpandedNodeDim(250.0f)
#elif 1            
         maxNodeDim(200.0f),        // nodes with dimensions larger than this will be split     
         minNodeObjects(2500),      // min # of objects for a node to be splittable
         maxNodeObjects(4000),      // if node exceeds this # of objects, it will always be split even if it's small
         smallObjIndexGroupThresh(1250), // tris in index groups smaller than this will be forced to a separate left tree
         minSplitObjects(100),     // controls the min # of objects penalty
         maxMergeNodeObjects(5000),
         maxExpandedNodeDim(250.0f)
#else
         maxNodeDim(20000000.0f),        // nodes with dimensions larger than this will be split     
         minNodeObjects(1500000),      // min # of objects for a node to be splittable
         maxNodeObjects(2000),      // if node exceeds this # of objects, it will always be split even if it's small
         smallObjIndexGroupThresh(500), // tris in index groups smaller than this will be forced to a separate left tree
         minSplitObjects(100),    // controls the min # of objects penalty
         maxMergeNodeObjects(5000),
         maxExpandedNodeDim(250.0f)
#endif            
      {
      }

      void dump(void) const
      {
#if AABB_TREE_BUILDER_MESSAGES         
         trace("BAABBTreeBuilder::BParams::dump:");
         trace("  axisSizeWeight: %f", axisSizeWeight);
         trace("  spatialBalanceWeight: %f", spatialBalanceWeight);
         trace("  faceBalanceWeight: %f", faceBalanceWeight);
         trace("  facesSplitWeight: %f", facesSplitWeight);
         trace("  boundsOverlapWeight: %f", boundsOverlapWeight);
         trace("  uniqueIndicesWeight: %f", uniqueIndicesWeight);
         trace("  maxNodeDim: %f", maxNodeDim);
         trace("  minNodeObjects: %i", minNodeObjects);
         trace("  maxNodeObjects: %i", maxNodeObjects);
         trace("  smallObjIndexGroupThresh: %i", smallObjIndexGroupThresh);
         trace("  minSplitObjects: %i", minSplitObjects);
         trace("  minSplitObjectWeight: %f", minSplitObjectWeight);
         trace("  optimizeForRendering: %i", optimizeForRendering);
         trace("  splitLargestAxis: %i", splitLargestAxis);
         trace("  splitAxisHints: %i", splitAxisHints);
         trace("  buildBSP: %i", buildBSP);
         trace("  BSPSpatialCutObjThresh: %f", BSPSpatialCutObjThresh);
         trace("  BSPMaxFacesSplitThresh: %f", BSPMaxFacesSplitThresh);
         trace("  BSPScoreThresh: %f", BSPScoreThresh);
#endif            
      }
   };

   //-------------------------------------------------------------------------------------------------      
   // BAABBTreeBuilder::BAABBTreeBuilder
   //-------------------------------------------------------------------------------------------------      
   BAABBTreeBuilder(
      const ObjCont& objects, 
      int numObjIndices = 1,
      bool fast = false,
      const BParams& params = BParams()) :
      mObjects(objects),
      mTotalObjIndices(numObjIndices),
      mParams(params),
      mFast(fast),
      mNumLeafTris(0)
   {
      mTree.prepareForBuild();
      
      const int numObjects = static_cast<int>(objects.size());
      
      mLeftIndices.reserve(numObjects);
      mRightIndices.reserve(numObjects);
      mBestLeftIndices.reserve(numObjects);
      mBestRightIndices.reserve(numObjects);
      
      mParams.dump();

      clearStats();

      uint startTime = GetTickCount();         
      startTime;      
      
      build();
                  
#if AABB_TREE_BUILDER_MESSAGES         
      uint endTime = GetTickCount();
      trace("BAABBTreeBuilder::BAABBTreeBuilder: Tree construction time: %f secs", (endTime - startTime) / 1000.0f);
#endif         
   }

   //-------------------------------------------------------------------------------------------------            
   // BAABBTreeBuilder::~BAABBTreeBuilder
   //-------------------------------------------------------------------------------------------------      
   ~BAABBTreeBuilder()
   {
   }

   //-------------------------------------------------------------------------------------------------      
   // BAABBTreeBuilder::tree
   //-------------------------------------------------------------------------------------------------      
   const BAABBTree& tree(void) const   { return mTree; }
         BAABBTree& tree(void)         { return mTree; }

private:
   enum { MaxSpatialCuts = 5 };
   
   //-------------------------------------------------------------------------------------------------      
   // member vars
   //-------------------------------------------------------------------------------------------------      
   const ObjCont&       mObjects;
   int                  mTotalObjIndices;
   const BParams&       mParams;
   bool                 mFast;

   BAABBTree            mTree;

   IntVec               mObjIndexHist;

   BDynamicArray<BAABBTree::BNode*> mNodeParents;

   int                  mLeafNodeMinObjects;
   int                  mLeafNodeMaxObjects;
   int                  mLeafNodeAveObjects;
   float                mLeafNodeMinDim;
   float                mLeafNodeMaxDim;
   float                mLeafNodeAveDim;
   float                mLeafNodeMinVol;
   float                mLeafNodeMaxVol;
   float                mLeafNodeAveVol;
   float                mLeafNodeMinSurfArea;
   float                mLeafNodeMaxSurfArea;
   float                mLeafNodeAveSurfArea;
   int                  mNumNodesTraversed;
   int                  mNumLeafNodesTraversed;
   int                  mNumEmptyLeafNodes;
   float                mMaxNodeOverlap;
   float                mAveNodeOverlap;
   int                  mNumNodesOverlap;
   
   std::set<const BAABBTree::BNode*> mSeenNodes;
   BDynamicArray<unsigned char> mObjCount;

   //-------------------------------------------------------------------------------------------------      
   // clearStats
   //-------------------------------------------------------------------------------------------------      
   void clearStats(void)
   {
      mSeenNodes.clear();
      mObjCount.clear();

      mLeafNodeMinObjects  = INT_MAX;
      mLeafNodeMaxObjects  = INT_MIN;
      mLeafNodeAveObjects  = 0;
      mLeafNodeMinDim      = +Math::fNearlyInfinite;
      mLeafNodeMaxDim      = -Math::fNearlyInfinite;
      mLeafNodeAveDim      = 0.0f;
      mLeafNodeMinVol      = +Math::fNearlyInfinite;
      mLeafNodeMaxVol      = -Math::fNearlyInfinite;
      mLeafNodeAveVol      = 0.0f;
      mLeafNodeMinSurfArea = +Math::fNearlyInfinite;
      mLeafNodeMaxSurfArea = -Math::fNearlyInfinite;
      mLeafNodeAveSurfArea = 0;
      mNumNodesTraversed   = 0;
      mNumLeafNodesTraversed = 0;
      mNumEmptyLeafNodes   = 0;
      mMaxNodeOverlap      = -Math::fNearlyInfinite;
      mAveNodeOverlap      = 0;
      mNumNodesOverlap     = 0;
   }

   //-------------------------------------------------------------------------------------------------      
   // getObj
   //-------------------------------------------------------------------------------------------------      
   const ObjType& getObj(int i) 
   { 
      return mObjects[debugRangeCheck(i, mObjects.size())];
   }
   
   //-------------------------------------------------------------------------------------------------      
   // getObj
   //-------------------------------------------------------------------------------------------------      
   int getObjIndex(const ObjType& o) const
   { 
      return GetObjIndexPolicy::get(o);
   }

   //-------------------------------------------------------------------------------------------------      
   // updateNodeBounds
   //-------------------------------------------------------------------------------------------------
   void updateNodeBounds(BAABBTree::BNode& node) 
   {
      //BASSERT(node.numObjects());

      node.bounds().initExpand();

      for (int objIter = 0; objIter < node.numObjects(); objIter++)
         node.bounds().expand(getObj(node.objIndex(objIter)).bounds());
   }

   //-------------------------------------------------------------------------------------------------      
   // checkNode
   //-------------------------------------------------------------------------------------------------
   void checkNode(BAABBTree::BNode* pNode)
   {
      pNode->bounds().debugCheck();

      for (int objIndexIter = 0; objIndexIter < (int)pNode->objIndices().size(); objIndexIter++)
      {
         const int objIndex = pNode->objIndex(objIndexIter);
         const ObjType& obj = getObj(objIndex);

         const AABB objBounds(obj.bounds());
         const BVec3 objCenter(obj.centroid());

         objBounds.debugCheck();
         if (mParams.buildBSP)
         {
            BASSERT(pNode->bounds().overlaps(objBounds));
         }
         else
         {
            BASSERT(objBounds.contains(objCenter));
            BASSERT(pNode->bounds().contains(objBounds));
            BASSERT(pNode->bounds().contains(objCenter));
         }
      }
   }

   //-------------------------------------------------------------------------------------------------
   // sub-class BSplitEvent
   //-------------------------------------------------------------------------------------------------      
   class BSplitEvent
   {
   public:
      enum  EEventType
      { 
         eEvalEnterEventType = -2,
         eEnterEventType = -1, 
         eMiddleEventType = 0,
         eExitEventType  = 1,
         eEvalExitEventType = 2
      };

      BSplitEvent() { }

      BSplitEvent(float coord, int type, int objIndex) :
         mCoord(coord), mType(type), mObjIndex(objIndex)
      {
      }

      friend bool operator== (const BSplitEvent& a, const BSplitEvent& b)         
      {
         return (a.mCoord == b.mCoord) && (a.mType == b.mType) && (a.mObjIndex == b.mObjIndex);
      }

      friend bool operator< (const BSplitEvent& a, const BSplitEvent& b)
      {
         // in case of FP precision problems
         if (a.mObjIndex == b.mObjIndex)
         {
            if (a.mType < b.mType)
               return true;
         }
         else
         {
            if (a.mCoord < b.mCoord)
               return true;
            else if (a.mCoord == b.mCoord)
            {
               if (a.mType < b.mType)
                  return true;
               else if (a.mType == b.mType)
               {
                  if (a.mObjIndex < b.mObjIndex)
                     return true;
               }
            }
         }          

         return false;
      }

      float coord(void) const { return mCoord; }
      float& coord(void)      { return mCoord; }

      int type(void) const { return mType; }
      int& type(void)      { return mType; }

      int objIndex(void) const   { return mObjIndex; }
      int& objIndex(void)        { return mObjIndex; }

   private:
      float mCoord;         
      int mType;     
      int mObjIndex;
   };

   typedef BDynamicArray<BSplitEvent> BSplitEventVector;

   //-------------------------------------------------------------------------------------------------            
   // sub-class BUniqueIndexTracker
   //-------------------------------------------------------------------------------------------------      
   class BUniqueIndexTracker
   {
   public:
      BUniqueIndexTracker() : mNumUniqueIndices(0)
      {
      }

      BUniqueIndexTracker(int numIndices) :
         mIndexHist(numIndices),
         mNumUniqueIndices(0)
      {
      }

      void resize(int numIndices)
      {
         mIndexHist.resize(numIndices);
      }

      int numIndices(void) const
      {
         return static_cast<int>(mIndexHist.size());
      }

      void addIndex(int index)
      {
         debugRangeCheck(index, static_cast<int>(mIndexHist.size()));
         if (0 == mIndexHist[index])
            mNumUniqueIndices++;

         mIndexHist[index]++;               
      }       

      void removeIndex(int index)
      {
         debugRangeCheck(index, static_cast<int>(mIndexHist.size()));

         mIndexHist[index]--;
         BASSERT(mIndexHist[index] >= 0);

         if (0 == mIndexHist[index])
         {
            mNumUniqueIndices--;
            BASSERT(mNumUniqueIndices >= 0);
         }
      }

      int numUniqueIndices(void) const { return mNumUniqueIndices; }

      int indexFreq(int i) const { return mIndexHist[debugRangeCheck(i, static_cast<int>(mIndexHist.size()))]; }
      void setIndexFreq(int i, int f) { mIndexHist[debugRangeCheck(i, static_cast<int>(mIndexHist.size()))] = f; }

   private:
      IntVec mIndexHist;
      int mNumUniqueIndices;
   };

   typedef BDynamicArray<BUniqueIndexTracker> BUniqueIndexTrackerVec;
   BUniqueIndexTrackerVec mLeafUniqueIndicesVec;

   //-------------------------------------------------------------------------------------------------      
   // sub-class BSplit                  
   //-------------------------------------------------------------------------------------------------      
   class BSplit
   {
   public:
      BSplit() : 
         mCoord(0), 
         mScore(Math::fNearlyInfinite),
         mDimension(cInvalidIndex),
         mSplitScore(0.0f),
         mSpatialPercent(0.0f)
      {
      }

      bool valid(void) const
      {
         return cInvalidIndex != mDimension;
      }

      friend bool operator< (const BSplit& a, const BSplit& b)
      {
         // HIGHER scores are worse (i.e. less)
         return a.mScore > b.mScore;
      }

      BSplit& operator= (const BSplit& b)
      {
         mCoord = b.mCoord;
         mScore = b.mScore;
         mDimension = b.mDimension;
         mSplitScore = b.mSplitScore;
         mSpatialPercent = b.mSpatialPercent;
         return *this;
      }

      float coord(void) const { return mCoord; }
      float score(void) const { return mScore; }
      int dimension(void) const { return mDimension; }
                  
      float splitScore(void) const { return mSplitScore; }
      float spatialPercent(void) const { return mSpatialPercent; }
      
      // returns score
      float set(
         const BParams& params,
         const AABB& bounds, 
         int dimension,
         float coord, 
         const BInterval& onInterval,
         int numLeft, 
         int numOn, 
         int numRight,
         // FIXME: "Actual" is a bad name, it's actually the # of objects that have their 
         // centers to the left or right of the split plane.
         int numActualLeft,
         int numActualRight,
         int totalUniqueIndices,
         const BUniqueIndexTracker& uniqueLeftIndices,
         const BUniqueIndexTracker& uniqueRightIndices,
         float cph)
      {
         const int majorDimension = bounds.majorDimension();
         
         const int numObjects = numLeft + numRight + numOn;
         BASSERT(numObjects == numActualLeft + numActualRight);
         
         // fraction along the split dimension [0,1]
         const float spatialPercent = 
            (coord - bounds[0][dimension]) / bounds.dimension(dimension);
         debugRangeCheck(spatialPercent, 0.0f, 1.0f);
         
         // split score, 0 = best
         const float splitScore = numOn / float(numObjects);
         BASSERT(splitScore >= 0.0f);
         BASSERT(splitScore <= 1.0f);
         
         // axis score, 0 = best
         const float axisScore = 
            Math::Clamp(1.0f - (bounds.dimension(dimension) / bounds.dimension(majorDimension)), 0.0f, 1.0f);

         mSplitScore = splitScore;
         mSpatialPercent = spatialPercent;
         
         if (params.buildBSP)
         {
            const float ConstantNodeCost = 1.0f;
            mScore = numOn +
               ((numLeft + numRight) * cph) +
               (numLeft * spatialPercent * (1.0f - cph)) +
               (numRight * (1.0f - spatialPercent) * (1.0f - cph));
            ConstantNodeCost;
            BVERIFY(mScore >= -.125f);
            BVERIFY(mScore <= numObjects + .125f);
            //mScore += ConstantNodeCost + axisScore * 750.0f;
         }
         else
         {
            const int numUniqueLeftIndices = uniqueLeftIndices.numUniqueIndices();
            const int numUniqueRightIndices = uniqueRightIndices.numUniqueIndices();
         
            // spatial balance score, 0 = best
            const float volumeScore = 2.0f * fabs(0.5f - spatialPercent);

            // face balance score, 0 = best
            const float faceScore = fabs(float(numActualLeft - numActualRight)) / float(numObjects);
            debugRangeCheck(faceScore, 0.0f, 1.0f);
            
            // volume overlap score, 0 = best
            const float overlapScore =  
               onInterval.valid() ? (onInterval.dimension(0) / bounds.dimension(dimension)) : 0.0f;
            BASSERT(overlapScore >= 0.0f);

            // score for the total # of sections that will be created (attempt to minimize the total # of sections)
            BASSERT(totalUniqueIndices > 0);
            const float uniqueIndicesScore = 
               ((numUniqueLeftIndices + numUniqueRightIndices) / float(totalUniqueIndices)) - 1.0f;
            BASSERT(uniqueIndicesScore >= 0.0f);

            //--- try to penalize the creation of tiny nodes

            float minSplitObjectScore = 0.0f;
            if (numObjects > params.minSplitObjects * 2)
            {
               if (numActualLeft < params.minSplitObjects)
                  minSplitObjectScore += 1.0f - numActualLeft / float(params.minSplitObjects);

               if (numActualRight < params.minSplitObjects)
                  minSplitObjectScore += 1.0f - numActualRight / float(params.minSplitObjects);
            }               

            mScore =
               axisScore            * params.axisSizeWeight    +
               volumeScore          * params.spatialBalanceWeight  +
               faceScore            * params.faceBalanceWeight    +
               splitScore           * params.facesSplitWeight   +
               overlapScore         * params.boundsOverlapWeight +
               uniqueIndicesScore   * params.uniqueIndicesWeight +
               minSplitObjectScore  * params.minSplitObjectWeight;
         }                     

         mCoord = coord;               
         mDimension = dimension;

         return mScore;               
      }

   private:
      float mCoord;
      float mScore;
      int mDimension;
      
      float mSplitScore;
      float mSpatialPercent;
   };

   //-------------------------------------------------------------------------------------------------       
   // splitNode
   //-------------------------------------------------------------------------------------------------      
   void splitNode(
      const BSplit& bestSplit,
      BAABBTree::BNode* pNode, 
      BAABBTree::BNode* pLeft, 
      BAABBTree::BNode* pRight,
      const IntVec& leftIndices,
      const IntVec& rightIndices,
      const AABB& leftBSPBounds,
      const AABB& rightBSPBounds)
   {
      if (mParams.buildBSP)
      {
         pLeft->bounds() = leftBSPBounds;
         pRight->bounds() = rightBSPBounds;
      }
      else
      {
         BVERIFY(!leftIndices.empty());
         BVERIFY(!rightIndices.empty());
      }
      
      for (uint i = 0; i < leftIndices.size(); i++)
      {
         const int objIndex = leftIndices[i];
         const ObjType& obj = getObj(objIndex);
         const AABB objBounds(obj.bounds());
         
         pLeft->objIndices().push_back(objIndex);
         if (!mParams.buildBSP)
         {
            pLeft->bounds().expand(objBounds);
            BASSERT(pLeft->bounds().contains(objBounds));   
         }
      }
      
      for (uint i = 0; i < rightIndices.size(); i++)
      {
         const int objIndex = rightIndices[i];
         const ObjType& obj = getObj(objIndex);
         const AABB objBounds(obj.bounds());

         pRight->objIndices().push_back(objIndex);
         if (!mParams.buildBSP)
         {
            pRight->bounds().expand(objBounds);
            BASSERT(pRight->bounds().contains(objBounds));   
         }
      }
      
      if (mParams.buildBSP)
      {
         BVERIFY(pLeft->numObjects() + pRight->numObjects() >= pNode->numObjects());
      }
      else
      {
         BVERIFY(pLeft->numObjects() + pRight->numObjects() == pNode->numObjects());
         BVERIFY(pLeft->numObjects() >= 1);
         BVERIFY(pRight->numObjects() >= 1);
      }
      
      checkNode(pLeft);
      checkNode(pRight);

      pLeft->parent() = pNode;
      pRight->parent() = pNode;

      pNode->child(0) = pLeft;
      pNode->child(1) = pRight;
      pNode->objIndices().clear();
      
      if (mParams.splitAxisHints)
      {
         pNode->index() = bestSplit.dimension();
         pNode->splitPlane() = bestSplit.coord();
      }
   }

   //-------------------------------------------------------------------------------------------------             
   // sub-class BIntervalTracker
   // Container for 1D intervals -- efficiently tracks the extents of the union of all intervals.
   //-------------------------------------------------------------------------------------------------       
   class BIntervalTracker
   {
   public:
      BIntervalTracker() : 
         mInterval(BInterval::eInitExpand)
      {
      }

      void insert(const BInterval& i)
      {
         mInterval.expand(i);

         insertFloat(i[0][0]);
         insertFloat(i[1][0]);
      }

      void remove(const BInterval& i)
      {
         removeFloat(i[0][0]);
         removeFloat(i[1][0]);

         if ((i[0][0] == mInterval[0][0]) || (i[1][0] == mInterval[1][0]))
         {
            FloatMap::const_iterator begin(mFloatMap.begin());
            if (begin == mFloatMap.end())
            {
               mInterval.initExpand();
            }
            else
            {
               FloatMap::const_iterator first(mFloatMap.lower_bound(-Math::fNearlyInfinite));

               BASSERT(first == begin);

               FloatMap::const_iterator end(mFloatMap.end());

               mInterval[0][0] = begin->first;

               mInterval[1][0] = (--end)->first;

               BASSERT(mInterval.valid());
               mInterval.debugCheck();               
            }                  
         }
      }

      const BInterval& interval(void) const { return mInterval; }

   private:
      typedef std::map<float, int> FloatMap;
      FloatMap mFloatMap;

      BInterval mInterval;

      void insertFloat(float f)
      {
         std::pair<FloatMap::iterator, bool> res;
         res = mFloatMap.insert(std::make_pair(f, 1));
         if (!res.second)
            (*res.first).second = (*res.first).second + 1;
      }               

      void removeFloat(float f)
      {
         FloatMap::iterator res = mFloatMap.find(f);
         BASSERT(res != mFloatMap.end());

         res->second = res->second - 1;
         BASSERT(res->second >= 0);

         if (0 == res->second)
            mFloatMap.erase(res);
      }               
   };

   //-------------------------------------------------------------------------------------------------                         
   // findBestSplit
   //-------------------------------------------------------------------------------------------------             
   BSplit findBestSplit(BAABBTree::BNode* pNode, int dimension, IntVec& leftIndices, IntVec& rightIndices, float spatialCutCount, const BSplit& splitToBeat)
   {
      BSplit bestSplit;
      BSplitEventVector events;
      events.reserve((pNode->numObjects() + 16) * 5);

      BUniqueIndexTracker leftUniqueIndices(mTotalObjIndices);
      BUniqueIndexTracker rightUniqueIndices(mTotalObjIndices);

      const float bspLowLimit = pNode->bounds()[0][dimension];
      const float bspHighLimit = pNode->bounds()[1][dimension];
      
      // See: http://www.acm.org/tog/resources/RTNews/html/rtnv17n1.html#art8
      const int axis0 = Math::NextWrap(dimension, 3);
      const int axis1 = Math::NextWrap(axis0, 3);
      
      const float v = (pNode->bounds().dimension(0) * pNode->bounds().dimension(1) + 
         pNode->bounds().dimension(0) * pNode->bounds().dimension(2) + 
         pNode->bounds().dimension(1) * pNode->bounds().dimension(2));
      
      const float p = (pNode->bounds().dimension(axis0) * pNode->bounds().dimension(axis1));
      
      if (v == 0.0f)
         return bestSplit;
                  
      // cph = Chance of Plane Hit
      const float cph = (v == 0.0f) ? 1.0f : p / v;
      BVERIFY(cph >= 0.0f && cph <= 1.0f);             
               
#if AABB_TREE_BUILDER_DEBUG                  
      trace("  Trying Dimension: %i, cph: %f (%f / %f), %f %f %f", dimension, cph, p, v, pNode->bounds().dimension(0), pNode->bounds().dimension(1), pNode->bounds().dimension(2));
#endif
          
      const AABB splitScanBounds(pNode->bounds());
      
      int numLeft = 0;
      int numRight = 0;//pNode->numObjects();
      int numActualLeft = 0;
      int numActualRight = 0;//pNode->numObjects();
          
      // --- Create a sorted array of interval begin/middle/end events
      for (uint objIndexIter = 0; objIndexIter < pNode->objIndices().size(); objIndexIter++)
      {
         const int objIndex = pNode->objIndex(objIndexIter); 
         const ObjType& obj = getObj(objIndex);

         const AABB objBounds(obj.bounds());
        
         // Expand the interval a tiny bit to avoid having the split plane straddle an object!
         const float BoundsEps = .000125f * .5f;
         float s = objBounds[0][dimension];
         float e = objBounds[1][dimension];
         
         if (mParams.buildBSP)
         {
            if (e < bspLowLimit)
            {
               numLeft++;
               numActualLeft++;
               continue;
            }
            else if (s > bspHighLimit)
            {
               numRight++;
               numActualRight++;
               continue;
            }
         }
         
         numRight++;
         numActualRight++;
                                 
         s = Math::Max(s, splitScanBounds[0][dimension]);
         e = Math::Min(e, splitScanBounds[1][dimension]);
         const float es = Math::Max(s - BoundsEps, splitScanBounds[0][dimension]);
         const float ee = Math::Min(e + BoundsEps, splitScanBounds[1][dimension]);
         
         events.push_back(BSplitEvent(es, BSplitEvent::eEvalEnterEventType, objIndex));
         events.push_back(BSplitEvent(s, BSplitEvent::eEnterEventType, objIndex));
         events.push_back(BSplitEvent((s + e) * .5f, BSplitEvent::eMiddleEventType, objIndex));
         events.push_back(BSplitEvent(e, BSplitEvent::eExitEventType, objIndex));
         events.push_back(BSplitEvent(ee, BSplitEvent::eEvalExitEventType, objIndex));

         rightUniqueIndices.addIndex(getObjIndex(obj));
      }
      
      const float center = splitScanBounds.center()[dimension];
               
      events.push_back(BSplitEvent(center, BSplitEvent::eEvalEnterEventType, -1));
      events.push_back(BSplitEvent(center, BSplitEvent::eEnterEventType, -1));
      events.push_back(BSplitEvent(center, BSplitEvent::eMiddleEventType, -1));
      events.push_back(BSplitEvent(center, BSplitEvent::eExitEventType, -1));
      events.push_back(BSplitEvent(center, BSplitEvent::eEvalExitEventType, -1));
      
      if (mParams.buildBSP)
      {
         const float left = Math::Lerp(splitScanBounds[0][dimension], splitScanBounds[1][dimension], 1.0f/3.0f);
         events.push_back(BSplitEvent(left, BSplitEvent::eEvalEnterEventType, -2));
         events.push_back(BSplitEvent(left, BSplitEvent::eEnterEventType, -2));
         events.push_back(BSplitEvent(left, BSplitEvent::eMiddleEventType, -2));
         events.push_back(BSplitEvent(left, BSplitEvent::eExitEventType, -2));
         events.push_back(BSplitEvent(left, BSplitEvent::eEvalExitEventType, -2));
         
         const float right = Math::Lerp(splitScanBounds[0][dimension], splitScanBounds[1][dimension], 2.0f/3.0f);
         events.push_back(BSplitEvent(right, BSplitEvent::eEvalEnterEventType, -3));
         events.push_back(BSplitEvent(right, BSplitEvent::eEnterEventType, -3));
         events.push_back(BSplitEvent(right, BSplitEvent::eMiddleEventType, -3));
         events.push_back(BSplitEvent(right, BSplitEvent::eExitEventType, -3));
         events.push_back(BSplitEvent(right, BSplitEvent::eEvalExitEventType, -3));
      }
      
      const int nodeNumUniqueIndices = rightUniqueIndices.numUniqueIndices();

      std::sort(events.begin(), events.end());
      
      int numOn = 0;      
#if DEBUG_INTERVAL_TRACKER                                                                                   
      BInterval onInterval(BInterval::eInitExpand);
      std::set<int> onObjIndices;
#endif         
      
      BIntervalTracker intervalTracker;
      
      int bestSplitIter = -1;

      int eventIter = 0;
#if AABB_TREE_BUILDER_DEBUG         
      BUniqueIndexTracker bestLeftUniqueIndices(mTotalObjIndices);
      BUniqueIndexTracker bestRightUniqueIndices(mTotalObjIndices);
#endif         
      int bestNumActualLeft = -1;
      int bestNumActualRight = -1;
      int bestLeft = -1;
      int bestOn = -1;
      int bestRight = -1;

      // --- Iterate through sorted intervals
      while (eventIter < (int)events.size())
      {
         const float curEventCoord = events[eventIter].coord();
                     
         int lastEventIter = eventIter + 1;
         
         while ((lastEventIter < (int)events.size()) && (events[lastEventIter].coord() == curEventCoord))
            lastEventIter++;
         
         if (lastEventIter < (int)events.size())
         {
            BASSERT(curEventCoord < events[lastEventIter].coord());
         }
            
         bool validSplitPos = false;
         
         for (int i = eventIter; i < lastEventIter; i++)
         {
            const BSplitEvent& event = events[i];
                     
            if (event.objIndex() < 0)
               validSplitPos = true;
            else
            {
               const ObjType& eventObj = getObj(event.objIndex());
               const AABB& bounds = eventObj.bounds();

               switch (event.type())
               {
                  case BSplitEvent::eEvalEnterEventType:
                  case BSplitEvent::eEvalExitEventType:
                  {
                     validSplitPos = true;
                     break;
                  }
                  case BSplitEvent::eEnterEventType:
                  {
                     numOn++;
                     numRight--;

#if DEBUG_INTERVAL_TRACKER                                                                                                         
                     onObjIndices.insert(event.objIndex());
                     onInterval.expand(BInterval(bounds[0][dimension], bounds[1][dimension]));
#endif                     
                     intervalTracker.insert(BInterval(bounds[0][dimension], bounds[1][dimension]));

                     break;
                  }
                  
                  case BSplitEvent::eMiddleEventType:
                  {
                     numActualRight--;
                     numActualLeft++;

                     rightUniqueIndices.removeIndex(getObjIndex(eventObj));
                     leftUniqueIndices.addIndex(getObjIndex(eventObj));

                     break;
                  }
               } // switch (event.type())
            } // if (event.objIndex() < 0)
         }

#if 0            
         int numL = 0;
         int numR = 0;
         int numO = 0;
         for (int objIndexIter = 0; objIndexIter < pNode->objIndices().size(); objIndexIter++)
         {
            const int objIndex = pNode->objIndex(objIndexIter); 
            const ObjType& obj = getObj(objIndex);
            const AABB objBounds(obj.bounds());
            if (objBounds[1][dimension] < curEventCoord)
               numL++;
            else if (objBounds[0][dimension] > curEventCoord)
               numR++;
            else
               numO++;
         }
         BASSERT(numL == numLeft);
         BASSERT(numR == numRight);
         BASSERT(numO == numOn);
#endif
                                                                        
         if (!mParams.buildBSP)
            validSplitPos = validSplitPos && ((numActualLeft > 0) && (numActualRight > 0));
                     
         if (validSplitPos) 
         {
            BSplit trialSplit;
            trialSplit.set(
               mParams,
               splitScanBounds,
               dimension,
               curEventCoord,
               intervalTracker.interval(),
               numLeft, numOn, numRight,
               numActualLeft, numActualRight,
               nodeNumUniqueIndices,
               leftUniqueIndices,
               rightUniqueIndices,
               cph);
            
//               printf("eventIter: %i lastEventIter: %i coord: %f l: %f h: %f score: %f", eventIter, lastEventIter, curEventCoord, intervalTracker.interval().low()[0], intervalTracker.interval().high()[0], trialSplit.score());

            bool badSplit = false;
            
            if (mParams.buildBSP)
            {
               if ((trialSplit.spatialPercent() == 0.0f) || (trialSplit.spatialPercent() == 1.0f))
               {
                  badSplit = true;
               }
               else if (numOn == pNode->numObjects())
               {
                  badSplit = true;
               }
               else if (((numLeft + numOn) == pNode->numObjects()) || ((numRight + numOn) == pNode->numObjects()))
               {
                  if ((trialSplit.score() > (pNode->numObjects() * mParams.BSPSpatialCutObjThresh)) || (spatialCutCount >= MaxSpatialCuts))
                     badSplit = true;
               }

               badSplit |= (trialSplit.splitScore() > mParams.BSPMaxFacesSplitThresh);

               badSplit |= (trialSplit.score() > (pNode->numObjects() * mParams.BSPScoreThresh));              
            }
            
            if ((!badSplit) && (bestSplit < trialSplit) && (splitToBeat < trialSplit))
            {
               bestSplit = trialSplit;
               // Changed while working on BSP stuff!! Need to test.
               bestSplitIter = lastEventIter;
#if AABB_TREE_BUILDER_DEBUG                  
               bestLeftUniqueIndices = leftUniqueIndices;
               bestRightUniqueIndices = rightUniqueIndices;
#endif                  
               bestNumActualLeft = numActualLeft;
               bestNumActualRight = numActualRight;
               bestLeft = numLeft;
               bestOn = numOn;
               bestRight = numRight;

#if 0                  
               printf("new best split: %f iter: %i bestNumActualLeft: %i bestNumActualRight: %i bestLeft: %i bestOn: %i bestRight: %i",
                  bestSplit.coord(),
                  bestSplitIter,
                  bestNumActualLeft,
                  bestNumActualRight,
                  bestLeft,
                  bestOn,
                  bestRight);                  
#endif                     
            }
         }
         
         for (int i = eventIter; i < lastEventIter; i++)
         {
            const BSplitEvent& event = events[i];

            if (event.objIndex() >= 0)
            {
               const ObjType& eventObj = getObj(event.objIndex());
               const AABB& bounds = eventObj.bounds();

               switch (event.type())
               {
                  case BSplitEvent::eExitEventType:
                  {
                     
                     numOn--;
                     numLeft++;
                     BASSERT(numOn >= 0);                  
                     BASSERT(numRight >= 0);

                     intervalTracker.remove(BInterval(bounds[0][dimension], bounds[1][dimension]));

#if DEBUG_INTERVAL_TRACKER                                                               
                     onObjIndices.erase(event.objIndex());
                     if ((bounds[0][dimension] == onInterval[0][0]) || (bounds[1][dimension] == onInterval[1][0]))
                     {
                        onInterval.initExpand();

                        for (
                           std::set<int>::const_iterator it = onObjIndices.begin(); 
                           it != onObjIndices.end(); 
                        ++it)
                        {
                           const AABB bounds(getObj(*it).bounds());
                           onInterval.expand(BInterval(bounds[0][dimension], bounds[1][dimension]));
                        }
                     }

                     BASSERT(onInterval == intervalTracker.interval());
#endif                     

                     break;
                  }
               }                     
            }
         }
                     
         eventIter = lastEventIter;
      }  // while (eventIter < events.size())
               
#if AABB_TREE_BUILDER_DEBUG
      trace("  Dim: %i, Mid Tris: %i %i, Mats: %i %i, Left: %i, On: %i, Right: %i, Score: %f, Coord: %f", 
         dimension,
         bestNumActualLeft,
         bestNumActualRight,
         bestLeftUniqueIndices.numUniqueIndices(), 
         bestRightUniqueIndices.numUniqueIndices(),
         bestLeft,
         bestOn,
         bestRight,
         bestSplit.score(),
         bestSplit.spatialPercent()
         );
#endif

      // --- create left and right object indices

      leftIndices.resize(0); //erase(leftIndices.begin(), leftIndices.end());
      rightIndices.resize(0); //erase(rightIndices.begin(), rightIndices.end());
      
      if (bestSplit.valid())
      {
         if (mParams.buildBSP)
         {
            int numOn = 0;
            for (uint objIndexIter = 0; objIndexIter < pNode->objIndices().size(); objIndexIter++)
            {
               const int objIndex = pNode->objIndex(objIndexIter); 
               const ObjType& obj = getObj(objIndex);

               const AABB objBounds(obj.bounds());
               
               if (objBounds[1][dimension] < bestSplit.coord())
                  leftIndices.push_back(objIndex);
               else if (objBounds[0][dimension] > bestSplit.coord())
                  rightIndices.push_back(objIndex);
               else
               {
                  leftIndices.push_back(objIndex);
                  rightIndices.push_back(objIndex);
                  numOn++;
               }
            }
            
            int numLeft = static_cast<int>(leftIndices.size());
            int numRight = static_cast<int>(rightIndices.size());
            BVERIFY((numLeft + numRight) >= (int)pNode->objIndices().size());
            BVERIFY(bestLeft + bestOn == numLeft);
            BVERIFY(bestRight + bestOn == numRight);
         }
         else
         {
            if (bestNumActualLeft != -1)
            {
               for (int eventIter = 0; eventIter < (int)events.size(); eventIter++)
               {
                  const BSplitEvent& event = events[eventIter];
                  if (event.objIndex() < 0)
                     continue;
                     
                  switch (event.type())
                  {
                     case BSplitEvent::eEnterEventType:
                     {
                        break;
                     }
                     case BSplitEvent::eMiddleEventType:
                     {
                        if (eventIter < bestSplitIter)
                           leftIndices.push_back(event.objIndex());
                        else
                           rightIndices.push_back(event.objIndex());
                        break;
                     }
                     case BSplitEvent::eExitEventType:
                     {
                        break;
                     }
                  }
               }
               
               BVERIFY((int)leftIndices.size() == bestNumActualLeft);
               BVERIFY((int)rightIndices.size() == bestNumActualRight);
            }
         }
      }

      return bestSplit;          
   }

   //-------------------------------------------------------------------------------------------------                   
   // subdivideNode
   //-------------------------------------------------------------------------------------------------             
   
   IntVec mBestLeftIndices, mBestRightIndices;
   IntVec mLeftIndices, mRightIndices;
   int mNumLeafTris;
         
   bool subdivideNode(BAABBTree::BNode* pNode, int level, int spatialCutCount = 0)
   {
      const int MaxSubdivisionLevel = 60;
      if (level > MaxSubdivisionLevel)
         return false;
      
      const int numObjects = static_cast<int>(pNode->objIndices().size());
      const AABB bounds(pNode->bounds());
      const int majorDimension = bounds.majorDimension();

      // --- is node splittable at all?
      const bool splittable = numObjects >= mParams.minNodeObjects;
      if (!splittable)
         return false;

      BSplit bestSplit;
      int dimension = (mParams.splitLargestAxis) ? majorDimension : 0;
               
      const float MinimumDimension = .00000125f; 
               
      for (int i = 0; i < 3; i++)
      {
         const float dimensionLen = pNode->bounds()[1][dimension] - pNode->bounds()[0][dimension];
         BASSERT(dimensionLen >= 0.0f);

         if (
               ((numObjects > mParams.maxNodeObjects) || (dimensionLen > mParams.maxNodeDim)) && 
               (dimensionLen > MinimumDimension)
            )
         {
            const BSplit split(findBestSplit(pNode, dimension, mLeftIndices, mRightIndices, (float)spatialCutCount, bestSplit));
            
            bool badSplit = !split.valid();
            if ((mParams.buildBSP) && (!badSplit))
            {
               // This is redundant, but I'm paranoid!
               if (((int)mLeftIndices.size() == numObjects) || ((int)mRightIndices.size() == numObjects))
               {
                  if ((split.score() > (numObjects * mParams.BSPSpatialCutObjThresh)) || (spatialCutCount >= MaxSpatialCuts))
                     badSplit = true;
               }
            }
                                          
            if ((!badSplit) && (bestSplit < split))
            {
               mBestLeftIndices.swap(mLeftIndices);
               mBestRightIndices.swap(mRightIndices);
               bestSplit = split;
               if (bestSplit.score() < .001f)
                  break;
            }
         }                  

         if (mParams.splitLargestAxis)
         {
            // Slows the builder down, but it'll produce less leaf nodes (but more ave. tris per leaf)
            //if (bestSplit.valid())
            break;   
         }

         dimension = Math::NextWrap(dimension, 3);
      }

#if AABB_TREE_BUILDER_DEBUG
      trace("subdivideNode: Height: %i, Objects: %i, Maj Dim: %i, Best split dim: %i, Coord: %f, Score: %f, SplitScore: %f, LeftInd: %i, RightInd: %i",
         mTree.root()->maxDepth(),
         numObjects,
         majorDimension,
         bestSplit.dimension(),
         bestSplit.spatialPercent(),
         bestSplit.score(),
         bestSplit.splitScore(),
         mBestLeftIndices.size(), 
         mBestRightIndices.size());
#else
      static int counter = 0;
      counter++;
      if ((counter & 4095) == 0)            
      {  
         printf("\b\b\b\b\b\b\b\b\b\b%i", mNumLeafTris);
      }
#endif
                                 
      if (!bestSplit.valid())
      {
#if AABB_TREE_BUILDER_DEBUG         
         trace("Not splitting");
#endif            
         return false;
      }
      
      if (mParams.buildBSP)
      {
         if (((int)mBestLeftIndices.size() == numObjects) || ((int)mBestRightIndices.size() == numObjects))
            spatialCutCount++;
         else
            spatialCutCount = 0;
      }

      BAABBTree::BNode* pLeft = mTree.allocNode();
      BAABBTree::BNode* pRight = mTree.allocNode();         

      AABB leftBSPBounds(pNode->bounds());
      AABB rightBSPBounds(pNode->bounds());

      leftBSPBounds[1][bestSplit.dimension()] = bestSplit.coord();
      rightBSPBounds[0][bestSplit.dimension()] = bestSplit.coord();
      
      splitNode(bestSplit, pNode, pLeft, pRight, mBestLeftIndices, mBestRightIndices, leftBSPBounds, rightBSPBounds);
      
      if (!subdivideNode(pLeft, level + 1, spatialCutCount))
         mNumLeafTris += pLeft->numObjects();
      
      if (!subdivideNode(pRight, level + 1, spatialCutCount))
         mNumLeafTris += pRight->numObjects();
      
      return true;
   }

   //-------------------------------------------------------------------------------------------------                         
   // initObjIndexHist
   //-------------------------------------------------------------------------------------------------             
   void initObjIndexHist(void)
   {
      mObjIndexHist.resize(mTotalObjIndices);

      for (uint i = 0; i < mObjects.size(); i++)
      {
         const ObjType& obj = mObjects[i];
         const int index = getObjIndex(obj);
         debugRangeCheck(index, mTotalObjIndices);
         mObjIndexHist[index]++;
      }
   }

   //-------------------------------------------------------------------------------------------------                   
   // buildTree
   //-------------------------------------------------------------------------------------------------             
   void buildTree(void)
   {
      mTree.allocNode();

      bool foundSmallObjIndexGroup = false;
      bool foundLargeObjIndexGroup = false;
      for (uint i = 0; i < mObjects.size(); i++)
      {
         const ObjType& obj = mObjects[i];
         const int index = getObjIndex(obj);
         debugRangeCheck(index, mTotalObjIndices);

         if (mObjIndexHist[index] < mParams.smallObjIndexGroupThresh)
            foundSmallObjIndexGroup = true;
         else
            foundLargeObjIndexGroup = true;
      }

      if ((mParams.buildBSP) || (mTotalObjIndices == 1) || (!foundSmallObjIndexGroup) || (!foundLargeObjIndexGroup))
      {
         mTree.root()->objIndices().reserve(mObjects.size());
         for (uint i = 0; i < mObjects.size(); i++)
            mTree.root()->objIndices().push_back(i);

         updateNodeBounds(*mTree.root());
         if (!mFast)
            subdivideNode(mTree.root(), 1);
      }
      else
      {
         // left will have tris from small material groups, right big
         mTree.root()->child(0) = mTree.allocNode();
         mTree.root()->child(0)->parent() = mTree.root();

         mTree.root()->child(1) = mTree.allocNode();
         mTree.root()->child(1)->parent() = mTree.root();

         for (uint i = 0; i < mObjects.size(); i++)
         {
            const ObjType& obj = mObjects[i];
            const int index = getObjIndex(obj);
            debugRangeCheck(index, mTotalObjIndices);

            if (mObjIndexHist[index] < mParams.smallObjIndexGroupThresh)
               mTree.root()->child(0)->objIndices().push_back(i);
            else
               mTree.root()->child(1)->objIndices().push_back(i);
         }

         updateNodeBounds(*mTree.root()->child(0));
         updateNodeBounds(*mTree.root()->child(1));

         mTree.root()->bounds() = mTree.root()->child(0)->bounds();
         mTree.root()->bounds().expand(mTree.root()->child(1)->bounds());
         
         if (mParams.splitAxisHints)
         {
            // indicate that this wasn't a spatial split
            mTree.root()->index() = (uint32)cInvalidIndex;
         }
            
         if (!mFast)
         {
            subdivideNode(mTree.root()->child(0), 1);
            subdivideNode(mTree.root()->child(1), 1);
         }
      }          
      
      printf("");     
   }

   //-------------------------------------------------------------------------------------------------             
   // createLeafUniqueIndicesVec
   // returns number of leaf nodes
   //-------------------------------------------------------------------------------------------------             
   int createLeafUniqueIndicesVec(void)
   {
      int numLeafNodes = 0;

      mLeafUniqueIndicesVec.resize(mTree.numNodes());
      for (int nodeIter = 0; nodeIter < mTree.numNodes(); nodeIter++)
      {
         const BAABBTree::BNode& node = mTree.node(nodeIter);
         if (!node.leaf())
            continue;

         BUniqueIndexTracker& uniqueIndexTracker = mLeafUniqueIndicesVec[nodeIter];
         uniqueIndexTracker.resize(mTotalObjIndices);

         for (int objIter = 0; objIter < node.numObjects(); objIter++)
         {
            const ObjType& obj = getObj(node.objIndex(objIter));
            uniqueIndexTracker.addIndex(getObjIndex(obj));
         }

         numLeafNodes++;
      }

      return numLeafNodes;
   }

   //-------------------------------------------------------------------------------------------------             
   // findBestMergeNode
   // cInvalidIndex if no node found to merge with
   //-------------------------------------------------------------------------------------------------             
   int findBestMergeNode(const int srcNode, const int objIndex)
   {
      const BAABBTree::BNode& sourceNode = mTree.node(srcNode);
      BUniqueIndexTracker& sourceNodeUniqueIndexTracker = mLeafUniqueIndicesVec[srcNode];
      const int sourceFreq = sourceNodeUniqueIndexTracker.indexFreq(objIndex);
      sourceFreq;

      AABB sourceBounds(AABB::eInitExpand);
      for (int objIter = 0; objIter < sourceNode.numObjects(); objIter++)
         sourceBounds.expand(getObj(sourceNode.objIndex(objIter)).bounds());

      int bestNodeIndex = cInvalidIndex;
      float bestExpansionRatio = Math::fNearlyInfinite;

      for (int trialNodeIter = 0; trialNodeIter < mTree.numNodes(); trialNodeIter++)
      {
         if (srcNode == trialNodeIter)
            continue;

         const BAABBTree::BNode& trialNode = mTree.node(trialNodeIter);
         if (!trialNode.leaf())
            continue;
         
         if (trialNode.numObjects() > mParams.maxMergeNodeObjects)
            continue;

         BUniqueIndexTracker& trialNodeUniqueIndexTracker = mLeafUniqueIndicesVec[trialNodeIter];
         const int trialFreq = trialNodeUniqueIndexTracker.indexFreq(objIndex);
         if (0 == trialFreq)
            continue;

         const AABB origNodeAABB = trialNode.bounds();
         const float origNodeSurfaceArea = origNodeAABB.surfaceArea();

         AABB newNodeAABB(origNodeAABB);
         newNodeAABB.expand(sourceBounds);
         const float newNodeMaxDim = newNodeAABB.dimension(newNodeAABB.majorDimension());
         const float newNodeSurfaceArea = newNodeAABB.surfaceArea();

         const float expansionRatio = newNodeSurfaceArea / ((origNodeSurfaceArea > 0.0f) ? origNodeSurfaceArea : 1.0f);

         if ((expansionRatio < bestExpansionRatio) && 
             (expansionRatio < mParams.maxNodeExpansionRatio) &&
             (newNodeMaxDim <= mParams.maxExpandedNodeDim))
         {
            bestExpansionRatio = expansionRatio;
            bestNodeIndex = trialNodeIter;
         }
      }
#if AABB_TREE_BUILDER_DEBUG
      trace("findBestMergeNode: bestNodeIndex: %i, Best expansion ratio: %f", bestNodeIndex, bestExpansionRatio);
#endif
      return bestNodeIndex;
   }

   //-------------------------------------------------------------------------------------------------                   
   // checkNode
   //-------------------------------------------------------------------------------------------------             
   void checkNode(int nodeIndex)
   {  
      BAABBTree::BNode& node = mTree.node(nodeIndex);
      BUniqueIndexTracker uniqueIndexTracker(mTotalObjIndices);

      for (int objIter = 0; objIter < node.numObjects(); objIter++)
      {
         const ObjType& obj = getObj(node.objIndex(objIter));

         uniqueIndexTracker.addIndex(getObjIndex(obj));
      }

      for (int objIndexIter = 0; objIndexIter < mTotalObjIndices; objIndexIter++)
         BASSERT(uniqueIndexTracker.indexFreq(objIndexIter) == mLeafUniqueIndicesVec[nodeIndex].indexFreq(objIndexIter));
   }

   //-------------------------------------------------------------------------------------------------                   
   // moveObjects
   // true if actually moved
   //-------------------------------------------------------------------------------------------------                   
   bool moveObjects(const int srcNodeIndex, const int objIndex)
   {
      const int dstNodeIndex = findBestMergeNode(srcNodeIndex, objIndex);
      if (cInvalidIndex == dstNodeIndex)
         return false;

      BAABBTree::BNode& srcNode = mTree.node(srcNodeIndex);
      BAABBTree::BNode& dstNode = mTree.node(dstNodeIndex);
      IntVec newSrcNodeIndices;
      newSrcNodeIndices.reserve(srcNode.numObjects());

      const int totalObjects = srcNode.numObjects() + dstNode.numObjects();
      totalObjects;

      for (int srcObjIter = 0; srcObjIter < srcNode.numObjects(); srcObjIter++)
      {
         const int srcIndex = srcNode.objIndex(srcObjIter);
         debugRangeCheck(srcIndex, static_cast<int>(mObjects.size()));

         const ObjType& srcObj = getObj(srcIndex);

         const int srcObjIndex = getObjIndex(srcObj);
         debugRangeCheck(srcObjIndex, mTotalObjIndices);

         if (srcObjIndex != objIndex)
         {
            newSrcNodeIndices.push_back(srcIndex);
            continue;
         }

         dstNode.objIndices().push_back(srcIndex);
      }

      srcNode.objIndices().swap(newSrcNodeIndices);

      BASSERT(srcNode.numObjects() + dstNode.numObjects() == totalObjects);

      updateNodeBounds(srcNode);
      updateNodeBounds(dstNode);

      // update histograms
      BUniqueIndexTracker& srcNodeUniqueIndexTracker = mLeafUniqueIndicesVec[srcNodeIndex];
      BUniqueIndexTracker& dstNodeUniqueIndexTracker = mLeafUniqueIndicesVec[dstNodeIndex];

      const int origSrcNodeIndexFreq = srcNodeUniqueIndexTracker.indexFreq(objIndex);
      const int origDstNodeIndexFreq = dstNodeUniqueIndexTracker.indexFreq(objIndex);
      dstNodeUniqueIndexTracker.setIndexFreq(objIndex, origDstNodeIndexFreq + origSrcNodeIndexFreq);

      srcNodeUniqueIndexTracker.setIndexFreq(objIndex, 0);

      checkNode(srcNodeIndex);
      checkNode(dstNodeIndex);

#ifdef AABB_TREE_BUILDER_DEBUG
      trace("findBestMergeNode: moveObjects: Node moved, Orig dest freq: %i, New dest freq: %i",
         origDstNodeIndexFreq, dstNodeUniqueIndexTracker.indexFreq(objIndex));
#endif
      return true;
   }

   //-------------------------------------------------------------------------------------------------                   
   // fixTreeBoundsTraverseUp
   //-------------------------------------------------------------------------------------------------                   
   void fixTreeBoundsTraverseUp(BAABBTree::BNode* pNode, const AABB& bounds)
   {
      BDEBUG_ASSERT(pNode);

      do
      {
         const AABB origBounds(pNode->bounds());
         const float origSurfArea = origBounds.surfaceArea();

         pNode->bounds().expand(bounds);
         const float newSurfArea = pNode->bounds().surfaceArea();
         if (!Math::EqualTol(origSurfArea, newSurfArea))
         {
            BASSERT(pNode->parent() != NULL);
#ifdef AABB_TREE_BUILDER_DEBUG               
            trace("BAABBTreeBuilder::fixTreeBoundsTraverseUp: Expanded node %08X maxdepth %i by %f", pNode, pNode->maxDepth(), newSurfArea / origSurfArea);
#endif               
         }

         pNode = pNode->parent();

      } while (pNode);
   }

   //-------------------------------------------------------------------------------------------------                            
   // fixTreeBounds
   // Recalculates the AABB's of all internal nodes
   //-------------------------------------------------------------------------------------------------                   
   void fixTreeBounds(void)
   {
      for (int nodeIter = 0; nodeIter < mTree.numNodes(); nodeIter++)
      {
         BAABBTree::BNode& node = mTree.node(nodeIter);
         if (!node.leaf())
            continue;

         if (!node.numObjects())
            continue;

         if (node.parent())
            fixTreeBoundsTraverseUp(node.parent(), node.bounds());                        
      }
   }

   //-------------------------------------------------------------------------------------------------                         
   // deleteEmptyLeaves
   //-------------------------------------------------------------------------------------------------                   
   void deleteEmptyLeaves(void)
   {
      mTree.checkTree();

      int nodeIter = 0;

      while (nodeIter < mTree.numNodes())
      {
         const BAABBTree::BNode& node = mTree.node(nodeIter);
         if (!node.leaf())
         {
            nodeIter++;
            continue;
         }

         if (0 == node.numObjects())   
         {     
#ifdef AABB_TREE_BUILDER_DEBUG            
            trace("BAABBTreeBuilder::deleteEmptyLeaves: deleting empty leaf node %i", nodeIter);
#endif               
            mTree.deleteLeafNode(nodeIter);

            // start against because an empty leaf sibling node could have been moved before nodeIter
            nodeIter = 0;
         }
         else
            nodeIter++;
      }

      // deletion is tricky, so check the results early
      for (nodeIter = 0; nodeIter < mTree.numNodes(); nodeIter++)
      {
         const BAABBTree::BNode& node = mTree.node(nodeIter);
         BASSERT(node.bounds().valid());

         if (node.leaf())
         {
            BASSERT(node.numObjects() > 0);
         }
         else
         {
            BASSERT(node.numObjects() == 0);
         }
      }
   }

   //-------------------------------------------------------------------------------------------------                               
   // optimizeTree
   // Attempts to move objects that will create tiny sections into other leaf nodes.
   //-------------------------------------------------------------------------------------------------                         
   void optimizeTree(void)
   {
      if (!mParams.optimizeForRendering)
         return;

      treeStats();
            
      if (createLeafUniqueIndicesVec() <= 2)
         return;

      gConsoleOutput.printf("Optimizing tree for rendering purposes\n");
            
      uint totalNodesMoved = 0;
      for (int nodeIter = 0; nodeIter < mTree.numNodes(); nodeIter++)
      {
         const BAABBTree::BNode& node = mTree.node(nodeIter);
         if (!node.leaf())
            continue;

         BUniqueIndexTracker& uniqueIndexTracker = mLeafUniqueIndicesVec[nodeIter];

         for (int objIndexIter = 0; objIndexIter < mTotalObjIndices; objIndexIter++)
         {
            if (uniqueIndexTracker.indexFreq(objIndexIter))
            {
               if (uniqueIndexTracker.indexFreq(objIndexIter) < mParams.reassignThresh)
               {
#ifdef AABB_TREE_BUILDER_DEBUG                  
                  trace("BAABBTreeBuilder::optimizeTree: Trying to move node %i objindex %i freq %i", 
                     nodeIter, objIndexIter, uniqueIndexTracker.indexFreq(objIndexIter));
#endif
                  if (moveObjects(nodeIter, objIndexIter))
                  {
                     totalNodesMoved++;
                  }
               }
            }
         }
      }
      
      gConsoleOutput.printf("Total calls to moveObjects(): %u\n", totalNodesMoved);

      if (totalNodesMoved)
      {
         fixTreeBounds();
         deleteEmptyLeaves();
      }
   }

   //-------------------------------------------------------------------------------------------------                                                
   // treeStatsTraverse
   //-------------------------------------------------------------------------------------------------                                     
   void treeStatsTraverse(const BAABBTree::BNode* pNode)
   {
      BVERIFY(pNode->bounds().valid());

      if (pNode->leaf())
      {
         if (!mParams.buildBSP)
            BVERIFY(pNode->numObjects() > 0);
      }
      else
         BVERIFY(pNode->numObjects() == 0);

      BVERIFY(mSeenNodes.find(pNode) == mSeenNodes.end());
      mSeenNodes.insert(pNode);

      mNumNodesTraversed++;

      for (int i = 0; i < pNode->numObjects(); i++)
      {
         const int objIndex = pNode->objIndex(i);
         BVERIFY((objIndex >= 0) && (objIndex < (int)mObjects.size()));

         if (mObjCount[objIndex] < 255)
            mObjCount[objIndex]++;
      }

      if (pNode->leaf())
      {
         if (0 == pNode->numObjects())
            mNumEmptyLeafNodes++;
            
         mNumLeafNodesTraversed++;

         mLeafNodeMinObjects = Math::Min(mLeafNodeMinObjects, pNode->numObjects());
         mLeafNodeMaxObjects = Math::Max(mLeafNodeMaxObjects, pNode->numObjects());
         mLeafNodeAveObjects += pNode->numObjects();

         const float nodeDim = pNode->bounds().dimension(pNode->bounds().majorDimension());
         mLeafNodeMinDim = Math::Min(mLeafNodeMinDim, nodeDim);
         mLeafNodeMaxDim = Math::Max(mLeafNodeMaxDim, nodeDim);
         mLeafNodeAveDim += nodeDim;

         const float nodeVol = pNode->bounds().volume();
         mLeafNodeMinVol = Math::Min(mLeafNodeMinVol, nodeVol);
         mLeafNodeMaxVol = Math::Max(mLeafNodeMaxVol, nodeVol);
         mLeafNodeAveVol += nodeVol;

         const float surfArea = pNode->bounds().surfaceArea();
         mLeafNodeMinSurfArea = Math::Min(mLeafNodeMinSurfArea, surfArea);
         mLeafNodeMaxSurfArea = Math::Max(mLeafNodeMaxSurfArea, surfArea);
         mLeafNodeAveSurfArea += surfArea;
      }
      else
      {
         BAABBTree::BNode* pLeft = pNode->child(0);
         BAABBTree::BNode* pRight = pNode->child(1);

         if (pLeft)
         {
            BVERIFY(pRight);

            std::pair<AABB, bool> intersection(AABB::intersectionOp(pLeft->bounds(), pRight->bounds()));
            if (intersection.second)
            {
               float volLeft = pLeft->bounds().volume();
               float volRight = pRight->bounds().volume();
               float volTotal = volLeft + volRight;
               volTotal;

               if ((volLeft > 0.0f) && (volRight > 0.0f))
               {
                  const float overlapPercentage = 
                     Math::Max(intersection.first.volume() / volLeft, intersection.first.volume() / volRight);

                  mMaxNodeOverlap = Math::Max(mMaxNodeOverlap, overlapPercentage);
                  mAveNodeOverlap += overlapPercentage;
                  mNumNodesOverlap++;
               }
            }                  
         }

         if (pNode->child(0)) 
         {
            if (mParams.buildBSP)
               BVERIFY(pNode->bounds().overlaps(pNode->child(0)->bounds()));
            else
               BVERIFY(pNode->bounds().contains(pNode->child(0)->bounds()));
               
            treeStatsTraverse(pNode->child(0));
         }

         if (pNode->child(1)) 
         {
            if (mParams.buildBSP)
               BVERIFY(pNode->bounds().overlaps(pNode->child(1)->bounds()));
            else
               BVERIFY(pNode->bounds().contains(pNode->child(1)->bounds()));
               
            treeStatsTraverse(pNode->child(1));
         }
      }
   }

   //-------------------------------------------------------------------------------------------------                                                                        
   // treeStats
   //-------------------------------------------------------------------------------------------------                                                
   void treeStats(void)
   {
      clearStats();
#if AABB_TREE_BUILDER_MESSAGES         
      gConsoleOutput.printf(
         "BAABBTreeBuilder::treeStats:\nNum nodes: %i\nAve. objects per node: %f\nMax depth: %i\n", 
         mTree.numNodes(), 
         mObjects.size() / float(mTree.numNodes()), 
         mTree.root()->maxDepth());
#endif
      mObjCount.resize(mObjects.size());
      treeStatsTraverse(mTree.root());            

      // ensure all nodes where seen one time
      BVERIFY(mSeenNodes.size() == (uint)mTree.numNodes());

      // Ensure all object indices are present exactly once (duplicate/missing object index test), or once or more for BSP's.
      for (uint i = 0; i < mObjects.size(); i++)
      {
         if (mParams.buildBSP)
            BVERIFY(mObjCount[i] >= 1);
         else
            BVERIFY(mObjCount[i] == 1);
      }
      
      BVERIFY(mNumNodesTraversed == mTree.numNodes());
#if AABB_TREE_BUILDER_MESSAGES         
      gConsoleOutput.printf("BAABBTreeBuilder::treeStats: Tree validity verified\n");

      gConsoleOutput.printf("Total nodes: %i\n", mTree.numNodes());
      gConsoleOutput.printf("Num leaf nodes: %i\n", mNumLeafNodesTraversed);
      gConsoleOutput.printf("Num empty leaf nodes: %i\n", mNumEmptyLeafNodes);
      gConsoleOutput.printf("Leaf node stats:\n");
      gConsoleOutput.printf("  Min objects: %i\n", mLeafNodeMinObjects);
      gConsoleOutput.printf("  Max objects: %i\n", mLeafNodeMaxObjects);
      gConsoleOutput.printf("  Ave objects: %f\n", mLeafNodeAveObjects / float(mNumLeafNodesTraversed));
      gConsoleOutput.printf("  Min dim: %f\n", mLeafNodeMinDim);
      gConsoleOutput.printf("  Max dim: %f\n", mLeafNodeMaxDim);
      gConsoleOutput.printf("  Ave dim: %f\n", mLeafNodeAveDim / float(mNumLeafNodesTraversed));
      gConsoleOutput.printf("  Min vol: %f\n", mLeafNodeMinVol);
      gConsoleOutput.printf("  Max vol: %f\n", mLeafNodeMaxVol);
      gConsoleOutput.printf("  Ave vol: %f\n", mLeafNodeAveVol / float(mNumLeafNodesTraversed));
      gConsoleOutput.printf("  Min surf area: %f\n", mLeafNodeMinSurfArea);
      gConsoleOutput.printf("  Max surf area: %f\n", mLeafNodeMaxSurfArea);
      gConsoleOutput.printf("  Ave surf area: %f\n", mLeafNodeAveSurfArea / float(mNumLeafNodesTraversed));
      gConsoleOutput.printf("  Num nodes overlap: %i\n", mNumNodesOverlap);
      gConsoleOutput.printf("  Max node overlap: %f\n", mMaxNodeOverlap);
      gConsoleOutput.printf("  Ave node overlap: %f\n", mAveNodeOverlap / float(mNumNodesOverlap));
#endif         
   }

   //-------------------------------------------------------------------------------------------------                                                      
   // build
   //-------------------------------------------------------------------------------------------------                                                
   void build(void)
   {
      initObjIndexHist();
               
      buildTree();

      optimizeTree();
               
      treeStats();            
   }
}; // class AABBTreeBuilder

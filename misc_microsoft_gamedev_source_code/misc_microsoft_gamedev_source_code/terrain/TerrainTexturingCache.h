//============================================================================
//
//  TerrainTexturingCache.h
//  
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once
//------------------------------------------------------------
/*
BTextureCache - CLM [08.22.06]
Characteristics of this cache:
   It is Embedded (lower resolutions occupy same memory space as higher resolutions)
   It is multilevel (hierachial)
   It is multidirectional (smaller nodes are inserted from the back)
   It uses the 2 Finger Mark-Replace Defragmentation algorithm.
   Each level of a hierachy will evaluate IF it needs to defragment, before actually defragging
   It uses AGE and COST metrics (see int BTerrainQuadNode::calcCost()) 
   This creates an implicit static / dynamic segmentation for cache pages
   It can be visualized (draws cache to screen)
*/
enum eCacheNumbers
{
   cFastCacheSizeCutoff  = 63
};

enum eCachePropigation
{
   cPropUp  =1,
   cPropDown =2,
   cPropBoth =3,
};
struct BCacheCreationParams
{
   int                           mNumCachePages;              //number of highest level mip cache pages to allocate (IE this is the size of the cache)
   int                           mNumChannels;                //How many channels should we support (albedo, normal, spec etc)
   int                           mNumDiscreteLevels;          //How many Discrete mips we support (main sorting / searching defines)
   int                           mNumMipsAtEachLevel;         //How many mips will each level have (D3D) 
   int                           mMip0Width;                  //Width (in pixels) of mip0 NOTE: NON-SQUARE TEXTURES NOT TESTED
   bool                          mAllowFastCache;             //FALSE - wil not allow a fast cache to be used. TRUE - allows the cache to decide if it NEEDS to use it

   BDynamicArray<D3DFORMAT>      mChannelFormats;        
   BDynamicArray<bool>           mReverseDirection;            //1:1 ratio w/ mNumDiscreteLevels. TRUE if you'd like that level to operate from the BACK of the cache
                                                                                                //FALSE if you'de like it toperate from the standard front
};

class BTextureCache;

//-----------------------------------------------------
class BTextureCacheNode
{
public:
   BTextureCacheNode();
   ~BTextureCacheNode();
   void destroy(void);
   void free(void);

   void create( int width, int height,int mainCachelevel,int indexInParent,int numMipsAtEachLevel,const BDynamicArray<D3DFORMAT> &channelFormats, unsigned char **mpPhysDevPtr,BTextureCache* mOwnerCache);
   void copyTo(BTextureCacheNode *output);

   void        setCacheState(eCacheState state, eCachePropigation prop= cPropBoth);
   eCacheState getCacheState();

   void cascadeState();
   void enforceParent();
   void decouple();

   bool checkStatesOK();




   BTerrainCachedCompositeTexture   *mTexture;
   BTextureCache    *mOwnerCache;

   int mWidth;
   int mHeight;
   int mIndexInParentContainer;
   int mMainCacheLevelIndex;
private:

   eCacheState    mCacheState;
};

//-----------------------------------------------------
/*
CLM : This container holds the actual cacheNodes. It also holds a faster state mirror to be used for searching / defragmentation
*/
class BTextureCacheSizeContainer
{
public:
   BTextureCacheSizeContainer();
   ~BTextureCacheSizeContainer();
   void destroy(void);
   void create(int width, int height,int cacheLevel,bool reverseDirection,bool allowFastCache, int numTexturesAtThisLevel, int numMipsAtEachLevel,const BDynamicArray<D3DFORMAT> &channelFormats, unsigned char **mpPhysDevPtr,BTextureCache    *mOwnerCache);
   void freeAll(void);

   BTextureCacheNode *giveFreeNode();
   BTextureCacheNode *giveUsedNode();

   void defragment();
   bool isDefragmentNeeded();
   void swapNodes(BTextureCacheNode *usedNode, BTextureCacheNode *freeNode);
   void setFastNodeState(int index, eCacheState state);


   int                         mNumNodes;
   BTextureCacheNode                   *mpNodes;

   //IF we have more than 64 elements, it's better to use the fast binary freelist, rather than the small list
   bool                          mUsingFastList;
   BStaticBitStateArray          *mpNodeCaceStatesFast;
   eCacheState                   *mpNodeCacheStates;

   int                         mWidth;
   int                         mHeight;
   bool                        mReverseDirection;
};
//-----------------------------------------------------
class BTextureCache
{
public:
   BTextureCache();
   ~BTextureCache();

   void  create(const BCacheCreationParams & params);
   void  freeCache();
   void  destroy();
   void  defragment(void);

   void  setCachedNodeState(BTextureCacheNode *node,eCacheState state);
   bool  isStillInCache(BTextureCacheNode *node);
   BTextureCacheNode* getAvailableCachedTexture(int width, int height);

   BTextureCacheNode* getNode(int mipIndex, int index);

   void  getMetrics(void);
   void  drawCacheToScreen(int xOffset, int yOffset);
   void  drawNodeToScreen(BTextureCacheNode *node, int &x, int &y,int wrapX,int resetX);
   void  dumpCachesToLog(int appendNum);
   void  dumpCacheNodeToLog(BTextureCacheNode *node, void *file);

   friend class BTextureCacheNode;
   friend class BTextureCacheSizeContainer;

private:
   int         giveMemoryRequirement(int numCachePages, int mip0Width, int numMips, D3DFORMAT fmt );

   BTextureCacheNode* clearAvailLowerTextures(int mipIndex);
   BTextureCacheNode* clearAvailHigherTextures(int mipIndex);
   int         giveUsedCascadedCost(BTextureCacheNode *node);

   void setFastNodeState(int mipLevel, int index, eCacheState state);

   BTextureCacheSizeContainer       *mCaches;
   int                              mNumCacheLevels;

   void                             *mpPhysicalDevicePointer;
   long                             mPhysicalAllocationSize;
};
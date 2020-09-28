//============================================================================
//
//  TerrainTexturingCache.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
//terrain
#include "TerrainPCH.h"
#include "TerrainTexturing.h"
#include "TerrainTexturingCache.h"
#include "Terrain.h"
#include "TerrainRender.h"
#include "TerrainMetric.h"

//xcore
#include "reloadManager.h"
#include "consoleOutput.h"
#include "math\vmxIntersection.h"
#include "mathutil.h"
#include "file\win32file.h"

//xgameRender
#include "tiledAA.h"
#include "primDraw2D.h"
#include "render.h"

//-------------------------------------------------------------------------
BTextureCache::BTextureCache():
mNumCacheLevels(0)
{

}
//-------------------------------------------------------------------------
BTextureCache::~BTextureCache()
{
   destroy();
}
//-------------------------------------------------------------------------
int BTextureCache::giveMemoryRequirement(int numCachePages, int mip0Width, int numMips, D3DFORMAT fmt )
{
   int bitsPerPixel = XGBitsPerPixelFromFormat(fmt);
   int mip0Size = (numCachePages * (mip0Width*mip0Width) * bitsPerPixel)>>3;

   int mipChainSize = 0;
   for(int R=1;R<numMips;R++)
      mipChainSize +=  mip0Size>>(2*R);

   return mip0Size + mipChainSize;
}
//-------------------------------------------------------------------------
void BTextureCache::create(const BCacheCreationParams & params)
{
   BDEBUG_ASSERT(params.mNumDiscreteLevels == (int)params.mReverseDirection.size() && "Reverse Direction must match number of descrete levels for this cache");
   BDEBUG_ASSERT(params.mNumDiscreteLevels>0);
   BDEBUG_ASSERT(params.mChannelFormats.size()>0);
   BDEBUG_ASSERT(params.mNumCachePages>0);
   BDEBUG_ASSERT(params.mMip0Width>0);
   BDEBUG_ASSERT(params.mNumChannels>0);


   mCaches = new BTextureCacheSizeContainer[params.mNumDiscreteLevels];

   int *channelMemSizes = new int[params.mNumChannels];

   int totalMemSize=0;
   int numMipsInMainCache=params.mNumDiscreteLevels;
   numMipsInMainCache;


   //allocate our memory blocks first
   for(int q=0;q<(int)params.mNumChannels;q++)
   {
      if(!params.mChannelFormats[q])
      {
         channelMemSizes[q]=0;
         continue;
      }

      channelMemSizes[q] = giveMemoryRequirement(params.mNumCachePages,params.mMip0Width,params.mNumMipsAtEachLevel,params.mChannelFormats[q]);
      totalMemSize += channelMemSizes[q];
      BTerrainMetrics::addCacheChannelGPUMem(channelMemSizes[q],q );
   }



   //allocate everything in one, big, massive block
   mPhysicalAllocationSize = totalMemSize;
   mpPhysicalDevicePointer = XPhysicalAlloc( totalMemSize, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);



   //now, create our texture headers for the entire area of memory
   // createCacheSizeContainers(params, 0,numMipsInMainCache, channelMemSizes, (unsigned char*)mpPhysicalDevicePointer);
   unsigned char **mMainChannelPtrCounter = new unsigned char*[params.mNumChannels];
   int numTexturesToCreateAtThisLevel = params.mNumCachePages;
   for(int j=0;j<params.mNumDiscreteLevels;j++)
   {
      int width = params.mMip0Width>>j;
      int height = params.mMip0Width>>j;

      //reset our pointer to look at the start of our memory block
      mMainChannelPtrCounter[0] = (unsigned char*)mpPhysicalDevicePointer;

      for(int i=1;i<params.mNumChannels;i++)
         mMainChannelPtrCounter[i] = mMainChannelPtrCounter[i-1] + channelMemSizes[i-1];

      mCaches[j].create(width,height,
         j,
         params.mReverseDirection[j],
         params.mAllowFastCache,
         numTexturesToCreateAtThisLevel, 
         params.mNumMipsAtEachLevel, 
         params.mChannelFormats,
         mMainChannelPtrCounter,this);

      numTexturesToCreateAtThisLevel=numTexturesToCreateAtThisLevel<<2;
   }
   delete []mMainChannelPtrCounter;
   delete []channelMemSizes;


}

//-------------------------------------------------------------------------
void BTextureCache::freeCache()
{
   for(unsigned int i=0;i<cNumMainCacheLevels;i++)
   {
      mCaches[i].freeAll();
   }
}
//-------------------------------------------------------------------------
void BTextureCache::destroy()
{
   if(mCaches)
   {
      for(int i=0;i<mNumCacheLevels;i++)
      {
         mCaches[i].destroy();
      }
      delete [] mCaches;
      mCaches =NULL;
   }


   if(mpPhysicalDevicePointer)
   {
      XPhysicalFree(mpPhysicalDevicePointer);
      mpPhysicalDevicePointer=NULL;
   }
}

//-------------------------------------------------------------------------
BTextureCacheNode* BTextureCache::getAvailableCachedTexture(int width, int height)
{

   SCOPEDSAMPLE(BTerrainGetAvailableCachedTexture);
   BTextureCacheNode *availNode=NULL;

   for(unsigned int i=0;i<cNumMainCacheLevels;i++)
   {
      if(width == mCaches[i].mWidth)
      {
         availNode = mCaches[i].giveFreeNode();
         if(availNode)
         {
            //   gTerrainTexturing.decoupleCachedTexture(availNode->mTexture);
            // targetTex = availNode->mTexture;

            BASSERT(availNode->checkStatesOK());     

            break;
         }
         else  //look in our used bin
         { 
            availNode = mCaches[i].giveUsedNode();
            if(availNode)
            {
               //   gTerrainTexturing.decoupleCachedTexture(availNode->mTexture);
               //    targetTex = availNode->mTexture;

               BASSERT(availNode->checkStatesOK());     

               break;
            }
            else  //can we free a higher level
            {
               availNode = clearAvailHigherTextures(i);
               if(availNode)
               {
                  //      gTerrainTexturing.decoupleCachedTexture(availNode->mTexture);

                  //     targetTex = getAvailableCachedTexture(width,height);

                  BASSERT(availNode->checkStatesOK());     

                  break;
               }
               else  //can we free a lower level?
               {
                  availNode = clearAvailLowerTextures(i);
                  if(availNode)
                  {
                     //        gTerrainTexturing.decoupleCachedTexture(availNode->mTexture);
                     //     targetTex = availNode->mTexture;

                     BASSERT(availNode->checkStatesOK());     

                     break;
                  }
               }
            }
         }
      }
   }

   return availNode;//targetTex;

}

//-------------------------------------------------------------------------
void  BTextureCache::setCachedNodeState(BTextureCacheNode *node,eCacheState state)
{
   SCOPEDSAMPLE(BTerrainSetCacheNodeState);

   node->setCacheState(state);

}
//-------------------------------------------------------------------------
bool  BTextureCache::isStillInCache(BTextureCacheNode *node)
{
   BASSERT(node->mIndexInParentContainer<(int)mCaches[node->mMainCacheLevelIndex].mNumNodes);
   bool isInCache =node->getCacheState()==cCS_Used || node->getCacheState()==cCS_UsedThisFrame;
   return  isInCache;

}
//------------------------------------------------------------------------
void  BTextureCache::defragment(void)
{
   SCOPEDSAMPLE(BTerrainTexCache_Defragment);

   for(int i=cNumMainCacheLevels-1;i>=0;i--)
   {
      mCaches[i].defragment();
   }
}
//------------------------------------------------------------------------
int  BTextureCache::giveUsedCascadedCost(BTextureCacheNode *node)
{
   if(node->getCacheState() == cCS_Used)
   {
      BASSERT((node->mTexture) && (node->mTexture->mpOwnerQuadNode));
      return node->mTexture->mpOwnerQuadNode->getCost();
   }
   else if(node->getCacheState() == cCS_Free ||node->getCacheState() == cCS_Blocked)
      return 0;
   else if(node->getCacheState() == cCS_UsedThisFrame)
      return -1;

   if(node->getCacheState() == cCS_Subdivided)
   {
      int mipLevel = node->mMainCacheLevelIndex;
      int lowerLevelIndex = node->mIndexInParentContainer;
      int incremn = 4;
      int cost=0;
      for(int i=mipLevel+1;i<cNumMainCacheLevels;i++)
      {
         lowerLevelIndex = lowerLevelIndex<<2;
         for(int k=0;k<incremn;k++)
         {
            int c=  giveUsedCascadedCost(&mCaches[i].mpNodes[lowerLevelIndex+k]);
            if(c==-1)
               return -1;  //this block has a held texture
            cost +=c;
         }
         incremn = incremn<< 2;
      }
      return cost;
   }

   //
   return -1;
}
//------------------------------------------------------------------------
BTextureCacheNode* BTextureCache::clearAvailHigherTextures(int mipIndex)
{
   BTextureCacheNode * availNode = NULL;
   //try to find a higher level node to free
   for(int q=mipIndex-1;q>=0;q--)
   {     
      availNode = mCaches[q].giveUsedNode();

      //if we found a larger mip in our used this frame bin, free it, and start the loop again.
      if(availNode)
      {
         gTerrainTexturing.freeCachedTexture(availNode->mTexture);
         return availNode;
      }
   }

   return NULL;
}
//------------------------------------------------------------------------
BTextureCacheNode* BTextureCache::clearAvailLowerTextures(int mipIndex)
{
   //scan this mip level. find the smallest cost, replace that one.
   int smallestCost = 255;
   int smallestIndex =-1;
   if(mCaches[mipIndex].mReverseDirection)
   {
      for (int i=(int)mCaches[mipIndex].mNumNodes-1;i>=0;i--)
      {
         if(mCaches[mipIndex].mpNodes[i].getCacheState()==cCS_Subdivided)
         {
            int cost = giveUsedCascadedCost(&mCaches[mipIndex].mpNodes[i]);
            if(cost!=-1 && cost < smallestCost)
            {
               smallestCost = cost;
               smallestIndex = i;
            }
         }
      }
   }
   else
   {
      for (int i=0;i<mCaches[mipIndex].mNumNodes;i++)
      {
         if(mCaches[mipIndex].mpNodes[i].getCacheState()==cCS_Subdivided)
         {
            int cost = giveUsedCascadedCost(&mCaches[mipIndex].mpNodes[i]);
            if(cost!=-1 && cost < smallestCost)
            {
               smallestCost = cost;
               smallestIndex = i;
            }
         }
      }
   }


   if(smallestIndex==-1)
      return NULL;

   gTerrainTexturing.freeCachedTexture(mCaches[mipIndex].mpNodes[smallestIndex].mTexture);

   return &mCaches[mipIndex].mpNodes[smallestIndex];
}
//------------------------------------------------------------------------
BTextureCacheNode*   BTextureCache::getNode(int mipIndex, int index)
{
   BASSERT(mipIndex < cNumMainCacheLevels && mipIndex >=0 );
   BASSERT(index < (int)mCaches[mipIndex].mNumNodes && index >=0 );
   return &mCaches[mipIndex].mpNodes[index];
}
//------------------------------------------------------------------------
void  BTextureCache::dumpCacheNodeToLog(BTextureCacheNode *node, void *Ffile)
{
#ifndef BUILD_FINAL

   BWin32File *file = (BWin32File*)Ffile;

   BFixedString256 str(cVarArg, "%i : s: %i o:%i\n", node->getCacheState(),node->mWidth,node->mTexture->mpOwnerQuadNode);
   file->write(str.c_str(),str.getLen());

   if(node->mMainCacheLevelIndex+1 >= cNumMainCacheLevels)
      return;

   int lowerLevelIndex = node->mIndexInParentContainer;
   int incremn = 4;
   {
      lowerLevelIndex = lowerLevelIndex<<2;
      BFixedString256 dash;
      for(int j=0;j<node->mMainCacheLevelIndex+1;j++)
         dash.append(BFixedString16("-"));
      for(int k=0;k<incremn;k++)
      {
         file->write(dash.c_str(),dash.getLen());
         dumpCacheNodeToLog(&mCaches[node->mMainCacheLevelIndex+1].mpNodes[lowerLevelIndex+k],Ffile);
      }
      incremn = incremn<< 2;
   }
#endif
}
//------------------------------------------------------------------------
void  BTextureCache::dumpCachesToLog(int appendNum)
{
#ifndef BUILD_FINAL

   BWin32File file;
   BFixedString256 fName(cVarArg, "d:\\texCacheDump%i.txt", appendNum);
   if (!file.open(fName.c_str(), BWin32File::cCreateAlways | BWin32File::cWriteAccess))
      return;

   for(int q=0;q<mCaches[0].mNumNodes;q++)
   {
      dumpCacheNodeToLog(&mCaches[0].mpNodes[q],&file);

   }


   file.close();

#endif
}
//------------------------------------------------------------------------
void  BTextureCache::drawNodeToScreen(BTextureCacheNode *node, int &x, int &y,int wrapX,int resetX)
{
#ifndef BUILD_FINAL

   int kWidth = node->mWidth>>2;
   int kHeight = node->mHeight>>2;

   if(node->getCacheState() == cCS_Free)
   {
      //draw some lines around me
      BPrimDraw2D::drawLine2D(x+1,          y+1,          x+kWidth-1, y+1,         0xFF00FF00);
      BPrimDraw2D::drawLine2D(x+1,          y+kHeight-1,  x+kWidth-1, y+kHeight-1, 0xFF00FF00);
      BPrimDraw2D::drawLine2D(x+1,          y+1,          x+1,        y+kHeight-1, 0xFF00FF00);
      BPrimDraw2D::drawLine2D(x+kWidth-1,   y+1,          x+kWidth-1, y+kHeight-1, 0xFF00FF00);
   }
   else if(node->getCacheState()== cCS_Used || node->getCacheState()== cCS_UsedThisFrame)
   {
      //figure out what texture to show
      int visMode = BTerrain::getVisMode();
      int texToDraw = -1;
      
      switch (visMode)
      {
         case cVMDisabled:     
         case cVMAlbedo:         texToDraw = cTextureTypeAlbedo; break;
         case cVMSelf:           texToDraw = cTextureTypeSelf; break;
         case cVMEnvMask:        texToDraw = cTextureTypeEnvMask; break;
         case cVMTangentNormal:  texToDraw = cTextureTypeNormal; break;
         case cVMSpecColor:      texToDraw = cTextureTypeSpecular; break;
      };

      if (texToDraw == -1)
         BD3D::mpDev->SetTexture(0, gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack));
      else 
         BD3D::mpDev->SetTexture(0, node->mTexture->mTextures[texToDraw]);

      BPrimDraw2D::drawSolidRect2D(x, y, x+kWidth, y+kHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0);

      //draw some lines around me
      BPrimDraw2D::drawLine2D(x+1,          y+1,          x+kWidth-1, y+1,         0xFFFFFFFF);
      BPrimDraw2D::drawLine2D(x+1,          y+kHeight-1,  x+kWidth-1, y+kHeight-1, 0xFFFFFFFF);
      BPrimDraw2D::drawLine2D(x+1,          y+1,          x+1,        y+kHeight-1, 0xFFFFFFFF);
      BPrimDraw2D::drawLine2D(x+kWidth-1,   y+1,          x+kWidth-1, y+kHeight-1, 0xFFFFFFFF);

   }
   else if(node->getCacheState()== cCS_Subdivided)
   {
      int kX = x;
      int kY = y;

      int resX = x;
      int lowerLevelIndex = node->mIndexInParentContainer;
      int incremn = 4;
      {
         lowerLevelIndex = lowerLevelIndex<<2;

         for(int k=0;k<incremn;k++)
         {
            drawNodeToScreen(&mCaches[node->mMainCacheLevelIndex+1].mpNodes[lowerLevelIndex+k],x,y,kX+kWidth,resX);
         }
         incremn = incremn<< 2;
      }

      x = kX;
      y = kY;

   }

   x+=kWidth;
   if(x>=wrapX)
   {
      y+=kHeight;
      x=resetX;
   }

#endif
}

//-------------------------------------------------------------------------
void  BTextureCache::drawCacheToScreen(int xOffset, int yOffset)
{
#ifndef BUILD_FINAL

   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

   //draw each cache texture to the screen


   int xStart=xOffset;//50;
   int yStart=yOffset;//50;
   int x=xStart;
   int y=yStart;
   int screenWidth = 1200;

   for(int k=0;k<mCaches[0].mNumNodes;k++)
   {
      drawNodeToScreen(&mCaches[0].mpNodes[k],x,y,screenWidth,xStart);
   }      

   BD3D::mpDev->SetTexture(0, NULL);
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

#endif
}
//------------------------------------------------------------------------
void  BTextureCache::getMetrics()
{
#ifndef BUILD_FINAL

   for(int i=0;i<cNumMainCacheLevels;i++)
   {
      int numTex=0;
      for(int k=0;k<mCaches[i].mNumNodes;k++)
      {
         if(mCaches[i].mpNodes[k].getCacheState()==cCS_Used)
            numTex++;  
      }
      BTerrainMetrics::setNumCompositeTextures(numTex,i);
   }
#endif
}
//------------------------------------------------------------------------
void BTextureCache::setFastNodeState(int mipLevel, int index, eCacheState state)
{
   mCaches[mipLevel].setFastNodeState(index,state);
}
//------------------------------------------------------------------------


//------------------------------------------------------------------------
BTextureCacheSizeContainer::BTextureCacheSizeContainer():
mWidth(0),
mHeight(0),
mpNodes(0),
mReverseDirection(false),
mUsingFastList(false),
mpNodeCaceStatesFast(0),
mpNodeCacheStates(0)
{
}
//------------------------------------------------------------------------
BTextureCacheSizeContainer::~BTextureCacheSizeContainer()
{
   destroy();
}
//------------------------------------------------------------------------
void BTextureCacheSizeContainer::create(int width, int height,int cacheLevel,bool reverseDirection,bool allowFastCache,int numTexturesAtThisLevel ,int numMipsAtEachLevel,const BDynamicArray<D3DFORMAT> &channelFormats, unsigned char **mpPhysDevPtr,BTextureCache    *OwnerCache)
{
   mWidth = width;
   mHeight = height;
   mNumNodes = numTexturesAtThisLevel;
   mpNodes = new BTextureCacheNode[numTexturesAtThisLevel];
   BTerrainMetrics::addCacheMemCPU(sizeof(BTextureCacheNode) * numTexturesAtThisLevel);

   for(int x=0;x<numTexturesAtThisLevel;x++)
   {
      mpNodes[x].create(width,height,cacheLevel,x,numMipsAtEachLevel,channelFormats,mpPhysDevPtr,OwnerCache);
   }

   if(numTexturesAtThisLevel > cFastCacheSizeCutoff && allowFastCache)
   {
      mUsingFastList = true;
      mpNodeCaceStatesFast = new BStaticBitStateArray[cCS_StatesInFastList];
      mpNodeCaceStatesFast[cCS_Free].init(numTexturesAtThisLevel,true);
      mpNodeCaceStatesFast[cCS_Used].init(numTexturesAtThisLevel,true);
   }
   else
   {
      mUsingFastList=false;
      mpNodeCacheStates = new eCacheState[numTexturesAtThisLevel];
      for(int x=0;x<numTexturesAtThisLevel;x++)
         mpNodeCacheStates[x] = cCS_Free;

      BTerrainMetrics::addCacheMemCPU(sizeof(eCacheState) * numTexturesAtThisLevel);
   }


   mReverseDirection = reverseDirection;
}
//------------------------------------------------------------------------
void BTextureCacheSizeContainer::destroy(void)
{
   if(mpNodes)
   {
      for(int i=0;i<mNumNodes;i++)
      {
         mpNodes[i].destroy();
      }
      delete [] mpNodes;
      mpNodes=NULL;
   }

   if(mpNodeCaceStatesFast)
   {
      for(int i=0;i<cCS_StatesInFastList;i++)
         mpNodeCaceStatesFast[i].destroy();
      mpNodeCaceStatesFast=NULL;
   }
   if(mpNodeCacheStates)
   {
      delete [] mpNodeCacheStates;
      mpNodeCacheStates=NULL;
   }

}
//------------------------------------------------------------------------
void BTextureCacheSizeContainer::freeAll(void)
{
   for(int i=0;i<mNumNodes;i++)
      mpNodes[i].free();    
}
//------------------------------------------------------------------------
BTextureCacheNode * BTextureCacheSizeContainer::giveUsedNode()
{
   SCOPEDSAMPLE(BTerrainGetUsedCachedTexture);
   //walk our list, find all used nodes. Caclulate their costs
   //return the lowest cost
   int cheapestCost=200;
   int cheapestIndex=-1;
   if(mUsingFastList)
   {
      int ind=-1;
      do
      {
         ind = mpNodeCaceStatesFast[cCS_Used].giveNextTRUE(ind+1);
         if(ind!=-1)
         {
            int element = ind;
            if(mReverseDirection)
               element = (mNumNodes-1) - element;

            BASSERT((mpNodes[element].mTexture) && (mpNodes[element].mTexture->mpOwnerQuadNode));
            
            int cost = mpNodes[element].mTexture->mpOwnerQuadNode->getCost();
            if(cost < cheapestCost)
            {
               cheapestCost = cost;
               cheapestIndex = element;
            }
                          
         }
      }while(ind !=-1);

      //we didn't find a cheap texture!
      if(cheapestIndex==-1)
         return NULL;

      return &mpNodes[cheapestIndex];
   }
   else
   {


      if(mReverseDirection)
      {
         for(int i=(int)mNumNodes-1;i>=0;i--)
         {
            eCacheState cs = mpNodeCacheStates[i];
            if(cs==cCS_Used)
            {
               BASSERT((mpNodes[i].mTexture) && (mpNodes[i].mTexture->mpOwnerQuadNode));
               
               int cost = mpNodes[i].mTexture->mpOwnerQuadNode->getCost();
               if(cost < cheapestCost)
               {
                  cheapestCost = cost;
                  cheapestIndex = i;
               }
                                 
            }
         }
      }
      else
      {
         for(int i=0;i<mNumNodes;i++)
         {
            eCacheState cs = mpNodeCacheStates[i];
            if(cs==cCS_Used)
            {
               BASSERT ((mpNodes[i].mTexture) && (mpNodes[i].mTexture->mpOwnerQuadNode));
               
               int cost = mpNodes[i].mTexture->mpOwnerQuadNode->getCost();
               if(cost < cheapestCost)
               {
                  cheapestCost = cost;
                  cheapestIndex = i;
               }
                                 
            }
         }
      }

      //we didn't find a cheap texture!
      if(cheapestIndex==-1)
         return NULL;

      return &mpNodes[cheapestIndex];
   }

   return NULL;
}
//------------------------------------------------------------------------
BTextureCacheNode *BTextureCacheSizeContainer::giveFreeNode()
{
   SCOPEDSAMPLE(BTerrainGetFreeCachedTexture);
   if(mUsingFastList)
   {
      int element = mpNodeCaceStatesFast[cCS_Free].giveNextTRUE(0);
      if(element!=-1)
      {
         if(mReverseDirection)
            element = (mNumNodes-1) - element;
         return &mpNodes[element];
      }
      return NULL;
   }
   else
   {
      if(mReverseDirection)
      {
         for(int i=(int)mNumNodes-1;i>=0;i--)
         {
            eCacheState cs =mpNodeCacheStates[i];
            if(cs==cCS_Free)
            {
               return &mpNodes[i];
            }
         }
      }
      else
      {
         for(int i=0;i<mNumNodes;i++)
         {
            eCacheState cs =mpNodeCacheStates[i];
            if(cs==cCS_Free)
            {
               return &mpNodes[i];
            }
         }
      }
   }

   return NULL;
}

//------------------------------------------------------------------------
bool BTextureCacheSizeContainer::isDefragmentNeeded()
{
   SCOPEDSAMPLE(BTerrain_DefragEval);
   //calculate the avg stride amt for freeblocks
   int avgDist =0;
   int numRuns=0;
   //Simple sliding compacting
   if(mReverseDirection)
   {
      //find the end of our list
      int listEnd = 0;
      for(int i=0;i<(int)mNumNodes;i++)
      {
         if(mpNodes[i].getCacheState() == cCS_Used)
         {
            listEnd = i;
            break;
         }
      }


      for(int target=(int)mNumNodes-1;target>listEnd;target--)
      {
         if(mpNodes[target].getCacheState() == cCS_Free)
         {
            int k=0;
            for(k=target;k>listEnd;k--)
            {
               if(mpNodes[k].getCacheState() != cCS_Free)
               {
                  break;
               }
            }

            numRuns++;
            avgDist += target-k;
            target = k;
         }
      }
   }
   else
   {
      //find the end of our list
      int listEnd = 0;
      for(int i=(int)mNumNodes-1;i>=0;i--)
      {
         if(mpNodes[i].getCacheState() == cCS_Used)
         {
            listEnd = i;
            break;
         }
      }


      for(int target=0;target<listEnd;target++)
      {
         if(mpNodes[target].getCacheState() == cCS_Free)
         {
            int k=0;
            for(k=target+1;k<listEnd;k++)
            {
               if(mpNodes[k].getCacheState() != cCS_Free)
               {
                  break;
               }
            }

            numRuns++;
            avgDist += k-target;
            target = k;
         }
      }
   }

   if(!numRuns)
      return false;

   const int cDefragNeededStride = 2;

   avgDist /= numRuns;
   return avgDist >= cDefragNeededStride;
}
//------------------------------------------------------------------------
void BTextureCacheSizeContainer::swapNodes(BTextureCacheNode *usedNode, BTextureCacheNode *freeNode)
{
   //SWAP THESE TWO TEXTURES

   usedNode->copyTo(freeNode);

   //this will free the qn handle pointing to usedNode 
   gTerrainTexturing.freeCachedTexture(usedNode->mTexture);

   //reassign the qn pointer.
   BASSERT( (freeNode->mTexture->mpOwnerQuadNode)&&
                  (freeNode->mTexture->mpOwnerQuadNode->mRenderPacket) &&
                  (freeNode->mTexture->mpOwnerQuadNode->mRenderPacket->mTexturingData));

   freeNode->mTexture->mpOwnerQuadNode->mRenderPacket->mTexturingData->mCachedUniqueTexture = freeNode->mTexture;

   //cascade the state
   gTerrainTexturing.useCachedTexture(freeNode->mTexture);



}
//------------------------------------------------------------------------
void BTextureCacheSizeContainer::defragment()
{
   if(!isDefragmentNeeded())
      return;

   SCOPEDSAMPLE(BTerrain_Defragment);
   //Simple sliding compacting
   if(mReverseDirection)
   {
      int listEnd = 0;
      for(int i=0;i<(int)mNumNodes;i++)
      {
         if(mpNodes[i].getCacheState() == cCS_Used)
         {
            listEnd = i;
            break;
         }
      }

      for(int freeTex=(int)mNumNodes-1;freeTex>=listEnd;freeTex--)
      {
         if(mpNodes[freeTex].getCacheState() == cCS_Free)
         {
            bool swapped=false;
            int usedTex=0;
            for(usedTex=listEnd;usedTex<freeTex;usedTex++)
            {
               if(mpNodes[usedTex].getCacheState() == cCS_Used)
               {
                  BASSERT(mpNodes[usedTex].mTexture);
                  BASSERT(mpNodes[freeTex].mTexture);
                  swapNodes(&mpNodes[usedTex],&mpNodes[freeTex]);

                  swapped=true;
                  break;
               }
            }
            if(swapped)
               continue;

            //there's no used blocks past this one to flip.
            if(usedTex==freeTex)
               break;
         }
      }
   }
   else
   {
      //find the end of our list
      int listEnd = 0;
      for(int i=(int)mNumNodes-1;i>=0;i--)
      {
         if(mpNodes[i].getCacheState() == cCS_Used)
         {
            listEnd = i;
            break;
         }
      }

      for(int freeTex=0;freeTex<=listEnd;freeTex++)
      {
         if(mpNodes[freeTex].getCacheState() == cCS_Free)
         {
            bool swapped=false;
            int usedTex=0;
            for(usedTex=listEnd;usedTex>=freeTex;usedTex--)
            {
               if(mpNodes[usedTex].getCacheState() == cCS_Used)
               {
                  BASSERT(mpNodes[usedTex].mTexture);
                  BASSERT(mpNodes[freeTex].mTexture);
                  swapNodes(&mpNodes[usedTex],&mpNodes[freeTex]);

                  swapped=true;
                  break;
               }
            }
            if(swapped)
               continue;

            //there's no used blocks past this one to flip.
            if(usedTex==freeTex)
               break;
         }
      }
   }
}
//------------------------------------------------------------------------
void BTextureCacheSizeContainer::setFastNodeState(int index, eCacheState state)
{
   if(mUsingFastList)
   {
      int idx =index;
      if(mReverseDirection)
         idx = (mNumNodes-1) - index;

      for(int i=0;i<cCS_StatesInFastList;i++)
         mpNodeCaceStatesFast[i].setState(idx,false);

      if(state < cCS_StatesInFastList)
         mpNodeCaceStatesFast[(int)state].setState(idx,true);
   }
   else
   {
      mpNodeCacheStates[index]=state;
   }

}
//------------------------------------------------------------------------



//------------------------------------------------------------------------
BTextureCacheNode::BTextureCacheNode():
mWidth(0),
mHeight(0),
mTexture(0),
mCacheState(cCS_Free),
mIndexInParentContainer(0),
mMainCacheLevelIndex(0),
mOwnerCache(0)
{

}
//-------------------------------------------------------------------------
BTextureCacheNode::~BTextureCacheNode()
{
   destroy();
}
//-------------------------------------------------------------------------
void BTextureCacheNode::free(void)
{
   gTerrainTexturing.freeCachedTexture(mTexture);
}
//-------------------------------------------------------------------------
void BTextureCacheNode::destroy(void)
{
   if(mTexture)
   {
      mTexture->freeDeviceData();
      delete mTexture;
      mTexture = NULL;
   }
}
//-------------------------------------------------------------------------
void BTextureCacheNode::create( int width, int height,int mainCachelevel,int indexInParent,int numMipsAtEachLevel,const BDynamicArray<D3DFORMAT> &channelFormats, unsigned char **mpPhysDevPtr,BTextureCache* OwnerCache)
{
   mWidth = width;
   mHeight = height;
   mIndexInParentContainer = indexInParent;
   mMainCacheLevelIndex = mainCachelevel;
   mOwnerCache = OwnerCache;

   //create our local cache texture
   mTexture = new BTerrainCachedCompositeTexture();
   mTexture->mWidth = width;
   mTexture->mHeight = height;
   mTexture->mpOwnerCacheNode = this;
   mTexture->mpOwnerQuadNode = NULL;
   BTerrainMetrics::addCacheMemCPU(sizeof(BTerrainCachedCompositeTexture));


   int imgSize=0;   

   for(int q=0;q<(int)channelFormats.size();q++)
   {
      if(!channelFormats[q])
         continue;

      mTexture->mTextures[q] = new D3DTexture();

      imgSize = XGSetTextureHeader( width,
         height,
         numMipsAtEachLevel,
         0,
         channelFormats[q],0,0,
         XGHEADER_CONTIGUOUS_MIP_OFFSET,0,
         mTexture->mTextures[q],NULL,NULL);

      XGOffsetResourceAddress( mTexture->mTextures[q], mpPhysDevPtr[q]); 
      mpPhysDevPtr[q]+=imgSize;

      BCOMPILETIMEASSERT(sizeof(gTerrainChannelParams) / sizeof(gTerrainChannelParams[0]) == cTextureTypeMax);
      const bool srgbRead = gTerrainChannelParams[q].mCacheSRGBRead;

      if (srgbRead)
      {
         GPUTEXTURE_FETCH_CONSTANT& fc = mTexture->mTextures[q]->Format;
         fc.SignX = GPUSIGN_GAMMA;
         fc.SignY = GPUSIGN_GAMMA;
         fc.SignZ = GPUSIGN_GAMMA;
      }         
   }
}

//-------------------------------------------------------------------------
void BTextureCacheNode::copyTo(BTextureCacheNode *output)
{
   if(this==output)
      return;

   //output->mCacheState              = mCacheState;
   setCacheState(mCacheState);

   mTexture->copyTo(output->mTexture);   
}
//-------------------------------------------------------------------------
void BTextureCacheNode::setCacheState(eCacheState state, eCachePropigation prop/*= cPropBoth*/)
{
   mCacheState = state;

   mOwnerCache->setFastNodeState(mMainCacheLevelIndex,mIndexInParentContainer,state);


   if(prop & cPropDown)
      cascadeState();
   if(prop & cPropUp)
      enforceParent();

   BASSERT(checkStatesOK());
}
//-------------------------------------------------------------------------
eCacheState BTextureCacheNode::getCacheState()
{
   return mCacheState;
}
//-------------------------------------------------------------------------
void BTextureCacheNode::cascadeState()
{
   //cascade my state downward
   eCacheState lowerState = cCS_Free;
   if(mCacheState == cCS_Used  || mCacheState == cCS_UsedThisFrame || mCacheState == cCS_Blocked)
      lowerState = cCS_Blocked;

   if(mMainCacheLevelIndex+1 >=cNumMainCacheLevels)
      return ;

   int lowerLevelIndex = mIndexInParentContainer<<2;
   for(int k=0;k<4;k++)
   {
      int tt = lowerLevelIndex+k;
      mOwnerCache->getNode(mMainCacheLevelIndex+1,tt)->setCacheState(lowerState, cPropDown);
   }
}
//-------------------------------------------------------------------------
void BTextureCacheNode::enforceParent()
{

   //enforce myself as a parent
   if(mMainCacheLevelIndex+1 < cNumMainCacheLevels)
   {
      //check the state of my kids.
      int lowerLevelTarget = mIndexInParentContainer<<2;
      bool kidIsUsed=false;
      for(int k=0;k<4;k++)
      {
         eCacheState cs = mOwnerCache->getNode(mMainCacheLevelIndex+1,lowerLevelTarget+k)->mCacheState;
         kidIsUsed |= ((cs==cCS_Used) || (cs==cCS_Subdivided) || (cs == cCS_UsedThisFrame));
      }
      if(kidIsUsed)
      {
         mCacheState = cCS_Subdivided;
         mOwnerCache->setFastNodeState(mMainCacheLevelIndex,mIndexInParentContainer,mCacheState);
      }
      else if(mCacheState == cCS_Subdivided)
      {
         mCacheState = cCS_Free;
         mOwnerCache->setFastNodeState(mMainCacheLevelIndex,mIndexInParentContainer,mCacheState);
      }

   }
   //tell my parents to enforce
   if(mMainCacheLevelIndex==0)
      return;

   BTextureCacheNode *cn = mOwnerCache->getNode(mMainCacheLevelIndex-1,mIndexInParentContainer>>2);
   cn->enforceParent();

}
//-------------------------------------------------------------------------
void BTextureCacheNode::decouple()
{  
   //CLM [11.16.07] We no longer potentially composite during the 3D pass, so decoupling
   // can't occur.
#if 0
   if(mTexture->mpOwnerQuadNode)
   {
      if(mTexture->mpOwnerQuadNode->mRenderPacket)
      {
         mTexture->mpOwnerQuadNode->mRenderPacket->mTexturingData->mCachedUniqueTexture = NULL;
      }
      mTexture->mpOwnerQuadNode=NULL;
   }

   if(mMainCacheLevelIndex + 1 >= cNumMainCacheLevels)
      return ;

   int lowerLevelIndex = mIndexInParentContainer << 2;
   for(int k=0;k<4;k++)
   {
      int tt = lowerLevelIndex+k;
      mOwnerCache->getNode(mMainCacheLevelIndex + 1, tt)->decouple();
   }
#endif
}
//-------------------------------------------------------------------------
bool BTextureCacheNode::checkStatesOK()
{
#ifdef BUILD_FINAL
   return true;
#endif

   int mipIndex = mMainCacheLevelIndex;
   int index = mIndexInParentContainer;


   //set me.
   eCacheState cs = mCacheState;



   bool higherOK=true;
   //set my parents.
   int higherlevelindex = index;
   for(int i=mipIndex-1;i>=0;i--)
   {     
      higherlevelindex = higherlevelindex >> 2;

      eCacheState hls = mOwnerCache->getNode(i,higherlevelindex)->getCacheState();
      if(cs == cCS_Free)
         higherOK &=  hls==cCS_Subdivided || hls==cCS_Free;
      else if(cs == cCS_Subdivided)
         higherOK &= hls==cCS_Subdivided;
      else if(cs == cCS_Used || cs == cCS_UsedThisFrame)
         higherOK &= hls==cCS_Subdivided;
      else if(cs == cCS_Blocked)
         higherOK &= hls==cCS_Used || hls==cCS_Blocked || hls == cCS_UsedThisFrame || hls==cCS_Subdivided;
   }
   if(!higherOK)
      return false;


   //set my children.
   bool lowerOK=true;
   int lowerLevelIndex = index;
   int incremn = 4;
   for(int i=mipIndex+1;i<cNumMainCacheLevels;i++)
   {
      lowerLevelIndex = lowerLevelIndex<<2;
      for(int k=0;k<incremn;k++)
      {
         eCacheState lls = mOwnerCache->getNode(i,lowerLevelIndex+k)->getCacheState();

         if(cs == cCS_Free)
            lowerOK &=  lls ==cCS_Free;
         else if(cs == cCS_Subdivided)
            lowerOK &=  lls==cCS_Subdivided || lls==cCS_Used || lls==cCS_Free || lls == cCS_UsedThisFrame || lls==cCS_Blocked;
         else if(cs == cCS_Used || cs == cCS_UsedThisFrame)
            lowerOK &= lls==cCS_Blocked;
         else if(cs == cCS_Blocked)
            lowerOK &= lls==cCS_Blocked;
      }
      incremn = incremn<< 2;
   }

   if(!lowerOK)
      return false;

   return true;
}
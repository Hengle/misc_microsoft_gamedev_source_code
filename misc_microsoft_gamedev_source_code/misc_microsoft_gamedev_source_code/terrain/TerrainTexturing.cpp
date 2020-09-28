//============================================================================
//
//  TerrainTexturing.cpp
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

//xrender
#include "renderThread.h"
#include "D3DTextureManager.h"
#include "renderDraw.h"
#include "renderThread.h"
#include "asyncFileManager.h"
#include "gpuHeap.h"
#include "effectFileLoader.h"

//xgameRender
#include "tiledAA.h"
#include "primDraw2D.h"
#include "render.h"


#define COMPOSITE_EFFECT_FILENAME "terrain\\gpuTerrainComposite.bin"
//-------------------------------------------------------------------------
BTerrainTexturing gTerrainTexturing;
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
BTerrainTexturingRenderData::BTerrainTexturingRenderData():
   mCachedUniqueTexture(0)
{
}

//-------------------------------------------------------------------------
BTerrainTexturingRenderData::~BTerrainTexturingRenderData()
{
  freeDeviceData();
}

//-------------------------------------------------------------------------
void  BTerrainTexturingRenderData::freeDeviceData(void)
{
   mLayerContainer.freeDeviceData();

   if(mCachedUniqueTexture)
   {
	   mCachedUniqueTexture->free();
      mCachedUniqueTexture=NULL;
   }
}
//-------------------------------------------------------------------------



//-------------------------------------------------------------------------
BTerrainCompositeSurface::BTerrainCompositeSurface(void):
   mRenderSurface(0),
   mTempResolveTarget(0),
   mWidth(0),
   mHeight(0),
   mIsHDR(false)
{

}
//-------------------------------------------------------------------------
BTerrainCompositeSurface::~BTerrainCompositeSurface(void)
{
	freeDeviceData();
}
//-------------------------------------------------------------------------
void BTerrainCompositeSurface::freeDeviceData()
{
	if(mRenderSurface)
	{
		gRenderDraw.releaseD3DResource(mRenderSurface);
		mRenderSurface=NULL;
	}
	
	releaseTempResources();
}
//-------------------------------------------------------------------------
void BTerrainCompositeSurface::create(int width, int height,bool isHDR /*=false*/)
{
	mWidth=width;
	mHeight=height;
	mIsHDR=isHDR;

   D3DSURFACE_PARAMETERS colorParams = 
   { 
    //  gTiledAAManager.getTotalEDRAMUsed() 
      0
   };

   if(!isHDR)
   {
      gRenderDraw.createRenderTarget( width, height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, 0, &mRenderSurface, &colorParams);
   }
   else
   {
      gRenderDraw.createRenderTarget( width, height, D3DFMT_A2B10G10R10F_EDRAM, D3DMULTISAMPLE_NONE, 0, 0, &mRenderSurface, &colorParams);
   }
   
   allocateTempResources();
}
//-------------------------------------------------------------------------
void BTerrainCompositeSurface::allocateTempResources()
{
   BDEBUG_ASSERT((mWidth > 0) && (mHeight > 0));
   
   if (mTempResolveTarget)
      return;
      
   HRESULT hres;
      
   if(!mIsHDR)
   {
      //add 1 here so that our lowest mip has 2 mip levels
      hres = gGPUFrameHeap.createTexture(mWidth,mHeight,cNumMainCacheLevels+1,0,D3DFMT_A8R8G8B8,0,&mTempResolveTarget,0);
   }
   else
   {
      //add 1 here so that our lowest mip has 2 mip levels
      hres = gGPUFrameHeap.createTexture(mWidth,mHeight,2,0,D3DFMT_A16B16G16R16F_EXPAND,0,&mTempResolveTarget,0);
   }
   
   BVERIFYM(SUCCEEDED(hres),"GPU Frame Heap Allocation Failed!!");
}
//-------------------------------------------------------------------------
void BTerrainCompositeSurface::releaseTempResources(void)
{
   if (gEventDispatcher.getThreadIndex() != cThreadIndexRender)
      return;
      
   if (mTempResolveTarget)
   {
      gRenderDraw.unsetTextures();
      
      gGPUFrameHeap.releaseD3DResource(mTempResolveTarget);
      mTempResolveTarget = NULL;
   }      
}
//-------------------------------------------------------------------------





//-------------------------------------------------------------------------
BTerrainCachedCompositeTexture::BTerrainCachedCompositeTexture(void):
mpOwnerQuadNode(0),
mpOwnerCacheNode(0),
mHeight(0),
mWidth(0)
{
   for(int i=0;i<cNumCachedChannels;i++)
	   mTextures[i]=0;
}
//-------------------------------------------------------------------------
BTerrainCachedCompositeTexture::~BTerrainCachedCompositeTexture(void)
{
	freeDeviceData();
} 

//-------------------------------------------------------------------------
void  BTerrainCachedCompositeTexture::freeDeviceData(void)
{
   ASSERT_RENDER_THREAD
	for(int i=0;i<cNumCachedChannels;i++)
	{
		if(mTextures[i])
		{
			delete mTextures[i];
			mTextures[i]=NULL;
		}
	}
}
//-------------------------------------------------------------------------
void  BTerrainCachedCompositeTexture::free()
{
   gTerrainTexturing.freeCachedTexture(this);
};
//-------------------------------------------------------------------------
void  BTerrainCachedCompositeTexture::copyTo(BTerrainCachedCompositeTexture *output)
{
   BASSERT(0);

   if(this==output)
      return;

   output->mpOwnerQuadNode = mpOwnerQuadNode;
   output->mHeight = mHeight;
   output->mWidth = mWidth;

   for(int i=0;i<cNumCachedChannels;i++)
   {
      if(mWidth<=cTexSize360Cutoff && !gTerrainChannelParams[i].mSmallCacheFormat)
         continue;

      if(output->mTextures[i] && mTextures[i])    
         gTerrainTexturing.copyTexturesGPU(mTextures[i],output->mTextures[i],mWidth,mHeight);
   }
   
};
//-------------------------------------------------------------------------



//-------------------------------------------------------------------------
BTerrainLayerContainerTextureSetDecalData::BTerrainLayerContainerTextureSetDecalData()
{

}
//-------------------------------------------------------------------------
BTerrainLayerContainerTextureSetDecalData::~BTerrainLayerContainerTextureSetDecalData()
{
   mTextureSetDecalInstances.clear();
}
//-------------------------------------------------------------------------







//-------------------------------------------------------------------------
BTerrainLayerContainerDecalData::BTerrainLayerContainerDecalData():
mpPhysicalMemoryPtr(0),
mAlphaLayerTexture(0),
mActiveDecalIndexes(0)
{
}
//-------------------------------------------------------------------------
BTerrainLayerContainerDecalData::~BTerrainLayerContainerDecalData()
{
   freeDeviceData();
   if(mActiveDecalIndexes)
   {
      delete []mActiveDecalIndexes;
      mActiveDecalIndexes=NULL;
   }
}
//-------------------------------------------------------------------------
void BTerrainLayerContainerDecalData::freeDeviceData()
{
   if(mpPhysicalMemoryPtr)
   {
      XPhysicalFree(mpPhysicalMemoryPtr);
      mpPhysicalMemoryPtr=0;
   }

   if(mAlphaLayerTexture)
   {
      delete mAlphaLayerTexture;
      mAlphaLayerTexture=NULL;
   }
}
//-------------------------------------------------------------------------
int BTerrainLayerContainerDecalData::getNumLayers()
{
   return mNumLayers;
}
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
BTerrainLayerContainerSplatData::BTerrainLayerContainerSplatData():
mpPhysicalMemoryPtr(0),
mAlphaLayerTexture(0)
{
}
//-------------------------------------------------------------------------
BTerrainLayerContainerSplatData::~BTerrainLayerContainerSplatData()
{
   freeDeviceData();

  if(mLayerData)
  {
     delete [] mLayerData;
     mLayerData=NULL;
  }

}
//-------------------------------------------------------------------------
void BTerrainLayerContainerSplatData::freeDeviceData()
{
   if(mpPhysicalMemoryPtr)
   {
      XPhysicalFree(mpPhysicalMemoryPtr);
      mpPhysicalMemoryPtr=0;
   }

   if(mAlphaLayerTexture)
   {
      delete mAlphaLayerTexture;
      mAlphaLayerTexture=NULL;
   }
}
//-------------------------------------------------------------------------
int BTerrainLayerContainerSplatData::getNumLayers()
{
   return mNumLayers;
}
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
BTerrainTextureLayerContainer::BTerrainTextureLayerContainer():
mSpecPassNeeded(0),
mSelfPassNeeded(0),
mEnvMaskPassNeeded(0)
{
}
//-------------------------------------------------------------------------
BTerrainTextureLayerContainer::~BTerrainTextureLayerContainer()
{
   freeDeviceData();

   mSplatData.freeDeviceData();
   mDecalData.freeDeviceData();
  
}
//-------------------------------------------------------------------------
void BTerrainTextureLayerContainer::freeDeviceData()
{
  
}
//-------------------------------------------------------------------------
int BTerrainTextureLayerContainer::getNumSplatLayers()
{
   return mSplatData.getNumLayers();
}
//-------------------------------------------------------------------------
int BTerrainTextureLayerContainer::getNumDecalLayers()
{
   return mDecalData.getNumLayers();
}
//-------------------------------------------------------------------------
int BTerrainTextureLayerContainer::getNumTextureSetDecalLayers()
{
   return mTextureSetDecalData.getNumAddedLayers();
}

//-------------------------------------------------------------------------
int BTerrainTextureLayerContainer::getNumLayers()
{
   return mSplatData.getNumLayers() + mDecalData.getNumLayers();
}
//-------------------------------------------------------------------------







//-------------------------------------------------------------------------
BTerrainTexturing::BTerrainTexturing():
   BEventReceiver(),
   mAlbedoTexturesDirty(0),
   mNormalTexturesDirty(0),
   mSpecularTexturesDirty(0),
   mSelfTexturesDirty(0),
   mEnvMaskTexturesDirty(0),
   mDecalTexturesDirty(0),
   mDirID(0),
   mNextAsyncFileSetRequestID(1),
   m_pQuadVertexDecl(0),
   m_pQuadVertexBuffer(0),
   mTerrainEnvMapHDRScale(1.0f),
   mShowMips(false),
   mShowCache(false),
   mCacheDefragCounter(0),
#if 0   
   mpDXTGPUPacker(0),
#endif   
   mActiveTexInfo(0),
   mDoFreeCache(false),
   mActiveTextureSetsLoaded(0),
   mActiveDecals(0),
   mActiveDecalInfo(0),
   mActiveDecalInstanceInfo(0),
   mpEDRAMSaveTexture(NULL),
   mLODEnabled(true),
   mLargeUniqueAlbedoTexture(0),
   mpLargeUniqueAlbedoDataPointer(0),
   mLargeUniqueTextureWidth(0),
   mLargeUniqueTextureHeight(0),
   mLargeUniqueTextureMipCount(1),
   mpEffectLoader(NULL),
   mSpecExponentPower(25.0f),
   mSpecOnlyDir_Dir_Int(0,1,0,100),
   mSpecOnlyDir_Col_Shad(1,1,1,1),
   mBumpPower(1.0f),
   mBuildingSplatIndexUNSC(0),
   mBuildingSplatIndexCOVN(0),
   mSavedColorWriteEnable(D3DCOLORWRITEENABLE_ALL),
   mDynamicCacheMaxLOD(0),
   mCacheSkipSpecular(false),
   mCacheSkipEmissive(false),
   mCacheSkipEnvMap(false)
{
   mXTDTextureInfo.clear();
   Utils::ClearObj(mAsyncTextureLoadState);
 }

//-------------------------------------------------------------------------
BTerrainTexturing::~BTerrainTexturing()
{
}

//-------------------------------------------------------------------------
int BTerrainTexturing::getAlphaTextureWidth()
{
   return cAlphaTextureWidth;
}

//-------------------------------------------------------------------------
int BTerrainTexturing::getAlphaTextureHeight()
{
   return cAlphaTextureHeight;
}

//-------------------------------------------------------------------------
int  BTerrainTexturing::getMaxNumberMipLevels(void)
{
   return cNumMainCacheLevels;
}
//-------------------------------------------------------------------------
void BTerrainTexturing::init()
{
   ASSERT_THREAD(cThreadIndexSim);

   eventReceiverInit(cThreadIndexRender);

   commandListenerInit();
}
//-------------------------------------------------------------------------
void BTerrainTexturing::reloadInit(long dirID, const char* pBasePath, const XTTHeader& texNames)
{
   BReloadManager::BPathArray paths;

   //our splat textures
   for(int i=0;i<mXTDTextureInfo.mNumActiveTextures;i++)
   {
      BFixedString256 filename(pBasePath);
      filename += BFixedString256(cVarArg, "%s*.ddx", mActiveTexInfo[i].mFilename.c_str());

      BString reloadFilename;
      eFileManagerError result = gFileManager.getDirListEntry(reloadFilename, dirID);
      BVERIFY(cFME_SUCCESS == result);
      strPathAddBackSlash(reloadFilename);

      // huh?
      if (filename.get(0) == '\\')
         reloadFilename.append(filename.getPtr() + 1);
      else
         reloadFilename.append(filename.getPtr());

      paths.pushBack(reloadFilename);
   }

   if(mActiveDecalInfo)
   {
      //our decal textures
      for(int i=0;i<mXTDTextureInfo.mNumActiveDecals;i++)
      {
         BFixedString256 filename(pBasePath);
         filename += BFixedString256(cVarArg, "%s*.ddx", mActiveDecalInfo[i].mFilename.c_str());

         BString reloadFilename;
         eFileManagerError result = gFileManager.getDirListEntry(reloadFilename, dirID);
         BVERIFY(cFME_SUCCESS == result);
         strPathAddBackSlash(reloadFilename);

         // huh?
         if (filename.get(0) == '\\')
            reloadFilename.append(filename.getPtr() + 1);
         else
            reloadFilename.append(filename.getPtr());

         paths.pushBack(reloadFilename);
      }
   }
 
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, cTerrainTexturingReloadFileEvent, 0);
}   
//-------------------------------------------------------------------------
void BTerrainTexturing::initDeviceData()
{
   TRACEMEM
   
	D3DVERTEXELEMENT9 decl[] =
	{
		{ 0,  0, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION, 0 },
		{ 0,  8, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD, 0 },
      { 0,  12, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END()
	};
	BD3D::mpDev->CreateVertexDeclaration( decl, &m_pQuadVertexDecl );



   D3DXFLOAT16 negOne(-1.0f);
   D3DXFLOAT16 negHalf(-0.5f);
   D3DXFLOAT16 zer0(0.0f);
   D3DXFLOAT16 posQtr(0.25f);
   D3DXFLOAT16 posHalf(0.5f);
   D3DXFLOAT16 pos3Qtr(0.75f);
   D3DXFLOAT16 posOne(1.0f);

   //2nd UV channel to take into account guttered blending fixes
   float sizeScalar = 64;//128;
   float minFX = 1.0f / sizeScalar;
   float maxFX = 1.0f - minFX;
   float lenFX = (maxFX-minFX);
   float qtrFX = lenFX*0.25f + minFX;
   float hlfFX = lenFX*0.50f + minFX;
   float trdFX = lenFX*0.75f + minFX;

   float minFY = 1.0f / sizeScalar;
   float maxFY = 1.0f - minFY;
   
   D3DXFLOAT16 minFX16(minFX);
   D3DXFLOAT16 maxFX16(maxFX);
   D3DXFLOAT16 qtrFX16(qtrFX);
   D3DXFLOAT16 hlfFX16(hlfFX);
   D3DXFLOAT16 qt3FX16(trdFX);

   D3DXFLOAT16 minFY16(minFY);
   D3DXFLOAT16 maxFY16(maxFY);
   
   
   
   
   /*
   CLM [09.06.06] According to ATG, it's better to render vertical strips
   instead of one large quad. This minimizes number of texture cache misses while sampling
   ALSO, note that we're using D3DPT_RECTLIST for our draw, which takes 3 verts per quad, instead of 4
   it also has some other fastpath features that gets us small speed boosts.
   */


   mNumVertsInQuad=12;
   static const GPU_QUAD_VERTEX g_QuadVertices[12] = 
   {
      //  x       y        z    w            tu          tv
      { negOne,   posOne, zer0, posOne,      zer0,     zer0,      minFX16, minFY16},
      { negHalf,  posOne, zer0, posOne,      posQtr,   zer0,      qtrFX16, minFY16},
      //{ negHalf,  negOne, zer0, posOne,    posQtr,   posOne,    qtrFX16, maxFY16},
      { negOne,   negOne, zer0, posOne,      zer0,     posOne,    minFX16,maxFY16},


      { negHalf,   posOne, zer0, posOne,  posQtr,     zer0,       qtrFX16,minFY16},
      { zer0,      posOne, zer0, posOne,  posHalf,    zer0,       hlfFX16,minFY16},
   //   { zer0,      negOne, zer0, posOne,  posHalf,  posOne,     hlfFX16,maxFY16},
      { negHalf,   negOne, zer0, posOne,  posQtr,     posOne,     qtrFX16,maxFY16},
      

      { zer0,        posOne, zer0, posOne,  posHalf,     zer0,       hlfFX16,minFY16},
      { posHalf,      posOne, zer0, posOne,  pos3Qtr,    zer0,       qt3FX16,minFY16},
     // { posHalf,      negOne, zer0, posOne,  pos3Qtr,    posOne,   qt3FX16,maxFY16},
      { zer0,        negOne, zer0, posOne,  posHalf,     posOne,     hlfFX16,maxFY16},
      

      { posHalf,        posOne, zer0, posOne,  pos3Qtr,  zer0,       qt3FX16,minFY16},
      { posOne,         posOne, zer0, posOne,  posOne,   zer0,       maxFX16,minFY16},
     // { posOne,         negOne, zer0, posOne,  posOne,    posOne,  maxFX16,maxFY16},
      { posHalf,        negOne, zer0, posOne,  pos3Qtr,  posOne,     qt3FX16,maxFY16},
      

   };

   
   BD3D::mpDev->CreateVertexBuffer(cNumMaxBlendsDoneAtOnce * sizeof(GPU_QUAD_VERTEX) * mNumVertsInQuad,0,0,0,&m_pQuadVertexBuffer,0);
   void *pvDev=NULL;
   int copySize = sizeof(GPU_QUAD_VERTEX)*mNumVertsInQuad;
   m_pQuadVertexBuffer->Lock(0,0,&pvDev,0);
   GPU_QUAD_VERTEX *vWalk = (GPU_QUAD_VERTEX*)pvDev;

   for(int i=0;i<cNumMaxBlendsDoneAtOnce;i++)
   {
      memcpy(vWalk, g_QuadVertices,copySize);
      vWalk+=mNumVertsInQuad;
   }
      
   m_pQuadVertexBuffer->Unlock();

   // Create texture to hold EDRAM contents during dumping. It sucks to have to create this texture, we should be able to alias it overtop of something else.
   // FIXME: use the gpu heap to allocate this texture as needed
   gRenderDraw.createTexture(cUniqueTextureWidth, cUniqueTextureHeight, 1, 0, D3DFMT_A8R8G8B8, 0, &mpEDRAMSaveTexture, NULL);
   
	loadEffect();
		   	
	TRACEMEM

}
//-------------------------------------------------------------------------
void BTerrainTexturing::deinit()
{
   ASSERT_THREAD(cThreadIndexSim);
   
   commandListenerDeinit();
               
   eventReceiverDeinit();

}

//-------------------------------------------------------------------------
void BTerrainTexturing::destroy()
{
   ASSERT_THREAD(cThreadIndexSim);

   gAsyncFileManager.syncAll();


   gRenderThread.submitCommand(mCommandHandle,cTTC_Destroy);

   gReloadManager.deregisterClient(mEventHandle);
}

//-------------------------------------------------------------------------
void BTerrainTexturing::destroyInternal()
{
   ASSERT_THREAD(cThreadIndexRender);

   
   destroyCaches();

   for(int i=0;i<cTextureTypeMax;i++)
   {
      mActiveTextures[i].mD3DTextureArrayHandle.release();
      if(mActiveTextures[i].mHDRScales)
      {
         delete []mActiveTextures[i].mHDRScales;
         mActiveTextures[i].mHDRScales = NULL;
      }
   }   

   if(mActiveTexInfo)
   {
      delete [] mActiveTexInfo;
      mActiveTexInfo=NULL;
   }
  

   for (uint i = 0; i < cTextureTypeMax; i++)
   {
      delete mAsyncTextureLoadState[i].mpTextureLoader;
      mAsyncTextureLoadState[i].mpTextureLoader = NULL;

      mAsyncTextureLoadState[i].mValidLoadedTexture.clear();

     // if(mAsyncTextureLoadState[i].mValidLoadedTexture)
      {
      //   delete mAsyncTextureLoadState[i].mpValidLoadedTexture;
       //  mAsyncTextureLoadState[i].mpValidLoadedTexture[i] = NULL;
      }
   }
  
   Utils::ClearObj(mAsyncTextureLoadState);

   mTerrainEnvMapTexture.release();
   mActiveTextureSetsLoaded=0;

   if(mActiveDecals)
   {
      for(int i=0;i<mXTDTextureInfo.mNumActiveDecals;i++)
      {
         for(int q=0;q<cTextureTypeMax;q++)
         {
//            if(mActiveDecals[i].mTextures[q])
            {
               mActiveDecals[i].mTextures[q].release();
            }
         }     
      }

      delete [] mActiveDecals;
      mActiveDecals=NULL;

      if(mActiveDecalInfo)
      {
         delete mActiveDecalInfo;
         mActiveDecalInfo=NULL;
      }
      if(mActiveDecalInstanceInfo)
      {
         delete mActiveDecalInstanceInfo;
         mActiveDecalInstanceInfo=NULL;
      }
   }

   if(mpLargeUniqueAlbedoDataPointer)
   {
      XPhysicalFree(mpLargeUniqueAlbedoDataPointer);
      mpLargeUniqueAlbedoDataPointer = NULL;

      if(mLargeUniqueAlbedoTexture)
      {
         delete mLargeUniqueAlbedoTexture;
         mLargeUniqueAlbedoTexture = NULL;
      }
     
   }
}




//-------------------------------------------------------------------------
void BTerrainTexturing::deinitDeviceData()
{
   if (mpEffectLoader)
   {
      ALIGNED_DELETE(mpEffectLoader, gRenderHeap);
      mpEffectLoader = NULL;
   }
   
   mCompShader.clear();

	if(m_pQuadVertexDecl)
	{
		m_pQuadVertexDecl->Release();
		m_pQuadVertexDecl=NULL;
	}
   if(m_pQuadVertexBuffer)
   {
      m_pQuadVertexBuffer->Release();
      m_pQuadVertexBuffer=NULL;
   }
		
	if (mpEDRAMSaveTexture)
	{
	   gRenderDraw.releaseD3DResource(mpEDRAMSaveTexture);
	   mpEDRAMSaveTexture = NULL;
	}
}
//-------------------------------------------------------------------------
void BTerrainTexturing::beginLevelLoad()
{
   destroyCaches();
}
//-------------------------------------------------------------------------
void BTerrainTexturing::endLevelLoad()
{                                      
   initCaches(mCacheSkipSpecular,mCacheSkipEmissive,mCacheSkipEnvMap);
}
//-------------------------------------------------------------------------
int BTerrainTexturing::computeUniqueTextureLOD(XMVECTOR minBB, XMVECTOR maxBB,const XMVECTOR camPos, float &distFromCam )
{  
   if (!mLODEnabled)
      return 0;
      
   
   XMVECTOR bbCenter = (minBB+maxBB)*0.5f;
   XMVECTOR dist = XMVector4Length(bbCenter - camPos);

   XMFLOAT4 distf4;
   XMStoreFloat4(&distf4,dist);
   distFromCam =distf4.x;

   for(int i=0;i<cTotalNumberOfLevels;i++)
   {
      XMFLOAT4 a(gTextureLODDistance[i],gTextureLODDistance[i],gTextureLODDistance[i],gTextureLODDistance[i]);
      XMVECTOR top = XMLoadFloat4(&a);
      if(XMVector4LessOrEqual(dist,top))
         return i;
   }

   return cTotalNumberOfLevels-1;
}


//-------------------------------------------------------------------------
BTerrainCachedCompositeTexture* BTerrainTexturing::getAvailableCachedTexture(int LODLevel)
{ 

   //[11.19.07] CLM - Allow us to set the max texture size dynamically..
   if(LODLevel < mDynamicCacheMaxLOD )
      LODLevel = mDynamicCacheMaxLOD;

   BTextureCacheNode *availNode =NULL;
 //  if(cUniqueTextureWidth >> LODLevel > cTexSize360Cutoff)
      availNode= mpMainCache->getAvailableCachedTexture( cUniqueTextureWidth>>LODLevel,cUniqueTextureHeight>>LODLevel);
 //  else //if(cUniqueTextureWidth >> LODLevel <= cTexSize360Cutoff)
 //     availNode= mpSmallCache->getAvailableCachedTexture( cUniqueTextureWidth>>LODLevel,cUniqueTextureHeight>>LODLevel);
   if(availNode)
   {
      decoupleCachedTexture(availNode->mTexture);
      return availNode->mTexture;
   }
   return NULL;
   
}
//-------------------------------------------------------------------------
void BTerrainTexturing::freeCachedTexture(BTerrainCachedCompositeTexture *tex)
{
   if(!tex)
      return;

   decoupleCachedTexture(tex);

 //  if(tex->mWidth > cTexSize360Cutoff)
      mpMainCache->setCachedNodeState(((BTextureCacheNode*)tex->mpOwnerCacheNode),cCS_Free);
 //  else
 //     mpSmallCache->setCachedNodeState(((BTextureCacheNode*)tex->mpOwnerCacheNode),cCS_Free);
   
   
}
//-------------------------------------------------------------------------
void BTerrainTexturing::decoupleCachedTexture(BTerrainCachedCompositeTexture *tex)
{
   if(!tex)
      return;
   if(tex->mpOwnerCacheNode)
   {
      //((BTextureCacheNode*)tex->mpOwnerCacheNode)->decouple()   ;
      if(tex->mpOwnerQuadNode)
      {
         if(tex->mpOwnerQuadNode->mRenderPacket)
         {
            tex->mpOwnerQuadNode->mRenderPacket->mTexturingData->mCachedUniqueTexture = NULL;
         }
         tex->mpOwnerQuadNode=NULL;
      }

     BTextureCacheNode *cn = (BTextureCacheNode*)tex->mpOwnerCacheNode;

     if(cn->mMainCacheLevelIndex+1 >=cNumMainCacheLevels)
        return ;

      int lowerLevelIndex = cn->mIndexInParentContainer<<2;
      for(int k=0;k<4;k++)
      {
         int tt = lowerLevelIndex+k;
        // if(tex->mWidth > cTexSize360Cutoff)
            decoupleCachedTexture(mpMainCache->getNode(cn->mMainCacheLevelIndex+1,tt)->mTexture);
      //   else
       //     decoupleCachedTexture(mpSmallCache->getNode(cn->mMainCacheLevelIndex+1,tt)->mTexture);
      }
   }

  
}
//-------------------------------------------------------------------------
void BTerrainTexturing::holdCachedTexture(BTerrainCachedCompositeTexture *tex)
{
   if(!tex)
      return;
  // if(tex->mWidth > cTexSize360Cutoff)
      mpMainCache->setCachedNodeState(((BTextureCacheNode*)tex->mpOwnerCacheNode),cCS_UsedThisFrame);
 //  else
 //     mpSmallCache->setCachedNodeState(((BTextureCacheNode*)tex->mpOwnerCacheNode),cCS_UsedThisFrame);

}
//-------------------------------------------------------------------------
void BTerrainTexturing::unholdCachedTexture(BTerrainCachedCompositeTexture *tex)
{
   useCachedTexture(tex);
}
//-------------------------------------------------------------------------
void BTerrainTexturing::useCachedTexture(BTerrainCachedCompositeTexture *tex)
{
   if(!tex)
      return;
 //  if(tex->mWidth > cTexSize360Cutoff)
      mpMainCache->setCachedNodeState(((BTextureCacheNode*)tex->mpOwnerCacheNode),cCS_Used);
 //  else
 //     mpSmallCache->setCachedNodeState(((BTextureCacheNode*)tex->mpOwnerCacheNode),cCS_Used);
}

//-------------------------------------------------------------------------
bool BTerrainTexturing::isTextureInCache(BTerrainCachedCompositeTexture *tex)
{
   if(!tex)
      return false;
 //  if(tex->mWidth > cTexSize360Cutoff)
      return mpMainCache->isStillInCache(((BTextureCacheNode*)tex->mpOwnerCacheNode));
 //  else
 //     return mpSmallCache->isStillInCache(((BTextureCacheNode*)tex->mpOwnerCacheNode));
}
//-------------------------------------------------------------------------
BTerrainCompositeSurface *BTerrainTexturing::getRenderSurface(bool isHDR/*=false*/)
{
   if(!isHDR)
   {
      if(!mSharedSurface )
      {
         mSharedSurface = new BTerrainCompositeSurface();
         mSharedSurface->create(cUniqueTextureWidth,cUniqueTextureHeight,false);
      }
      return mSharedSurface;
   }
   

   
   if(!mSharedHDRSurface )
   {
      mSharedHDRSurface = new BTerrainCompositeSurface();
      mSharedHDRSurface->create(cUniqueTextureWidth,cUniqueTextureHeight,true);
   }
   return mSharedHDRSurface;
   

   
}


//-------------------------------------------------------------------------
void BTerrainTexturing::freeAllCache(void)
{
   if(mpMainCache)
	   mpMainCache->freeCache();
  // if(mpSmallCache)
  //    mpSmallCache->freeCache();
}
//-------------------------------------------------------------------------
void BTerrainTexturing::initCaches(bool skipSpecular, bool skipEmissive, bool skipEnvMap)
{
   TRACEMEM
   SCOPEDSAMPLE(BTerrainTexturing_initCaches)

   if(!gTerrain.getLoadSuccessful())
      return;

   mpMainCache = new BTextureCache();

   BCacheCreationParams params;
   params.mNumChannels           = cNumCachedChannels;
   for(int i=0;i<cNumCachedChannels;i++)
      params.mChannelFormats.add(gTerrainChannelParams[i].mMainCacheFormat);
   if(skipSpecular)
      params.mChannelFormats[cTextureTypeSpecular]=(D3DFORMAT)0;
   if(skipEmissive)
      params.mChannelFormats[cTextureTypeSelf]=(D3DFORMAT)0;
   if(skipEnvMap)
      params.mChannelFormats[cTextureTypeEnvMask]=(D3DFORMAT)0;
   

   params.mReverseDirection.add(true);
   params.mReverseDirection.add(false);
   params.mNumCachePages         = cMaxNumCachePages;
   params.mNumDiscreteLevels     = cNumMainCacheLevels;
   params.mNumMipsAtEachLevel    = cNumMainTextureNumMips;
   params.mMip0Width             = cUniqueTextureWidth;
   params.mAllowFastCache        = false;
      

   
   mpMainCache->create(params);

   
  /* mpSmallCache = new BTextureCache();
   params.mChannelFormats.clear();
   params.mReverseDirection.clear();
   params.mNumChannels           = cNumCachedChannels;
   for(int i=0;i<cNumCachedChannels;i++)
      params.mChannelFormats.add(gTerrainChannelParams[i].mSmallCacheFormat);

   if(skipEmissive)
      params.mChannelFormats[cTextureTypeSelf]=(D3DFORMAT)0;
   if(skipEnvMap)
      params.mChannelFormats[cTextureTypeEnvMask]=(D3DFORMAT)0;

   params.mReverseDirection.add(true);
   params.mReverseDirection.add(false);
   params.mNumCachePages         = cMaxNumSmallCachePages;
   params.mNumDiscreteLevels     = cNumSmallCacheLevels;
   params.mNumMipsAtEachLevel    = cNumSmallTextureNumMips;
   params.mMip0Width             = cTexSize360Cutoff;
   params.mAllowFastCache        = true;


   mpSmallCache->create(params);*/
   
   TRACEMEM
}
//-------------------------------------------------------------------------
void BTerrainTexturing::destroyCaches()
{
   if(mpMainCache)
   {
      mpMainCache->destroy();
      delete mpMainCache;
      mpMainCache=NULL;
   }
  /* if(mpSmallCache)
   {
      mpSmallCache->destroy();
      delete mpSmallCache;
      mpSmallCache=NULL;
   }*/
   

   if(mSharedSurface)
   {
      mSharedSurface->freeDeviceData();
      delete mSharedSurface;
      mSharedSurface=NULL;
   }
   if(mSharedHDRSurface)
   {
      mSharedHDRSurface->freeDeviceData();
      delete mSharedHDRSurface;
      mSharedHDRSurface=NULL;
   }
   
#if 0
   if(mpDXTGPUPacker)
   {
      mpDXTGPUPacker->deinit();
      delete mpDXTGPUPacker;
      mpDXTGPUPacker=NULL;
   }
#endif   
}
//-------------------------------------------------------------------------
void BTerrainTexturing::preCompositeSaveEDRAM()
{
#ifdef BUILD_DEBUG   
   IDirect3DSurface9* pCurSurf;
   BD3D::mpDev->GetRenderTarget(0, &pCurSurf);
   pCurSurf->Release();
   
   D3DSURFACE_DESC desc;
   pCurSurf->GetDesc(&desc);
   BDEBUG_ASSERT((desc.Width == cUniqueTextureWidth) && (desc.Height == cUniqueTextureHeight) && (desc.Format == D3DFMT_A8R8G8B8));
#endif

   BD3D::mpDev->Resolve(
      D3DRESOLVE_RENDERTARGET0,
      NULL,
      mpEDRAMSaveTexture,
      NULL,
      0,
      0,
      NULL,
      1.0f,
      0,
      NULL);
}
//-------------------------------------------------------------------------
void BTerrainTexturing::preCompositeRestoreEDRAM()
{
   BTerrainCompositeSurface *surf=gTerrainTexturing.getRenderSurface();	
   BDEBUG_ASSERT(surf->mRenderSurface && surf->mTempResolveTarget);
   
   BD3D::mpDev->SetRenderTarget( 0, surf->mRenderSurface );
   
   BD3D::mpDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
   BD3D::mpDev->SetVertexDeclaration( m_pQuadVertexDecl );
   BD3D::mpDev->SetStreamSource(0,m_pQuadVertexBuffer,0,sizeof(GPU_QUAD_VERTEX));
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, false);
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, false);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,false);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,false);
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, false);
   
   DWORD prevHalfPixelOffset;
   BD3D::mpDev->GetRenderState(D3DRS_HALFPIXELOFFSET, &prevHalfPixelOffset);
   BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, false);
   
   DWORD prevFillmode;
   BD3D::mpDev->GetRenderState(D3DRS_FILLMODE, &prevFillmode);
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
   
   BD3D::mpDev->SetTexture(0, mpEDRAMSaveTexture);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);

   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);
   
   BPrimDraw2D::drawSolidRect2D(0, 0, cUniqueTextureWidth, cUniqueTextureHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0, cPosTex1VS, cTex1PS);  
   
   BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, prevHalfPixelOffset);
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, prevFillmode);
}
//-------------------------------------------------------------------------
void BTerrainTexturing::preCompositeSetup()
{
   BD3D::mpDev->GetRenderState(D3DRS_COLORWRITEENABLE, &mSavedColorWriteEnable);
   BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
   
#if 0   
   //CLM PLACED HERE BECAUSE TILEDAAMANAGER HAS TO BE INITALIZED FIRST
   if(!mpDXTGPUPacker)
   {
      mpDXTGPUPacker = new BGPUDXTPack();
      mpDXTGPUPacker->init(cUniqueTextureWidth);
   }
#endif   
        
   BD3D::mpDev->GetViewport(&mSavedViewPort);
   BD3D::mpDev->GetRenderTarget( 0, &mpRenderTarget0 );
   BD3D::mpDev->GetDepthStencilSurface(&mpDepthStencil0);
   
   // Set the render target to be our offscreen texture
   BTerrainCompositeSurface *surf=gTerrainTexturing.getRenderSurface();	
   BDEBUG_ASSERT(surf->mRenderSurface && surf->mTempResolveTarget);
   
   BD3D::mpDev->SetRenderTarget( 0, surf->mRenderSurface );
   BD3D::mpDev->SetDepthStencilSurface(NULL);

   //BD3D::mpDev->SetShaderGPRAllocation(0, 32, 96);
   BD3D::mpDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
   BD3D::mpDev->SetVertexDeclaration( m_pQuadVertexDecl );
   BD3D::mpDev->SetStreamSource(0,m_pQuadVertexBuffer,0,sizeof(GPU_QUAD_VERTEX));
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, false);
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, false);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
   BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, true);
   
   // FIXME: Only save EDRAM if something has been rendered in the 3d pass already!
  // preCompositeSaveEDRAM();

#ifdef BUILD_DEBUG   
   //  BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, TRUE); //CLM [08.23.06] this is being set by the xGameRender\tiledAA.cpp 
   DWORD val;
   BD3D::mpDev->GetRenderState(D3DRS_HALFPIXELOFFSET, &val);
   //if(!val)
  //    BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, TRUE);
   BDEBUG_ASSERT(val == TRUE && "D3DRS_HALFPIXELOFFSET not set for Cache compositing");
#endif 

   tickEffect();

   mCompShader.updateIntrinsicParams();
}
//-------------------------------------------------------------------------
void BTerrainTexturing::postCompositeSetup()
{
  // preCompositeRestoreEDRAM();
   
   BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, mSavedColorWriteEnable);

   if(mpRenderTarget0)
   {
      BD3D::mpDev->SetRenderTarget( 0, mpRenderTarget0 );
      mpRenderTarget0->Release();
      mpRenderTarget0=NULL;
   }

   if(mpDepthStencil0)
   {
      BD3D::mpDev->SetDepthStencilSurface(mpDepthStencil0);
      mpDepthStencil0->Release();
      mpDepthStencil0=NULL;
   }
   

   BD3D::mpDev->SetViewport(&mSavedViewPort);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,false);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, true);
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, true);
   //   BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, FALSE); //CLM [08.23.06] -this is being set to by the xGameRender\tiledAA.cpp 
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, false);


   //BD3D::mpDev->SetShaderGPRAllocation(0, 64, 64);
}
//-------------------------------------------------------------------------
void BTerrainTexturing::allocateTempResources()
{
   // Allocate any resources that are only temporarily needed while rendering terrain.
   if (mSharedSurface)
      mSharedSurface->allocateTempResources();
      
   if (mSharedHDRSurface)
      mSharedHDRSurface->allocateTempResources();
}
//-------------------------------------------------------------------------
void BTerrainTexturing::releaseTempResources()
{
   // Release any resources that are only temporarily needed while rendering terrain.
   if (mSharedSurface)
      mSharedSurface->releaseTempResources();

   if (mSharedHDRSurface)
      mSharedHDRSurface->releaseTempResources();
}

//-------------------------------------------------------------------------
void BTerrainTexturing::copyTexturesGPU(LPDIRECT3DTEXTURE9 src, LPDIRECT3DTEXTURE9 dst,int width, int height)
{

   if(src==dst)
      return;

   tickEffect();
                 
//CLM Right now this assumes we'll always be copying DXT1 / DXT5 textures
   XGTEXTURE_DESC srcTexDesc;
   XGTEXTURE_DESC dstTexDesc;
        
    mCurrTechnique = mCompShader.getTechnique("copyTextures");//getTechniqueFromIndex(1);

    mCurrTechnique.beginRestoreDefaultState();
    mCurrTechnique.beginPass(0);
    mCurrTechnique.commit();

    
    bool isSmallCache = width <=cTexSize360Cutoff;
    int numMips = isSmallCache?cNumSmallTextureNumMips:cNumMainTextureNumMips;

    for(int i=0;i<numMips;i++)
   {
      XGGetTextureDesc(src, i, &srcTexDesc);
      XGGetTextureDesc(dst, i, &dstTexDesc);
      int numVertices = srcTexDesc.WidthInBlocks * srcTexDesc.HeightInBlocks;
      int memSize = numVertices * srcTexDesc.BytesPerBlock;

      VOID* pDestTexData = NULL;      
      VOID* pSrcTexData = NULL;  
      if (!i)
      {
         pDestTexData = reinterpret_cast<BYTE*>((DWORD)dst->Format.BaseAddress << 12U);
      }
      else
      {
         const UINT mipLevelOffset = XGGetMipLevelOffset(dst, 0, i);
         BDEBUG_ASSERT(dst->Format.MipAddress);
         pDestTexData = reinterpret_cast<BYTE*>(((DWORD)dst->Format.MipAddress << 12U) + mipLevelOffset);
      }

      //find our target dst texture memory address
      if (!i)
      {
         pSrcTexData = reinterpret_cast<BYTE*>((DWORD)src->Format.BaseAddress << 12U);
      }
      else
      {
         const UINT mipLevelOffset = XGGetMipLevelOffset(src, 0, i);
         BDEBUG_ASSERT(src->Format.MipAddress);
         pSrcTexData = reinterpret_cast<BYTE*>(((DWORD)src->Format.MipAddress << 12U) + mipLevelOffset);
      }


      IDirect3DVertexBuffer9 *vb=new IDirect3DVertexBuffer9();
      XGSetVertexBufferHeader(memSize, 0, 0,0,vb);
      XGOffsetResourceAddress(vb, pSrcTexData);
      
      
      
      // Notify the device that an export is beginning into the given vertex buffer
#if _XDK_VER >= 6274         
      BD3D::mpDev->BeginExport( 0, dst, D3DBEGINEXPORT_VERTEXSHADER );
#else
      BD3D::mpDev->BeginExport( 0, dst );
#endif      

      //set dummy vb
      BD3D::mpDev->SetVertexDeclaration(BTLVertex::msVertexDecl);
      BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);

      BD3D::mpDev->SetVertexFetchConstant(0, vb, 0);

      //grab 2 blocks at one time
      if (srcTexDesc.BytesPerBlock == 1)
      numVertices = numVertices >>4;
      else if (srcTexDesc.BytesPerBlock == 2)
      numVertices = numVertices >> 3;
      else if (srcTexDesc.BytesPerBlock == 4)
      numVertices = numVertices >> 2;
      else if (srcTexDesc.BytesPerBlock == 8)
         numVertices = numVertices >> 1;
      else if (srcTexDesc.BytesPerBlock == 16)
         numVertices = numVertices;
      else
      {
         BDEBUG_ASSERT(0);
      }

      // Set up the Memory Export Stream Constant using the macro defined in d3d9gpu.h
      GPU_MEMEXPORT_STREAM_CONSTANT streamConstant;
      GPU_SET_MEMEXPORT_STREAM_CONSTANT(&streamConstant,
         ((BYTE *)pDestTexData),                 // pointer to the data
         numVertices,                             // max index = # of vertices * stride
         SURFACESWAP_LOW_RED,                           // whether to output ABGR or ARGB           
         GPUSURFACENUMBER_FLOAT,                      // data type 
         GPUCOLORFORMAT_32_32_32_32_FLOAT,                     // data format
         GPUENDIAN128_8IN32);                            // endian swap

      
      BD3D::mpDev->SetVertexShaderConstantF( 4, streamConstant.c, 1 );
      BD3D::mpDev->DrawPrimitive( D3DPT_POINTLIST, 0, numVertices );

      //CLM turn this on for debugging....
     // BD3D::mpDev->Present(0,0,0,0);

   
      BD3D::mpDev->EndExport( 0, dst, 0 );
      
      delete vb;
   }
   
   mCurrTechnique.endPass();
   mCurrTechnique.end();

   mCurrTechnique		= mCompShader.getTechniqueFromIndex(0);
}
//-------------------------------------------------------------------------
void BTerrainTexturing::setEnvMapTexture(int dirID, const char *filename)
{
   ASSERT_THREAD(cThreadIndexSim);


   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

   pPacket->setFilename(filename);
   pPacket->setDirID(dirID);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainTexturing::loadEnvMapTextureCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   // pPacket->setPrivateData0(i);
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);




}
//----------------------------------------
void BTerrainTexturing::loadEnvMapTextureCallback(void* pData)
{
   BAsyncFileManager::BRequestPacket* pPacket = static_cast<BAsyncFileManager::BRequestPacket*>(pData);

   if (!pPacket->getSucceeded())
   {
      BString buf;
      buf.format("BTerrainTexturing::loadTextureCallback: Unable to read file %s", pPacket->getFilename().c_str());
      gConsoleOutput.output(cMsgError, buf);

      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }

   BD3DTextureLoader* mpTextureLoader = new BD3DTextureLoader();

   BD3DTextureLoader::BCreateParams textureLoadParams;
   textureLoadParams.mBigEndian = true;
   textureLoadParams.mTiled = true;
   textureLoadParams.mManager = "TerrainTexturing";
   textureLoadParams.mName = pPacket->getFilename();

   bool status;
   status = mpTextureLoader->createFromDDXFileInMemory(static_cast<const uchar*>(pPacket->getData()), pPacket->getDataLen(), textureLoadParams);

   if (!status)
   {
      BString buf;
      buf.format("BTerrainTexturing::loadTextureCallback: Unable to read file %s", pPacket->getFilename().c_str());
      gConsoleOutput.output(cMsgError, buf);

      delete mpTextureLoader;
      mpTextureLoader = NULL;

      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }

   mTerrainEnvMapTexture.release();
   mTerrainEnvMapTexture = mpTextureLoader->getD3DTexture();
   mTerrainEnvMapHDRScale = mpTextureLoader->getHDRScale();
   mpTextureLoader->releaseOwnership();

   delete mpTextureLoader;
   mpTextureLoader = NULL;


   gAsyncFileManager.deleteRequestPacket(pPacket);

}
//-------------------------------------------------------------------------
void BTerrainTexturing::loadTextureCallback(void* pData)
{
   //SCOPEDSAMPLE(BTerrainTexturing_loadTextureCallback)
   BAsyncFileManager::BRequestPacket* pPacket = static_cast<BAsyncFileManager::BRequestPacket*>(pData);

   const uint textureIndex = pPacket->getPrivateData0();
   const uint textureType = pPacket->getPrivateData1() & 0xF;
   BDEBUG_ASSERT(textureType < cTextureTypeMax);
   const uint requestID = pPacket->getPrivateData1() >> 4;
   BAsyncTextureLoadState& state = mAsyncTextureLoadState[textureType];

   state.mValidLoadedTexture.resize(mXTDTextureInfo.mNumActiveTextures);

   if (state.mRequestID != requestID)
   {
      delete state.mpTextureLoader;
      state.mpTextureLoader = new BD3DTextureLoader;
      state.mRequestID = requestID;
      state.mNumLoadedTextures = 0;
   }
   else if (!state.mpTextureLoader)
   {
      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }

   if (!pPacket->getSucceeded())
   {
      BString buf;
      buf.format("BTerrainTexturing::loadTextureCallback: Unable to read file %s", pPacket->getFilename().c_str());
      gConsoleOutput.output(cMsgError, buf);
      state.mValidLoadedTexture[textureIndex]=false;
      
/*
      delete state.mpTextureLoader;
      state.mpTextureLoader = NULL;   

      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
      */
   }
   else
   {
      state.mValidLoadedTexture[textureIndex]=true;

      BD3DTextureLoader::BCreateParams textureLoadParams;
      textureLoadParams.mArraySize = (uchar)mXTDTextureInfo.mNumActiveTextures;
      textureLoadParams.mBigEndian = true;
      textureLoadParams.mTiled = true;
      textureLoadParams.mArrayIndex = static_cast<uchar>(textureIndex);
      textureLoadParams.mManager = "TerrainTexturing";
      textureLoadParams.mName = pPacket->getFilename();

      BCOMPILETIMEASSERT(sizeof(gTerrainChannelParams) / sizeof(gTerrainChannelParams[0]) == cTextureTypeMax);
      if (gTerrainChannelParams[textureType].mArraySRGBRead)
         textureLoadParams.mForceSRGBGamma = true;

      bool status;
      status = state.mpTextureLoader->createFromDDXFileInMemory(static_cast<const uchar*>(pPacket->getData()), pPacket->getDataLen(), textureLoadParams);

      if (!status)
      {
         BString buf;
         buf.format("BTerrainTexturing::loadTextureCallback: Unable to read file %s", pPacket->getFilename().c_str());
         gConsoleOutput.output(cMsgError, buf);

         state.mValidLoadedTexture[textureIndex]=false;
/*
         delete state.mpTextureLoader;
         state.mpTextureLoader = NULL;

         gAsyncFileManager.deleteRequestPacket(pPacket);
         
         return;*/
      }
   }

   state.mNumLoadedTextures++;

   if (state.mNumLoadedTextures == (uint)mXTDTextureInfo.mNumActiveTextures)
   {
      
      //quick out. If none of the textures for his array loaded, then don't create it..
      bool validArray=false;
      for(uint i=0;i<(uint)mXTDTextureInfo.mNumActiveTextures;i++)
         validArray |=state.mValidLoadedTexture[i];
      

      if(validArray)
      {
         mActiveTextures[textureType].mD3DTextureArrayHandle = state.mpTextureLoader->getD3DTexture();

         //Did we have ANY of this type?
         if(!mActiveTextures[textureType].mD3DTextureArrayHandle.getBaseTexture())
         {
            IDirect3DArrayTexture9* pArrayTex;
            gRenderDraw.createArrayTexture(cUniqueTextureWidth, cUniqueTextureHeight, mXTDTextureInfo.mNumActiveTextures, 1, 0, D3DFMT_DXT1, D3DPOOL_DEFAULT, &pArrayTex, NULL);
            mActiveTextures[textureType].mD3DTextureArrayHandle.setArrayTexture(pArrayTex, BD3DTexture::cD3D);
         }

         state.mpTextureLoader->releaseOwnership();

         if (textureType == cTextureTypeSelf)
         {
            mActiveTextures[textureType].mHDRScales = new D3DXVECTOR4[mXTDTextureInfo.mNumActiveTextures];
            const float* pHDRVals = state.mpTextureLoader->getArrayHDRScale();
            if(state.mpTextureLoader->getArrayHDRScale())
            {
               for(int k=0;k<mXTDTextureInfo.mNumActiveTextures;k++)
               {
                  mActiveTextures[textureType].mHDRScales[k].x=pHDRVals[k];
                  mActiveTextures[textureType].mHDRScales[k].y =0;
                  mActiveTextures[textureType].mHDRScales[k].z =0;
                  mActiveTextures[textureType].mHDRScales[k].w =0;
               }
            }
            else
            {
               memset(mActiveTextures[textureType].mHDRScales,0,sizeof(D3DXVECTOR4)*mXTDTextureInfo.mNumActiveTextures);
            }
         }

         
   #ifndef BUILD_FINAL
         //sum up memory for input textures

         int numMips = mActiveTextures[textureType].mD3DTextureArrayHandle.getArrayTexture()->GetLevelCount();
         XGTEXTURE_DESC desc;
         XGGetTextureDesc(mActiveTextures[textureType].mD3DTextureArrayHandle.getArrayTexture(),0,&desc);
         int mip0Count = desc.SlicePitch ;
         int MemTaken = mip0Count;
         for(int k=0;k<numMips;k++)
         {
            mip0Count = mip0Count>>2;
            MemTaken+=mip0Count;
         }

         BTerrainMetrics::addArtistTerrainTextureGPUMem(MemTaken * mXTDTextureInfo.mNumActiveTextures,textureType);
   #endif

         // rg - Shouldn't be needed now, because the valley allocator always clears texture memory to all 0's.
#if 0
         //if we have any non-valid slices, fill them appropriatly.
         for(int i=0;i<mXTDTextureInfo.mNumActiveTextures;i++)
         {
     
       if(!state.mValidLoadedTexture[i])  //this texture did not load, fill it.
            {
               gRenderDraw.clearTextureData(mActiveTextures[textureType].mD3DTextureArrayHandle.getBaseTexture());
            }
         }
#endif         
      }

      delete state.mpTextureLoader;
      state.mpTextureLoader = NULL;

      mDoFreeCache=true;
      mActiveTextureSetsLoaded++;
      
   }  

   gAsyncFileManager.deleteRequestPacket(pPacket);
}   

//-------------------------------------------------------------------------
bool BTerrainTexturing::loadTerrainTextureArray(long dirID, const char* pBasePath, const char* pFileSuffix, const XTTHeader& texNames, uint textureType)
{
   ASSERT_THREAD(cThreadIndexRender);
      
   BReloadManager::BPathArray paths;
         
   for(int i=0;i<mXTDTextureInfo.mNumActiveTextures;i++)
   {
      BFixedString256 filename(pBasePath);
      filename += BFixedString256(cVarArg, "%s%s.ddx", mActiveTexInfo[i].mFilename.c_str(), pFileSuffix);
                                    
      BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

      pPacket->setFilename(filename.c_str());
      pPacket->setDirID(dirID);
      pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainTexturing::loadTextureCallback));
      pPacket->setCallbackThreadIndex(cThreadIndexRender);
      pPacket->setPrivateData0(i);
      pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
      pPacket->setPriority(-3);
    //  pPacket->setSynchronousReply(true);
      gAsyncFileManager.submitRequest(pPacket);
   }
   
   mNextAsyncFileSetRequestID++;
      
   return true;
}


//-------------------------------------------------------------------------
bool BTerrainTexturing::loadAlbedoTextureArray(bool reloading)
{
   ASSERT_THREAD(cThreadIndexRender);
         
   loadTerrainTextureArray(mDirID, "", "_df", mXTDTextureInfo, cTextureTypeAlbedo);
   
   if (!reloading)
      reloadInit(mDirID, "", mXTDTextureInfo);
         
   return true;
}

//-------------------------------------------------------------------------
bool BTerrainTexturing::loadNormalTextureArray(bool reloading)
{
   ASSERT_THREAD(cThreadIndexRender);

   loadTerrainTextureArray(mDirID, "", "_nm", mXTDTextureInfo, cTextureTypeNormal);
   
   if (!reloading)
      reloadInit(mDirID, "", mXTDTextureInfo);
         
   return true;
}
//-------------------------------------------------------------------------
bool BTerrainTexturing::loadSpecularTextureArray(bool reloading)
{
   ASSERT_THREAD(cThreadIndexRender);

   loadTerrainTextureArray(mDirID, "", "_sp", mXTDTextureInfo, cTextureTypeSpecular);

   if (!reloading)
      reloadInit(mDirID, "", mXTDTextureInfo);

   return true;
}
//-------------------------------------------------------------------------
bool BTerrainTexturing::loadSelfTextureArray(bool reloading)
{
   ASSERT_THREAD(cThreadIndexRender);

   loadTerrainTextureArray(mDirID, "", "_em", mXTDTextureInfo, cTextureTypeSelf);

   if (!reloading)
      reloadInit(mDirID, "", mXTDTextureInfo);

   return true;
}
//-------------------------------------------------------------------------
bool BTerrainTexturing::loadEnvMaskTextureArray(bool reloading)
{
   ASSERT_THREAD(cThreadIndexRender);

   loadTerrainTextureArray(mDirID, "", "_rm", mXTDTextureInfo, cTextureTypeEnvMask);

   if (!reloading)
      reloadInit(mDirID, "", mXTDTextureInfo);

   return true;
}
//-------------------------------------------------------------------------
bool BTerrainTexturing::loadTextures(long dirID, const XTTHeader& texList, BTerrainActiveTextureInfo *texInfo)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   mDirID = dirID;
   mXTDTextureInfo = texList;
   mActiveTexInfo = texInfo;

   if (!loadAlbedoTextureArray())
      return false;
      
   if (!loadNormalTextureArray())
      return false;

   if (!loadSpecularTextureArray())
      return false;

   if (!loadSelfTextureArray())
      return false;

   if (!loadEnvMaskTextureArray())
      return false;
   
   return true;
}
//-------------------------------------------------------------------------
void BTerrainTexturing::loadDecalCallback(void* pData)
{
   BAsyncFileManager::BRequestPacket* pPacket = static_cast<BAsyncFileManager::BRequestPacket*>(pData);

   const uint textureIndex = pPacket->getPrivateData0();
   const uint textureType = pPacket->getPrivateData1() & 0xF;
   
   if (!pPacket->getSucceeded())
   {
      BString buf;
      buf.format("BTerrainTexturing::loadDecalCallback: %s not found and not used", pPacket->getFilename().c_str());
      gConsoleOutput.output(cMsgError, buf);
      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }

   BD3DTextureLoader* mpTextureLoader = new BD3DTextureLoader();

   BD3DTextureLoader::BCreateParams textureLoadParams;
   textureLoadParams.mBigEndian = true;
   textureLoadParams.mTiled = true;
   textureLoadParams.mManager = "TerrainTexturing";
   textureLoadParams.mName = pPacket->getFilename();

   bool status;
   status = mpTextureLoader->createFromDDXFileInMemory(static_cast<const uchar*>(pPacket->getData()), pPacket->getDataLen(), textureLoadParams);

   if (!status)
   {
      delete mpTextureLoader;
      mpTextureLoader = NULL;

      BString buf;
      buf.format("BTerrainTexturing::loadDecalCallback: %s not found and not used", pPacket->getFilename().c_str());
      gConsoleOutput.output(cMsgError, buf);

      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }

   if(!mActiveDecals)
   {
      mActiveDecals = new BTerrainActiveDecalHolder[mXTDTextureInfo.mNumActiveDecals];
      for(int i=0;i<mXTDTextureInfo.mNumActiveDecals;i++)
         mActiveDecals[i].mTextureTypesUsed = 0;
   }
   
   if(textureType >= cTextureTypeMax)
   {
      //this is our opacity texture      
      mActiveDecals[textureIndex].mOpactityTexture.release();
      mActiveDecals[textureIndex].mOpactityTexture = mpTextureLoader->getD3DTexture();
   }
   else
   {
      mActiveDecals[textureIndex].mTextures[textureType].release();

      mActiveDecals[textureIndex].mTextures[textureType] = mpTextureLoader->getD3DTexture();
      if(textureType == cTextureTypeSelf)
         mActiveDecals[textureIndex].mSelfHDRScale = mpTextureLoader->getHDRScale();

   }

   //mark used
   mActiveDecals[textureIndex].mTextureTypesUsed |= (1 << textureType);


   mpTextureLoader->releaseOwnership();
   delete mpTextureLoader;
   mpTextureLoader = NULL;

   mDoFreeCache=true;

   gAsyncFileManager.deleteRequestPacket(pPacket);
}
//-------------------------------------------------------------------------
bool BTerrainTexturing::setLoadDecalTextures(long dirID, const XTTHeader& texList,BTerrainActiveDecalInfo *decalInfo,BTerrainActiveDecalInstanceInfo *instanceInfo   )
{
   ASSERT_THREAD(cThreadIndexRender);

   mActiveDecalInfo = decalInfo;
   mActiveDecalInstanceInfo = instanceInfo;
   return loadDecalTextures();
}
//-------------------------------------------------------------------------
bool BTerrainTexturing::loadDecalTextures(bool reloading)
{
   BFixedString32 extentions[cTextureTypeMax] = {"_df","_nm","_sp","_em","_rm" };

   for(int i=0;i<mXTDTextureInfo.mNumActiveDecals;i++)
   {
      
      for(int q = 0; q < cTextureTypeMax;q++)
      {
         BFixedString256 filename("");
         filename += BFixedString256(cVarArg, "%s%s.ddx", mActiveDecalInfo[i].mFilename.c_str(), extentions[q].c_str());

         BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

         pPacket->setFilename(filename.c_str());
         pPacket->setDirID(mDirID);
         pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainTexturing::loadDecalCallback));
         pPacket->setCallbackThreadIndex(cThreadIndexRender);
         pPacket->setPrivateData0(i);
         pPacket->setPrivateData1(q | (mNextAsyncFileSetRequestID << 4));
         pPacket->setPriority(-3);
         //  pPacket->setSynchronousReply(true);
         gAsyncFileManager.submitRequest(pPacket);

         mNextAsyncFileSetRequestID++;
      }
   
      //send request for our opacity texture
      BFixedString256 filename("");
      filename += BFixedString256(cVarArg, "%s_op.ddx", mActiveDecalInfo[i].mFilename.c_str());

      BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

      pPacket->setFilename(filename.c_str());
      pPacket->setDirID(mDirID);
      pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainTexturing::loadDecalCallback));
      pPacket->setCallbackThreadIndex(cThreadIndexRender);
      pPacket->setPrivateData0(i);
      pPacket->setPrivateData1((cTextureTypeMax+1) | (mNextAsyncFileSetRequestID << 4));
      pPacket->setPriority(-3);
      //  pPacket->setSynchronousReply(true);
      gAsyncFileManager.submitRequest(pPacket);

      mNextAsyncFileSetRequestID++;


   }

      if (!reloading)
         reloadInit(mDirID, "", mXTDTextureInfo);
  
   

   return true;

}

//-------------------------------------------------------------------------
void BTerrainTexturing::loadEffect()
{
   ASSERT_RENDER_THREAD

   if (!mpEffectLoader)
   {
      mpEffectLoader = ALIGNED_NEW(BFXLEffectFileLoader, gRenderHeap);
      const bool status = mpEffectLoader->init(gRender.getEffectCompilerDefaultDirID(), COMPOSITE_EFFECT_FILENAME, true, false, true);
      BVERIFY(status);
   }
}
//-------------------------------------------------------------------------
void BTerrainTexturing::tickEffect(void)
{
   BDEBUG_ASSERT(mpEffectLoader);

   if (mpEffectLoader->tick(true))
   {
      mCompShader.attach(mpEffectLoader->getFXLEffect().getEffect());

      initEffectConstants();
   }

   // We now never allow the terrain to render without a valid effect.
   BVERIFY(mCompShader.getEffect());
}
//-------------------------------------------------------------------------
void BTerrainTexturing::initEffectConstants(void)
{
	ASSERT_RENDER_THREAD

   BDEBUG_ASSERT(mCompShader.getEffect());
      
	mCurrTechnique		                           = mCompShader.getTechniqueFromIndex(0);

	//TEXTURING
   mShaderTargetSamplerHandle                   = mCompShader("targetSampler");
   mShaderAlphaSamplerHandle                    = mCompShader("alphasSampler");

   mShaderNumLayers                             = mCompShader("g_RCPnumLayers");

   mShaderExplicitTargetMipLevel                = mCompShader("g_explicitMipValue");
   mShaderHighResHDRUVOffset                    = mCompShader("g_uvTileUVOffset");
   mshaderLayerIndex                            = mCompShader("g_LayerIndex");
   
   mShaderRcpNumAlignedLayers                   = mCompShader("g_RCPnumAlignedLayers");

   mShaderTargetDecalSamplerHandle              = mCompShader("targetDecalSampler");
   mShaderTargetDecalOpactiySamplerHandle       = mCompShader("decalOpacitySampler");
   mShaderTargetDecalAlphaSamplerHandle         = mCompShader("alphasDecalSampler");
}
//-------------------------------------------------------------------------
void BTerrainTexturing::frameBegin(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   
   if (mDecalTexturesDirty)
   {
      mDecalTexturesDirty--;
      if (mDecalTexturesDirty == 0)
      {
        // mActiveTextureSetsLoaded--;
         trace("BTerrainTexturing::frameBegin: Reloading decal textures\n");
         loadDecalTextures();
      }
   }

   if (mEnvMaskTexturesDirty)
   {
      mEnvMaskTexturesDirty--;
      if (mEnvMaskTexturesDirty == 0)
      {
         mActiveTextureSetsLoaded--;
         trace("BTerrainTexturing::frameBegin: Reloading EvnMask textures\n");
         loadEnvMaskTextureArray();
      }
   }

   if (mSelfTexturesDirty)
   {
      mSelfTexturesDirty--;
      if (mSelfTexturesDirty == 0)
      {
         mActiveTextureSetsLoaded--;
         trace("BTerrainTexturing::frameBegin: Reloading self textures\n");
         loadSelfTextureArray();
      }
   }
   

   if (mSpecularTexturesDirty)
   {
      mSpecularTexturesDirty--;
      if (mSpecularTexturesDirty == 0)
      {
         mActiveTextureSetsLoaded--;
         trace("BTerrainTexturing::frameBegin: Reloading specular textures\n");
         loadSpecularTextureArray();
      }
   }

   if (mNormalTexturesDirty)
   {
      mNormalTexturesDirty--;
      if (mNormalTexturesDirty == 0)
      {
         mActiveTextureSetsLoaded--;
         trace("BTerrainTexturing::frameBegin: Reloading normal textures\n");
         loadNormalTextureArray();
      }
   }
   
   if (mAlbedoTexturesDirty)
   {
      mAlbedoTexturesDirty--;
      if (mAlbedoTexturesDirty == 0)
      {
         mActiveTextureSetsLoaded--;
         trace("BTerrainTexturing::frameBegin: Reloading albedo textures\n");
         loadAlbedoTextureArray();
      }
   }

   if(mDoFreeCache)
   {
      freeAllCache();
      mDoFreeCache=false;
   }
}

//-------------------------------------------------------------------------
bool BTerrainTexturing::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   switch (event.mEventClass)
   {
   
      case cTerrainTexturingReloadFileEvent:
      {
//-- FIXING PREFIX BUG ID 6951
         const BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
//--

         
         //CLM do decals first
         if (strstr(pPayload->mPath.getPtr(), "_dcl") != NULL)
            mDecalTexturesDirty = 30;
         else if (strstr(pPayload->mPath.getPtr(), "_nm") != NULL)
            mNormalTexturesDirty = 30;
         else if (strstr(pPayload->mPath.getPtr(), "_sp") != NULL)
            mSpecularTexturesDirty = 30;
         else if (strstr(pPayload->mPath.getPtr(), "_em") != NULL)
            mSelfTexturesDirty = 30;
         else if (strstr(pPayload->mPath.getPtr(), "_rm") != NULL)
            mEnvMaskTexturesDirty = 30;
         
         else
            mAlbedoTexturesDirty = 30;
            
         break;            
      }
      case cTTC_Destroy:
         {
            destroyInternal();
            break;
         }
   }
   
   return false;
}


//-------------------------------------------------------------------------
void BTerrainTexturing::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
   
   case cTTC_Destroy:
      {
         destroyInternal();
         break;
      }
   case cTTC_SplatDecal:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BTerrainActiveTextureSetDecalInstanceInfo));

         const BTerrainActiveTextureSetDecalInstanceInfo* dcli = reinterpret_cast<const BTerrainActiveTextureSetDecalInstanceInfo*>(pData);
         addSplatDecalInternal(dcli->mTileCenterX, dcli->mTileCenterY, dcli->mActiveTextureIndex, dcli->mRotation, dcli->mUScale, dcli->mVScale, dcli->mpExternalAlphaTextureToUse);

         break;
      }
   }
}
//-------------------------------------------------------------------------
void BTerrainTexturing::debugDraw(ATG::Font& font)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if ((!mpMainCache) /*|| (!mpSmallCache)*/)
      return;
      
   if(mShowCache)
   {
      SCOPEDSAMPLE(BTerrainDrawCache);
      mpMainCache->drawCacheToScreen(50,180);
      //mpSmallCache->drawCacheToScreen(50,50);

      //print text
      BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);      

      WCHAR textBuf[256];
      StringCchPrintfW(textBuf, sizeof(textBuf)/sizeof(textBuf[0]), L"VisNodes : %i", BTerrainMetrics::mNumVisibleQuadNodes);
      font.DrawText(50.0f, 30.0f, 0xFFFFFFFF, textBuf);
      StringCchPrintfW(textBuf, sizeof(textBuf)/sizeof(textBuf[0]), L"ThrashCount : %i", BTerrainMetrics::mNumTrashes);
      font.DrawText(250.0f, 30.0f, 0xFFFFFFFF, textBuf); 

      StringCchPrintfW(textBuf, sizeof(textBuf)/sizeof(textBuf[0]), L"GPUMem : %i", BTerrainMetrics::getTotalCacheGPUMem());
      font.DrawText(500.0f, 30.0f, 0xFFFFFFFF, textBuf);   
      float ratio =BTerrainMetrics::mNumResolvedPixels?BTerrainMetrics::mNumResolvedPixels / (float)BTerrainMetrics::mNumResolves:0;
         StringCchPrintfW(textBuf, sizeof(textBuf)/sizeof(textBuf[0]), L"Resolves : %i, %i, %f", BTerrainMetrics::mNumResolves,BTerrainMetrics::mNumResolvedPixels,ratio);
      font.DrawText(800.0f, 30.0f, 0xFFFFFFFF, textBuf);   
         
      BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);      

   }
   
   mpMainCache->getMetrics();
   //mpSmallCache->getMetrics();
};
//-------------------------------------------------------------------------
void BTerrainTexturing::defragmentCaches(void)
{
   //CLM [03.11.08] removed due to state counts being out of wack from the defragmenting
   // we've removed the massive amount of textures that COULD exist (ie we had 256 small textures when this was written)
   // so it's not even remotely necessicary to do this any longer...
   return;//CLM

   //static int cMaxCacheDefragCounter = 50; // HACK
   //if(mCacheDefragCounter > cMaxCacheDefragCounter)
   //{
   //   mCacheDefragCounter = 0;
   //   if (mpMainCache)
   //      mpMainCache->defragment();
   //  // if (mpSmallCache)
   //  //    mpSmallCache->defragment();
   //}
   //mCacheDefragCounter++;
}
//-------------------------------------------------------------------------
void BTerrainTexturing::dumpCachesToLog(int appendNum)
{
   mpMainCache->dumpCachesToLog(appendNum);
 //  mpSmallCache->dumpCachesToLog(appendNum+3);
}
//-------------------------------------------------------------------------
//============================================================================
// BTerrainTexturing::convertUniqueAlbedoFromMemory
//============================================================================
bool BTerrainTexturing::convertUniqueAlbedoFromMemory()
{
   if(!mpLargeUniqueAlbedoDataPointer)
      return false;

   UINT imgSize =0;
   unsigned char *mDevicePtrCounter = (unsigned char *)mpLargeUniqueAlbedoDataPointer;


   int numxVerts = gTerrainVisual.getNumXVerts();
   numxVerts;

   //Unique albedo for the skirts
   mLargeUniqueAlbedoTexture = new D3DTexture();
   imgSize = XGSetTextureHeader(mLargeUniqueTextureWidth,mLargeUniqueTextureHeight,mLargeUniqueTextureMipCount,0,D3DFMT_DXT1,0,0,XGHEADER_CONTIGUOUS_MIP_OFFSET,0,mLargeUniqueAlbedoTexture,NULL,NULL);
   XGOffsetResourceAddress( mLargeUniqueAlbedoTexture, mDevicePtrCounter ); 
   mDevicePtrCounter+=imgSize;

   //sRGB space
   GPUTEXTURE_FETCH_CONSTANT& fc = mLargeUniqueAlbedoTexture->Format;
   fc.SignX = GPUSIGN_GAMMA;
   fc.SignY = GPUSIGN_GAMMA;
   fc.SignZ = GPUSIGN_GAMMA;

  /* D3DLOCKED_RECT rect;
   mLargeUniqueTextureWidth=64;
   mLargeUniqueTextureHeight=64;
   BD3D::mpDev->CreateTexture(mLargeUniqueTextureWidth,mLargeUniqueTextureHeight,1,0,D3DFMT_LIN_A8R8G8B8,0,&mLargeUniqueAlbedoTexture,0);   
   mLargeUniqueAlbedoTexture->LockRect(0,&rect,0,0);
   DWORD *vals4 =(DWORD*)rect.pBits;
   for(int i=0;i<mLargeUniqueTextureWidth*mLargeUniqueTextureHeight;i++)
   {
      vals4[i]=0xFFFFFFFF;
   }
   mLargeUniqueAlbedoTexture->UnlockRect(0);*/
   return true;
}




//-------------------------------------------------------------------------
void BTerrainTexturing::addSplatDecalInternal(float worldCenterX, float worldCenterY,int terrainSplatTextureIndex, float rotation, float xScale, float zScale, BManagedTextureHandle pExternalAlphaTex)
{
   ASSERT_THREAD(cThreadIndexRender);

   int numXChunks =gTerrain.getNumXChunks();
   int pixelCenterX = (int)(worldCenterX * (cUniqueTextureWidth * numXChunks));
   int pixelCenterY = (int)(worldCenterY * (cUniqueTextureWidth * numXChunks));

   

   BTerrainActiveTextureSetDecalInstanceInfo dli;
   dli.mActiveTextureIndex=terrainSplatTextureIndex;
   dli.mpExternalAlphaTextureToUse = pExternalAlphaTex;
   dli.mRotation=rotation;
   dli.mTileCenterX = (float)pixelCenterX;
   dli.mTileCenterY = (float)pixelCenterY;
   dli.mUScale =xScale;
   dli.mVScale =zScale;


   BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(pExternalAlphaTex);
   if(pTexture==NULL)
      return;
   D3DTexture *tex = pTexture->getD3DTexture().getTexture();

   D3DSURFACE_DESC desc;
   tex->GetLevelDesc(0,&desc);

   //safegaurd, if this texture hasn't been loaded yet, and we get a default texture, don't put it down..
   if(desc.Width <=32 || desc.Height <=32)
      return;


   int dclWidth = (int)(desc.Width * xScale);
   int dclHeight = (int)(desc.Height * zScale);

   

   int minx = pixelCenterX - (dclWidth>>1);
   int maxx = pixelCenterX + (dclWidth>>1);
   int miny = pixelCenterY - (dclHeight>>1);
   int maxy = pixelCenterY + (dclHeight>>1);

   //translate to quadnode indexes
   int qnMinX = Math::Clamp<int>((int)(minx / (float)cUniqueTextureWidth),0,numXChunks-1);
   int qnMaxX = Math::Clamp<int>((int)(maxx / (float)cUniqueTextureWidth),0,numXChunks-1);
   int qnMinY = Math::Clamp<int>((int)(miny / (float)cUniqueTextureWidth),0,numXChunks-1);
   int qnMaxY = Math::Clamp<int>((int)(maxy / (float)cUniqueTextureWidth),0,numXChunks-1);

   //find all quadnodes that intercept this decal. Add it to their list.
   const unsigned int cNumMaxSetDecals = 4;
   for(int y=qnMinY;y<=qnMaxY;y++)
   {
      for(int x=qnMinX;x<=qnMaxX;x++)
      {
         int idx = y + x * numXChunks;

         //CLM WARNING : this could get wonkey if two adjacent nodes don't share the same # of set decals...
         if(gTerrain.mpQuadGrid[idx].mRenderPacket->mTexturingData->mLayerContainer.mTextureSetDecalData.mTextureSetDecalInstances.size() >= cNumMaxSetDecals)
            continue;

         gTerrain.mpQuadGrid[idx].mRenderPacket->mTexturingData->mLayerContainer.mTextureSetDecalData.mTextureSetDecalInstances.push_back(dli);
         gTerrainTexturing.freeCachedTexture(gTerrain.mpQuadGrid[idx].mRenderPacket->mTexturingData->mCachedUniqueTexture);      
      }
   }
}
//-------------------------------------------------------------------------
void BTerrainTexturing::addSplatDecal(float worldCenterX, float worldCenterY,int terrainSplatTextureIndex, float rotation, float xScale, float zScale, BManagedTextureHandle pExternalAlphaTex)
{
   BTerrainActiveTextureSetDecalInstanceInfo* pData = static_cast<BTerrainActiveTextureSetDecalInstanceInfo*>(gRenderThread.submitCommandBegin(mCommandHandle, cTTC_SplatDecal, sizeof(BTerrainActiveTextureSetDecalInstanceInfo)));

   pData->mActiveTextureIndex = terrainSplatTextureIndex;
   pData->mpExternalAlphaTextureToUse = pExternalAlphaTex;
   pData->mUScale = xScale;
   pData->mVScale = zScale;
   pData->mTileCenterX = worldCenterX;
   pData->mTileCenterY = worldCenterY;
   pData->mRotation = rotation;

   gRenderThread.submitCommandEnd(sizeof(BTerrainActiveTextureSetDecalInstanceInfo));
}
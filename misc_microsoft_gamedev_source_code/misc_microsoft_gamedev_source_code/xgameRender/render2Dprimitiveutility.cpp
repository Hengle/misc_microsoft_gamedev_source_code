//==============================================================================
// render2Dprmitiveutility.cpp
//
// Copyright (c) 2003-2009, Ensemble Studios
//==============================================================================
#include "xgamerender.h"
#include "render2Dprimitiveutility.h"
#include "renderThread.h"
#include "renderDraw.h"
#include "vertexTypes.h"
#include "color.h"
#include "fixedFuncShaders.h"
#include "render.h"
#include "vectorFont.h"
#include "math\VMXUtils.h"
#include "asyncFileManager.h"
#include "reloadManager.h"
#include "consoleOutput.h"

const int cCircleProgressVertexCount = 24;

class BCircleProgressUV
{
   public:
      float u;
      float v;
};

const BCircleProgressUV cCircleProgressVerts[cCircleProgressVertexCount] =
{
   {0.5f, 0.5f},
   {0.5f, 0.0f},
   {1.0f, 0.0f},

   {0.5f, 0.5f},
   {1.0f, 0.0f},
   {1.0f, 0.5f},

   {0.5f, 0.5f},
   {1.0f, 0.5f},
   {1.0f, 1.0f},

   {0.5f, 0.5f},
   {1.0f, 1.0f},
   {0.5f, 1.0f},

   {0.5f, 0.5f},
   {0.5f, 1.0f},
   {0.0f, 1.0f},

   {0.5f, 0.5f},
   {0.0f, 1.0f},
   {0.0f, 0.5f},

   {0.5f, 0.5f},
   {0.0f, 0.5f},
   {0.0f, 0.0f},

   {0.5f, 0.5f},
   {0.0f, 0.0f},
   {0.5f, 0.0f}
};

//==============================================================================
// Globals
//==============================================================================
BRender2DPrimitiveUtility gRender2DPrimitiveUtility;


//==============================================================================
// BRender2DPrimitiveUtility
//==============================================================================

//==============================================================================
// BRender2DPrimitiveUtility::BRender2DPrimitiveUtility
//==============================================================================
BRender2DPrimitiveUtility::BRender2DPrimitiveUtility() :
   mCommandListenerHandle(cInvalidCommandListenerHandle),
   mpPieVertexDecl(NULL)
{
   Utils::ClearObj(mUpdateData);
}

//==============================================================================
// BRender2DPrimitiveUtility::~BRender2DPrimitiveUtility
//==============================================================================
BRender2DPrimitiveUtility::~BRender2DPrimitiveUtility()
{
}

//==============================================================================
// BRender2DPrimitiveUtility::init
//==============================================================================
void BRender2DPrimitiveUtility::init(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD

   if (gRenderThread.isSimThread())
   {
      if (mCommandListenerHandle == cInvalidCommandListenerHandle)
         mCommandListenerHandle = gRenderThread.registerCommandListener(this);   
   }      

   initVertexDeclarations();
   eventReceiverInit(cThreadIndexRender);
   reloadInit();
   loadEffect();
}

//==============================================================================
// BRender2DPrimitiveUtility::initVertexDeclarations
//==============================================================================
void BRender2DPrimitiveUtility::initVertexDeclarations(void)
{
   const D3DVERTEXELEMENT9 BPieVertex_Elements[] =
   {    
      { 0, 0,   D3DDECLTYPE_FLOAT4,     D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 16,  D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
      { 0, 20,  D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  1 },
      { 0, 24,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BPieVertex_Elements, &mpPieVertexDecl);

   const D3DVERTEXELEMENT9 BSpriteVertex_Elements[] =
   {    
      { 0, 0,   D3DDECLTYPE_FLOAT4,     D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 16,  D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
      { 0, 20,  D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  1 },
      { 0, 24,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BSpriteVertex_Elements, &mpSpriteVertexDecl);
}

//==============================================================================
// BRender2DPrimitiveUtility::deInit
//==============================================================================
void BRender2DPrimitiveUtility::deInit(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   
   if (gRenderThread.isSimThread())
   {
      gRenderThread.blockUntilGPUIdle();
            
      if (mCommandListenerHandle != cInvalidCommandListenerHandle)
      {
         gRenderThread.freeCommandListener(mCommandListenerHandle);
         mCommandListenerHandle = cInvalidCommandListenerHandle;
      }
   }  

   if (mpPieVertexDecl)
   {
      mpPieVertexDecl->Release();
      mpPieVertexDecl = NULL;
   }

   
   Utils::ClearObj(mUpdateData);    

   reloadDeinit();
   eventReceiverDeinit();
}
//==============================================================================
// BRender2DPrimitiveUtility::reloadInit
//==============================================================================
void BRender2DPrimitiveUtility::reloadInit(void)
{
   BReloadManager::BPathArray paths;
   BString effectFilename;
   eFileManagerError result = gFileManager.getDirListEntry(effectFilename, gRender.getEffectCompilerDefaultDirID());
   BVERIFY(result == cFME_SUCCESS);
   strPathAddBackSlash(effectFilename);
   effectFilename += "render2Dprimitive\\primitive2D*.bin";
   paths.pushBack(effectFilename);

   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, eEventClassReloadEffects);
}
//==============================================================================
// BRender2DPrimitiveUtility::reloadDeinit
//==============================================================================
void BRender2DPrimitiveUtility::reloadDeinit(void)
{
   gReloadManager.deregisterClient(mEventHandle);
}
//==============================================================================
// BRender2DPrimitiveUtility::loadEffect
//==============================================================================
void BRender2DPrimitiveUtility::loadEffect()
{
   ASSERT_MAIN_OR_WORKER_THREAD

      //-- loads all of the effects up
      for (uint i = 0; i < eRender2DCommandTotal; i++)
      {  
         BFixedString256 filename;
         filename.format("render2Dprimitive\\primitive2D%i.bin", i);

         BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

         pPacket->setDirID(gRender.getEffectCompilerDefaultDirID());
         pPacket->setFilename(filename);
         pPacket->setReceiverHandle(mEventHandle);
         pPacket->setPrivateData0(i); //-- effect slot;
         pPacket->setSynchronousReply(true);
         //pPacket->setDiscardOnClose(true);

         gAsyncFileManager.submitRequest(pPacket);
      }
}

//==============================================================================
// BRender2DPrimitiveUtility::receiveEvent
//==============================================================================
bool BRender2DPrimitiveUtility::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD
      switch (event.mEventClass)
   {
      case cEventClassAsyncFile:
         {
            initEffect(event);
            break;
         }
      case eEventClassReloadEffects:
         {
            loadEffect();
            break;
         }
   }
   return false;
}

//==============================================================================
// BRender2DPrimitiveUtility::initEffect
//==============================================================================
void BRender2DPrimitiveUtility::initEffect(const BEvent& event)
{
   ASSERT_RENDER_THREAD
   BAsyncFileManager::BRequestPacket* pPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket *>(event.mpPayload);
   BDEBUG_ASSERT(pPacket!=NULL);
   
   if (!pPacket->getSucceeded())
   {
      gConsoleOutput.output(cMsgError, "BRender2DPrimitiveUtility::initEffect: Async load of file %s failed", pPacket->getFilename().c_str());
   }
   else
   {
      // rg [5/28/06] - This will randomly fail if the effect is ever left set on the D3D device! 
      // As of today, it appears 
      mEffect.clear();
      HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, pPacket->getData());
      if (FAILED(hres))
      {
         gConsoleOutput.output(cMsgError, "BRender2DPrimitiveUtility::initEffect: Effect creation of file %s failed", pPacket->getFilename().c_str());
      }
      else
      {
         trace("BRender2DPrimitiveUtility::initEffect: Effect creation of file %s succeeded", pPacket->getFilename().c_str());
      }

      BDEBUG_ASSERT(mEffect.getNumTechniques() == 3);

      // Get techniques and params
      mTechnique[eRender2DTechniquePie]      = mEffect.getTechnique("Pie"); 
      mTechnique[eRender2DTechniqueSprite]   = mEffect.getTechnique("Sprite"); 
      mTechnique[eRender2DTechnique2DSprite] = mEffect.getTechnique("Sprite2D"); 
      mMaskTexture[eRender2DTechniquePie]    = mEffect("gMaskSampler");
      mMaskTexture[eRender2DTechniqueSprite] = mEffect("gMaskSampler");
      mMaskTexture[eRender2DTechnique2DSprite] = mEffect("gMaskSampler");
   }
}

//==============================================================================
// BRender2DPrimitiveUtility::initDeviceData
// init will be called from the worker thread after the D3D device is initialized.
//==============================================================================
void BRender2DPrimitiveUtility::initDeviceData(void)
{
   ASSERT_RENDER_THREAD   
}

//==============================================================================
// void BRender2DPrimitiveUtility::frameBegin(void);
// Called from worker thread.
//==============================================================================
void BRender2DPrimitiveUtility::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// BRender2DPrimitiveUtility::processCommand
// Called from the worker thread to process commands.
//==============================================================================
void BRender2DPrimitiveUtility::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD   
   BDEBUG_ASSERT(header.mLen == sizeof(BR2DPUUpdateData));
   workerUpdate((eBR2DUCommand)header.mType, (void*) pData);
}

//==============================================================================
//==============================================================================
void BRender2DPrimitiveUtility::workerUpdate(eBR2DUCommand type, void* pData)
{
   ASSERT_RENDER_THREAD
   switch (type)
   {      
      case eBR2DUUpdatePie: 
      {         
         const BR2DPUUpdateData* pUpdateData = reinterpret_cast<const BR2DPUUpdateData*>(pData);
         BDEBUG_ASSERT(pUpdateData != NULL);

         const B2DPrimitivePie* pPrimitives = static_cast<const B2DPrimitivePie*>(pUpdateData->mpData);

         if (pUpdateData->mThreadIndex == cThreadIndexSim)
            updatePrimitives(mSimPiePrimitives, pPrimitives, pUpdateData->mCount);
         else if (pUpdateData->mThreadIndex == cThreadIndexRender)
            updatePrimitives(mRenderPiePrimitives, pPrimitives, pUpdateData->mCount);

         break;
      }

      case eBR2DUUpdateSprite: 
      {         
         const BR2DPUUpdateData* pUpdateData = reinterpret_cast<const BR2DPUUpdateData*>(pData);
         BDEBUG_ASSERT(pUpdateData != NULL);

         const B2DPrimitiveSprite* pPrimitives = static_cast<const B2DPrimitiveSprite*>(pUpdateData->mpData);
         if (pUpdateData->mThreadIndex == cThreadIndexSim)
            updatePrimitives(mSimSpritePrimitives, pPrimitives, pUpdateData->mCount);
         else if (pUpdateData->mThreadIndex == cThreadIndexRender)
            updatePrimitives(mRenderSpritePrimitives, pPrimitives, pUpdateData->mCount);
                 
         break;
      }

      case eBR2DUUpdate2DSprite: 
      {         
         const BR2DPUUpdateData* pUpdateData = reinterpret_cast<const BR2DPUUpdateData*>(pData);
         BDEBUG_ASSERT(pUpdateData != NULL);

         const B2DPrimitiveSprite* pPrimitives = static_cast<const B2DPrimitiveSprite*>(pUpdateData->mpData);
         if (pUpdateData->mThreadIndex == cThreadIndexSim)
            updatePrimitives(mSimSprite2DPrimitives, pPrimitives, pUpdateData->mCount);
         else if (pUpdateData->mThreadIndex == cThreadIndexRender)
            updatePrimitives(mRenderSprite2DPrimitives, pPrimitives, pUpdateData->mCount);
                 
         break;
      }
   }         
}

//==============================================================================
// template<class T> void BDebugPrimitives::updatePrimitives(T& list)
//==============================================================================
template<class T, class P> void BRender2DPrimitiveUtility::updatePrimitives(T& list, P* pPrimitives, int count)
{   
   list.resize(0);
   list.resize(count);
   for (int i = 0; i < count; ++i)
   {
      list[i] = pPrimitives[i];
   }                 

   if (list.size())
      std::sort(list.begin(), list.end(), BPrimitiveKeySorter(*this));
}

//==============================================================================
// BRender2DPrimitiveUtility::frameEnd
// Called from worker thread.
//==============================================================================
void BRender2DPrimitiveUtility::frameEnd(void)
{
   ASSERT_RENDER_THREAD
   
   Utils::ClearObj(mUpdateData);
}

//==============================================================================
// BRender2DPrimitiveUtility::deinitDeviceData
// deinit will be called from the worker thread before the RCL is freed, but always before the D3D device is release.
//==============================================================================
void BRender2DPrimitiveUtility::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
   //-- deallocate our vertex buffers here
   releaseBuffers();
   
   mEffect.clear();   
}


//==============================================================================
// BRender2DPrimitiveUtility::update(void* pData)
//==============================================================================
void BRender2DPrimitiveUtility::update(eBR2DUCommand type, int count, void* pData, BThreadIndex ownerThreadIndex)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT(type < eBR2DUCommandTotal);

   if (count <= 0)
      return;

   BDEBUG_ASSERT(pData != NULL);

   switch (type)
   {
      case eBR2DUUpdatePie:
      {         
         int frameStorageSize = sizeof(B2DPrimitivePie) * count;
         uchar* pFrameStorage = static_cast<uchar*>(gRenderThread.allocateFrameStorage(frameStorageSize, 16));
         
         Utils::FastMemCpy(pFrameStorage, pData, frameStorageSize);
         void* pVoid = gRenderThread.submitCommandBegin(mCommandListenerHandle, type, sizeof(BR2DPUUpdateData));
         BDEBUG_ASSERT(pVoid);

         BR2DPUUpdateData* pRenderData = reinterpret_cast<BR2DPUUpdateData*>(pVoid);
         pRenderData->mCount = count;
         pRenderData->mpData = pFrameStorage;
         pRenderData->mThreadIndex = ownerThreadIndex;

         gRenderThread.submitCommandEnd(sizeof(BR2DPUUpdateData));         
         break;
      }
      case eBR2DUUpdateSprite:
      case eBR2DUUpdate2DSprite:
      {
         int frameStorageSize = sizeof(B2DPrimitiveSprite) * count;
         uchar* pFrameStorage = static_cast<uchar*>(gRenderThread.allocateFrameStorage(frameStorageSize, 16));
         
         Utils::FastMemCpy(pFrameStorage, pData, frameStorageSize);
         void* pVoid = gRenderThread.submitCommandBegin(mCommandListenerHandle, type, sizeof(BR2DPUUpdateData));
         BDEBUG_ASSERT(pVoid);

         BR2DPUUpdateData* pRenderData = reinterpret_cast<BR2DPUUpdateData*>(pVoid);
         pRenderData->mCount = count;
         pRenderData->mpData = pFrameStorage;
         pRenderData->mThreadIndex = ownerThreadIndex;

         gRenderThread.submitCommandEnd(sizeof(BR2DPUUpdateData));         
         break;
      }      
   }         
}

//==============================================================================
// BRender2DPrimitiveUtility::createPieVB
//==============================================================================
template<class T> IDirect3DVertexBuffer9* BRender2DPrimitiveUtility::createPieVB(T& list, int startIndex, int endIndex, int& vertexCount)
{
   int count = endIndex - startIndex + 1;

   if (count <= 0)
      return NULL;
   
   const float cOneOverEight = 0.125f;
   const int cMaxSegments = 8;
   
   float end;
   float start;
   float bCW;
   bool bFill;
   float startValue = 0.0f;
   float endValue = 0.0f;
      
   int maxVertices = 3 * cMaxSegments* count;
   IDirect3DVertexBuffer9* pVB = gRenderDraw.createDynamicVB(maxVertices * sizeof(BPieVertex));
   if (!pVB)
      return NULL;

   BPieVertex* pV = NULL;
//-- FIXING PREFIX BUG ID 6559
   const BPieVertex* pStartV = NULL;
//--
   pVB->Lock(0, 0, (void**) &pV, 0);
   BDEBUG_ASSERT(pV);
   pStartV = pV;
   vertexCount = 0;
   XMVECTOR position;

   for (int j = startIndex; j <= endIndex; ++j)
   {
      const B2DPrimitivePie* pPie = &list[j];
      if (!pPie)
         continue;

      position = pPie->mMatrix.r[3];

      end   = Math::Clamp(pPie->mEnd, 0.0f, 1.0f);
      start = Math::Clamp(pPie->mStart, 0.0f, end);
      bCW   = pPie->mCW;
      bFill = pPie->mFill;
   
      startValue = 0.0f;
      endValue = 1.0f;

      //-- Do we want to fill the circle or empty it?
      if (bFill)  
      {
         // do it clock wise or counter clock wise
         if(bCW)
         { 
            startValue = start;
            endValue   = end;           
         }
         else /*ccw*/   
         { 
            startValue = 1.0f - end;
            endValue   = 1.0f - start;
         }
      }
      else // emptying
      {
         if (bCW)
         { 
            startValue = end;
            endValue   = 1.0f - start;
         }
         else /*ccw*/
         { 
            startValue = start;
            endValue   = 1.0f - end;
         }
      }

      BDEBUG_ASSERT(startValue <= endValue);
      if( fabs(endValue - startValue) <= cFloatCompareEpsilon )
         continue;
            
      int firstPiece = 0;
      int lastPiece = cMaxSegments - 1;
      long numFullPieces = 0;
      float valueStartAlpha = -1.0f;
      float valueEndAlpha = -1.0f;
   
      float curValue;
      float nextValue;
      bool  fullPiece;
      float delta;

      for(int n=0; n < cMaxSegments; n++)
      {
         curValue  = n * cOneOverEight;
         nextValue = curValue+cOneOverEight;
         fullPiece = true;

         if(startValue >= curValue)
         {
            if(startValue > curValue)
               fullPiece = false;

            if(startValue < nextValue) // if starting in this piece, get the offset
            {
               firstPiece = n;
               delta = (startValue - curValue);
               if(delta > 0.0f)
                  valueStartAlpha = delta / cOneOverEight; // 0 -> 1 in this piece
            }
         }

         if(endValue <= nextValue)
         {
            if(endValue < nextValue)
               fullPiece = false;

            if(endValue > curValue) // if starting in this piece, get the offset
            {
               lastPiece = n;

               delta = (endValue - curValue);
               if(delta < cOneOverEight)
                  valueEndAlpha = delta / cOneOverEight; // 0 -> 1 in this piece
            }
         }

         if(fullPiece)
            numFullPieces++;
      }
                       
      float deltaX = pPie->mScaleX;
      float deltaY = pPie->mScaleY;
      float offsetX = pPie->mOffsetX;
      float offsetY = pPie->mOffsetY;
      float texU = 0.0f;
      float texV = 0.0f;

      BPieVertex v0;
      int vertexOffset = 0;
      for(int i=firstPiece; i<=lastPiece; i++)
      {
         BDEBUG_ASSERT(i>=0);
         BDEBUG_ASSERT(i<cMaxSegments);

         vertexOffset = i * 3;

         XMStoreFloat4NC(&pV[0].mPos, (XMVECTOR) position);

         pV[0].mOffset = XMHALF2(((cCircleProgressVerts[vertexOffset].u - 0.5f ) * deltaX)+offsetX, ((cCircleProgressVerts[vertexOffset].v - 0.5f )* deltaY) + offsetY);
         pV[0].mUV     = XMHALF2(cCircleProgressVerts[vertexOffset].u, cCircleProgressVerts[vertexOffset].v);
         pV[0].mColor = pPie->mColor;         

         vertexOffset++;
         if( i==firstPiece && (valueStartAlpha >= 0.0f)) // first piece and incomplete?
         {
            texU = cCircleProgressVerts[vertexOffset].u + ((cCircleProgressVerts[vertexOffset+1].u-cCircleProgressVerts[vertexOffset].u) * valueStartAlpha);
            texV = cCircleProgressVerts[vertexOffset].v + ((cCircleProgressVerts[vertexOffset+1].v-cCircleProgressVerts[vertexOffset].v) * valueStartAlpha);
         }         
         else
         {
            texU = cCircleProgressVerts[vertexOffset].u;
            texV = cCircleProgressVerts[vertexOffset].v;
         }

         XMStoreFloat4NC(&pV[1].mPos, (XMVECTOR) position);         
         pV[1].mOffset= XMHALF2(((texU - 0.5f) * deltaX)+offsetX, ((texV - 0.5f) * deltaY)+offsetY);
         pV[1].mUV    = XMHALF2(texU,texV);
         pV[1].mColor = pPie->mColor;
         
         if ( i == lastPiece  && (valueEndAlpha >= 0.0f))
         {
            texU = cCircleProgressVerts[vertexOffset].u + ((cCircleProgressVerts[vertexOffset+1].u-cCircleProgressVerts[vertexOffset].u) * valueEndAlpha);
            texV = cCircleProgressVerts[vertexOffset].v + ((cCircleProgressVerts[vertexOffset+1].v-cCircleProgressVerts[vertexOffset].v) * valueEndAlpha);
         }
         else
         {
            vertexOffset++;
            texU = cCircleProgressVerts[vertexOffset].u;
            texV = cCircleProgressVerts[vertexOffset].v;
         }

         XMStoreFloat4NC(&pV[2].mPos, (XMVECTOR) position);  
         pV[2].mOffset = XMHALF2(((texU-0.5f) * deltaX)+offsetX, ((texV-0.5f) * deltaY)+offsetY);
         pV[2].mUV     = XMHALF2(texU, texV);
         pV[2].mColor = pPie->mColor;

         pV+=3;
         vertexCount+=3;
      }
   }

   pVB->Unlock();
   return pVB;
}


//==============================================================================
// BRender2DPrimitiveUtility::createSpriteVB
//==============================================================================
template<class T> IDirect3DVertexBuffer9* BRender2DPrimitiveUtility::createSpriteVB(T& list, int startIndex, int endIndex, int& vertexCount)
{
   int count = endIndex - startIndex + 1;
   if (count <= 0)
      return NULL;   

   BDEBUG_ASSERT(startIndex < list.getNumber());
   BDEBUG_ASSERT(endIndex   < list.getNumber());
            
   int maxVertices = count;
   IDirect3DVertexBuffer9* pVB = gRenderDraw.createDynamicVB(maxVertices * sizeof(BSpriteVertex));
   if (!pVB)
      return NULL;

   BSpriteVertex* pV = NULL;
//-- FIXING PREFIX BUG ID 6561
   const BSpriteVertex* pStartV = NULL;
//--
   pVB->Lock(0, 0, (void**) &pV, 0);
   BDEBUG_ASSERT(pV);
   pStartV = pV;
   vertexCount = 0;
   XMVECTOR position;
   for (int j = startIndex; j <= endIndex; ++j)
   {
      const B2DPrimitiveSprite* pPrimitive = &list[j];
      if (!pPrimitive)
         continue;

      position = pPrimitive->mMatrix.r[3];
      XMStoreFloat4NC(&pV->mPos, (XMVECTOR) position);      
      pV->mSize   = XMHALF2(pPrimitive->mScaleX, pPrimitive->mScaleY);
      pV->mOffset = XMHALF2(pPrimitive->mOffsetX, pPrimitive->mOffsetY);
      pV->mColor = pPrimitive->mColor;

      pV++;     
      vertexCount++;
   }
   pVB->Unlock();
   return pVB;
}

//==============================================================================
// BRender2DPrimitiveUtility::render()
//==============================================================================
void BRender2DPrimitiveUtility::render(BThreadIndex ownerThreadIndex)
{
   ASSERT_MAIN_THREAD 

   BRenderData* pData = reinterpret_cast<BRenderData*>(gRenderThread.allocateFrameStorage(sizeof(BRenderData)));
   pData->mThreadIndex = ownerThreadIndex;

   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRender2DPrimitiveUtility::workerRender), pData);
}

//==============================================================================
// BRender2DPrimitiveUtility::workerRender
//==============================================================================
void BRender2DPrimitiveUtility::workerRender(void* pData)
{
   ASSERT_RENDER_THREAD

   BRenderData renderData = *reinterpret_cast<const BRenderData*>(pData);
   int threadIndex = renderData.mThreadIndex;
      
   if (threadIndex == cThreadIndexSim)
   {
      renderSortedSprites(mSimSpritePrimitives, false);
      renderSortedSprites(mSimSprite2DPrimitives, true);
      renderSortedPies(mSimPiePrimitives);

      mSimPiePrimitives.resize(0);
      mSimSpritePrimitives.resize(0);
      mSimSprite2DPrimitives.resize(0);      
   }
   else if (threadIndex == cThreadIndexRender)
   {
      renderSortedSprites(mRenderSpritePrimitives, false);
      renderSortedSprites(mRenderSprite2DPrimitives, true);
      renderSortedPies(mRenderPiePrimitives);

      mRenderPiePrimitives.resize(0);
      mRenderSpritePrimitives.resize(0);
      mRenderSprite2DPrimitives.resize(0);
   }
}

//==============================================================================
//==============================================================================
template <class T> void BRender2DPrimitiveUtility::renderSortedPies(T& list)
{
   ASSERT_RENDER_THREAD

   int curBlendMode = -1;
   int curCategory = -1;
   int curLayer = -1;
   BManagedTextureHandle curTextureHandle = cInvalidManagedTextureHandle;
   int firstBatchIndex = -1;
   int lastBatchIndex  = -1;   

   for (uint i = 0; i < list.getSize(); ++i)
   {
      const B2DPrimitivePie* pPie = &list[i];

      if ( (curCategory      != pPie->mCategory)      ||
           (curLayer         != pPie->mLayer)         ||
           (curBlendMode     != pPie->mBlendMode)     ||
           (curTextureHandle != pPie->mTextureHandle)
         )
      {
         if (firstBatchIndex != -1)
         {
            renderPieRange(list, firstBatchIndex, lastBatchIndex);
         }

         curBlendMode     = pPie->mBlendMode;
         curCategory      = pPie->mCategory;
         curLayer         = pPie->mLayer;
         curTextureHandle = pPie->mTextureHandle;
         firstBatchIndex  = i;
         lastBatchIndex   = i;
      }
      else
      {
         BDEBUG_ASSERT(firstBatchIndex != -1);
         lastBatchIndex = i;
      }
   }

   if (firstBatchIndex != -1)
      renderPieRange(list, firstBatchIndex, lastBatchIndex);
}

//==============================================================================
//==============================================================================
template<class T> void BRender2DPrimitiveUtility::renderSortedSprites(T& list, bool b2DSprite)
{
   ASSERT_RENDER_THREAD

   int curBlendMode = -1;
   int curCategory = -1;
   int curLayer = -1;
   BManagedTextureHandle curTextureHandle = cInvalidManagedTextureHandle;
   int firstBatchIndex = -1;
   int lastBatchIndex  = -1;   

   for (uint i = 0; i < list.getSize(); ++i)
   {
      const B2DPrimitiveSprite* pPrimitive = &list[i];

      if ( (curCategory      != pPrimitive->mCategory)      ||
           (curLayer         != pPrimitive->mLayer)         ||
           (curBlendMode     != pPrimitive->mBlendMode)     ||
           (curTextureHandle != pPrimitive->mTextureHandle)
         )
      {
         if (firstBatchIndex != -1)
         {
            renderSpriteRange(list, firstBatchIndex, lastBatchIndex, b2DSprite);
         }

         curBlendMode     = pPrimitive->mBlendMode;
         curCategory      = pPrimitive->mCategory;
         curLayer         = pPrimitive->mLayer;
         curTextureHandle = pPrimitive->mTextureHandle;
         firstBatchIndex  = i;
         lastBatchIndex   = i;
      }
      else
      {
         BDEBUG_ASSERT(firstBatchIndex != -1);
         lastBatchIndex = i;
      }
   }

   if (firstBatchIndex != -1)
      renderSpriteRange(list, firstBatchIndex, lastBatchIndex, b2DSprite);
}

//==============================================================================
//==============================================================================
template<class T> void BRender2DPrimitiveUtility::renderPieRange(T& list, int firstIndex, int lastIndex)
{
   if (firstIndex < 0)
      return;

   if (lastIndex >= list.getNumber())
      return;

   BManagedTextureHandle textureHandle = list[firstIndex].mTextureHandle;
   int blendmode = list[firstIndex].mBlendMode;

   int vertexCount = 0;
   IDirect3DVertexBuffer9* pVB = createPieVB(list, firstIndex, lastIndex, vertexCount);
   if (!pVB)
      return;

   if (vertexCount <= 0)
      return;

   drawPrimitiveInternal(eRender2DTechniquePie, 0, D3DPT_TRIANGLELIST, (eRender2DBlendMode) blendmode, mpPieVertexDecl, sizeof(BPieVertex), pVB, vertexCount, textureHandle);     
}

//==============================================================================
//==============================================================================
template<class T> void BRender2DPrimitiveUtility::renderSpriteRange(T& list, int firstIndex, int lastIndex, bool b2DSprite)
{
   if (firstIndex < 0)
      return;

   if (lastIndex >= list.getNumber())
      return;

   BManagedTextureHandle textureHandle = list[firstIndex].mTextureHandle;
   int blendmode = list[firstIndex].mBlendMode;

   int vertexCount = 0;
   IDirect3DVertexBuffer9* pVB = createSpriteVB(list, firstIndex, lastIndex, vertexCount);
   if (!pVB)
      return;

   if (vertexCount <= 0)
      return;
   
   int techniqueIndex = eRender2DTechniqueSprite;
   if (b2DSprite)
      techniqueIndex = eRender2DTechnique2DSprite;

   drawPrimitiveInternal(techniqueIndex, 0, D3DPT_QUADLIST, (eRender2DBlendMode) blendmode, mpSpriteVertexDecl, sizeof(BSpriteVertex), pVB, vertexCount*4, textureHandle);        
}

//==============================================================================
// BRender2DPrimitiveUtility::drawPieInternal
//==============================================================================
void BRender2DPrimitiveUtility::drawPrimitiveInternal(int techniqueIndex, int techniquePassIndex, D3DPRIMITIVETYPE primType, eRender2DBlendMode blendMode, IDirect3DVertexDeclaration9* pDecl, int vertexSize, IDirect3DVertexBuffer9* pVB, int vertexCount, BManagedTextureHandle textureHandle)
{
   ASSERT_RENDER_THREAD

   BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(textureHandle);
   if (!pTexture)
      return;

   BDEBUG_ASSERT(pVB);
   BDEBUG_ASSERT(vertexCount > 0);
      
   BD3D::mpDev->SetVertexDeclaration(pDecl);
   gRenderDraw.setStreamSource(0, pVB, 0, vertexSize);

   mEffect.updateIntrinsicParams();
   
   mTechnique[techniqueIndex].beginRestoreDefaultState();
      applyBlendMode(blendMode);
      mTechnique[techniqueIndex].beginPass(techniquePassIndex);
         mMaskTexture[techniqueIndex] = pTexture->getD3DTexture().getTexture();
         mTechnique[techniqueIndex].commit();   
         BD3D::mpDev->DrawVertices(primType, 0, vertexCount);

      mTechnique[techniqueIndex].endPass();
   mTechnique[techniqueIndex].end();

   BD3D::mpDev->SetVertexDeclaration(NULL);
   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
   BD3D::mpDev->SetPixelShader(NULL);
   BD3D::mpDev->SetVertexShader(NULL);         
}

//==============================================================================
// BRender2DPrimitiveUtility::setWorldTransform
//==============================================================================
void BRender2DPrimitiveUtility::setWorldTransform(BMatrix worldMatrix, BVector scaleVector)
{
//   ASSERT_RENDER_THREAD

   XMVECTOR scale;
   scale.x = scaleVector.x;
   scale.y = scaleVector.y;
   scale.z = scaleVector.z;
   scale.w = 0.0f;
   XMMATRIX scaleMatrix = XMMatrixScalingFromVector(scale);
   XMMATRIX scaledWorldMatrix = XMMatrixMultiply(scaleMatrix, worldMatrix);
   
   XMMATRIX worldToProj = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj, false);
   XMMATRIX finalMatrix = XMMatrixMultiplyTranspose(scaledWorldMatrix, worldToProj);            
   BD3D::mpDev->SetVertexShaderConstantF(0, (float*) finalMatrix.m , 4);
}

//==============================================================================
// BRender2DPrimitiveUtility::setWorldTransform
//==============================================================================
void BRender2DPrimitiveUtility::setWorldTransform(BMatrix worldMatrix)
{
   //   ASSERT_RENDER_THREAD

   XMMATRIX worldToProj = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj, false);
   XMMATRIX finalMatrix = XMMatrixMultiplyTranspose(worldMatrix, worldToProj);            
   BD3D::mpDev->SetVertexShaderConstantF(0, (float*) finalMatrix.m , 4);
}

//==============================================================================
// BRender2DPrimitiveUtility::releaseBuffers
//==============================================================================
void BRender2DPrimitiveUtility::releaseBuffers()
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// BRender2DPrimitiveUtility::applyBlendMode
//==============================================================================
void BRender2DPrimitiveUtility::applyBlendMode(eRender2DBlendMode mode)
{
   switch (mode)
   {
      case eRender2DBlendMode_Alphablend:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,          TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_SRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_INVSRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_ADD);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
            BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_NONE);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
            
         break;
         }
      case eRender2DBlendMode_Additive:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,          TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_ADD);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
            BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_NONE);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
         }
         break;
      case eRender2DBlendMode_Subtractive:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_SRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_REVSUBTRACT);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
            BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_NONE);                  
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
         }
         break;
   }
}

//==============================================================================
//==============================================================================
bool BRender2DPrimitiveUtility::BPrimitiveKeySorter::operator() (const B2DPrimitivePie& lhs, const B2DPrimitivePie& rhs) const
{   
   return lhs.keyCompare(rhs);
}

//==============================================================================
//==============================================================================
bool BRender2DPrimitiveUtility::BPrimitiveKeySorter::operator() (const B2DPrimitiveSprite& lhs, const B2DPrimitiveSprite& rhs) const
{   
   return lhs.keyCompare(rhs);
}
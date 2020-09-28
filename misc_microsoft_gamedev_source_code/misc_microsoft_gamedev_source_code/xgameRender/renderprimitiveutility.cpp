//==============================================================================
// renderutility.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================
#include "xgamerender.h"
#include "renderprimitiveutility.h"
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

//==============================================================================
// Globals
//==============================================================================
BRenderPrimitiveUtility gRenderPrimitiveUtility;

const int cBoxVertexCount = 8;
const D3DVECTOR cBoxVerts[cBoxVertexCount]= 
{
   {1.0f,  1.0f, -1.0f},
   {1.0f, -1.0f, -1.0f},
   {-1.0f,-1.0f, -1.0f},
   {-1.0f, 1.0f, -1.0f},

   {1.0f,  1.0f,  1.0f},
   {1.0f, -1.0f,  1.0f},
   {-1.0f,-1.0f,  1.0f},
   {-1.0f, 1.0f,  1.0f}
};

const int cBoxIndexCount = 24;
const WORD cBoxIndecies[cBoxIndexCount] = 
{0,1,2,3,  // front
 0,4,5,1,  // right
 4,7,6,5,  // back
 3,2,6,7,  // left
 7,4,0,3,  // top
 1,5,6,2};


const int cArrowVertexCount  = 7;
const D3DVECTOR cArrowVerts[cArrowVertexCount]=
{
   {-1.0f, 0.0f,  0.0f},
   { 0.0f, 0.0f,  1.0f},
   { 1.0f, 0.0f,  0.0f},

   { 0.5f, 0.0f,  0.0f},
   { 0.5f, 0.0f, -1.0f},
   {-0.5f, 0.0f, -1.0f},
   {-0.5f, 0.0f,  0.0f}   
};
const int cArrowIndexCount = 9;
const WORD cArrowIndecies[cArrowIndexCount]= {0,1,2,3,4,5,3,5,6};

const int cCircleDefaultNumSegments = 16;
const int cCircleNumVerts           = cCircleDefaultNumSegments + 1;

const int cThickCircleDefaultNumSegments = 128;
const int cThickCircleNumVerts           = cThickCircleDefaultNumSegments*2;


const int cSphereDefaultNumSegments = 16;
const int cSphereNumVerts           = cSphereDefaultNumSegments * (cSphereDefaultNumSegments+1);

const int cAxisVertexCount = 6;
const D3DVECTOR cAxisVerts[cAxisVertexCount]=
{
   { 1.0f,  0.0f,  0.0f},
   {-1.0f,  0.0f,  0.0f},
   { 0.0f,  1.0f,  0.0f},
   { 0.0f, -1.0f,  0.0f},
   { 0.0f,  0.0f,  1.0f},
   { 0.0f,  0.0f, -1.0f}
};

//==============================================================================
// BRenderPrimitiveUtility
//==============================================================================

//==============================================================================
// BRenderPrimitiveUtility::BRenderPrimitiveUtility
//==============================================================================
BRenderPrimitiveUtility::BRenderPrimitiveUtility() :
   mCommandListenerHandle(cInvalidCommandListenerHandle),
   mpBoxVB(NULL),
   mpBoxIB(NULL),
   mpCircleVB(NULL),
   mpThickCircleVB(NULL),
   mpArrowVB(NULL),
   mpArrowIB(NULL),
   mpSphereVB(NULL),
   mpTickLineVertexDecl(NULL)
{
   Utils::ClearObj(mUpdateData);
}

//==============================================================================
// BRenderPrimitiveUtility::~BRenderPrimitiveUtility
//==============================================================================
BRenderPrimitiveUtility::~BRenderPrimitiveUtility()
{
}

//==============================================================================
// BRenderPrimitiveUtility::init
//==============================================================================
void BRenderPrimitiveUtility::init(void)
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
// BRenderPrimitiveUtility::initVertexDeclarations
//==============================================================================
void BRenderPrimitiveUtility::initVertexDeclarations(void)
{
   const D3DVERTEXELEMENT9 BThickLineVertex_Elements[] =
   {    
      { 0, 0,   D3DDECLTYPE_FLOAT4,     D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 16,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BThickLineVertex_Elements, &mpTickLineVertexDecl);
}

//==============================================================================
// BRenderPrimitiveUtility::deInit
//==============================================================================
void BRenderPrimitiveUtility::deInit(void)
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

   if (mpTickLineVertexDecl)
   {
      mpTickLineVertexDecl->Release();
      mpTickLineVertexDecl = NULL;
   }

   
   Utils::ClearObj(mUpdateData);    

   reloadDeinit();
   eventReceiverDeinit();
}
//==============================================================================
// BRenderPrimitiveUtility::reloadInit
//==============================================================================
void BRenderPrimitiveUtility::reloadInit(void)
{
   BReloadManager::BPathArray paths;
   BString effectFilename;
   eFileManagerError result = gFileManager.getDirListEntry(effectFilename, gRender.getEffectCompilerDefaultDirID());
   BVERIFY(result == cFME_SUCCESS);
   strPathAddBackSlash(effectFilename);
   effectFilename += "renderprimitive\\primitive*.bin";
   paths.pushBack(effectFilename);

   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, eEventClassReloadEffects);
}
//==============================================================================
// BRenderPrimitiveUtility::reloadDeinit
//==============================================================================
void BRenderPrimitiveUtility::reloadDeinit(void)
{
   gReloadManager.deregisterClient(mEventHandle);
}
//==============================================================================
// BRenderPrimitiveUtility::loadEffect
//==============================================================================
void BRenderPrimitiveUtility::loadEffect()
{
   ASSERT_MAIN_OR_WORKER_THREAD

      //-- loads all of the effects up
      for (uint i = 0; i < eRenderCommandTotal; i++)
      {  
         BFixedString256 filename;
         filename.format("renderprimitive\\primitive%i.bin", i);

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
// BRenderPrimitiveUtility::receiveEvent
//==============================================================================
bool BRenderPrimitiveUtility::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
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
// BRenderPrimitiveUtility::initEffect
//==============================================================================
void BRenderPrimitiveUtility::initEffect(const BEvent& event)
{
   ASSERT_RENDER_THREAD
   BAsyncFileManager::BRequestPacket* pPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket *>(event.mpPayload);
   BDEBUG_ASSERT(pPacket!=NULL);

   uint effectID = pPacket->getPrivateData0();

   if (!pPacket->getSucceeded())
   {
      gConsoleOutput.output(cMsgError, "BRenderPrimitiveUtility::initEffect: Async load of file %s failed", pPacket->getFilename().c_str());
   }
   else
   {
      // rg [5/28/06] - This will randomly fail if the effect is ever left set on the D3D device! 
      // As of today, it appears 
      mEffects[effectID].clear();
      HRESULT hres = mEffects[effectID].createFromCompiledData(BD3D::mpDev, pPacket->getData());
      if (FAILED(hres))
      {
         gConsoleOutput.output(cMsgError, "BRenderPrimitiveUtility::initEffect: Effect creation of file %s failed", pPacket->getFilename().c_str());
      }
      else
      {
         trace("BRenderPrimitiveUtility::initEffect: Effect creation of file %s succeeded", pPacket->getFilename().c_str());
      }

      BDEBUG_ASSERT(mEffects[effectID].getNumTechniques() == 1);

      // Get techniques and params
      mTechnique[effectID]  = mEffects[effectID].getTechnique("Default");
      mColorParam[effectID] = mEffects[effectID]("gColor");
      mThicknessParam[effectID] = mEffects[effectID]("gThickness");
      mScaleParam[effectID] = mEffects[effectID]("gScaleMatrix");

      BDEBUG_ASSERT(mColorParam[effectID].getValid());
      BDEBUG_ASSERT(mScaleParam[effectID].getValid());
      BDEBUG_ASSERT(mThicknessParam[effectID].getValid());
   }
}

//==============================================================================
// BRenderPrimitiveUtility::initDeviceData
// init will be called from the worker thread after the D3D device is initialized.
//==============================================================================
void BRenderPrimitiveUtility::initDeviceData(void)
{
   ASSERT_RENDER_THREAD

   initBoxBuffers();
   initCircleBuffers();
   initThickCircleBuffers();
   initSphereBuffers();
   initArrowBuffers();
}

//==============================================================================
// void BRenderPrimitiveUtility::frameBegin(void);
// Called from worker thread.
//==============================================================================
void BRenderPrimitiveUtility::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// BRenderPrimitiveUtility::processCommand
// Called from the worker thread to process commands.
//==============================================================================
void BRenderPrimitiveUtility::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD   

   BDEBUG_ASSERT(header.mLen == sizeof(BRPUUpdateData));
   const BRPUUpdateData* pUpdateData = reinterpret_cast<const BRPUUpdateData*>(pData);            
   
   if ((header.mType >= 0) && (header.mType < eBRUCommandTotal))
   {
      BDEBUG_ASSERT(!mUpdateData[0][header.mType].mCount);
      mUpdateData[0][header.mType] = *pUpdateData;
   }
}

//==============================================================================
// BRenderPrimitiveUtility::frameEnd
// Called from worker thread.
//==============================================================================
void BRenderPrimitiveUtility::frameEnd(void)
{
   ASSERT_RENDER_THREAD
   
   Utils::ClearObj(mUpdateData);
}

//==============================================================================
// BRenderPrimitiveUtility::deinitDeviceData
// deinit will be called from the worker thread before the RCL is freed, but always before the D3D device is release.
//==============================================================================
void BRenderPrimitiveUtility::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
   //-- deallocate our vertex buffers here
   releaseBuffers();
   
   for (uint i = 0; i < eRenderCommandTotal; i++)
      mEffects[i].clear();
}

//==============================================================================
// BRenderPrimitiveUtility::update(void* pData)
//==============================================================================
void BRenderPrimitiveUtility::update(eBRUCommand type, int count, void* pData)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT(type < eBRUCommandTotal);

   if (count <= 0)
      return;

   BDEBUG_ASSERT(pData != NULL);
   
   int typeSize = 0;
   void* pBuffer = NULL;
   
   switch(type)
   {
      case eBRUUpdateLine:
      {
         const BPrimitiveLine* pLines = static_cast<const BPrimitiveLine*>(pData);
         
         BPDVertex* pV = static_cast<BPDVertex*>(gRenderDraw.lockDynamicVB(2 * count, sizeof(BPDVertex)));
         
         Utils::BPrefetchState prefetch = Utils::BeginPrefetch(pLines, pLines + count, 3);

         for (int i = 0; i < count; ++i)
         {
            prefetch = Utils::UpdatePrefetch(prefetch, &pLines[i], pLines + count, 3);
            
            pV->pos.x = pLines[i].mPoint1.x;
            pV->pos.y = pLines[i].mPoint1.y;
            pV->pos.z = pLines[i].mPoint1.z;
            pV->diffuse = pLines[i].mColor1;
            pV++;

            pV->pos.x = pLines[i].mPoint2.x;
            pV->pos.y = pLines[i].mPoint2.y;
            pV->pos.z = pLines[i].mPoint2.z;
            pV->diffuse = pLines[i].mColor2;
            pV++;
         }

         pBuffer = gRenderDraw.getDynamicVB();

         gRenderDraw.unlockDynamicVB();
         
         break;
      }

      case eBRUUpdateThickLine:
      {
         const BPrimitiveLine* pLines = static_cast<const BPrimitiveLine*>(pData);

         BThickLineVertex* pV = static_cast<BThickLineVertex*>(gRenderDraw.lockDynamicVB(2 * count, sizeof(BThickLineVertex)));

         Utils::BPrefetchState prefetch = Utils::BeginPrefetch(pLines, pLines + count, 3);

         XMVECTOR thicknessV, color1, color2;
         for (int i = 0; i < count; ++i)
         {
            prefetch = Utils::UpdatePrefetch(prefetch, &pLines[i], pLines + count, 3);

            thicknessV = XMLoadScalar(&pLines[i].mThickness);
            color1 = XMLoadColor((XMCOLOR*) &pLines[i].mColor1);
            color2 = XMLoadColor((XMCOLOR*) &pLines[i].mColor2);
                        
            XMStoreFloat4NC(&pV->mPos, __vrlimi(*(XMVECTOR*)&pLines[i].mPoint1, thicknessV, VRLIMI_CONST(0,0,0,1), 1));
            XMStoreColor(&pV->mColor, color1);
            pV++;

            XMStoreFloat4NC(&pV->mPos, __vrlimi(*(XMVECTOR*)&pLines[i].mPoint2, thicknessV, VRLIMI_CONST(0,0,0,1), 1));
            XMStoreColor(&pV->mColor, color2);
            pV++;
         }

         pBuffer = gRenderDraw.getDynamicVB();

         gRenderDraw.unlockDynamicVB();
         break;
      }

      case eBRUUpdateCircle:
      case eBRUUpdateSphere:
      case eBRUUpdateThickCircle:
      {
         typeSize = sizeof(BPrimitiveSphere);
         break;
      }
      case eBRUUpdateBox:
      case eBRUUpdateArrow:
      {
         typeSize = sizeof(BPrimitiveBox);
         break;
      }
      case eBRUUpdateText:
      {
         typeSize = sizeof(BPrimitiveText);
         break;
      }
   }

   const BOOL isSimThread = gRenderThread.isSimThread();
   
   if (!pBuffer)
   {
      BDEBUG_ASSERT(typeSize);
      
      int bufferSize = count * typeSize;
      pBuffer = isSimThread ? gRenderThread.allocateFrameStorage(bufferSize, 16) : gRenderThread.workerAllocateFrameStorage(bufferSize, 16);
   
      Utils::FastMemCpy(pBuffer, pData, bufferSize);
   }
   
   if (isSimThread)
   {
      void* pVoid = gRenderThread.submitCommandBegin(mCommandListenerHandle, type, sizeof(BRPUUpdateData));
      BDEBUG_ASSERT(pVoid);

      BRPUUpdateData* pRenderData = reinterpret_cast<BRPUUpdateData*>(pVoid);
      pRenderData->mCount = count;
      pRenderData->mpData = pBuffer;

      gRenderThread.submitCommandEnd(sizeof(BRPUUpdateData));
   }
   else
   {
      mUpdateData[1][type].mCount = count;
      mUpdateData[1][type].mpData = pBuffer;
   }      
}

//==============================================================================
// BRenderPrimitiveUtility::workerRender
//==============================================================================
void BRenderPrimitiveUtility::workerRender(void)
{
   ASSERT_RENDER_THREAD
   
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
         
   for (uint i = 0; i < cNumUpdateDataContexts; i++)
   {
      if (mUpdateData[i][eBRUUpdateLine].mCount)
         drawLinesInternal(&mUpdateData[i][eBRUUpdateLine]);

      if (mUpdateData[i][eBRUUpdateThickLine].mCount)
         drawThickLinesInternal(&mUpdateData[i][eBRUUpdateThickLine]);
         
      if (mUpdateData[i][eBRUUpdateCircle].mCount)
         drawCirclesInternal(&mUpdateData[i][eBRUUpdateCircle]);

      if (mUpdateData[i][eBRUUpdateThickCircle].mCount)
         drawThickCirclesInternal(&mUpdateData[i][eBRUUpdateThickCircle]);
         
      if (mUpdateData[i][eBRUUpdateBox].mCount)
         drawBoxesInternal(&mUpdateData[i][eBRUUpdateBox]);
         
      if (mUpdateData[i][eBRUUpdateArrow].mCount)
         drawArrowsInternal(&mUpdateData[i][eBRUUpdateArrow]);
      
      if (mUpdateData[i][eBRUUpdateSphere].mCount)
         drawSpheresInternal(&mUpdateData[i][eBRUUpdateSphere]);
         
      if (mUpdateData[i][eBRUUpdateText].mCount)
         drawTextInternal(&mUpdateData[i][eBRUUpdateText]);
   }         
   
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
}

//==============================================================================
// BRenderPrimitiveUtility::drawLinesInternal
//==============================================================================
void BRenderPrimitiveUtility::drawLinesInternal(const BRPUUpdateData* pData)
{
   ASSERT_RENDER_THREAD

   IDirect3DVertexBuffer9* pVB = static_cast<IDirect3DVertexBuffer9*>(pData->mpData);

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, pVB, 0, sizeof(BPDVertex));

   // Set vertex declaration to match, disable any vertex shaders.
   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);

   const XMMATRIX& worldToProj = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj, true);
   BD3D::mpDev->SetVertexShaderConstantF(0, (float*) worldToProj.m, 4);       

   BD3D::mpDev->SetVertexDeclaration(BPDVertex::msVertexDecl);

   // Draw the line.
   BD3D::mpDev->DrawVertices(D3DPT_LINELIST, 0, pData->mCount * 2);

   gRenderDraw.clearStreamSource(0);  
}

//==============================================================================
// BRenderPrimitiveUtility::drawThickLinesInternal
//==============================================================================
void BRenderPrimitiveUtility::drawThickLinesInternal(const BRPUUpdateData* pData)
{
   SCOPEDSAMPLE(RenderPrimitiveUtility_DrawThickLines);

   ASSERT_RENDER_THREAD

   if (!mEffects[eRenderThickLine].getEffect())
      return;

   IDirect3DVertexBuffer9* pVB = static_cast<IDirect3DVertexBuffer9*>(pData->mpData);

   // declare vertex type
   BD3D::mpDev->SetVertexDeclaration(mpTickLineVertexDecl);

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, pVB, 0, sizeof(BThickLineVertex));

   // update the intrinsics
   mEffects[eRenderThickLine].updateIntrinsicParams();

   uint techniquePassIndex = 0;
   mTechnique[eRenderThickLine].beginRestoreDefaultState();
      mTechnique[eRenderThickLine].beginPass(techniquePassIndex);
         
         mTechnique[eRenderThickLine].commit();   
         BD3D::mpDev->DrawVertices(D3DPT_QUADLIST, 0, pData->mCount * 4);

      mTechnique[eRenderThickLine].endPass();
   mTechnique[eRenderThickLine].end();

   BD3D::mpDev->SetVertexDeclaration(NULL);
   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
   BD3D::mpDev->SetPixelShader(NULL);
   BD3D::mpDev->SetVertexShader(NULL);
}

//==============================================================================
// BRenderPrimitiveUtility::setWorldTransform
//==============================================================================
void BRenderPrimitiveUtility::setWorldTransform(BMatrix worldMatrix, BVector scaleVector)
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
// BRenderPrimitiveUtility::setWorldTransform
//==============================================================================
void BRenderPrimitiveUtility::setWorldTransform(BMatrix worldMatrix)
{
   //   ASSERT_RENDER_THREAD

   XMMATRIX worldToProj = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj, false);
   XMMATRIX finalMatrix = XMMatrixMultiplyTranspose(worldMatrix, worldToProj);            
   BD3D::mpDev->SetVertexShaderConstantF(0, (float*) finalMatrix.m , 4);
}

//==============================================================================
// BRenderPrimitiveUtility::drawBoxesInternal
//==============================================================================
void BRenderPrimitiveUtility::drawBoxesInternal(const BRPUUpdateData* pData)
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pData != NULL);
   DWORD oldFillMode; 
   BD3D::mpDev->GetRenderState(D3DRS_FILLMODE, &oldFillMode);
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
   
   DWORD oldCullMode;
   BD3D::mpDev->GetRenderState(D3DRS_CULLMODE, &oldCullMode);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

   //-- set vertex type
   BD3D::mpDev->SetVertexDeclaration(BPDVertex::msVertexDecl);
   BD3D::mpDev->SetStreamSource(0, mpBoxVB, 0, sizeof(BPVertex));
   BD3D::mpDev->SetIndices(mpBoxIB);

   BD3D::mpDev->SetVertexShader(gFixedFuncShaders.workerGetVertexShader(cPosConstantDiffuseVS));
   BD3D::mpDev->SetPixelShader(gFixedFuncShaders.workerGetPixelShader(cDiffusePS));   
   
   BPrimitiveBox* pBoxes = reinterpret_cast<BPrimitiveBox*>(pData->mpData);
   BDEBUG_ASSERT(Utils::IsAligned(pBoxes, 16));
   BMatrix wMatrix;
   
   for (int i = 0; i < pData->mCount; i++)
   {      
      wMatrix = pBoxes[i].mMatrix;

      setWorldTransform(wMatrix, pBoxes[i].mScale);
      
      XMVECTOR colorValue = XMLoadColor((const XMCOLOR*)&pBoxes[i].mColor);
         
      BD3D::mpDev->SetVertexShaderConstantF(4, (const float*)&colorValue, 1);
      
      //-- draw
      BD3D::mpDev->DrawIndexedVertices(D3DPT_QUADLIST, 0, 0, cBoxIndexCount);      
   }
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, oldFillMode);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, oldCullMode);
}

//==============================================================================
// BRenderPrimitiveUtility::drawCirclesInternal
//==============================================================================
void BRenderPrimitiveUtility::drawCirclesInternal(const BRPUUpdateData* pData)
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pData != NULL);
   BDEBUG_ASSERT(pData->mpData != NULL);

   DWORD oldFillMode; 
   BD3D::mpDev->GetRenderState(D3DRS_FILLMODE, &oldFillMode);
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);


   //-- set vertex type
   BD3D::mpDev->SetVertexDeclaration(BPDVertex::msVertexDecl);
   BD3D::mpDev->SetStreamSource(0, mpCircleVB, 0, sizeof(BPVertex));

   BD3D::mpDev->SetVertexShader(gFixedFuncShaders.workerGetVertexShader(cPosConstantDiffuseVS));
   BD3D::mpDev->SetPixelShader(gFixedFuncShaders.workerGetPixelShader(cDiffusePS));   

   BVector scaleVector;
   
   BPrimitiveSphere* pCircle = reinterpret_cast<BPrimitiveSphere*>(pData->mpData);   
   BDEBUG_ASSERT(Utils::IsAligned(pCircle, 16));
   for (int i = 0; i < pData->mCount; i++)
   {            
      scaleVector.set(pCircle[i].mRadius, pCircle[i].mRadius, pCircle[i].mRadius);
      setWorldTransform(pCircle[i].mMatrix, scaleVector);
      
      XMVECTOR colorValue = XMLoadColor((const XMCOLOR*)&pCircle[i].mColor);
      
      BD3D::mpDev->SetVertexShaderConstantF(4, (const float*)&colorValue, 1);      
      
      //-- draw
      BD3D::mpDev->DrawVertices(D3DPT_LINESTRIP, 0, cCircleNumVerts);                              
   }
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, oldFillMode);
}

//==============================================================================
// BRenderPrimitiveUtility::drawThickCirclesInternal
//==============================================================================
void BRenderPrimitiveUtility::drawThickCirclesInternal(const BRPUUpdateData* pData)
{
   SCOPEDSAMPLE(RenderPrimitiveUtility_DrawThickCircles);
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pData != NULL);
   BDEBUG_ASSERT(pData->mpData != NULL);

   if (!mEffects[eRenderThickCircle].getEffect())
      return;

   DWORD oldFillMode; 
   BD3D::mpDev->GetRenderState(D3DRS_FILLMODE, &oldFillMode);
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

   //-- set vertex type
   BD3D::mpDev->SetVertexDeclaration(mpTickLineVertexDecl);
   BD3D::mpDev->SetStreamSource(0, mpThickCircleVB, 0, sizeof(BThickLineVertex));

   // update the intrinsics
   mEffects[eRenderThickCircle].updateIntrinsicParams();

   uint techniquePassIndex = 0;
   mTechnique[eRenderThickCircle].beginRestoreDefaultState();
   mTechnique[eRenderThickCircle].beginPass(techniquePassIndex);
   
   
   XMVECTOR scaleV, colorV;
   XMMATRIX scaleMatrix;
   BPrimitiveSphere* pCircle = reinterpret_cast<BPrimitiveSphere*>(pData->mpData);   
   BDEBUG_ASSERT(Utils::IsAligned(pCircle, 16));
   for (int i = 0; i < pData->mCount; i++)
   {                  
      scaleV = XMVectorReplicate(pCircle[i].mRadius);
      scaleV.w = 1.0f;
      scaleMatrix = XMMatrixScalingFromVector(scaleV);
      scaleMatrix = XMMatrixMultiply(scaleMatrix, *(XMMATRIX*)&(pCircle[i].mMatrix));

      colorV = XMLoadColor((const XMCOLOR*)&pCircle[i].mColor);

      mThicknessParam[eRenderThickCircle] = pCircle[i].mThickness;
      mColorParam[eRenderThickCircle]     = colorV;
      mScaleParam[eRenderThickCircle]     = scaleMatrix;

      mTechnique[eRenderThickCircle].commit();   

      //-- draw
      BD3D::mpDev->DrawVertices(D3DPT_QUADLIST, 0, cThickCircleNumVerts*4);                              
   }
   
   mTechnique[eRenderThickCircle].endPass();
   mTechnique[eRenderThickCircle].end();

   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, oldFillMode);
   BD3D::mpDev->SetVertexDeclaration(NULL);
   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
   BD3D::mpDev->SetPixelShader(NULL);
   BD3D::mpDev->SetVertexShader(NULL);
}

//==============================================================================
// BRenderPrimitiveUtility::drawArrowsInternal
//==============================================================================
void BRenderPrimitiveUtility::drawArrowsInternal(const BRPUUpdateData* pData)
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pData != NULL);
   BDEBUG_ASSERT(pData->mpData != NULL);

   DWORD oldFillMode;    
   BD3D::mpDev->GetRenderState(D3DRS_FILLMODE, &oldFillMode);      
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

   //-- set vertex type
   BD3D::mpDev->SetVertexDeclaration(BPDVertex::msVertexDecl);
   BD3D::mpDev->SetStreamSource(0, mpArrowVB, 0, sizeof(BPVertex));
   BD3D::mpDev->SetIndices(mpArrowIB);   

   BD3D::mpDev->SetVertexShader(gFixedFuncShaders.workerGetVertexShader(cPosConstantDiffuseVS));
   BD3D::mpDev->SetPixelShader(gFixedFuncShaders.workerGetPixelShader(cDiffusePS));   
   
   BPrimitiveBox* pPrimitive = reinterpret_cast<BPrimitiveBox*>(pData->mpData);
   BDEBUG_ASSERT(Utils::IsAligned(pPrimitive, 16));
   for (int i = 0; i < pData->mCount; i++)
   {            
      setWorldTransform(pPrimitive[i].mMatrix, pPrimitive[i].mScale);
      
      XMVECTOR colorValue = XMLoadColor((const XMCOLOR*)&pPrimitive[i].mColor);
      
      BD3D::mpDev->SetVertexShaderConstantF(4, (const float*)&colorValue, 1);
      
      //-- draw
      BD3D::mpDev->DrawIndexedVertices(D3DPT_TRIANGLELIST, 0, 0, cArrowIndexCount);
   }
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, oldFillMode);   
}

//==============================================================================
// BRenderPrimitiveUtility::drawSpheresInternal
//==============================================================================
void BRenderPrimitiveUtility::drawSpheresInternal(const BRPUUpdateData* pData)
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pData != NULL);
   BDEBUG_ASSERT(pData->mpData != NULL);

   DWORD oldFillMode; 
   BD3D::mpDev->GetRenderState(D3DRS_FILLMODE, &oldFillMode);
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

   //-- set vertex type
   BD3D::mpDev->SetVertexDeclaration(BPDVertex::msVertexDecl);
   BD3D::mpDev->SetStreamSource(0, mpSphereVB, 0, sizeof(BPVertex));

   BD3D::mpDev->SetVertexShader(gFixedFuncShaders.workerGetVertexShader(cPosConstantDiffuseVS));
   BD3D::mpDev->SetPixelShader(gFixedFuncShaders.workerGetPixelShader(cDiffusePS));   

   BVector scaleVector;
   
   BPrimitiveSphere* pPrimitive = reinterpret_cast<BPrimitiveSphere*>(pData->mpData);   
   BDEBUG_ASSERT(Utils::IsAligned(pPrimitive, 16));
   for (int i = 0; i < pData->mCount; i++)
   {            
      scaleVector.set(pPrimitive[i].mRadius, pPrimitive[i].mRadius, pPrimitive[i].mRadius);
      setWorldTransform(pPrimitive[i].mMatrix, scaleVector);

      XMVECTOR colorValue = XMLoadColor((const XMCOLOR*)&pPrimitive[i].mColor);

      BD3D::mpDev->SetVertexShaderConstantF(4, (const float*)&colorValue, 1);      
      
      //-- draw
      BD3D::mpDev->DrawVertices(D3DPT_TRIANGLESTRIP, 0, cSphereNumVerts);                              
   }
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, oldFillMode);
}

//==============================================================================
// BRenderPrimitiveUtility::drawTextInternal
//==============================================================================
void BRenderPrimitiveUtility::drawTextInternal(const BRPUUpdateData* pData)
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pData != NULL);
   BDEBUG_ASSERT(pData->mpData != NULL);
   
   const BPrimitiveText* pText = reinterpret_cast<const BPrimitiveText*>(pData->mpData);   
   
   BD3D::mpDev->SetVertexShader(gFixedFuncShaders.workerGetVertexShader(cPosDiffuseVS));
   BD3D::mpDev->SetPixelShader(gFixedFuncShaders.workerGetPixelShader(cDiffusePS));   
 
   const BVec3 posVec3(0.0f);
   const BVec3 rightVec3(1.0f, 0.0f, 0.0f);
   const BVec3 upVec3(0.0f, 1.0f, 0.0f);
   
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

   const int num = pData->mCount;
   Utils::BPrefetchState prefetchState = Utils::BeginPrefetch(pText, pText + num, 3);
   
   const XMVECTOR textScale = XMVectorSet(1.0f/8.0f, 1.0f/8.0f, 1.0f/8.0f, 0.0f);
   const XMMATRIX& viewToWorld = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTViewToWorld);
   const XMMATRIX worldToProj = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj);
   const XMVECTOR right = viewToWorld.r[0];
   const XMVECTOR up = viewToWorld.r[1];
   const XMVECTOR at = viewToWorld.r[2];

   const uint n = pData->mCount;      
   for (uint i = 0; i < n; i++)
   {
      prefetchState = Utils::UpdatePrefetch(prefetchState, &pText[i], pText + num, 3);
      const BPrimitiveText& text = pText[i];
      
      XMVECTOR scaleVec = textScale * XMVectorLoadFloatAndReplicate(&text.mSize);
      
      XMMATRIX modelToWorld;
      
      modelToWorld.r[0] = right * scaleVec;
      modelToWorld.r[1] = up * scaleVec;
      modelToWorld.r[2] = at * scaleVec;
      modelToWorld.r[3] = XMVectorSetWToOne(XMLoadFloat3((const XMFLOAT3*)text.mPos));
      
      XMMATRIX modelToProj;
      modelToProj = modelToWorld * worldToProj;
                        
      XMMATRIX modelToProjT = XMMatrixMultiplyTranspose(modelToWorld, worldToProj);            
            
      BD3D::mpDev->SetRenderState(D3DRS_ZFUNC, text.mZTest ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS);
      
      BD3D::mpDev->SetVertexShaderConstantF(0, (float*)&modelToProjT , 4);
                  
      BVectorFont::renderText(text.mText.c_str(), posVec3, upVec3, rightVec3, text.mColor);
   }
   
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
}

//==============================================================================
// BRenderPrimitiveUtility::initBoxBuffers
//==============================================================================
void BRenderPrimitiveUtility::initBoxBuffers()
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(BD3D::mpDev);
   
   int bufferSize = sizeof(BPVertex)*cBoxVertexCount;
   HRESULT hr = BD3D::mpDev->CreateVertexBuffer(bufferSize, D3DUSAGE_WRITEONLY, 0, 0, &mpBoxVB, NULL);
   BDEBUG_ASSERT(SUCCEEDED(hr));

   BPVertex* pVerts = NULL;
   mpBoxVB->Lock(0, 0, (void**) &pVerts, D3DLOCK_NOOVERWRITE);
   for (int i = 0; i < cBoxVertexCount; i++)
   {      
      pVerts->pos     = cBoxVerts[i];
      pVerts++;
   }
   mpBoxVB->Unlock();
   BDEBUG_ASSERT(SUCCEEDED(hr));

   int ibBufferSize = sizeof(WORD)*cBoxIndexCount;
   hr = BD3D::mpDev->CreateIndexBuffer(ibBufferSize, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, 0, &mpBoxIB, NULL);
   BDEBUG_ASSERT(SUCCEEDED(hr));
   WORD* pIndecies = NULL;

   mpBoxIB->Lock(0, 0, (void**) &pIndecies, D3DLOCK_NOOVERWRITE);
   BDEBUG_ASSERT(SUCCEEDED(hr));
   memcpy(pIndecies, cBoxIndecies, ibBufferSize);
   mpBoxIB->Unlock();
   BDEBUG_ASSERT(SUCCEEDED(hr));
}

//==============================================================================
// BRenderPrimitiveUtility::initCircleBuffers
//==============================================================================
void BRenderPrimitiveUtility::initCircleBuffers()
{   
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(BD3D::mpDev);

   float radius      = 1.0f;
   float radiansPerSegment = cTwoPi / cCircleDefaultNumSegments;
   float angularOffset = radiansPerSegment;
   
   int bufferSize = sizeof(BPVertex)*(cCircleNumVerts);

   HRESULT hr = BD3D::mpDev->CreateVertexBuffer(bufferSize, D3DUSAGE_WRITEONLY, 0, 0, &mpCircleVB, NULL);
   hr;
   BDEBUG_ASSERT(SUCCEEDED(hr));

   BPVertex* pVerts = NULL;
   mpCircleVB->Lock(0, 0, (void**) &pVerts, D3DLOCK_NOOVERWRITE);

   //-- add the first point   
   pVerts->pos.x = 0.0f;
   pVerts->pos.y = 0.0f;
   pVerts->pos.z = radius;
   pVerts++;

   for (int segment = 1; segment < cCircleDefaultNumSegments; ++segment)
   {
      float sinAngle = (float)sin(angularOffset);
      float cosAngle = (float)cos(angularOffset);

      pVerts->pos.x = sinAngle*radius;
      pVerts->pos.y = 0.0f;
      pVerts->pos.z = cosAngle*radius;
      pVerts++;
      angularOffset += radiansPerSegment;
   }

   pVerts->pos.x = 0.0f;
   pVerts->pos.y = 0.0f;
   pVerts->pos.z = radius;
      
   mpCircleVB->Unlock();
   BDEBUG_ASSERT(SUCCEEDED(hr));   
}

//==============================================================================
// BRenderPrimitiveUtility::initCircleBuffers
//==============================================================================
void BRenderPrimitiveUtility::initThickCircleBuffers()
{
   
   XMVECTOR points[cThickCircleDefaultNumSegments];
   float radiansPerSegment = Math::fTwoPi / cThickCircleDefaultNumSegments;
   float angularOffset = -XM_PI;

   float sinAngle, cosAngle;
   float radius = 1.0f;
   for (long segment = 0; segment < cThickCircleDefaultNumSegments; ++segment)
   {
      XMVECTOR& pt=points[segment];
      XMScalarSinCosEst(&sinAngle, &cosAngle, angularOffset);
      pt.x = sinAngle * radius;
      pt.y = 0.0f;
      pt.z = cosAngle * radius;
      pt.w = 0.2;
      angularOffset += radiansPerSegment;
   }

   int bufferSize = sizeof(BThickLineVertex)*(cThickCircleNumVerts);

   HRESULT hr = BD3D::mpDev->CreateVertexBuffer(bufferSize, D3DUSAGE_WRITEONLY, 0, 0, &mpThickCircleVB, NULL);
   hr;
   BDEBUG_ASSERT(SUCCEEDED(hr));

   BThickLineVertex* pVB = NULL;
   XMVECTOR oneV = XMVectorReplicate(1.0f);
   mpThickCircleVB->Lock(0, 0, (void**) &pVB, D3DLOCK_NOOVERWRITE);
   for(long i=0; i<cThickCircleDefaultNumSegments; i++)
   {
      XMStoreFloat4NC(&pVB->mPos, *(XMVECTOR*)&points[i]);
      XMStoreColor(&pVB->mColor, oneV);
      pVB++;

      if (i == cThickCircleDefaultNumSegments-1)
         XMStoreFloat4NC(&pVB->mPos, *((XMVECTOR*)&points[0]));
      else
         XMStoreFloat4NC(&pVB->mPos, *((XMVECTOR*)&points[i+1]));

      XMStoreColor(&pVB->mColor, oneV);
      pVB++;
   }

   mpThickCircleVB->Unlock();
}

//==============================================================================
// BRenderPrimitiveUtility::initSphereBuffers
//==============================================================================
void BRenderPrimitiveUtility::initSphereBuffers()
{
   ASSERT_RENDER_THREAD

   int n = cSphereDefaultNumSegments;
   int sphereVerts = cSphereNumVerts;
   int bufferSize = sizeof(BPVertex)*sphereVerts;

   HRESULT hr = BD3D::mpDev->CreateVertexBuffer(bufferSize, D3DUSAGE_WRITEONLY, 0, 0, &mpSphereVB, NULL);
   hr;
   BDEBUG_ASSERT(SUCCEEDED(hr));

   BPVertex* pVerts = NULL;
   mpSphereVB->Lock(0, 0, (void**) &pVerts, D3DLOCK_NOOVERWRITE);

   float theta1,theta2,theta3;
   BVector p;
   int vertCount = 0;
   for (int j=0;j<n/2;j++) 
   {
      theta1 = j * cTwoPi / n - Math::fHalfPi;
      theta2 = (j + 1) * cTwoPi / n - Math::fHalfPi;

      for (int i=0;i<=n;i++) 
      {
         theta3 = i * cTwoPi / n;
                 
         pVerts->pos.x = cos(theta2) * cos(theta3);
         pVerts->pos.y = sin(theta2);
         pVerts->pos.z = -(cos(theta2) * sin(theta3));
         pVerts++;

         pVerts->pos.x = cos(theta1) * cos(theta3);
         pVerts->pos.y = sin(theta1);
         pVerts->pos.z = -(cos(theta1) * sin(theta3));
         pVerts++;         

         vertCount += 2;
      }
   }

   BDEBUG_ASSERT(vertCount == cSphereNumVerts);
   mpSphereVB->Unlock();
}

//==============================================================================
// BRenderPrimitiveUtility::initArrowBuffers
//==============================================================================
void BRenderPrimitiveUtility::initArrowBuffers()
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(BD3D::mpDev);

   int bufferSize = sizeof(BPVertex)*cArrowVertexCount;
   HRESULT hr = BD3D::mpDev->CreateVertexBuffer(bufferSize, D3DUSAGE_WRITEONLY, 0, 0, &mpArrowVB, NULL);
   BDEBUG_ASSERT(SUCCEEDED(hr));

   BPVertex* pVerts = NULL;
   mpArrowVB->Lock(0, 0, (void**) &pVerts, D3DLOCK_NOOVERWRITE);
   for (int i = 0; i < cArrowVertexCount; i++)
   {      
      pVerts->pos     = cArrowVerts[i];
      pVerts++;
   }
   mpArrowVB->Unlock();
   BDEBUG_ASSERT(SUCCEEDED(hr));

   int ibBufferSize = sizeof(WORD)*cArrowIndexCount;
   hr = BD3D::mpDev->CreateIndexBuffer(ibBufferSize, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, 0, &mpArrowIB, NULL);
   BDEBUG_ASSERT(SUCCEEDED(hr));
   WORD* pIndecies = NULL;

   mpArrowIB->Lock(0, 0, (void**) &pIndecies, D3DLOCK_NOOVERWRITE);
   BDEBUG_ASSERT(SUCCEEDED(hr));
   memcpy(pIndecies, cArrowIndecies, ibBufferSize);
   mpArrowIB->Unlock();
   BDEBUG_ASSERT(SUCCEEDED(hr));
}

//==============================================================================
// BRenderPrimitiveUtility::releaseBuffers
//==============================================================================
void BRenderPrimitiveUtility::releaseBuffers()
{
   ASSERT_RENDER_THREAD
   if (mpBoxIB)
      mpBoxIB->Release();

   if (mpBoxVB)
      mpBoxVB->Release();

   if (mpCircleVB)
      mpCircleVB->Release();

   if (mpThickCircleVB)
      mpThickCircleVB->Release();

   if (mpArrowIB)
      mpArrowIB->Release();

   if (mpArrowVB)
      mpArrowVB->Release();

   if (mpSphereVB)
      mpSphereVB->Release();
}
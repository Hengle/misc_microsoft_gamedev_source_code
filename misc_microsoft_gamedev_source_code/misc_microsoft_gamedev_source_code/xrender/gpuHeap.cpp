//============================================================================
//
// File: gpuHeap.cpp
//  
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "gpuHeap.h"
#include "renderDraw.h"
#include "threading\eventDispatcher.h"

BGPUHeap gGPUFrameHeap;

//============================================================================
// BGPUHeap::BGPUHeap
//============================================================================
BGPUHeap::BGPUHeap() :
   mAllocatorFlags(0),
   mTotalResources(0),
   mInitialBlockSize(0),
   mBlockGrowSize(0),
   mProtect(0)
{
}

//============================================================================
// BGPUHeap::~BGPUHeap
//============================================================================
BGPUHeap::~BGPUHeap()
{
}

//============================================================================
// BGPUHeap::init
//============================================================================   
void BGPUHeap::init(uint initialBlockSize, uint blockGrowSize, DWORD protect)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if (mAllocator.getInitialized())
      return;
      
   if ((!initialBlockSize) && (!blockGrowSize))
      return;
   
   mInitialBlockSize = initialBlockSize;
   mBlockGrowSize = blockGrowSize;
   mProtect = protect;
            
   mAllocatorFlags = 0;
   mTotalResources = 0;
   
   if (!mAllocator.init(mInitialBlockSize, mBlockGrowSize, mProtect))
      BFATAL_FAIL("Out of memory");
   
   commandListenerInit();
}

//============================================================================
// BGPUHeap::deinit
//============================================================================
void BGPUHeap::deinit(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if (mTotalResources)
   {
      trace("GPUHeap::deinit: Heap is going away, but there are %u active resources!", mTotalResources);
   }
               
   mAllocator.kill();
   mTotalResources = 0;
   mInitialBlockSize = 0;
   mBlockGrowSize = 0;
   mProtect = 0;
   
   commandListenerDeinit();
}
 
//============================================================================
// BGPUHeap::createArrayTexture
//============================================================================     
HRESULT BGPUHeap::createArrayTexture(
   UINT Width,
   UINT Height,
   UINT ArraySize,
   UINT Levels,
   DWORD Usage,
   D3DFORMAT Format,
   D3DPOOL UnusedPool,
   IDirect3DArrayTexture9 **ppArrayTexture,
   HANDLE *pUnusedSharedHandle)
{
   ASSERT_THREAD(cThreadIndexRender);
         
   UnusedPool;
   pUnusedSharedHandle;
   
   BDEBUG_ASSERT(ppArrayTexture);
   
   *ppArrayTexture = NULL;
   
   if (!mAllocator.getInitialized())
      return E_FAIL;
      
   IDirect3DArrayTexture9* pArrayTex = (IDirect3DArrayTexture9*)gRenderHeap.New(sizeof(IDirect3DArrayTexture9), NULL, true);
   if (!pArrayTex)
      return E_OUTOFMEMORY;
      
   const uint physicalSize = XGSetArrayTextureHeader(Width, Height, ArraySize, Levels, Usage, Format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, pArrayTex, NULL, NULL);
   
   if (!physicalSize)
   {
      gRenderHeap.Delete(pArrayTex);
      return E_FAIL;
   }
   
   void* pPhysicalAlloc = mAllocator.alloc(physicalSize, GPU_TEXTURE_ALIGNMENT, mAllocatorFlags);
   if (!pPhysicalAlloc)
   {
      gRenderHeap.Delete(pArrayTex);
      return E_FAIL;
   }
   
   XGOffsetResourceAddress(pArrayTex, pPhysicalAlloc);
   
   *ppArrayTexture = pArrayTex;
   
   mTotalResources++;
      
   return S_OK;
}

//============================================================================
// BGPUHeap::createTexture
//============================================================================
HRESULT BGPUHeap::createTexture(
   UINT Width,
   UINT Height,
   UINT Levels,
   DWORD Usage,
   D3DFORMAT Format,
   D3DPOOL UnusedPool,
   IDirect3DTexture9 **ppTexture,
   HANDLE *pUnusedSharedHandle)
{
   ASSERT_THREAD(cThreadIndexRender);
 
   UnusedPool;
   pUnusedSharedHandle;

   BDEBUG_ASSERT(ppTexture);
   
   *ppTexture = NULL;
   
   if (!mAllocator.getInitialized())
      return E_FAIL;
   
   IDirect3DTexture9* pTex = (IDirect3DTexture9*)gRenderHeap.New(sizeof(IDirect3DTexture9), NULL, true);
   if (!pTex)
      return E_OUTOFMEMORY;

   const uint physicalSize = XGSetTextureHeader(Width, Height, Levels, Usage, Format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, pTex, NULL, NULL);

   if (!physicalSize)
   {
      gRenderHeap.Delete(pTex);
      return E_FAIL;
   }

   void* pPhysicalAlloc = mAllocator.alloc(physicalSize, GPU_TEXTURE_ALIGNMENT, mAllocatorFlags);
   if (!pPhysicalAlloc)
   {
      gRenderHeap.Delete(pTex);
      return E_FAIL;
   }

   XGOffsetResourceAddress(pTex, pPhysicalAlloc);

   *ppTexture = pTex;
   
   mTotalResources++;

   return S_OK;  
}

//============================================================================
// BGPUHeap::createVertexBuffer
//============================================================================
HRESULT BGPUHeap::createVertexBuffer(
   UINT Length,
   DWORD Usage,
   DWORD UnusedFVF,
   D3DPOOL UnusedPool,
   IDirect3DVertexBuffer9 **ppVertexBuffer,
   HANDLE *pUnusedSharedHandle)
{
   ASSERT_THREAD(cThreadIndexRender);
      
   UnusedPool;
   pUnusedSharedHandle;

   BDEBUG_ASSERT(ppVertexBuffer);

   *ppVertexBuffer = NULL;   
   
   if (!mAllocator.getInitialized())
      return E_FAIL;
   
   if (!Length)
      return E_FAIL;
   
   IDirect3DVertexBuffer9* pVB = (IDirect3DVertexBuffer9*)gRenderHeap.New(sizeof(IDirect3DVertexBuffer9), NULL, true);
   if (!pVB)
      return E_OUTOFMEMORY;
      
   XGSetVertexBufferHeader(Length, Usage, 0, 0, pVB);

   void* pPhysicalAlloc = mAllocator.alloc(Length, BPhysicalAllocator::cMinAlignment, mAllocatorFlags);
   if (!pPhysicalAlloc)
   {
      gRenderHeap.Delete(pVB);
      return E_FAIL;
   }

   XGOffsetResourceAddress(pVB, pPhysicalAlloc);

   *ppVertexBuffer = pVB;
   
   mTotalResources++;

   return S_OK;  
}   

//============================================================================
// BGPUHeap::createIndexBuffer
//============================================================================
HRESULT BGPUHeap::createIndexBuffer(
   UINT Length,
   DWORD Usage,
   D3DFORMAT Format,
   D3DPOOL UnusedPool,
   IDirect3DIndexBuffer9 **ppIndexBuffer,
   HANDLE *pUnusedSharedHandle)
{
   ASSERT_THREAD(cThreadIndexRender);
         
   UnusedPool;
   pUnusedSharedHandle;
      
   BDEBUG_ASSERT(ppIndexBuffer);

   *ppIndexBuffer = NULL;   
   
   if (!mAllocator.getInitialized())
      return E_FAIL;
   
   if (!Length)
      return E_FAIL;
   
   IDirect3DIndexBuffer9* pIB = (IDirect3DIndexBuffer9*)gRenderHeap.New(sizeof(IDirect3DIndexBuffer9), NULL, true);
   if (!pIB)
      return E_OUTOFMEMORY;

   XGSetIndexBufferHeader(Length, Usage, Format, 0, 0, pIB);

   void* pPhysicalAlloc = mAllocator.alloc(Length, BPhysicalAllocator::cMinAlignment, mAllocatorFlags);
   if (!pPhysicalAlloc)
   {
      gRenderHeap.Delete(pIB);
      return E_FAIL;
   }

   XGOffsetResourceAddress(pIB, pPhysicalAlloc);

   *ppIndexBuffer = pIB;
   
   mTotalResources++;

   return S_OK;  
}   

//============================================================================
// BGPUHeap::createRenderTarget
//============================================================================
HRESULT BGPUHeap::createRenderTarget(
   UINT Width,
   UINT Height,
   D3DFORMAT Format,
   D3DMULTISAMPLE_TYPE MultiSample,
   DWORD UnusedMultisampleQuality,
   BOOL UnusedLockable,
   IDirect3DSurface9 **ppSurface,
   CONST D3DSURFACE_PARAMETERS *pParameters)
{
   UnusedMultisampleQuality;
   UnusedLockable;
   
   ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(ppSurface);
   
   // pParameters cannot be NULL because we don't have access to D3D's EDRAM allocator.
   BDEBUG_ASSERT(pParameters);

   *ppSurface = NULL;

   if (!mAllocator.getInitialized())
      return E_FAIL;
      
   IDirect3DSurface9* pSurf = (IDirect3DSurface9*)gRenderHeap.New(sizeof(IDirect3DSurface9), NULL, true);
   if (!pSurf)
      return E_OUTOFMEMORY;
   
   uint hierarchicalZSize = 0;
   uint sizeInTiles = XGSetSurfaceHeader(Width, Height, Format, MultiSample, pParameters, pSurf, &hierarchicalZSize);
   sizeInTiles;
   hierarchicalZSize;
   
   // bugfix for XDK v5426
   pSurf->Identifier = 0;
   pSurf->ReadFence = 0;
   pSurf->Fence = 0;
   
   XGOffsetSurfaceAddress(pSurf, 0, 0);
   
   *ppSurface = pSurf;

   mTotalResources++;

   return S_OK;  
}      

//============================================================================
// BGPUHeap::releaseResource
//============================================================================
void BGPUHeap::releaseD3DResource(D3DResource* pResource)
{
   ASSERT_THREAD(cThreadIndexRender);
      
   if (!pResource)
      return;
      
   if (!mAllocator.getInitialized())
      return;
      
   // if ReferenceCount > 1, you probably have the resource set to the device or a state block!
   BDEBUG_ASSERT(pResource->ReferenceCount == 1);
      
   BDEBUG_ASSERT(mTotalResources > 0);
      
   BDEBUG_ASSERT((pResource->Common & D3DCOMMON_D3DCREATED) == 0);
   
   void* pPhysicalAlloc = gRenderDraw.getResourceAddress(pResource, false);
            
   if (pPhysicalAlloc)
   {
      bool success = mAllocator.free(pPhysicalAlloc);
      if (!success)
      {
         BFATAL_FAIL("BGPUHeap::releaseResource: Free failed");
      }
   }      
   
   gRenderHeap.Delete(pResource);
   
   mTotalResources--;
}

//============================================================================
// BGPUHeap::beginLevelLoad
//============================================================================
void BGPUHeap::beginLevelLoad(void)
{
   BASSERT(!mTotalResources);
   if (mTotalResources)
      return;
      
   mAllocator.kill();         
}

//============================================================================
// BGPUHeap::endLevelLoad
//============================================================================
void BGPUHeap::endLevelLoad(void)
{
   if (mTotalResources)
      return;
      
   if (!mAllocator.init(mInitialBlockSize, mBlockGrowSize, mProtect))
   {
      BFATAL_FAIL("Out of memory");
   }
}


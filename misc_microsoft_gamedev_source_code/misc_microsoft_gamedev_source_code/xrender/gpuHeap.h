//============================================================================
//
// File: gpuHeap.h
//  
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
#include "physicalAllocator.h"
#include "renderThread.h"

//============================================================================
// class BGPUHeap
// Only callable from the render thread!
// No CPU/GPU synchronization is done by this class!
//============================================================================
class BGPUHeap : public BRenderCommandListener
{
public:
   BGPUHeap();
   ~BGPUHeap();
   
   void init(uint blockSize, uint blockGrowSize, DWORD protect = PAGE_READWRITE | PAGE_WRITECOMBINE);
   
   // Don't call deinit() until you are sure the GPU is no longer using any resources in the heap!
   void deinit(void);

   // Be very careful freeing blocks back to the OS! If the GPU is currently using a block, bad things could happen.
   const BPhysicalAllocator& getAllocator(void) const { return mAllocator; }
         BPhysicalAllocator& getAllocator(void)       { return mAllocator; }
   
   uint getAllocatorFlags(void) { return mAllocatorFlags; }
   void setAllocatorFlags(uint flags) { mAllocatorFlags = flags; }
   
   uint getTotalResources(void) const { return mTotalResources; }

   // Will return E_FAIL or E_OUTOFMEMORY if the allocation fails.
   HRESULT createArrayTexture(
      UINT Width,
      UINT Height,
      UINT ArraySize,
      UINT Levels,
      DWORD Usage,
      D3DFORMAT Format,
      D3DPOOL UnusedPool,
      IDirect3DArrayTexture9 **ppArrayTexture,
      HANDLE *pUnusedSharedHandle);
   
   // Will return E_FAIL or E_OUTOFMEMORY if the allocation fails.
   HRESULT createTexture(
      UINT Width,
      UINT Height,
      UINT Levels,
      DWORD Usage,
      D3DFORMAT Format,
      D3DPOOL UnusedPool,
      IDirect3DTexture9 **ppTexture,
      HANDLE *pUnusedSharedHandle);

   // Will return E_FAIL or E_OUTOFMEMORY if the allocation fails.
   HRESULT createVertexBuffer(
      UINT Length,
      DWORD Usage,
      DWORD UnusedFVF,
      D3DPOOL UnusedPool,
      IDirect3DVertexBuffer9 **ppVertexBuffer,
      HANDLE *pUnusedSharedHandle);
   
   // Will return E_FAIL or E_OUTOFMEMORY if the allocation fails.   
   HRESULT createIndexBuffer(
      UINT Length,
      DWORD Usage,
      D3DFORMAT Format,
      D3DPOOL UnusedPool,
      IDirect3DIndexBuffer9 **ppIndexBuffer,
      HANDLE *pUnusedSharedHandle);

   // Will return E_FAIL or E_OUTOFMEMORY if the allocation fails.      
   HRESULT createRenderTarget(
      UINT Width,
      UINT Height,
      D3DFORMAT Format,
      D3DMULTISAMPLE_TYPE MultiSample,
      DWORD UnusedMultisampleQuality,
      BOOL UnusedLockable,
      IDirect3DSurface9 **ppSurface,
      CONST D3DSURFACE_PARAMETERS *pParameters);
      
   // It is the caller's responsibility to unsure the resource is not set on the device or any state blocks!
   void releaseD3DResource(D3DResource* pResource);
               
private:
   BPhysicalAllocator mAllocator;
   uint mAllocatorFlags;
   uint mTotalResources;
   
   uint mInitialBlockSize;
   uint mBlockGrowSize;
   DWORD mProtect;
   
   virtual void beginLevelLoad(void);
   virtual void endLevelLoad(void);
};

//============================================================================
// Rules for using the GPU frame heap, in addition to the rules for using BGPUHeap:
// - You cannot lock any resources on the GPU frame heap.
// - You cannot read or write to any resources on the GPU frame heap with the CPU.
// - You must always free resources created by this class by the end of the frame.
// - The GPU frame heap is not growable. Always check the HRESULT return value
//   from allocations.
//============================================================================
extern BGPUHeap gGPUFrameHeap;


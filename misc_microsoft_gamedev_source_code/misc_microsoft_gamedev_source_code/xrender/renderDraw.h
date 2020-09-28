//============================================================================
//
//  renderDraw.h
//  
//  Copyright (c) 2006-2008, Ensemble Studios
//
//============================================================================
#pragma once

// local
#include "renderViewport.h"
#include "matrixTracker.h"
#include "renderCommand.h"
#include "D3DTextureManager.h"
#include "volumeCuller.h"
#include "RGBAImage.h"

class BDynamicGPUBuffer;

// Render draw commands
enum eRDCommands
{
   cRDSetSceneRenderViewport = 200,
   cRDSetActiveRenderViewport,
   
   cRDSetSceneMatrixTracker,
   cRDSetActiveMatrixTracker,
   
   cRDSetSceneVolumeCuller,
   cRDSetActiveVolumeCuller,
   
   cRDClear,
   cRDClearF,
   
   cRDPresent,
   cRDBeginScene,
   cRDEndScene,
   
   cRDStateBlockCapture,
   
   cRDSetRenderTarget,
   cRDSetDepthStencilSurface,
   cRDSetRenderState,
   cRDSetSamplerState,
   cRDSetScissorRect,
   cRDSetViewport,
   cRDSetTexture,

   cRDSetStreamSource,
   cRDSetIndices,
   cRDDrawVertices,
   cRDDrawVerticesUP,
   cRDDrawIndexedVertices,
   cRDDrawIndexedVerticesUP,
   cRDSetPixelShader,
   cRDSetVertexShader,
   cRDSetPixelShaderConstantF,
   cRDSetVertexShaderConstantF,
   cRDSetVertexDeclaration,
   cRDSetFVF,
   cRDResolve,
   cRDUnsetAll,
   
   cRDStateBlockApply,
      
   cRDReleaseResource,     //(D3DResource* pResource);
   cRDReleaseVertexShader, //(IDirect3DVertexShader9* pResource);
   cRDReleasePixelShader,  //(IDirect3DPixelShader9* pResource);
   cRDReleaseVertexDecl,   //(IDirect3DVertexDeclaration9* pResource);
   
   cRDInvalidateGPUCache,
   
   cRDClearStreamSource,
   
   cRDSetDefaultRenderStates,
   cRDSetDefaultSamplerStates,
   
   cRDUnsetResources,
   cRDUnsetTextures,
   
   cRDSaveScreenshot,
   
   cRDMax
};

#define RENDERDRAW_BACKBUFFER_PTR ((IDirect3DSurface9*)0x1)
#define BRENDERDRAW_DEPTHSTENCIL_PTR ((IDirect3DSurface9*)0x2)

//============================================================================
// class BRenderDraw
//============================================================================
class BRenderDraw : public BRenderCommandListenerInterface
{
public:
   BRenderDraw();

   // Must call after BRenderThread is initialized
   bool init(void);
   
   // Must call before BRenderThread is deinitialized
   bool deinit(void);
   
   // Callable from any thread after the D3D device is initialized.
   const D3DPRESENT_PARAMETERS& getPresentParams(void) const { return mPresentParams; }
   const D3DDISPLAYMODE& getDisplayMode(void) const { return mDisplayMode; }
         
   // Callable from any thread after the D3D device is initialized.
   IDirect3DBaseTexture9* getDevFrontBuffer(void) const;
   IDirect3DSurface9* getDevBackBuffer(void) const;
   IDirect3DSurface9* getDevDepthStencil(void) const;
         
   // The create methods can be called from any thread.
   // releaseD3DResource should be called to release the returned object.
   // (Unless you are VERY careful, because if you submit rendering commands with any D3D resource, 
   // you can't free them until the worker thread and the GPU have finished using this resource.)
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

   HRESULT createCubeTexture(
      UINT EdgeLength,
      UINT Levels,
      DWORD Usage,
      D3DFORMAT Format,
      D3DPOOL UnusedPool,
      IDirect3DCubeTexture9 **ppCubeTexture,
      HANDLE *pUnusedSharedHandle);

   HRESULT createDepthStencilSurface(
      UINT Width,
      UINT Height,
      D3DFORMAT Format,
      D3DMULTISAMPLE_TYPE MultiSample,
      DWORD UnusedMultisampleQuality,
      BOOL UnusedDiscard,
      IDirect3DSurface9 **ppSurface,
      CONST D3DSURFACE_PARAMETERS *pParameters);

   HRESULT createIndexBuffer(
      UINT Length,
      DWORD Usage,
      D3DFORMAT Format,
      D3DPOOL UnusedPool,
      IDirect3DIndexBuffer9 **ppIndexBuffer,
      HANDLE *pUnusedSharedHandle);

   HRESULT createPixelShader(
      CONST DWORD *pFunction,
      IDirect3DPixelShader9 **ppShader);

   HRESULT createRenderTarget(
      UINT Width,
      UINT Height,
      D3DFORMAT Format,
      D3DMULTISAMPLE_TYPE MultiSample,
      DWORD UnusedMultisampleQuality,
      BOOL UnusedLockable,
      IDirect3DSurface9 **ppSurface,
      CONST D3DSURFACE_PARAMETERS *pParameters);

   HRESULT createLineTexture(
      UINT Width,
      UINT Levels,
      DWORD Usage,
      D3DFORMAT Format,
      D3DPOOL UnusedPool,
      IDirect3DLineTexture9 **ppTexture,
      HANDLE *pUnusedSharedHandle);

   HRESULT createTexture(
      UINT Width,
      UINT Height,
      UINT Levels,
      DWORD Usage,
      D3DFORMAT Format,
      D3DPOOL UnusedPool,
      IDirect3DTexture9 **ppTexture,
      HANDLE *pUnusedSharedHandle);

   HRESULT createVertexBuffer(
      UINT Length,
      DWORD Usage,
      DWORD UnusedFVF,
      D3DPOOL UnusedPool,
      IDirect3DVertexBuffer9 **ppVertexBuffer,
      HANDLE *pUnusedSharedHandle);

   HRESULT createVertexDeclaration(
      CONST D3DVERTEXELEMENT9 *pVertexElements,
      IDirect3DVertexDeclaration9 **ppVertexDeclaration);

   HRESULT createVertexShader(
      CONST DWORD *pFunction,
      IDirect3DVertexShader9 **ppShader);

   HRESULT createVolumeTexture(
      UINT Width,
      UINT Height,
      UINT Depth,
      UINT Levels,
      DWORD Usage,
      D3DFORMAT Format,
      D3DPOOL UnusedPool,
      IDirect3DVolumeTexture9 **ppVolumeTexture,
      HANDLE *pUnusedSharedHandle);

   // Safely releases a D3D resource.
   // Currently, these are only callable from the main or worker threads.
   // If releaseInWorkerThread is true, the call to Release() will be queued into the command buffer and processed by the
   // worker thread. DO NOT use the resource pointer after submitting a async/deferred release!
   // Otherwise, the method will block the calling thread until the worker thread is idle before releasing the resource. 
   // The resource must be unset from the device before calling release!
   void releaseD3DResource(D3DResource* pResource, bool releaseInWorkerThread = true);
   void releaseD3DResource(IDirect3DVertexShader9* pResource, bool releaseInWorkerThread = true);
   void releaseD3DResource(IDirect3DPixelShader9* pResource, bool releaseInWorkerThread = true);
   void releaseD3DResource(IDirect3DVertexDeclaration9* pResource, bool releaseInWorkerThread = true);

   // These direct rendering methods are intended for 2D/UI/debug rendering.
   // These are now callable from the main or render threads.
   // If called from the main thread, all parameters are copied into the command buffer. 
   // The UP() draw methods automatically copy the vertices/indices into frame storage.
   void stateBlockCapture(void);
   void stateBlockApply(void);
   void setRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9 *pRenderTarget);
   void setDepthStencilSurface(IDirect3DSurface9 *pZStencilSurface);
   void setRenderState(D3DRENDERSTATETYPE State, DWORD Value);
   void setSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
   void setScissorRect(CONST RECT *pRect);
   void setViewport(CONST D3DVIEWPORT9* pViewport);
   void setTexture(DWORD Sampler, IDirect3DBaseTexture9 *pTexture);
   void setTextureByHandle(DWORD Sampler, BManagedTextureHandle textureHandle, eDefaultTexture defaultTexture = cDefaultTextureInvalid);
   void setStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride);
   void clearStreamSource(UINT StreamNumber);
   void setIndices(IDirect3DIndexBuffer9 *pIndexData);
   void drawVertices(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT VertexCount);
   void drawVerticesUP(D3DPRIMITIVETYPE PrimitiveType, UINT VertexCount, CONST void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
   void drawIndexedVertices(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex,UINT StartIndex,UINT IndexCount);
   void drawIndexedVerticesUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT IndexCount, CONST void *pIndexData, D3DFORMAT IndexDataFormat, CONST void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
   void setPixelShader(IDirect3DPixelShader9 *pShader);
   void setVertexShader(IDirect3DVertexShader9 *pShader);
   void setPixelShaderConstantF(UINT StartRegister, CONST float *pConstantData, DWORD Vector4fCount);
   void setVertexShaderConstantF(UINT StartRegister, CONST float *pConstantData, DWORD Vector4fCount);
   void setVertexDeclaration(IDirect3DVertexDeclaration9 *pDecl);
   void setFVF(DWORD fvf);
   void resolve(
      DWORD Flags,
      CONST D3DRECT *pSourceRect,
      IDirect3DBaseTexture9 *pDestTexture,
      CONST D3DPOINT *pDestPoint,
      UINT DestLevel,
      UINT DestSliceOrFace,
      CONST D3DVECTOR4 *pClearColor,
      float ClearZ,
      DWORD ClearStencil,
      CONST D3DRESOLVE_PARAMETERS *pParameters);

   void unsetAll(void);
         
   void dummyPresent(void);
   
   void present(CONST RECT *pSourceRect = NULL, CONST RECT *pDestRect = NULL);

   void clear(
      DWORD Count,
      CONST D3DRECT *pRects,
      DWORD Flags,
      D3DCOLOR Color,
      float Z,
      DWORD Stencil);
   
   void clear(DWORD Flags = D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR Color = 0xFFFFFFFF, float Z = 1.0f, DWORD Stencil = 0)
   {
      clear(0, NULL, Flags, Color, Z, Stencil);
   }
   
   void clearF(DWORD Flags, CONST D3DRECT *pRect, CONST D3DVECTOR4 *pColor, float Z, DWORD Stencil);
     
   void setDefaultRenderStates(void);
   
   void setDefaultSamplerStates(void);

   void beginScene(void);
   void endScene(void);
   
   void kickPushBuffer(void);

   // Updates the scene matrix tracker on the main thread, and submits a copy to update the worker thread.
   // Also updates the culling manager on the main and worker threads.
   void setMainSceneMatrixTracker(const BMatrixTracker& tracker);
   
   // Updates the active matrix tracker on the main thread, and submits a copy to update the worker thread.
   // setActiveMatrixTracker() will also update the matrix related effect intrinsics, and the culling manager, on the main and worker threads.
   void setMainActiveMatrixTracker(const BMatrixTracker& tracker);
   
   // updateEffectIntrinsics() is called by setActiveMatrixTracker(), so it shouldn't be necessary to call it normally.
   // Callable from the main or worker threads.
   void updateEffectIntrinsics(void);
   
   // Updates the scene or active render viewport on the main thread, and submits a copy to update the worker thread.
   void setMainSceneRenderViewport(const BRenderViewport& viewport);
   void setMainActiveRenderViewport(const BRenderViewport& viewport);
   
   // Updates the scene or active render viewport on the main thread, and submits a copy to update the worker thread.
   void setMainSceneVolumeCuller(const BVolumeCuller& volumeCuller);
   void setMainActiveVolumeCuller(const BVolumeCuller& volumeCuller);
               
   // callable from main thread only
   BRenderViewport& getMainSceneRenderViewport(void);
   BRenderViewport& getMainActiveRenderViewport(void);

   // callable from main thread only         
   BMatrixTracker& getMainSceneMatrixTracker(void);
   BMatrixTracker& getMainActiveMatrixTracker(void);

   // callable from main thread only                  
   BVolumeCuller& getMainSceneVolumeCuller(void);
   BVolumeCuller& getMainActiveVolumeCuller(void);
   
   // callable from worker thread only
   BRenderViewport& getWorkerSceneRenderViewport(void);
   BRenderViewport& getWorkerActiveRenderViewport(void);
   void setWorkerActiveRenderViewport(const BRenderViewport& renderViewport);

   // callable from worker thread only         
   BMatrixTracker& getWorkerSceneMatrixTracker(void);
   BMatrixTracker& getWorkerActiveMatrixTracker(void);
   void setWorkerActiveMatrixTracker(const BMatrixTracker& matrixTracker);
         
   // callable from worker thread only         
   BVolumeCuller& getWorkerSceneVolumeCuller(void);
   BVolumeCuller& getWorkerActiveVolumeCuller(void);
   void setWorkerActiveVolumeCuller(const BVolumeCuller& volumeCuller);
   
   // Sets the active matrix tracker, render viewport, and volume culler to the scene's.
   void resetMainActiveMatricesAndViewport(void);
   void resetWorkerActiveMatricesAndViewport(void);
   
   struct BViewportDesc
   {
      // These values describe the UI viewport - mXOfs, mYOfs will be non-zero for some viewports.
      uint mXOfs;
      uint mYOfs;
      uint mWidth;
      uint mHeight;
      BRenderViewport mRenderViewport;
      BMatrixTracker  mMatrixTracker;
      BVolumeCuller   mVolumeCuller;
   };
   enum { cMaxViewports = 2 };
   
   // Viewport methods callable from the main or render threads.
   uint getNumViewports() const;
   BOOL getVerticalSplit() const;
   // Returns -1 for full-screen UI
   int getViewportIndex() const;
   const BViewportDesc& getViewportDesc(uint viewportIndex) const;
   void setViewportIndex(int viewportIndex);
   void setViewports(uint numViewports, BOOL verticalSplit, const BViewportDesc* pViewports) const;              
   
   // callable from the main or worker threads
   // The GPU cache must be manually invalidated if the CPU writes to GPU resources from anywhere other than the render thread!
   void invalidateGPUCache(void *pBaseAddress, DWORD Size, DWORD Flags = 0);
   
   // Creates a dynamic VB for streaming purposes using CPU and GPU frame storage.
   // This VB's lifetime can ONLY be for the current frame! 
   // The resource will be created in write combined memory. Don't read from it.
   // Usable from main/worker threads.
   // The GPU's cache must be invalidated if you write to the dynamic VB from the main thread.
   IDirect3DVertexBuffer9* createDynamicVB(uint lengthInBytes);
   
   // Creates a dynamic IB for streaming purposes using CPU and GPU frame storage.
   // This IB's lifetime can ONLY be for the current frame!
   // The resource will be created in write combined memory. Don't read from it.
   // Usable from main/worker threads.
   // The GPU's cache must be invalidated if you write to the dynamic VB from the main thread.
   IDirect3DIndexBuffer9* createDynamicIB(uint lengthInBytes, D3DFORMAT indexType = D3DFMT_INDEX16);
         
   // lockDynamicVB and unlockDynamicVB are wrappers around createDynamicVB/releaseDynamicVB.
   // Usable from main/worker threads.
   // Must be unlocked and unset from the device before the frame ends.
   // The GPU's cache must be invalidated if you write to the dynamic VB from the main thread.
   void* lockDynamicVB(uint numVerts, uint vertSize);
   //void* lockDynamicVB(uint numVerts, uint vertSize, DWORD& startOffset) { startOffset = 0; return lockDynamicVB(numVerts, vertSize); }
   IDirect3DVertexBuffer9* getDynamicVB(void) const { BDEBUG_ASSERT(mpThreadData->mpCurDynamicVB); return mpThreadData->mpCurDynamicVB; } 
   // unlockDynamicVB will submit an invalidateGpuCache command for you if you call it from the main thread. So don't call it while you are submitting a command via submitCommandBegin()!
   void unlockDynamicVB(void);
            
   // lockDynamicIB and unlockDynamicIB are wrappers around createDynamicIB/releaseDynamicIB.
   // Usable from main/worker threads.
   // Must be unlocked and unset from the device before the frame ends.
   // The GPU's cache must be invalidated if you write to the dynamic VB from the main thread.
   void* lockDynamicIB(uint numIndices, D3DFORMAT indexType = D3DFMT_INDEX16);
   void* lockDynamicIB(uint numIndices, DWORD& startOffset, D3DFORMAT indexType = D3DFMT_INDEX16) { startOffset = 0; return lockDynamicIB(numIndices, indexType); }
   IDirect3DIndexBuffer9* getDynamicIB(void) const { BDEBUG_ASSERT(mpThreadData->mpCurDynamicIB); return mpThreadData->mpCurDynamicIB; } 
   // unlockDynamicIB will submit an invalidateGpuCache command for you if you call it from the main thread. So don't call it while you are submitting a command via submitCommandBegin()!
   void unlockDynamicIB(void);
         
   // Creates a unmipped 2D texture for streaming purposes.
   // Usable from main/worker threads.
   // Must be unlocked and unset from the device before the frame ends.
   // The GPU's cache must be invalidated if you write to the dynamic VB from the main thread.
   IDirect3DTexture9* lockDynamicTexture(uint width, uint height, D3DFORMAT format, D3DLOCKED_RECT* pLockedRect, uint* pLen = NULL, bool lineTexture = false);
   IDirect3DTexture9* getDynamicTexture(void) const { return mpThreadData->mpCurDynamicTexture; }
   // unlockDynamicTexture will submit an invalidateGpuCache command for you if you call it from the main thread. So don't call it while you are submitting a command via submitCommandBegin()!
   void unlockDynamicTexture(void);
   
   // Returns NULL if frame storage is full.
   IDirect3DTexture9* createDynamicTexture(uint width, uint height, D3DFORMAT format, uint* pLen = NULL);
      
   // Sets most of the D3D render states back to their default values.
   // This does not set every last state, just the most relevant ones to us.
   // Callable from worker thread only.
   void workerSetDefaultRenderStates(IDirect3DDevice9* pDev = NULL);
   
   // Callable from worker thread only.
   void workerSetDefaultSamplerStates(IDirect3DDevice9* pDev = NULL);
   
   // Main or worker threads.
   enum
   {
      cUnsetTextures       = 1,
      cUnsetRenderTargets  = 2,
      cUnsetDepthStencil   = 4,
      cUnsetStreams        = 8,
      cUnsetIndices        = 16,
      cUnsetStateBlock     = 32,
      cUnsetShaders        = 64,
      cUnsetAll            = 0xFFFFFFFF
   };
   void unsetResources(DWORD flags = cUnsetAll);
      
   void unsetTextures(void);
   
   // Main or worker threads.
   void saveScreenshot(const char* pFilename);
   
   // Returns TRUE if BeginScene() has been called.
   BOOL getBegunScene(void) const { return mpThreadData->mBegunScene; }
   
   // Do not read from a resource until you are sure the GPU has finished writing to it!
   void* getResourceAddress(D3DResource* pResource, bool cachedReadOnlyView);
   
   // Render thread only
   DWORD pushRenderState(D3DRENDERSTATETYPE state);
   DWORD pushAndSetRenderState(D3DRENDERSTATETYPE state, DWORD newValue);
   
   // Render states must always be popped in LIFO order.
   void popRenderState(D3DRENDERSTATETYPE state);
   
   uint getRenderStateStackTop(void) const { return mRSStackTop; }
   void setRenderStateStackTop(uint stackTop);
         
   // Render thread only
   // rg [2/15/07] - For now, we don't have a global dynamic GPU buffer. Instead the particle engine will manage its own.
   BDynamicGPUBuffer* getDynamicGPUBuffer(void) const { return mpDynamicGPUBuffer; }
   
   void workerGetFrontBuffer(BRGBAImage& image);
   
   // These routine safely clears/copies/flushes the memory of the specified texture, WITHOUT clearing the unused regions which may hold heap allocations.
   void clearTextureData(IDirect3DBaseTexture9* pTex);
   void copyTextureData(IDirect3DBaseTexture9* pDstTex, const void* pSrc, uint srcSize);
   // Flushes cached texture data from the CPU's caches.
   void flushTextureData(IDirect3DBaseTexture9* pDstTex);
   
   void checkForFailOrMeltdown(void);
   
   static void getTextureLayout(
      D3DBaseTexture*  pTexture,             
      UINT*            pBaseData,            
      UINT*            pBaseSize,            
      XGLAYOUT_REGION* pBaseRegionList,      
      UINT*            pBaseRegionListCount, 
      UINT             BaseRegionAlignment,  
      UINT*            pMipData,             
      UINT*            pMipSize,             
      XGLAYOUT_REGION* pMipRegionList,       
      UINT*            pMipRegionListCount,  
      UINT             MipRegionAlignment);
                                 
private:
   BRenderViewport         mMainSceneRenderViewport;
   BMatrixTracker          mMainSceneMatrixTracker;
   BVolumeCuller           mMainSceneVolumeCuller;
   BRenderViewport         mMainActiveRenderViewport;
   BMatrixTracker          mMainActiveMatrixTracker;
   BVolumeCuller           mMainActiveVolumeCuller;
      
   BRenderViewport         mWorkerSceneRenderViewport;
   BMatrixTracker          mWorkerSceneMatrixTracker;
   BVolumeCuller           mWorkerSceneVolumeCuller;
   BRenderViewport         mWorkerActiveRenderViewport;
   BMatrixTracker          mWorkerActiveMatrixTracker;
   BVolumeCuller           mWorkerActiveVolumeCuller;
            
   D3DDISPLAYMODE          mDisplayMode;
   D3DPRESENT_PARAMETERS   mPresentParams;
   
   IDirect3DStateBlock9*   mpStateBlock;
   
   BCommandListenerHandle  mCommandListenerHandle;
   
   struct BStateStackEntry
   {
      DWORD mState;
      DWORD mValue;
   };

   enum { cRSStackSize = 32 };
   BStateStackEntry        mRSStack[cRSStackSize];
   uint                    mRSStackTop;
   
   BDynamicGPUBuffer*      mpDynamicGPUBuffer;
   
   struct BViewportState
   {
      uint                 mNumViewports;   
      int                  mViewportIndex; // -1 for full-screen rendering
      BOOL                 mVerticalSplit;
      BViewportDesc        mDesc[cMaxViewports];
      
      void clear()
      {
         Utils::ClearObj(*this);
         mViewportIndex = -1;
      }
   };
         
   struct BThreadData
   {
      BThreadData() { clear(); }
      
      void clear()
      {
         mpCurDynamicTexture = NULL;
         mpCurDynamicTextureData = NULL;
         mCurDynamicTextureSize = 0;

         mpCurDynamicVB = NULL;
         mpCurDynamicVBData = NULL;
         mCurDynamicVBSize = 0;

         mpCurDynamicIB = NULL;
         mpCurDynamicIBData = NULL;
         mCurDynamicIBSize = 0;
         
         mBegunScene = FALSE;
         
         mViewportState.clear();
      }
      
      IDirect3DTexture9*      mpCurDynamicTexture;
      void*                   mpCurDynamicTextureData;
      uint                    mCurDynamicTextureSize;
      
      IDirect3DVertexBuffer9* mpCurDynamicVB;
      void*                   mpCurDynamicVBData;
      uint                    mCurDynamicVBSize;
      
      IDirect3DIndexBuffer9*  mpCurDynamicIB;
      void*                   mpCurDynamicIBData;
      uint                    mCurDynamicIBSize;
      
      BOOL                    mBegunScene;
                              
      BViewportState          mViewportState;
   };
         
   static __declspec(thread) BThreadData* mpThreadData;

   void initViewportState(BViewportState& viewportState);
   void workerUnsetResources(DWORD flags = cUnsetAll);
   void workerUnsetTextures(void);
   bool workerSaveScreenshotTGA(const char* pFilename);
   bool workerSaveScreenshotJPEG(const char* pFilename);
   bool workerSaveScreenshot(const char* pFilename);
   static void workerSetViewportIndex(void* pData);
   static void workerSetViewports(void* pData);
                     
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void initDeviceData(void);
   virtual void deinitDeviceData(void);
   virtual void frameBegin(void);
   virtual void frameEnd(void);   
};

extern BRenderDraw gRenderDraw;

class BRenderStateStackState
{
   BRenderStateStackState(const BRenderStateStackState&);
   BRenderStateStackState& operator= (const BRenderStateStackState&);
   
public:
   BRenderStateStackState();
   ~BRenderStateStackState();

private:
   DWORD mStackTop;
};

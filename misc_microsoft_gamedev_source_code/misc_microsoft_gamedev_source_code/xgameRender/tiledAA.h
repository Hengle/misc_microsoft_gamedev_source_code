//============================================================================
//
//  File: tiledAA.h
//
//  Copyright (c) 2006 Ensemble Studios
//
// rg [6/14/06] - Initial implementation
//============================================================================
#pragma once

#include "threading\eventDispatcher.h"
#include "math\generalMatrix.h"

#include "effect.h"
#include "renderThread.h"
#include "volumeCuller.h"
#include "renderToTextureXbox.h"

//============================================================================
// class BTiledAAManager
//============================================================================
class BTiledAAManager : public BRenderCommandListener
{
public:
   BTiledAAManager();
   ~BTiledAAManager();

   void init(uint width, uint height, uint numTiles, D3DFORMAT edramColorFormat, D3DFORMAT texColorFormat, D3DFORMAT depthFormat);
   void deinit(void);

   bool isInitialized(void) const { return 0 != mWidth; }   
         
   // Enables/disables tiled anti-aliasing. If disabled, only 1 tile is used to render to EDRAM, and it's the user's 
   // responsibility to resolve EDRAM to a texture.
   void enableTiling(bool enabled, uint noTilingWidth = 0, uint noTilingHeight = 0, D3DMULTISAMPLE_TYPE multisampleType = D3DMULTISAMPLE_NONE);
   bool getTilingEnabled(void) const { return mTilingEnabled; }
      
   // setNumTiles() is very slow!
   void setNumTiles(uint numTiles);
   
   // The number of tiles returned depends on whether or not AA is enabled! 
   // If disabled this always returns 1.
   uint getNumTiles(void) const;
   
   // If AA is enabled, getWidth()/getHeight() returns the AA width/height, otherwise it returns the no-AA width/height.
   uint getWidth(void) const;
   uint getHeight(void) const;
   
   uint getTilingWidth(void) const { return mWidth; }
   uint getTilingHeight(void) const { return mHeight; }
   
   uint getNoTilingWidth(void) const { return mNoTilingWidth; }
   uint getNoTilingHeight(void) const { return mNoTilingHeight; }
  
   //------
              
   void beginFrame(void);
   
   const RECT& getTileRect(uint tileIndex) const;
   XMMATRIX getTileProjMatrix(uint tileIndex) const;
   const BVolumeCuller& getTileVolumeCuller(uint tileIndex) const;
         
   void beginTiling(void);
   
   void beginTile(uint tileIndex, const D3DVECTOR4* pClearColor = NULL, DWORD stencil = 0);
   void endTileDepth(
      uint tileIndex, 
      const D3DRESOLVE_PARAMETERS* pDepthResolveParams = NULL, 
      bool resolveNoTilingDepthSurf = false,
      DWORD clearStencil = 0);
   void endTileScene(
      uint tileIndex, 
      const D3DRESOLVE_PARAMETERS* pColorResolveParams = NULL, 
      const D3DVECTOR4* pClearColor = NULL, 
      bool resolveNoTilingColorSurf = false,
      DWORD clearStencil = 0);

   void endTiling(void);
   
   bool getIsTiling(void) const { return mIsTiling; }
   int getTileIndex(void) const { return mCurTileIndex; }
   
   void blitColorTexture(void);
   
   //------
      
   uint getTileMaxWidth(void) const { return mTileMaxWidth; }
   uint getTileMaxHeight(void) const { return mTileMaxHeight; }

   uint getTotalEDRAMUsed(void) const { return mTotalEDRAMUsed; }

   D3DFORMAT getEDRAMColorFormat(void) const { return mEDRAMColorFormat; }
   D3DFORMAT getColorFormat(void) const { return mTexColorFormat; }
   D3DFORMAT getDepthFormat(void) const { return mDepthFormat; }

   D3DMULTISAMPLE_TYPE getMultisampleType() const { return mMultisampleType; }
   
   IDirect3DSurface9* getColorSurf(void) const { return mpColorSurf; }
   IDirect3DSurface9* getDepthSurf(void) const { return mpDepthSurf; }
      
   IDirect3DTexture9* getColorTexture(void) const { return mpColorTexture; }
   IDirect3DTexture9* getDepthTexture(void) const { return mpDepthTexture; }
   
   IDirect3DSurface9* getTileRenderTarget(void) const { return mTilingEnabled ? getColorSurf() : mNoTilingSurfaces.getColorSurf(); }
   IDirect3DSurface9* getTileDepthStencilSurface(void) const { return mTilingEnabled ? getDepthSurf() : mNoTilingSurfaces.getDepthSurf(); }
   
   void getTileViewport(uint tileIndex, D3DVIEWPORT9& viewport) const;
   
   BRenderToTextureHelperXbox& getNoTilingSurfaces(void) { return mNoTilingSurfaces; }
   
   // tonemap() writes the tone mapped output to the backbuffer. If you want to use 
   // this output in more passes, you must resolve this output somewhere.
   // pTextureToOverwrite must be large enough to hold all the resolved bits.
   // To use this texture, call getBackbufferTexture().
   void resolveBackbuffer(IDirect3DTexture9* pTextureToOverwrite);
   IDirect3DTexture9* getBackbufferTexture(void) { return &mAliasedColorTexture; }
            
private:
   uint mWidth;
   uint mHeight;
   
   uint mTileMaxWidth;
   uint mTileMaxHeight;
   D3DMULTISAMPLE_TYPE mMultisampleType;
      
   D3DFORMAT mEDRAMColorFormat;
   D3DFORMAT mTexColorFormat;
   D3DFORMAT mDepthFormat;
   uint mTotalEDRAMUsed;
      
   enum { cMaxTiles = 4 };
   
   uint mNumTiles;
   uint mViewportLeft, mViewportTop;
   RECT mTiles[cMaxTiles];
   
   XMMATRIX mTileProjMatrices[cMaxTiles];
   BVolumeCuller mTileVolumeCullers[cMaxTiles];

   IDirect3DSurface9* mpColorSurf;
   IDirect3DSurface9* mpDepthSurf;
   
   IDirect3DTexture9* mpColorTexture;
   IDirect3DTexture9* mpDepthTexture;
   
   int mCurTileIndex;
      
   uint mNoTilingWidth;
   uint mNoTilingHeight;
   D3DMULTISAMPLE_TYPE mNoTilingMultisample;
   RECT mNoTilingRect;
   
   BRenderToTextureHelperXbox mNoTilingSurfaces;
   
   // The final tone mapped framebuffer, aliased overtop of the tiled AA manager's HDR color texture. 
   // This is a hack to save memory.
   IDirect3DTexture9 mAliasedColorTexture;
   
   bool mIsTiling : 1;
   
   bool mTilingEnabled : 1;
   
   void clear(void);
   void releaseD3DSurfaces(void);
   void createD3DTextures(void);
   void releaseD3DTextures(void);
   
   virtual void initDeviceData(void);
   virtual void frameBegin(void);

   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
   
   virtual void beginLevelLoad(void);
   virtual void endLevelLoad(void);
};

extern BTiledAAManager gTiledAAManager;

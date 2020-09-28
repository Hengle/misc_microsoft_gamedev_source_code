//============================================================================
// flashminimaprenderer.h
// renderer for the flash minimap
// Copyright (C) 2007 Ensemble Studios
//============================================================================

#pragma once
#include "effect.h"
#include "renderThread.h"
#include "renderCommand.h"
#include "renderToTextureXbox.h"


//============================================================================
//============================================================================
class BFlashMinimapItem
{
public:
   BFlashMinimapItem(){};
   ~BFlashMinimapItem(){};

   bool operator >  (const BFlashMinimapItem& i) const { return mPriority >  i.mPriority; };
   bool operator <  (const BFlashMinimapItem& i) const { return mPriority <  i.mPriority; };
   bool operator <= (const BFlashMinimapItem& i) const { return mPriority <= i.mPriority; };
   bool operator >= (const BFlashMinimapItem& i) const { return mPriority >= i.mPriority; };
   bool operator != (const BFlashMinimapItem& i) const { return mPriority != i.mPriority; };
   bool operator == (const BFlashMinimapItem& i) const { return mPriority == i.mPriority; };

   BVector  mPosition;
   BVector  mPosition2;
   XMHALF4  mUV;
   XMHALF2  mSize;   
   XMCOLOR  mColor;
   double   mDeathTime;
   BYTE     mType;
   BYTE     mPriority;
};

//============================================================================
//============================================================================
class BFlashMinimapVisibilityItem
{
   public:
      BFlashMinimapVisibilityItem() {};
     ~BFlashMinimapVisibilityItem() {};

      BVector mPosition;
      XMHALF2 mSize;
};

//============================================================================
// class BFlashMinimapRenderer
//============================================================================
class BFlashMinimapRenderer: public BRenderCommandListener, BEventReceiver
{
   public:
      BFlashMinimapRenderer();
     ~BFlashMinimapRenderer();

      enum 
      {
         eEventClassReloadEffects = cEventClassFirstUser
      };

      void init(void);
      void deInit(void);      
      void submitIconRender(XMMATRIX matrix, BManagedTextureHandle textureHandle, int flashmovieIndex, int numIcons, void* pVB);
      void submitMapRender(XMMATRIX matrix, BManagedTextureHandle textureHandle, BManagedTextureHandle maskTextureHandle, BManagedTextureHandle backgroundTextureHandle, float rotationAngle, XMCOLOR fogColor, float mapFogScalar, float mapSkirtFogScalar, void* pVB, DWORD samplingMode);
      void submitGenerateVisibility(XMMATRIX matrix, BManagedTextureHandle visMaskTexture, int numFog, void* pFogFrameStorage, int numVisibility, void* pVisibilityFrameStorage, int numBlocker, void* pBlockerFrameStorage);
      void submitResetVisibility(DWORD maxX, DWORD maxZ, bool* pVisibilityData);

      //-- BEvent Receiver interface
      bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
      
      //-- This may be called from the main or render threads
      IDirect3DTexture9* getVisibilityTexture() { return mpVisibilityTexture; }

      struct BMinimapResetVisibilityPacket
      {
         DWORD mMaxX;
         DWORD mMaxZ;
         float mfMaxX;
         float mfMaxZ;
         bool* pVisibilityData;
      };

   private:

      enum
      {
         eTechniquePassIcons,
         eTechniquePassMap,
         eTechniquePassTotal,
      };

      enum
      {
         cFMMCommandDrawIcons = 0,
         cFMMCommandDrawMap,
         cFMMCommandGenerateVisibility,
         cFMMCommandReset,
         cFMMCommandTotal
      };

      struct BMinimapDrawIconPacket
      {
         XMMATRIX                mMatrix;
         int                     numIcons;
         int                     mFlashMovieIndex;
         void*                   mpData;
         BManagedTextureHandle   mTextureHandle;
         BManagedTextureHandle   mMaskTextureHandle;
         BManagedTextureHandle   mBackgroundTextureHandle;
         XMCOLOR                 mColor;
         float                   mRotationAngle;
         float                   mMapFogScalar;
         float                   mMapSkirtFogScalar;
         DWORD                   mSamplingMode;
      };

      struct BMinimapGenerateVisibilityPacket
      {
         XMMATRIX mMatrix;
         int      mNumFog;
         int      mNumVisibility;
         int      mNumBlockers;
         void*    mpFogData;
         void*    mpVisibilityData;
         void*    mpBlockerData;
         BManagedTextureHandle mMaskTextureHandle;
      };

      struct BMinimapIconVertex
      {
         XMHALF4 mUV;
         XMHALF2 mPosition;
         XMHALF2 mSize;         
         XMCOLOR mColor;
      };

      struct BMinimapGenerateVisVertex
      {
         XMHALF2 mPosition;
         XMHALF2 mSize;
      };
            
      // reload interface
      void reloadInit();
      void reloadDeinit();

      // effect initialization
      void loadEffect();
      void initEffect(const BEvent& event);
      void initVertexDeclarations();
      void killEffect();

      // render functions
      void drawIconsInternal(const BMinimapDrawIconPacket* pPacket, int techniquePass);
      void generateVisibilityInternal(const BMinimapGenerateVisibilityPacket* pPacket);

      BRenderToTextureHelperXbox mVisibilityRenderTarget;
      BFXLEffect          mEffect;
      BFXLEffectTechnique mTechnique;
      BFXLEffectTechnique mVisTechnique;
      BFXLEffectParam     mTransformParam;
      BFXLEffectParam     mTextureParam;
      BFXLEffectParam     mMaskTextureParam;
      BFXLEffectParam     mBackgroundTextureParam;
      BFXLEffectParam     mRotationAngle;
      BFXLEffectParam     mVisTextureParam;
      BFXLEffectParam     mVisPointTextureParam;
      BFXLEffectParam     mColorParam;
      BFXLEffectParam     mFogColorParam;
      BFXLEffectParam     mFogScalar;

      IDirect3DVertexDeclaration9* mpIconVertexDecl;
      IDirect3DVertexDeclaration9* mpGenerateVisVertexDecl;
      IDirect3DVertexDeclaration9* mpGenerateVisSquareVertexDecl;
      IDirect3DTexture9*           mpVisibilityTexture;
      IDirect3DTexture9*           mpStencilTexture;
      BManagedTextureHandle               mInvalidMapTextureHandle;

      D3DXVECTOR4         mVisibilityColor;
      bool                mbInitialized : 1;
      bool                mbInit : 1;

      //Listener interface
      // init will be called from the worker thread after the D3D device is initialized.
      virtual void initDeviceData(void);
      // Called from worker thread.
      virtual void frameBegin(void);
      // Called from the worker thread to process commands.
      virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
      // Called from worker thread.
      virtual void frameEnd(void);
      // deinit will be called from the worker thread before the RCL is freed, but always before the D3D device is release.
      virtual void deinitDeviceData(void);      
};

extern BFlashMinimapRenderer gFlashMinimapRenderer;
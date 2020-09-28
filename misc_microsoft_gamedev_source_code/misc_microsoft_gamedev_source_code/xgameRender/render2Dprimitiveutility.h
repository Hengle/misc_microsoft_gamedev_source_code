//==============================================================================
// render2Dprmitiveutility.h
//
// Copyright (c) 2003-2008, Ensemble Studios
//==============================================================================
#pragma once 

#include "effect.h"
#include "renderThread.h"
#include "renderCommand.h"

//==============================================================================
//==============================================================================
struct BR2DPUUpdateData
{
   int   mCount;
   void* mpData;
   BThreadIndex mThreadIndex;
};

//==============================================================================
// class B2DPrimitivePie
//==============================================================================
class B2DPrimitivePie
{
   friend class BRender2DPrimitiveUtility;

   public: 

      B2DPrimitivePie()
      {
         clear();
      }

      void clear()
      {
         mTextureHandle = cInvalidManagedTextureHandle;
         mMatrix = XMMatrixIdentity();
         mColor = 0xFFFFFFFF;
         mStart = 0.0f;
         mEnd = 1.0f;
         mScaleX = 1.0f;
         mScaleY = 1.0f;
         mOffsetX = 0.0f;
         mOffsetY = 0.0f;
         mTimeout = -1.0f;
         mCategory = -1;
         mBlendMode = -1;
         mLayer = 0;
         mFill = true;
         mCW = true;
      }

      bool keyCompare(const B2DPrimitivePie& rhs) const
      {
         const B2DPrimitivePie& lhs = *this;
#define COMPARE(v) if (lhs.##v < rhs.##v) return true; else if (lhs.##v > rhs.v##) return false;          
         COMPARE(mCategory);
         COMPARE(mLayer);
         COMPARE(mBlendMode);
         COMPARE(mTextureHandle);
#undef COMPARE
      return false;
      }

      B2DPrimitivePie& operator=(const B2DPrimitivePie& rhs)
      {
         mMatrix = rhs.mMatrix;
         mTextureHandle = rhs.mTextureHandle;
         mColor = rhs.mColor;
         mStart = rhs.mStart;
         mEnd = rhs.mEnd;
         mScaleX = rhs.mScaleX;
         mScaleY = rhs.mScaleY;
         mOffsetX = rhs.mOffsetX;
         mOffsetY = rhs.mOffsetY;
         mTimeout = rhs.mTimeout;
         mBlendMode = rhs.mBlendMode;
         mCategory = rhs.mCategory;
         mFill = rhs.mFill;
         mCW = rhs.mCW;
         mLayer = rhs.mLayer;

         return *this;
      }
      
      BMatrix mMatrix;
      BManagedTextureHandle mTextureHandle;
      DWORD   mColor;
      float   mStart; // range 0.0f <==> 1.0f
      float   mEnd;   // range 0.0f <==> 1.0f
      float   mScaleX;
      float   mScaleY;
      float   mOffsetX;
      float   mOffsetY;
      float   mTimeout;   
      int     mBlendMode;
      int     mCategory;
      int     mLayer;
      bool    mFill;  // fill or empty
      bool    mCW;    // clock wise or counter clock wise
};

//==============================================================================
// class B2DPrimitiveSprite
//==============================================================================
class B2DPrimitiveSprite
{
   friend class BRender2DPrimitiveUtility;

   public: 

      B2DPrimitiveSprite()
      {
         clear();
      }

      void clear()
      {
         mTextureHandle = cInvalidManagedTextureHandle;
         mMatrix = XMMatrixIdentity();
         mColor = 0xFFFFFFFF;
         mScaleX = 1.0f;
         mScaleY = 1.0f;
         mOffsetX = 0.0f;
         mOffsetY = 0.0f;
         mTimeout = -1.0f;
         mCategory = -1;
         mBlendMode = -1;
         mLayer = 0;
      }

      bool keyCompare(const B2DPrimitiveSprite& rhs) const
      {
         const B2DPrimitiveSprite& lhs = *this;
#define COMPARE(v) if (lhs.##v < rhs.##v) return true; else if (lhs.##v > rhs.v##) return false;          
         COMPARE(mCategory);
         COMPARE(mLayer);
         COMPARE(mBlendMode);
         COMPARE(mTextureHandle);
#undef COMPARE
      return false;
      }

      B2DPrimitiveSprite& operator=(const B2DPrimitiveSprite& rhs)
      {
         mMatrix = rhs.mMatrix;
         mTextureHandle = rhs.mTextureHandle;
         mColor = rhs.mColor;
         mScaleX = rhs.mScaleX;
         mScaleY = rhs.mScaleY;
         mOffsetX = rhs.mOffsetX;
         mOffsetY = rhs.mOffsetY;
         mTimeout = rhs.mTimeout;
         mBlendMode = rhs.mBlendMode;
         mCategory = rhs.mCategory;
         mLayer = rhs.mLayer;
         return *this;
      }
      
      BMatrix mMatrix;
      BManagedTextureHandle mTextureHandle;
      DWORD   mColor;
      float   mScaleX;
      float   mScaleY;
      float   mOffsetX;
      float   mOffsetY;
      float   mTimeout;   
      int     mBlendMode;
      int     mCategory;
      int     mLayer;
};

//==============================================================================
// BRender2DPrimitiveUtility
//==============================================================================
class BRender2DPrimitiveUtility : public BRenderCommandListenerInterface, BEventReceiver
{
   public:      
               BRender2DPrimitiveUtility();
      virtual ~BRender2DPrimitiveUtility();

      enum eBR2DPrimType
      {
         ePrimTypePie,
         ePrimTypeSprite,
         ePrimTypeTotal
      };

      enum eBR2DUCommand
      {
         eBR2DUUpdatePie = 0,
         eBR2DUUpdateSprite,
         eBR2DUUpdate2DSprite,
         eBR2DUClearPrimitives,         
         eBR2DUCommandTotal
      };

      enum eRender2DCommand
      {
         eRender2DPrimitive = 0,
         eRender2DCommandTotal
      };

      enum eRender2DTechnique
      {
         eRender2DTechniquePie = 0,
         eRender2DTechniqueSprite,
         eRender2DTechnique2DSprite,
         eRender2DTechniqueTotal
      };
     
      enum eRender2DBlendMode
      {
         eRender2DBlendMode_Alphablend=0,
         eRender2DBlendMode_Additive,
         eRender2DBlendMode_Subtractive,
         eRenderPassTotal
      };

      enum 
      {
         eEventClassReloadEffects = cEventClassFirstUser
      };
      
      void init(void);
      void deInit(void);
      
      void update(eBR2DUCommand type, int count, void* pData, BThreadIndex ownerThreadIndex = cThreadIndexSim);
      void render(BThreadIndex ownerThreadIndex = cThreadIndexSim);

      void workerUpdate(eBR2DUCommand type, void* pData);
      void workerRender(void* pData);      

      //-- BEvent Receiver interface
      bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
                 
   private:

      struct BPrimitiveKeySorter
      {
         BRender2DPrimitiveUtility& m2DRenderPrimitiveUtility;
         BPrimitiveKeySorter(BRender2DPrimitiveUtility& primitiveUtility) : m2DRenderPrimitiveUtility(primitiveUtility) {}
         bool operator() (const B2DPrimitivePie& lhs, const B2DPrimitivePie& rhs) const;
         bool operator() (const B2DPrimitiveSprite& lhs, const B2DPrimitiveSprite& rhs) const;
      };

      struct BSpriteVertex
      {
         XMFLOAT4 mPos;
         XMHALF2  mSize;
         XMHALF2  mOffset;
         XMCOLOR  mColor;
      };
                  
      struct BPieVertex
      {  
         XMFLOAT4 mPos;      // position [xyz] thickness [w] 
         XMHALF2  mOffset;   
         XMHALF2  mUV;
         XMCOLOR  mColor;    // Color   
      };

      struct BRenderData
      {
         int mThreadIndex;
      };

      //-- WORKER THREAD

      static void setWorldTransform(BMatrix worldMatrix, BVector scaleVector);
      static void setWorldTransform(BMatrix worldMatrix);
      
      void applyBlendMode(eRender2DBlendMode mode);
      
      template<class T> IDirect3DVertexBuffer9* createPieVB(T& list, int startIndex, int endIndex, int& vertexCount);
      template<class T> IDirect3DVertexBuffer9* createSpriteVB(T& list, int startIndex, int endIndex, int& vertexCount);

      template<class T> void renderSortedPies(T& list);
      template<class T> void renderSortedSprites(T& list, bool b2DSprite);
      template<class T> void renderPieRange(T& list, int firstIndex, int lastIndex);      
      template<class T> void renderSpriteRange(T& list, int firstIndex, int lastIndex, bool b2DSprite);      
      void drawPrimitiveInternal(int techniqueIndex, int techniquePassIndex, D3DPRIMITIVETYPE primType, eRender2DBlendMode blendMode, IDirect3DVertexDeclaration9* pDecl, int vertexSize, IDirect3DVertexBuffer9* pVB, int vertexCount, BManagedTextureHandle textureHandle);
      
      // -- MAIN/WORKER
      void loadEffect();

      //-- MAIN
      void reloadInit();
      void reloadDeinit();

      //-- WORKER
      void initEffect(const BEvent& event);
      void workerClear(int type);
      
      //-- WORKER THREAD
      //-- Only access this data on the worker thread!
      
      //-- vertex decl initialization
      void initVertexDeclarations(void);
      
      void releaseBuffers();      

      BDynamicArray<B2DPrimitivePie, 16>    mSimPiePrimitives;
      BDynamicArray<B2DPrimitiveSprite, 16> mSimSpritePrimitives;
      BDynamicArray<B2DPrimitiveSprite, 16> mSimSprite2DPrimitives;

      BDynamicArray<B2DPrimitivePie, 16>    mRenderPiePrimitives;
      BDynamicArray<B2DPrimitiveSprite, 16> mRenderSpritePrimitives;
      BDynamicArray<B2DPrimitiveSprite, 16> mRenderSprite2DPrimitives;
      
      enum { cNumUpdateDataContexts = 2 };
      BR2DPUUpdateData mUpdateData[cNumUpdateDataContexts][eBR2DUCommandTotal];
      
      BCommandListenerHandle  mCommandListenerHandle;      
      
      IDirect3DVertexDeclaration9* mpPieVertexDecl;
      IDirect3DVertexDeclaration9* mpSpriteVertexDecl;

      BFXLEffect           mEffect;
      BFXLEffectTechnique  mTechnique     [eRender2DTechniqueTotal];      
      BFXLEffectParam      mMaskTexture   [eRender2DTechniqueTotal];
            
      //-- Listener interface
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


      // Helper template functions
      template<class T, class P> void updatePrimitives(T& list, P* pPrimitives, int count);
};

//==============================================================================
// Globals
//==============================================================================
extern BRender2DPrimitiveUtility gRender2DPrimitiveUtility;

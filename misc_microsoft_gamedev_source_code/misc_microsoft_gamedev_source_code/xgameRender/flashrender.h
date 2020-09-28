//============================================================================
// flashrender.h
// Ensemble Studios (C) 2007
//============================================================================

#pragma once
#include "scaleformIncludes.h"
#include "GRendererXbox360.h"
#include "renderThread.h"
#include "effect.h"
#include "renderCommand.h"
#include "dynamicGPUBuffer.h"
#include "Timer.h"

class BFlashTexture;

//============================================================================
//============================================================================
class BFlashFillStyle
{
   public:

      friend class BFlashRender;

      BFlashFillStyle();
     ~BFlashFillStyle();

      enum FillMode
      {
         FM_None,
         FM_Color,
         FM_Bitmap,
         FM_Gouraud
      };

   FillMode                mMode;
   GRenderer::GouraudFillType mGouraudType;
   GColor                  mColor;
   GRenderer::Cxform       mBitmapColorTransform;       
   GRenderer::FillTexture  mFill;
   GRenderer::FillTexture  mFill2;

   bool compareFill(const GRenderer::FillTexture& lhs, const GRenderer::FillTexture& rhs) const
   {
      if (lhs.pTexture != rhs.pTexture) 
         return false;
      if (lhs.SampleMode != rhs.SampleMode) 
         return false;
      if (lhs.WrapMode != rhs.WrapMode) 
         return false;      
      return true;
   }

   bool operator== (const BFlashFillStyle& rhs) const
   {
      const BFlashFillStyle& lhs = *this;
      #define COMPARE(v) if (!(lhs.##v == rhs.##v)) return false;
      if (!compareFill(lhs.mFill, rhs.mFill)) 
         return false;
      if (!compareFill(lhs.mFill2, rhs.mFill2)) 
         return false;      
      #undef COMPARE
      return true;
   }


   // Push our style into Direct3D
   void clear();
   void Apply(BFlashRender* pRenderer) const;
   void Disable();   
   void SetColor(GColor color);
   void SetCxform(const GRenderer::Cxform& colorTransform);
   void SetBitmap(const GRenderer::FillTexture* pft, const GRenderer::Cxform& colorTransform);

   // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
   // The specified textures are applied to vertices {0, 1, 2} of each triangle based
   // on factors of Complex vertex. Any or all subsequent pointers can be NULL, in which case
   // texture is not applied and vertex colors used instead.
   void SetGouraudFill(GRenderer::GouraudFillType gfill,
      const GRenderer::FillTexture *ptexture0,
      const GRenderer::FillTexture *ptexture1,
      const GRenderer::FillTexture *ptexture2, const GRenderer::Cxform& colorTransform);   

   bool IsValid() const;
};

//============================================================================
//============================================================================
class BFlashRender : public GRendererXbox360, BRenderCommandListener, BEventReceiver
{
   public:

      friend class BFlashFillStyle;
      friend class BFlashTexture;

      BFlashRender();
     ~BFlashRender();

      virtual void ReleaseResources();

      enum 
      {
         eEventClassReloadEffects = cEventClassFirstUser
      };

      enum BMaskModeType
      {
         eMaskModeNone,
         eMaskModeClear,
         eMaskModeIncrement,
         eMaskModeDecrement,
         eMaskModeKeep,
         eMaskModeTotal,
      };

      // Vertex shader declarations we can use
      enum BFlashRenderVDeclType
      {
         VD_None,
         VD_Strip,
         VD_Glyph,
         VD_XY16iC32,
         VD_XY16iCF32,

         VD_UberStart,
         VD_UberVertex = VD_UberStart,
         VD_UberGlyphVertex,

         VD_Count
      };

      enum BFlashRenderVShaderType
      {
         VS_None,
         VS_Strip,
         VS_Glyph,
         VS_XY16iC32,
         VS_XY16iCF32,
         VS_XY16iCF32_T2,         
         VS_Count
      };
      // Pixel shaders
      enum BFlashRenderPixelShaderType
      {
         PS_None = 0,
         PS_SolidColor,
         PS_CxformTexture,
         PS_CxformTextureMultiply,
         PS_TextTexture,


         PS_CxformGauraud,
         PS_CxformGauraudNoAddAlpha,
         PS_CxformGauraudTexture,
         PS_Cxform2Texture,

         // Multiplies - must come in same order as other gourauds
         PS_CxformGauraudMultiply,
         PS_CxformGauraudMultiplyNoAddAlpha,
         PS_CxformGauraudMultiplyTexture,
         PS_CxformMultiply2Texture,

         // premultiplied alpha
         PS_AcSolidColor,
         PS_AcCxformTexture,
         PS_AcCxformTextureMultiply,
         PS_AcTextTexture,

         PS_AcCxformGouraud,
         PS_AcCxformGouraudNoAddAlpha,
         PS_AcCxformGouraudTexture,
         PS_AcCxform2Texture,

         PS_AcCxformGouraudMultiply,
         PS_AcCxformGouraudMultiplyNoAddAlpha,
         PS_AcCxformGouraudMultiplyTexture,
         PS_AcCxformMultiply2Texture,

         PS_Count,
         PS_CountSource = PS_AcCxformTexture,
         PS_AcOffset = PS_AcCxformTexture-PS_CxformTexture
      };

   static BFlashRender* createRenderer();

   void setEnableBatching(bool bEnable);
   void setEnableWireframe(bool bEnable);
   
   // Helpers used to create vertex declarations and shaders. 
   bool CreateVertexDeclaration(GPtr<IDirect3DVertexDeclaration9> *pdeclPtr, const D3DVERTEXELEMENT9 *pvertexElements);   
   bool CreateVertexShader(GPtr<IDirect3DVertexShader9> *pshaderPtr, const DWORD* pshaderText);
   bool CreatePixelShader(GPtr<IDirect3DPixelShader9> *pshaderPtr, const DWORD* pshaderText);   

   // Initialize the vshader we use for SWF mesh rendering.
   bool InitShaders();   
   void ReleaseShaders();

   // Sets a shader to specified type.
   void SetVertexDecl(BFlashRenderVDeclType vd);   
   void SetVertexShader(BFlashRenderVShaderType vt);
   
   // Set shader to a device based on a constant
   void SetPixelShader(BFlashRenderPixelShaderType shaderTypein);
   
   // Utility.  Mutates *width, *height and *data to create the
   // next mip level.
   static void MakeNextMiplevel(int* width, int* height, UByte* data);   
   // Fill helper function:
   // Applies fill texture by setting it to the specified stage, initializing samplers and vertex constants
   bool ApplyFillTexture(const FillTexture &fill, UInt stageIndex, UInt matrixVertexConstIndex);   
   bool ApplyFillTexture(const FillTexture &fill, UInt stageIndex);   
   void ApplyPShaderCxform(const Cxform &cxform, UInt cxformPShaderConstIndex) const;
      
   // Given an image, returns a Pointer to a GTexture struct
   // that can later be passed to FillStyleX_bitmap(), to set a
   // bitmap fill style.
   GTextureD3D9* CreateTexture();   
   // Helper function to query renderer capabilities.
   bool GetRenderCaps(RenderCaps *pcaps);

   // Set up to render a full frame from a movie and fills the
   // background.  Sets up necessary transforms, to scale the
   // movie to fit within the given dimensions.  Call
   // EndDisplay() when you're done.
   //
   // The Rectangle (ViewportX0, ViewportY0, ViewportX0 +
   // ViewportWidth, ViewportY0 + ViewportHeight) defines the
   // window coordinates taken up by the movie.
   //
   // The Rectangle (x0, y0, x1, y1) defines the pixel
   // coordinates of the movie that correspond to the viewport
   // bounds.
   void BeginDisplay(
      GColor backgroundColor, const GViewport &vpin,
      Float x0, Float x1, Float y0, Float y1);
   
   // Clean up after rendering a frame.  Client program is still
   // responsible for calling Present or glSwapBuffers()
   void EndDisplay();
   
   // Set the current transform for mesh & line-strip rendering.
   void SetMatrix(const Matrix& m);
   void SetUserMatrix(const Matrix& m);
   // Set the current color transform for mesh & line-strip rendering.
   void SetCxform(const GRenderer::Cxform& cx);

   // Structure describing color combines applied for a given blend mode.
   struct BlendModeDesc
   {
      D3DBLENDOP  BlendOp;
      D3DBLEND    SrcArg, DestArg;
   };

   struct BlendModeDescAlpha
   {
      D3DBLENDOP  BlendOp;
      D3DBLEND    SrcArg, DestArg;
      D3DBLEND    SrcAlphaArg, DestAlphaArg;
   };

   void ApplyBlendMode(BlendType mode);   
   void ApplySampleMode(BitmapSampleMode mode);
   void ApplyMaskMode(int maskMode, bool bForce);
   
   // Pushes a Blend mode onto renderer.
   virtual void PushBlendMode(BlendType mode);   
   // Pops a blend mode, restoring it to the previous one. 
   virtual void PopBlendMode();
   // multiply current GRenderer::Matrix with d3d GRenderer::Matrix
   void ApplyMatrix(const Matrix& matIn);   
   // Set the given color. 
   void ApplyColor(GColor c);
   
   // Don't fill on the {0 == left, 1 == right} side of a path.
   void FillStyleDisable();   
   // Don't draw a line on this path.
   void LineStyleDisable();   
   // Set fill style for the left interior of the shape.  If
   // enable is false, turn off fill for the left interior.
   void FillStyleColor(GColor color);

   // Set the line style of the shape.  If enable is false, turn
   // off lines for following curve segments.
   void LineStyleColor(GColor color);
   void FillStyleBitmap(const FillTexture *pfill);
   // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
   void FillStyleGouraud(GouraudFillType gfill,
      const FillTexture *ptexture0,
      const FillTexture *ptexture1,
      const FillTexture *ptexture2);
   void SetVertexData(const void* pvertices, int numVertices, VertexFormat vf, CacheProvider *pcache);   
   void SetIndexData(const void* pindices, int numIndices, IndexFormat idxf, CacheProvider *pcache);   
   void ReleaseCachedData(CachedData *pdata, CachedDataType type);   

   void DrawIndexedTriList(int baseVertexIndex, int minVertexIndex, int numVertices,
      int startIndex, int triangleCount);   
   // Draw the line strip formed by the sequence of points.
   void DrawLineStrip(int baseVertexIndex, int lineCount);
  
   // Draw a set of rectangles textured with the given bitmap, with the given color.
   // Apply given transform; ignore any currently set transforms.  
   //
   // Intended for textured glyph rendering.
   void DrawBitmaps(BitmapDesc* pbitmapList, int listSize, int startIndex, int count,
      const GTexture* pti, const Matrix& m,
      CacheProvider *pcache);

   void BeginSubmitMask(SubmitMaskMode maskMode);   
   void EndSubmitMask();   
   void DisableMask();   
   virtual void GetRenderStats(Stats *pstats, bool resetStats);
   virtual bool SetDependentVideoMode(
      LPDIRECT3DDEVICE9 pd3dDevice,
      D3DPRESENT_PARAMETERS* ppresentParams,
      UInt32 vmConfigFlags,
      HWND hwnd);   
   // Returns back to original mode (cleanup)
   virtual bool ResetVideoMode();   
   virtual DisplayStatus CheckDisplayStatus() const { return mModeSet ? DisplayStatus_Ok : DisplayStatus_NoModeSet; }
   
   // Direct3D9 Access
   // Return various Dirext3D related information
   virtual LPDIRECT3D9 GetDirect3D() const;
   virtual LPDIRECT3DDEVICE9 GetDirect3DDevice() const;   
   virtual bool GetDirect3DPresentParameters(D3DPRESENT_PARAMETERS* ppresentParams) const;

   //-- BEvent Receiver interface
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   
   static BDynamicGPUBuffer* getDynamicGPUBuffer(void) { return mpFlashDynamicGPUBuffer; }
       
   // Direct3DDevice
   IDirect3DDevice9*   mpDevice;

   private:

      // Some renderer state.
      bool                mModeSet;
      SInt                mRenderMode;

      // Shaders and declarations that are currently set.
      BFlashRenderVDeclType           mVDeclIndex;
      BFlashRenderVShaderType         mVShaderIndex;

      // Current pixel shader index
      int                 mPShaderIndex;

      // Video Mode Configuration Flags (VMConfigFlags)
      UInt32              mVMCFlags;

      // Created vertex declarations and shaders.
      GPtr<IDirect3DVertexDeclaration9> mVertexDecls[VD_Count];
      GPtr<IDirect3DVertexShader9>      mVertexShaders[VS_Count];

      // Allocated pixel shaders
      GPtr<IDirect3DPixelShader9>       mPixelShaders[PS_Count];

      // This flag indicates whether we've checked for stencil after BeginDisplay or not.
      bool                mStencilChecked;
      // This flag is stencil is available, after check.
      bool                mStencilAvailable;
      DWORD               mStencilCounter;

      // Output size.
      Float               mDisplayWidth;
      Float               mDisplayHeight;

      Matrix              mUserMatrix;
      Matrix              mViewportMatrix;
      Matrix              mCurrentMatrix;
      Cxform              mCurrentCxform;

      // Link list of all allocated textures
      GRendererNode       mTextures;
      mutable GLock       mTexturesLock;

      Stats               mRenderStats;

      // Vertex data pointer
      const void*         mpVertexData;
      const void*         mpIndexData;
      int                 mIndexCount;
      VertexFormat        mVertexFmt;
      _D3DFORMAT          mIndexFmt;

      // Current sample mode
      BitmapSampleMode        mSampleMode;

      // Current blend mode
      BlendType               mBlendMode;
      BlendType               mBlendModeCache;
      GTL::garray<BlendType>  mBlendModeStack;
      int                     mMaskMode;

      // Presentation parameters specified to configure the mode.
      D3DPRESENT_PARAMETERS   mPresentParams;
      HWND                    mhWnd;

      // Buffer used to pass along glyph vertices
      enum { GlyphVertexBufferSize = 6 * 48 };

      // Vertex buffer structure used for glyphs.    
      struct BFlashGlyphVertex
      {
         float x,y,z,w;
         float u,v;
         GColor color;

         void SetVertex2D(float xx, float yy, float uu, float vv, GColor c)
         {
            x = xx; 
            y = yy;
            z = 0.0f;
            w = 1.0f;
            u = uu; 
            v = vv;
            color = c;
         }
      };

      struct BFlashGlyphUberVertex
      {
         float x;
         float y;
         float u;
         float v;         
         DWORD color;
         float paramIndex;
      };

      struct BFlashGlyphUberParameterVertex
      {
         float mMatrixV0[4];
         float mMatrixV1[4];
         float mCxForm0[4];
         float mCxForm1[4];         
      };

      struct BFlashUberVertex
      {
         SInt16  x;
         SInt16  y;
         UInt32  Color;
         UInt32  Factors;
         float   paramIndex;
      };

      struct BFlashUberParameterVertex
      {
         float mMatrixV0[4];
         float mMatrixV1[4];
         float mTexGen0[4];
         float mTexGen1[4];
         float mTexGen2[4];
         float mTexGen3[4];
         float mCxForm0[4];
         float mCxForm1[4];
         XMCOLOR mColor;
      };

      BFlashGlyphVertex       mGlyphVertexBuffer[GlyphVertexBufferSize];

      // Linked list used for buffer cache testing, otherwise holds no data.
      CacheNode                 mCacheList;
                  
      // Style state.
      enum StyleIndex
      {
         FILL_STYLE = 0,     
         LINE_STYLE,
         STYLE_COUNT
      };

      BFlashFillStyle mCurrentStyles[STYLE_COUNT];


      enum BBatchType
      {
         eBatchInvalid,
         eBatchLineStrip,
         eBatchBitmaps,
         eBatchTriangleLists,
         eBatchBeginSubmitMask,
         eBatchEndSubmitMask,
         eBatchDisableMask,
         eBatchTypeTotal,
      };

      class BBatchData
      {
         public:
            BBatchData () {clear();};
            //~BBatchData() {};

            void clear()
            {
               mBatchType  = eBatchInvalid;
               mVertexDecl = -1;
               mVSIndex    = -1;
               mPSIndex    = -1;
               mFillStyle.clear();
               mMatrix.SetIdentity();

               mpVertexData = NULL;
               mVertexCount = 0;
               mpIndexData  = NULL;
               mIndexCount  = 0;
               mPrimCount   = 0;
               mVertexStartIndex = 0;
               mVertexMinIndex = 0;
               mCxForm.SetIdentity();
               mpTexture = NULL;
               mBlendMode = GRenderer::Blend_None;
               mBitmapSampleMode = GRenderer::Sample_Linear;
               mMaskMode = eMaskModeNone;
               mBitmapListCount = 0;
               mpBitmapList = NULL;
               mIndexStartIndex = 0;
               mIndexFormat = D3DFMT_INDEX16;
               mVertexFormat = -1;

               mTechniquePassIndex = 0;
               mRenderStream0VertexOffset = 0;
               mRenderStream1VertexOffset = 0;
               mRenderStream2VertexOffset = 0;
               mRenderStream0IndexOffset  = 0;
               mEnableAlphaBlending = false;
               mbApplyFillTexture1 = false;
               mbApplyFillTexture2 = false;
               mbApplyColor        = false;
            };

            bool compare(const BBatchData& rhs) const
            {
               const BBatchData& lhs = *this;
               #define COMPARE(v) if (!(lhs.##v == rhs.##v)) return false;          
               COMPARE(mBatchType);
               COMPARE(mTechniquePassIndex);
               COMPARE(mBlendMode);
               COMPARE(mMaskMode);
               COMPARE(mFillStyle);
               COMPARE(mpTexture);
               COMPARE(mBitmapSampleMode);
               #undef COMPARE
               return true;
            }

            BBatchType                  mBatchType;
            int                         mVertexDecl;
            int                         mVSIndex;
            int                         mPSIndex;
            BFlashFillStyle             mFillStyle;
            GRenderer::Matrix           mMatrix;
            void*                       mpVertexData;
            int                         mVertexCount;
            int                         mVertexStartIndex;
            int                         mVertexMinIndex;
            void*                       mpIndexData;
            int                         mIndexCount;
            int                         mPrimCount;
            GRenderer::Cxform           mCxForm;
            IDirect3DTexture9*          mpTexture;
            GRenderer::BlendType        mBlendMode;
            GRenderer::BitmapSampleMode mBitmapSampleMode;
            GRenderer::BitmapDesc*      mpBitmapList;
            int                         mMaskMode;
            int                         mBitmapListCount;
            int                         mIndexStartIndex;
            int                         mIndexFormat;
            int                         mVertexFormat;

            //-- Render specific Data
            int                         mTechniquePassIndex;
            int                         mRenderStream0VertexOffset;
            int                         mRenderStream1VertexOffset;
            int                         mRenderStream2VertexOffset;
            int                         mRenderStream0IndexOffset;
            bool                        mEnableAlphaBlending:1;
            bool                        mbApplyFillTexture1 :1;
            bool                        mbApplyFillTexture2 :1;
            bool                        mbApplyColor        :1;
      };

      class BRenderBatchData
      {
         public:
            int  mTechniquePassIndex;
            int  mRenderStream0VertexOffset;
            int  mRenderStream1VertexOffset;
            int  mRenderStream0IndexOffset;
            bool mEnableAlphaBlending:1;
            bool mbApplyFillTexture1 :1;
            bool mbApplyFillTexture2 :1;
      };

      
      
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

      // reload interface
      void reloadInit();
      void reloadDeinit();

      // effect initialization
      void loadEffect();
      void initEffect(const BEvent& event);
      void initVertexDeclarations();
      void killEffect();


      //-- Batching interface
      void renderBatchingInit(void);
      void renderBatchingLock(uint vb1Size, uint vb2Size, uint vb3Size, uint ibSize);
      void renderBatchingUnlock();
      void renderBatchingEnd();
      void renderBatchingFlush();
            
      BDynamicArray<BBatchData> mBatches;
      BDynamicArray<BRenderBatchData> mRenderBatches;
      
      void addBatch(BBatchData& batch);

      void prepCommonRenderData(BBatchData& batch);
      void prepLineStripBatch(BBatchData& batch);
      void prepBitmapBatch(BBatchData& batch, bool bCombineBatch);      
      void prepTriangleListBatch(BBatchData& batch, bool bCombineBatch);

      void drawLineStripBatch(BBatchData& b);
      void drawBitmapsBatch(BBatchData& b);
      void drawTriangleListBatch(BBatchData& b);

      static BDynamicGPUBuffer* mpFlashDynamicGPUBuffer;
                        
      IDirect3DVertexBuffer9* mpDynamicVB1;      
      void* mpDynamicVB1Buffer;
      uint mDynamicVB1Size;
      uint mDynamicVB1BytesLeft;
      uint mDynamicVB1Offset;

      IDirect3DVertexBuffer9* mpDynamicVB2;
      void* mpDynamicVB2Buffer;
      uint mDynamicVB2Size;
      uint mDynamicVB2BytesLeft;
      uint mDynamicVB2Offset;
      float mParamIndex;

      IDirect3DVertexBuffer9* mpDynamicVB3;
      void* mpDynamicVB3Buffer;
      uint mDynamicVB3Size;
      uint mDynamicVB3BytesLeft;
      uint mDynamicVB3Offset;
      float mGlyphParamIndex;

      IDirect3DIndexBuffer9*  mpDynamicIB1;
      void* mpDynamicIB1Buffer;
      uint mDynamicIB1Size;
      uint mDynamicIB1BytesLeft;
      uint mDynamicIB1Offset;
      uint mIBStartIndex;

      BTimer mTimer;

      BFXLEffect          mEffect;
      BFXLEffectTechnique mTechnique;
      BFXLEffectTechnique mTechniqueAC;
      BFXLEffectParam     mTextureParam0;
      BFXLEffectParam     mTextureParam1;
      BFXLEffectParam     mCxForm0Param;
      BFXLEffectParam     mCxForm1Param;
      BFXLEffectParam     mMatrixV0Param;
      BFXLEffectParam     mMatrixV1Param;

      bool mbInitialized: 1;
      bool mbBatchingEnabled : 1;
      bool mbWireframeEnabled : 1;
      
}; // class GRendererXbox360Impl


//============================================================================
// graphrenderer.h
// Ensemble Studios (C) 2007
//============================================================================
#pragma once
#include "effect.h"
#include "renderThread.h"
#include "renderCommand.h"
#include "graphattribs.h"
#include "render2Dprimitiveutility.h"

//============================================================================
//============================================================================
class BGraphRenderer: public BRenderCommandListener, BEventReceiver
{
   public:
      BGraphRenderer();
     ~BGraphRenderer();

      enum
      {
         eEventClassReloadEffects = cEventClassFirstUser
      };

      bool init();
      void deinit();

      void render(const BGraphAttribs& attribs);

      bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);

   private:

      enum
      {
         cGRCommandDrawGraph = 0,
         cGRCommandTotal
      };

      enum
      {
         cBlendModeAlpha,
         cBlendModeAdditive,
         cBlendModeTotal,
      };

      struct BLineVertex
      {
         float x;
         float y;
         float z;
         float rhw;
         float tu;
         float tv;
         DWORD diffuse;         
      };

      struct BGraphRenderTimelineAttribs
      {
         int mBlendmode;
         int mDotBlendmode;
         float mColorAlpha;
         float mDotInterval;
         float mDotSizeX;
         float mDotSizeY;
         float mLineSize;
         XMCOLOR mLineColor;
         XMCOLOR mDotColor;
         BManagedTextureHandle mTextureHandle;
         bool mOverrideLineColor : 1;
         bool mOverrideDotColor : 1;
         bool mRenderDots : 1;
         bool mUseTexture : 1;
      };
      
      //-- effect management
      void reloadInit();
      void reloadDeinit();
      void loadEffect();
      void initEffect(const BEvent& event);
      void initVertexDeclarations();
      void killEffect();

      //-- rendering
      void drawBackgroundInternal(const BGraphAttribs& attribs);    
      void drawLegend(const BGraphAttribs& attribs);
      void drawGraphTimelinesInternal3(const BGraphAttribs& attribs, const BGraphRenderTimelineAttribs& timelineAttribs);      
      void drawFilledGraphTimelinesInternal(const BGraphAttribs& attribs);
      void drawGraphInternal(int techniquePass);
      void drawGraphTimelinesDots(const BGraphAttribs& attribs, const BGraphRenderTimelineAttribs& renderAttribs);

      void setBlendMode(uint blendmode);
      void tesselateLine(float x1, float y1, float x2, float y2, float thickness, DWORD color, void* pBuffer);
      
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


      template<class T> void clearPrimitives(T& list, int category);
      void addSprite(XMVECTOR position, int blendmode, BManagedTextureHandle texture, DWORD color, float scaleX, float scaleY, float offsetX, float offsetY, int layer, float timeout);
      BDynamicArray<B2DPrimitiveSprite, 16> mSpritePrimitives;

      // Data
      BFXLEffect          mEffect;
      BFXLEffectTechnique mTechnique;
      BFXLEffectParam     mDiffuseTexture;


      BManagedTextureHandle      mTextureDotAdditive;
      BManagedTextureHandle      mTextureDotAdditive2;
      BManagedTextureHandle      mTextureDotAlphablend;
      BManagedTextureHandle      mTextureLineAlphablend;
      BManagedTextureHandle      mTextureLineAdditive;
      BManagedTextureHandle      mTextureTick;



      IDirect3DVertexDeclaration9* mpLineVertexDecl;
      bool                mbInitialized : 1;
      bool                mbEdgeAntiAlias;
};

extern BGraphRenderer gGraphRenderer;
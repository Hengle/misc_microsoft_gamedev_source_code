//============================================================================
//
// flashmovie.h
//
//============================================================================

#pragma once 
#include "flashmoviedefinition.h"
#include "bitvector.h"
#include "D3DTextureManager.h"

//============================================================================
//============================================================================
class BFlashMovie 
{
   public: 
      BFlashMovie();
     ~BFlashMovie();

      enum BFlashMovieFlag
      {
        eFlagRenderToTexture = 0,
        eFlagScaleEnable,
        eFlagPaused,
        eFlagTotal,
      };
      
      bool init(int dataIndex, BFlashMovieDefinition* pData, bool bRenderToTexture);
      void deinit();
      BManagedTextureHandle getRenderTargetHandle() { return mRenderTargetHandle; };
      IDirect3DTexture9* getRenderTargetTexture() { return mpRenderTargetTexture; };
      
      bool isInitialized() const {return mbInitialized;};

      //-- Data Modification
      void invoke(const char* method, const char* fmt, const char* value);
      void invoke(const char* method, const GFxValue* pArgs, int argCount);
      void setVariable(const char* variablePath, const char* value, GFxMovie::SetVarType type = GFxMovie::SV_Normal);
      void setVariable(const char* variablePath, const GFxValue& value, GFxMovie::SetVarType type = GFxMovie::SV_Normal);
      void setVariableArray(const char* variablePath, const GFxValue* pValue, int count, int startIndex=0, GFxMovie::SetVarType type = GFxMovie::SV_Normal);
      void handleEvent(const GFxEvent& event);
      void setBackGroundColor(XMCOLOR color);
      void setBackGroundAlpha(float alpha);
      void setDimensions(int x, int y, int width, int height);

      void render();
      void releaseGPUHeapTexture();

      bool getFlag(int n) const { return(mFlags.isSet(n)!=0); }
      void setFlag(int n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

      int                    mDataIndex;
      BFlashMovieDefinition* mpData;
      GPtr<GFxMovieView>     mpMovie;

   private:
      void clear();
      void updateViewSize();
      
      void renderBegin();
      void renderEnd();
      void renderToTextureBegin();
      void renderToTextureEnd();

      UTBitVector<32>        mFlags;
      XMCOLOR                mBackgroundColor;
      BManagedTextureHandle         mRenderTargetHandle;

      // Movie timing state      
      float                  mSpeedScale;         // Advance speed, def 1.0f
      SInt                   mFrameCounter;       // Frames rendered, for FPS
      
      // Time ticks: always rely on a timer, for FPS
      UInt32                 mTimeStartTicks;     // Ticks during the start of playback
      UInt32                 mTimeTicks;          // Current ticks
      UInt32                 mLastLoggedFps;      // Time ticks during last FPS log

      // Movie logical ticks: either timer or setting controlled
      UInt32                 mMovieStartTicks;
      UInt32                 mMovieLastTicks;
      UInt32                 mMovieTicks;

      // View width and height
      float                  mScaleX;
      float                  mScaleY;
      int                    mX;
      int                    mY;
      SInt                   mWidth;
      SInt                   mHeight;

      //-- Render Target Cache
      IDirect3DSurface9* mpRenderTarget;      
      IDirect3DSurface9* mpDepthStencil;
      IDirect3DTexture9* mpRenderTargetTexture;


      IDirect3DSurface9* mpSavedColorSurf;
      IDirect3DSurface9* mpSavedDepthSurf;
      D3DVIEWPORT9 mSavedViewport;
      RECT mSavedScissorRect;

      bool mbInitialized : 1;
};
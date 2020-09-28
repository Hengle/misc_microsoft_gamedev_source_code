//============================================================================
// flashreticle.h
//
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "visual.h"

class BCost;

//============================================================================
//============================================================================
class BReticleHelpButtonState
{
   public:
      BReticleHelpButtonState(): mFrameID(-1), mVisible(false), mDirty(false) {};
      ~BReticleHelpButtonState() {};

      void clear()
      {
         mFrameID = -1;
         mVisible = false;
         mDirty = false;
      }

      int mFrameID;
      bool mVisible;
      bool mDirty;
};
//============================================================================
// class BFlashReticle
//============================================================================
class BFlashReticle : public BFlashScene
{
public:
   BFlashReticle();
   virtual ~BFlashReticle();

   enum
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
   };

   enum BReticleControls
   {
      cButtonA = 0,
      cButtonB,
      cButtonX,
      cButtonY,
      cButtonCount,
   };

   enum BReticleASFunctions
   {
      cReticleASFunctionUpdateButtonStates = 0,
      cReticleASFunctionTotal,
   };

   enum BReticleKeyframes
   {
      cReticleKeyframeOff = 0,
      cReticleKeyframeOn,
      cReticleKeyframeActive,
      cReticleKeyframeAlert,
      cReticleKeyframePending,
      cReticleKeyframeTotal,
   };

   bool init(const char* filename, const char* datafile);
   void deinit();
   void enter();
   void leave();
   void update(float elapsedTime);
   void renderBegin();
   void render();
   void renderEnd();
   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   void setDimension(int x, int y, int width, int height);
   void setVisible(bool bVisible) { mbVisible = bVisible; };
   bool getVisible() const { return mbVisible; }
   BManagedTextureHandle getRenderTargetTexture();
   BFlashMovieInstance*  getMovie() { return mpMovie; }

   //-- specific functions
   // void setMode(int mode);
   void setMode(int mode, uint goodAgainstRating, const BCost* pCost);
   void setReticleHelp(int buttonID, int frameID);
   const BReticleHelpButtonState* getButtonState(int buttonID);
   void updateButtons();

private:
   int                        mMode;   
   BReticleHelpButtonState    mHelpButtonState[cButtonCount];
   uint                       mGoodAgainstRating;
   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;
   bool                       mbVisible : 1;
};
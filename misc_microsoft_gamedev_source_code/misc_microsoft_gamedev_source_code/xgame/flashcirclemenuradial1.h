//============================================================================
// flashuiGame.h
//
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "texturemanager.h"
#include "visual.h"

//============================================================================
// class BFlashCircleMenuRadial1
//============================================================================
class BFlashCircleMenuRadial1 : public BFlashScene
{
public:
   BFlashCircleMenuRadial1();
   virtual ~BFlashCircleMenuRadial1();

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
   };

   void init(const char* filename);
   void deinit();
   void enter();
   void leave();
   void update();
   void renderBegin();
   void render();
   void renderEnd();
   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   BTextureHandle getRenderTargetTexture();

private:
   int                        mCivID;
   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;
};
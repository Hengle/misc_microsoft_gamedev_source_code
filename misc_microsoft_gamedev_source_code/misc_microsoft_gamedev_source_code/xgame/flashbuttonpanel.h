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
// class BFlashButtonPanel
//============================================================================
class BFlashButtonPanel : public BFlashScene
{
public:
   BFlashButtonPanel();
   virtual ~BFlashButtonPanel();

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
   };

   enum BUIButtonLabel
   {
      eLabelUp,
      eLabelDown,
      eLabelLeft,
      eLabelRight,
      eLabelTotal,
   };

   void init(const char* filename);
   void deinit();
   void enter();
   void leave();
   void update();
   void renderBegin();
   void render();
   void renderEnd();
   void handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   BTextureHandle getRenderTargetTexture();

   void setLabelText(int labelID, const char* text);
   void setLabelVisible(int labelID, bool bVisible);
   void setAllLabelsVisible(bool bVisible);
   void refresh();
private:

   void initPanel();
   void updatePanel();   
   void setText   (int labelID, const char* text);
   uint                       mCivID;
   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;
   BDynamicArray<BSimString>  mText;
   UTBitVector<eLabelTotal>   mEnableStates;  
};
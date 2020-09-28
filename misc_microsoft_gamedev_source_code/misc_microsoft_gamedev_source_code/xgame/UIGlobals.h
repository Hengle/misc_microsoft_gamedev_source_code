//============================================================================
// uiglobals.h
// Ensemble Studios (c) 2007
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "visual.h"


// class BUIGlobals;
// Global pointer to the one BUIGlobals object
// extern BUIGlobals gUIGlobals;

//============================================================================
// class BUIGlobals
//============================================================================
class BUIGlobals : public BFlashScene
{
public:
   BUIGlobals();
   virtual ~BUIGlobals();

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
   };

   enum
   {
      cDialogButtonsOK=0,
      cDialogButtonsOKCancel=1,
   };

   enum
   {
      cDialogResultOK=0,
      cDialogResultCancel
   };

   //Interfaces provided to other classes ********************************************
   // This is simple in it's first pass, but will grow, I'm sure.
   class yornHandlerInterface
   {
      public:
         virtual void yornResult(uint result, DWORD userContext, int port) {};
   };


   // Overhead functions
   bool init(const char* filename, const char* datafile);
   void deinit();
   void enter();
   void leave();
   BFlashMovieInstance*  getMovie() { return mpMovie; }
   void update(float elapsedTime);
   void renderBegin();
   void render();
   void renderEnd();
   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   void setDimension(int x, int y, int width, int height);

   BManagedTextureHandle getRenderTargetTexture();

   //-- Yorn methods
   void showYornBox(BUIGlobals::yornHandlerInterface* yornHandler, const BUString& message, uint8 dialogButtons, DWORD userContext=-1, const BUString& okText=sEmptyUString, const BUString& cancelText=sEmptyUString, bool allowAutoCloseOnGameDeinit=true, bool isDeviceRemovedMessage=false);
   bool IsYornHandler(BUIGlobals::yornHandlerInterface* yornHandler) const { return mpYornHandler == yornHandler; }
   void hideYorn();

   //-- Small version of the window
   void showYornBoxSmall(BUIGlobals::yornHandlerInterface* yornHandler, const BUString& message, uint8 dialogButtons, DWORD userContext=-1, const BUString& okText=sEmptyUString, const BUString& cancelText=sEmptyUString, bool allowAutoCloseOnGameDeinit=true, bool isDeviceRemovedMessage=false);

   bool isYorNBoxVisible() const { return mMessageBoxVisible; }
   bool getAllowAutoCloseOnGameDeinit() const { return mAllowAutoCloseOnGameDeinit; }
   bool getDeviceRemovedMessage() const { return mDeviceRemovedMessage; }

   //-- Wait Dialog
   void showWaitDialog(const BUString& text);
   void setWaitDialogVisible(bool bVisible);
   bool getWaitDialogVisible() { return mWaitDialogVisible; }

   void cancel(void);

private:
   void hideYornSmall();
   void setYornVisible(bool bVisible, const BUString& message, uint8 dialogButtons, const BUString& okText, const BUString& cancelText);
   void setYornSmallVisible(bool bVisible, const BUString& message, uint8 dialogButtons, const BUString& okText, const BUString& cancelText);

   // helper methods
   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;

   BUIGlobals::yornHandlerInterface* mpYornHandler;
   DWORD mUserContext;
   uint8 mDialogButtons;


   // Flags
   bool mUIGlobalsVisible  : 1;
   bool mWaitDialogVisible : 1;
   bool mMessageBoxVisible : 1;
   bool mAllowAutoCloseOnGameDeinit : 1;
   bool mDeviceRemovedMessage : 1;
};

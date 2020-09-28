//============================================================================
// UIGlobals.cpp
// Ensemble Studios (c) 2007
//============================================================================

#include "common.h"
#include "uiglobals.h"
#include "ui.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"

#include "visualmanager.h"
#include "database.h"
#include "FontSystem2.h"
#include "configsgame.h"
#include "soundmanager.h"
#include "usermanager.h"
#include "user.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUIGlobals::BUIGlobals():
mpMovie(NULL),
mpYornHandler(NULL),
mUserContext(0),
mUIGlobalsVisible(true),
mWaitDialogVisible(false),
mMessageBoxVisible(false),
mAllowAutoCloseOnGameDeinit(true),
mDeviceRemovedMessage(false)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUIGlobals::~BUIGlobals()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIGlobals::init(const char* filename, const char* datafile)
{
   if (!loadData(datafile))
      return false;

   gFlashGateway.getOrCreateData(filename, cFlashAssetCategoryInGame, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, false);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   setFlag(cFlagInitialized, true);

   initResolution();

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::deinit()
{
   if (mpMovie)
   {
      gFlashGateway.unregisterEventHandler(mpMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpMovie);
   }
   mpMovie = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::update(float elapsedTime)
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::render()
{   
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);
   if (mUIGlobalsVisible && mpMovie)
      mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIGlobals::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   if (!mMessageBoxVisible)
      return false;

// - Fix for bug  PHX-15489
   // if the primary user is active then we need to qualify the input, otherwise let it through.
   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   if (pPrimaryUser && pPrimaryUser->getFlagUserActive())
   {
      // we have a primary user, they have been assigned. qualify the input.
      if (pPrimaryUser->getPort() != port)
            return true;      // eat it.
   }


   bool handled = false;
   uint result = cDialogResultCancel;

   // The message box is up, let's see if it's for us
   if (event == cInputEventControlStart)
   {
      switch (controlType)
      {
      case cButtonA:
         {
            // gUI.playClickSound();
            gSoundManager.playCue("play_ui_game_menu_select");
            result = cDialogResultOK;
            handled=true;
         }
         break;
      case cButtonB:
         {
            // don't allow B to be handled if OK is the only valid option.
            if (mDialogButtons != cDialogButtonsOKCancel)
               break;

            // gUI.playClickSound(); 
            gSoundManager.playCue("play_ui_menu_back_button");
            result = cDialogResultCancel;
            handled=true;
         }
         break;
      }
   }

   // did we get input for this?
   if (handled)
   {
      BUIGlobals::yornHandlerInterface* pYornHandler = mpYornHandler;
      DWORD userContext = mUserContext;

      // bring down the dialog box
      hideYorn();
      hideYornSmall();

      // remove our callback handler
      mpYornHandler=NULL;
      mUserContext=0;

      // call the handler
      if (pYornHandler)
         pYornHandler->yornResult(result, userContext, port);

   }

   // we handled the input
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::showYornBox(BUIGlobals::yornHandlerInterface* yornHandler, const BUString& message, uint8 dialogButtons, DWORD userContext, const BUString& okText, const BUString& cancelText, bool allowAutoCloseOnGameDeinit, bool isDeviceRemovedMessage)
{ 
   mpYornHandler=yornHandler;
   mUserContext=userContext;
   mDialogButtons=dialogButtons;
   mAllowAutoCloseOnGameDeinit = allowAutoCloseOnGameDeinit;
   mDeviceRemovedMessage = isDeviceRemovedMessage;

   // show the dialog
   setYornVisible(true, message, dialogButtons, okText, cancelText);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::showYornBoxSmall(BUIGlobals::yornHandlerInterface* yornHandler, const BUString& message, uint8 dialogButtons, DWORD userContext, const BUString& okText, const BUString& cancelText, bool allowAutoCloseOnGameDeinit, bool isDeviceRemovedMessage)
{ 
   mpYornHandler=yornHandler;
   mUserContext=userContext;
   mDialogButtons=dialogButtons;
   mAllowAutoCloseOnGameDeinit = allowAutoCloseOnGameDeinit;
   mDeviceRemovedMessage = isDeviceRemovedMessage;

   // show the dialog
   setYornSmallVisible(true, message, dialogButtons, okText, cancelText);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::hideYorn()
{
   if (!mMessageBoxVisible)
      return;

   hideYornSmall();

   // only make the call if we are up
   mMessageBoxVisible=false;
   GFxValue values[3];
   values[0].SetBoolean(false);
   values[1].SetStringW(L"");
   values[2].SetNumber(0);
   mpMovie->invokeActionScript("setYornVisible", values, 3);
   gSoundManager.playCue( "play_ui_menu_open_and_close" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::hideYornSmall()
{
   if (!mMessageBoxVisible)
      return;

   // only make the call if we are up
   mMessageBoxVisible=false;
   GFxValue values[3];
   values[0].SetBoolean(false);
   values[1].SetStringW(L"");
   values[2].SetNumber(0);
   mpMovie->invokeActionScript("setYornSmallVisible", values, 3);
   gSoundManager.playCue( "play_ui_menu_open_and_close" );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::setYornSmallVisible(bool bVisible, const BUString& message, uint8 dialogButtons, const BUString& okText, const BUString& cancelText)
{
   mMessageBoxVisible=bVisible;

   GFxValue values[5];
   values[0].SetBoolean(bVisible);
   values[1].SetStringW(message.getPtr());
   values[2].SetNumber(dialogButtons);
   values[3].SetStringW(okText.isEmpty() ? gDatabase.getLocStringFromID(24696).getPtr() : okText.getPtr());
   values[4].SetStringW(cancelText.isEmpty() ? gDatabase.getLocStringFromID(101).getPtr() : cancelText.getPtr());
   mpMovie->invokeActionScript("setYornSmallVisible", values, 5);
   gSoundManager.playCue( "play_ui_menu_open_and_close" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::setYornVisible(bool bVisible, const BUString& message, uint8 dialogButtons, const BUString& okText, const BUString& cancelText)
{
   mMessageBoxVisible=bVisible;

   GFxValue values[5];
   values[0].SetBoolean(bVisible);
   values[1].SetStringW(message.getPtr());
   values[2].SetNumber(dialogButtons);
   values[3].SetStringW(okText.isEmpty() ? gDatabase.getLocStringFromID(24696).getPtr() : okText.getPtr());
   values[4].SetStringW(cancelText.isEmpty() ? gDatabase.getLocStringFromID(101).getPtr() : cancelText.getPtr());
   mpMovie->invokeActionScript("setYornVisible", values, 5);
   gSoundManager.playCue( "play_ui_menu_open_and_close" );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIGlobals::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BManagedTextureHandle BUIGlobals::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidManagedTextureHandle;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIGlobals::setDimension(int x, int y, int width, int height)
{
   if (!mpMovie)
      return;

   mpMovie->setDimension(x, y, width, height);
}

//-- Wait Dialog
//==============================================================================
// BUIGlobals::showWaitDialog
//==============================================================================
void BUIGlobals::showWaitDialog(const BUString& text)
{

   GFxValue values[1];
   values[0].SetStringW(text.getPtr());

   // do we call the show, or just update text?
   if (mWaitDialogVisible)
      mpMovie->invokeActionScript("updateWaitText", values, 1);
   else
      mpMovie->invokeActionScript("showWaitWithText", values, 1);

   mWaitDialogVisible=true;
}

//==============================================================================
// BUIGlobals::setWaitDialogVisible
//==============================================================================
void BUIGlobals::setWaitDialogVisible(bool bVisible)
{
   if (mWaitDialogVisible==bVisible)
      return;

   mWaitDialogVisible=bVisible;

   GFxValue values[1];
   values[0].SetBoolean(bVisible);
   mpMovie->invokeActionScript("setWaitDialogVisible", values, 1);
}

//============================================================================
//============================================================================
void BUIGlobals::cancel()
{
   mpYornHandler=NULL;
   mUserContext=0;

   hideYorn();
   hideYornSmall();
}
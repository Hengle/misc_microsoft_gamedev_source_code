//============================================================================
// UIMainMenuScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UIAttractScreen.h"

#include "usermanager.h"
#include "user.h"
#include "modemanager.h"
#include "modemenu.h"
#include "database.h"
#include "UIGlobals.h"
#include "game.h"

#include "configsgame.h"

//============================================================================
//============================================================================
BUIAttractScreen::BUIAttractScreen( void )
{
   mName.set("UIAttractScreen");
}

//============================================================================
//============================================================================
BUIAttractScreen::~BUIAttractScreen( void )
{
}

//============================================================================
//============================================================================
bool BUIAttractScreen::init(BXMLNode dataNode)
{
   __super::init(dataNode);

   // loadBackgroundMovie(dataNode);

   mESRBNotice.init(this, "mESRB");

   if( gConfig.isDefined(cConfigShowESRBNotice) )
      mESRBNotice.setText( gDatabase.getLocStringFromID(26003) );

   mTextField.init(this, "mText");

   mTextField.setText(gDatabase.getLocStringFromID(24943));

   mCopyright.init(this, "mCopyright");
   mCopyright.setText(gDatabase.getLocStringFromID(26007));

   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   if (puiGlobals)
      puiGlobals->setWaitDialogVisible(false);

   return true;
}

//============================================================================
//============================================================================
bool BUIAttractScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="assignPrimaryUser")
   {
      // fixme - this is where I will attach the primary user
      BUser* pUser = gUserManager.getPrimaryUser();
      gUserManager.setUserPort(BUserManager::cPrimaryUser, port);
      pUser->setFlagUserActive(true);
      gUserManager.updateSigninByPort();

      if (!pUser->isSignedIn())
      {
         // throw up a message saying that progress will not be saved.
         gGame.getUIGlobals()->showYornBox(this, gDatabase.getLocStringFromID(25577), BUIGlobals::cDialogButtonsOKCancel, 0, gDatabase.getLocStringFromID(25578), gDatabase.getLocStringFromID(25579) );
         return true;
      }

      gUI.playConfirmSound();

      if( mpHandler )
      {
         mpHandler->handleUIScreenResult( this, 0 );
      }
      
      return true;
   }

   return false;
}


//============================================================================
//============================================================================
bool BUIAttractScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;
   handled = BUIScreen::handleInput(port, event, controlType, detail);
   return handled;
}

//==============================================================================
void BUIAttractScreen::yornResult(uint result, DWORD userContext, int port)
{
   if( result == BUIGlobals::cDialogResultOK )
   {
      gUserManager.showSignInUI();
   }

   if( mpHandler )
   {
      mpHandler->handleUIScreenResult( this, 0 );
   }
}

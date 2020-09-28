//============================================================================
// UIEndGame.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIEndGame.h"
#include "usermanager.h"
#include "user.h"
#include "player.h"
#include "civ.h"
#include "soundmanager.h"
#include "uimanager.h"
#include "gamesettings.h"
#include "game.h"

//============================================================================
//============================================================================
BUIEndGame::BUIEndGame( void ) :
   mGameType( -1 ),
   bShown( false ),
   bDoneWaiting( false ),
   mAutoDelay( 3.0f )
{
}

//============================================================================
//============================================================================
BUIEndGame::~BUIEndGame( void )
{
}

//============================================================================
//============================================================================
bool BUIEndGame::init( BXMLNode root )
{
   bool bResult = __super::init( root );
   
   if( bResult )
   {
      bResult = mButtonBar.init( this, "mButtonBar" );
   }

   if( bResult )
   {
      mButtonBar.setButtonStates( BUIButtonBarControl::cFlashButtonA, BUIButtonBarControl::cFlashButtonB );
      mButtonBar.setButtonTexts( gDatabase.getLocStringFromID(25350), gDatabase.getLocStringFromID(25351) );       // stats, quit

      BXMLNode autoDelayNode;
      if( root.getChild( "autoDelay", &autoDelayNode ) )
      {
         autoDelayNode.getTextAsFloat( mAutoDelay );
      }
   }

   return bResult;
}


//============================================================================
//============================================================================
bool BUIEndGame::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( command == "stats" )
   {
      gUI.playConfirmSound();
      if( mpHandler )
         mpHandler->handleUIScreenResult( this, eResult_Stats );
      return true;
   }
   else if( command == "quit" )
   {
      gUI.playCancelSound();
      //gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(24048), BUIGlobals::cDialogButtonsOKCancel, eResult_Quit );
      mpHandler->handleUIScreenResult( this, eResult_Quit );
      return true;
   }

   return false;
}

//============================================================================
//============================================================================
bool BUIEndGame::handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail )
{
   // No pausing or objectives, please.
   if( controlType == cButtonBack || controlType == cButtonStart )
      return true;

   // No input handling in Campaign
   if( mGameType != BGameSettings::cGameTypeSkirmish )
      return true;

   if( event == cInputEventControlStart && !bDoneWaiting )
   {
      fadeOut();
      return true;
   }

   __super::handleInput( port, event, controlType, detail );

   return false;
}

//============================================================================
//============================================================================
void BUIEndGame::setResult( const BString& civName, long result, const BUString& message )
{
   if( !bShown )
   {
      BString resultStr;
      switch( result )
      {
         case BPlayer::cPlayerStateWon:      resultStr = "Win";  break;
         case BPlayer::cPlayerStateDefeated: resultStr = "Lose"; break;
         case BPlayer::cPlayerStateResigned: resultStr = "Quit"; break;
      }
      
      GFxValue args[3];
      args[0].SetString( civName);
      args[1].SetString( resultStr );
      args[2].SetStringW( message );

      if( mGameType == BGameSettings::cGameTypeSkirmish )
      {
         if (!gConfig.isDefined(cConfigNoMusic))
         {
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicStopInGame);
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundPlayerResigned);
         }
      }
      
      invokeActionScript( "setResult", args, 3 );

      gUIManager->setMinimapVisible( false );

      bShown = true;
   }
}


//============================================================================
//============================================================================
void BUIEndGame::enter( void )
{
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if(pSettings)
      pSettings->getLong(BGameSettings::cGameType, mGameType);

   // Show the minimap if necessary.
   if( mGameType == BGameSettings::cGameTypeSkirmish && bDoneWaiting )
   {
      gUIManager->setMinimapVisible(true);
      return;
   }

   gUIManager->getCurrentUser()->clearAllSelections();

   BString civStr;
   BUString message;
   long result = -1;

   // Campaign
   if( mGameType != BGameSettings::cGameTypeSkirmish )
   {
      result = gUIManager->getScenarioResult();

      switch( result )
      {
      case BPlayer::cPlayerStateWon:
         message = gDatabase.getLocStringFromID( 24829 );
         break;

      case BPlayer::cPlayerStateDefeated:
         message = gDatabase.getLocStringFromID( 24830 );
         break;

      case BPlayer::cPlayerStateResigned:
         message = gDatabase.getLocStringFromID( 24831 );
         break;
      }
      
      civStr = gUIManager->getCurrentUser()->getPlayer()->getCiv()->getCivName();
   }
   // Skirmish
   else
   {
      civStr = gUserManager.getPrimaryUser()->getPlayer()->getCiv()->getCivName();
      result = gUserManager.getPrimaryUser()->getPlayer()->getPlayerState();

      if( civStr == "UNSC" )
      {
         switch( result )
         {
         case BPlayer::cPlayerStateWon:
            message = gDatabase.getLocStringFromID( 24832 );
            break;

         case BPlayer::cPlayerStateDefeated:
            message = gDatabase.getLocStringFromID( 24833 );
            break;

         case BPlayer::cPlayerStateResigned:
            message = gDatabase.getLocStringFromID( 24834 );
            break;
         }
      }
      else if( civStr == "Covenant" )
      {
         switch( result )
         {
         case BPlayer::cPlayerStateWon:
            message = gDatabase.getLocStringFromID( 24835 );
            break;

         case BPlayer::cPlayerStateDefeated:
            message = gDatabase.getLocStringFromID( 24836 );
            break;

         case BPlayer::cPlayerStateResigned:
            message = gDatabase.getLocStringFromID( 24837 );
            break;
         }
      }
   }

   if( gUIManager->getWidgetUI() )
      gUIManager->getWidgetUI()->getTalkingHead().show();
   
   setResult( civStr, result, message );
}

//============================================================================
//============================================================================
void BUIEndGame::update( float dt )
{
   // Don't fade out in non-Skirmish games
   if( mGameType == BGameSettings::cGameTypeSkirmish )
   {
      if( !bDoneWaiting )
      {
         mAutoDelay -= dt;
         if( mAutoDelay <= 0.0f )
            fadeOut();
      }
   }
   else // Campaign/Scenario
   {
      // This screen is non-interactive in Campaign games, so it's possible to "hang" here.
      // This code is a sanity check for the non-interactive state (on this screen, game's over)
      // and it just dumps to the stats in that case.
      if( gUIManager->getCurrentUser() && gUIManager->getCurrentUser()->getPlayer() &&
          gUIManager->getCurrentUser()->getPlayerState() != BPlayer::cPlayerStatePlaying &&
          mpHandler )
         mpHandler->handleUIScreenResult( this, eResult_Stats );
   }
}

//============================================================================
//============================================================================
void BUIEndGame::leave( void )
{
   gUIManager->setMinimapVisible(false);

   if( gUIManager->getWidgetUI() )
      gUIManager->getWidgetUI()->getTalkingHead().hide();
}

//============================================================================
//============================================================================
void BUIEndGame::yornResult( uint result, DWORD userContext, int port )
{
   if( result == BUIGlobals::cDialogResultOK )
   {
      mpHandler->handleUIScreenResult( this, userContext );
   }
}

//============================================================================
//============================================================================
void BUIEndGame::fadeOut( void )
{
   invokeActionScript( "fadeOut" );
   bDoneWaiting = true;
   gUIManager->setMinimapVisible( true );
}
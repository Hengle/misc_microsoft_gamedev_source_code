//============================================================================
// UIGameMenu.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UIControllerScreen.h"
#include "usermanager.h"
#include "user.h"
#include "player.h"
#include "database.h"
#include "game.h"
#include "modegame.h"
#include "world.h"
#include "uimanager.h"
#include "configsgame.h"
#include "gamesettings.h"
#include "campaignmanager.h"
#include "humanPlayerAITrackingData.h"

//============================================================================
//============================================================================
#define LOC( id ) gDatabase.getLocStringFromID( id )

//============================================================================
//============================================================================
BUIControllerScreen::BUIControllerScreen( void ) :
   mSeconds( -1 )
{
}

//============================================================================
//============================================================================
BUIControllerScreen::~BUIControllerScreen( void )
{
}

//============================================================================
//============================================================================
bool BUIControllerScreen::init( BXMLNode root )
{
   __super::init( root );

   mTitle.init( this, "mTitle" );
   mDifficulty.init( this, "mDifficulty" );
   mGameTime.init( this, "mGameTime" );

   BGameSettings* pSettings = gDatabase.getGameSettings();
   if( pSettings )
   {
      long gameType = -1;
      pSettings->getLong( BGameSettings::cGameType, gameType );
      
      BSimString mapName;
      pSettings->getString( BGameSettings::cMapName, mapName );
      
      if ( (gameType == BGameSettings::cGameTypeSkirmish ) ||
           (gameType == BGameSettings::cGameTypeScenario ) )
      {
         const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapName.getPtr());
         if (pMap!=NULL)
         {
            mTitle.setText( pMap->getMapName() );
         }
      }
      else
      {
         BCampaign* pCampaign = gCampaignManager.getCampaign(0);
         if (pCampaign)
         {
            BCampaignNode* pNode = pCampaign->getNode(mapName.getPtr());
            if (pNode)
            {
               mTitle.setText( gDatabase.getLocStringFromID(pNode->getDisplayNameStringID()) );
            }
         }
      }
   }

   mLeftButton.init(this, "mControllerLabels.mLeftButton");
   mRightButton.init(this, "mControllerLabels.mRightButton");
   mLeftStick.init(this, "mControllerLabels.mLeftStick");
   mRightStick.init(this, "mControllerLabels.mRightStick");

   mLeftTrigger.init(this, "mControllerLabels.mLeftTrigger");
   mRightTrigger.init(this, "mControllerLabels.mRightTrigger");

   mBackButton.init(this, "mControllerLabels.mBackButton");
   mStartButton.init(this, "mControllerLabels.mStartButton");

   mDPadUp.init(this, "mControllerLabels.mDPadUp");
   mDPadDown.init(this, "mControllerLabels..mDPadDown");
   mDPadLeft.init(this, "mControllerLabels..mDPadLeft");
   mDPadRight.init(this, "mControllerLabels..mDPadRight");

   mAButton.init(this, "mControllerLabels.mAButton");
   mBButton.init(this, "mControllerLabels.mBButton");
   mXButton.init(this, "mControllerLabels.mXButton");
   mYButton.init(this, "mControllerLabels.mYButton");


   mLeftButton.setText( LOC(25356) );
   mRightButton.setText( LOC(25357) );

   mLeftStick.setText( LOC(25359) );
   mRightStick.setText( LOC(25729) );

   mBackButton.setText( LOC(25730) );
   mStartButton.setText( LOC(25731) );
   mDPadUp.setText( LOC(25732) );
   mDPadDown.setText( LOC(25733) );
   mDPadLeft.setText( LOC(25734) );
   mDPadRight.setText( LOC(25735) );

   mLeftTrigger.setText( LOC(25736) );
   mRightTrigger.setText( LOC(25737) );



   mAButton.setText( LOC(25363) );
   mBButton.setText( LOC(25361) );
   mXButton.setText( LOC(25362) );
   mYButton.setText( LOC(25360) );

   mButtonBar.init( this, "mButtonBar" );
   mButtonBar.setButtonStates( BUIButtonBarControl::cFlashButtonB );
   mButtonBar.setButtonTexts( gDatabase.getLocStringFromID(24065) );

   return true;
}

//============================================================================
//============================================================================
void BUIControllerScreen::deinit( void )
{
   __super::deinit();
}

//============================================================================
//============================================================================
bool BUIControllerScreen::handleUIControlEvent( BUIControlEvent& event )
{
   return true;
}

//============================================================================
//============================================================================
bool BUIControllerScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( command == "cancel" )
   {
      gUI.playCancelSound();
      mpHandler->handleUIScreenResult( this, 0 );
      return true;
   }
   return false;
}

//============================================================================
//============================================================================
bool BUIControllerScreen::handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail )
{
   bool bResult = false;
   
   bResult = __super::handleInput( port, event, controlType, detail );

   // Return true so that the game doesn't get any input
   return true;
}

//============================================================================
//============================================================================
void BUIControllerScreen::enter( void )
{
   __super::enter();

   // populateControllerLabels();
   
   populateDifficulty();

   updateGameTime(true);
}

//============================================================================
//============================================================================
void BUIControllerScreen::update( float dt )
{
   updateGameTime();
}

//============================================================================
//============================================================================
void BUIControllerScreen::render( void )
{
   __super::render();
}

//============================================================================
//============================================================================
void BUIControllerScreen::leave( void )
{
   __super::leave();
}
//============================================================================
//============================================================================
bool BUIControllerScreen::getVisible( void )
{
   return __super::getVisible();
}


//============================================================================
//============================================================================
void BUIControllerScreen::populateDifficulty()
{
   BPlayer* pPlayer = gWorld->getPlayer(1);        // any way to make this safer?
   if (pPlayer && pPlayer->isHuman())
   {
      BDifficultyType playerDifficulty = pPlayer->getDifficultyType();
      mDifficulty.update(playerDifficulty);

      BGameSettings* pSettings = gDatabase.getGameSettings();
      if( pSettings )
      {
         long diffType = -1;
         pSettings->getLong( BGameSettings::cPlayer1DifficultyType, diffType );

         if( diffType == DifficultyType::cAutomatic )
         {
            BHumanPlayerAITrackingData trackingData;

            int autoDiffLevel = 0;
            BUString autoText;
            autoText.empty();

            if (trackingData.loadValuesFromMemoryBlock(gUserManager.getPrimaryUser()->getProfile()->getAITrackingDataMemoryPointer()))
            {
               autoDiffLevel = trackingData.getAutoDifficultyLevel();
               autoText.locFormat(gDatabase.getLocStringFromID(25976).getPtr(), gDatabase.getDifficultyStringByType(DifficultyType::cAutomatic).getPtr(), autoDiffLevel);
            }
            else
            {
               autoText.set(gDatabase.getDifficultyStringByType(DifficultyType::cAutomatic));
            }
            mDifficulty.setText( autoText );
         }
      }
   }
}


//============================================================================
//============================================================================
void BUIControllerScreen::updateGameTime( bool force )
{
   DWORD gameTime=gWorld->getGametime();
   long mins = gameTime / 60000;
   long secs = (gameTime - (mins*60000)) / 1000;

   BUString timeStr;
   formatTime(gameTime, timeStr);

   if( force || secs != mSeconds )
   {
      mSeconds = secs;
      mGameTime.setText( timeStr );
   }
}

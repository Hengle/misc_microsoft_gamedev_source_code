//============================================================================
// UIGameMenu.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UIGameMenu.h"
#include "usermanager.h"
#include "user.h"
#include "player.h"
#include "database.h"
#include "game.h"
#include "modegame.h"
#include "LiveSystem.h"
#include "modemanager.h"
#include "world.h"
#include "UIOptionsMenu.h"
#include "scoremanager.h"
#include "gamesettings.h"
#include "recordgame.h"
#include "UISkullPicker.h"
#include "uimanager.h"
#include "campaignmanager.h"
#include "modemenu.h"
#include "configsgame.h"
#include "UIControllerScreen.h"
#include "humanPlayerAITrackingData.h"

//============================================================================
//============================================================================
#define LOC( id ) gDatabase.getLocStringFromID( id )

//============================================================================
//============================================================================
BUIGameMenu::BUIGameMenu( void ) :
   mpUser( NULL ),
   mpOptionsMenu( new BUIOptionsMenu() ),
   mpSkullPicker( new BUISkullPicker() ),
   mpControllerScreen(new BUIControllerScreen() ),
   mSeconds( -1 ),
   mbPaused( false ),
   mbSelectingDevice( false )
{
   mItems[0] = &mItem0;
   mItems[1] = &mItem1;
   mItems[2] = &mItem2;
   mItems[3] = &mItem3;
   mItems[4] = &mItem4;
   mItems[5] = &mItem5;
   mItems[6] = &mItem6;

   BASSERT( mpOptionsMenu );
   BASSERT( mpSkullPicker );
   BASSERT( mpControllerScreen );
}

//============================================================================
//============================================================================
BUIGameMenu::~BUIGameMenu( void )
{
   delete mpOptionsMenu;
   mpOptionsMenu = NULL;

   delete mpSkullPicker;
   mpSkullPicker = NULL;

   delete mpControllerScreen;
   mpControllerScreen=NULL;
}

//============================================================================
//============================================================================
bool BUIGameMenu::init( BXMLNode root )
{
   __super::init( root );

   mPlayerListLabel.init( this, "mPlayerListLabel" );
   mPlayerListLabel.setText( LOC(25364) );

/*
   mPrimaryGamerTag.init( this, "mGamerTagPrimary" );
   mSecondaryGamerTag.init( this, "mGamerTagSecondary" );
*/

   mTitleLabel.init( this, "mTitle" );
   mDifficulty.init( this, "mDifficulty" );
   mGameTime.init( this, "mGameTime" );

   // mDifficultyIcon.init( this, "mDifficultyIcon" );
   
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
            mTitleLabel.setText( pMap->getMapName() );
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
               mTitleLabel.setText( gDatabase.getLocStringFromID(pNode->getDisplayNameStringID()) );
            }
         }
      }
   }

   mLBLabel.init( this, "mControllerLabels.mLBLabel" );
   mLBLabel.setText( LOC(25356) );
   mRBLabel.init( this, "mControllerLabels.mRBLabel" );
   mRBLabel.setText( LOC(25357) );
   mLSLabel.init( this, "mControllerLabels.mLSLabel" );
   mLSLabel.setText( LOC(25359) );
   mALabel.init( this, "mControllerLabels.mALabel" );
   mALabel.setText( LOC(25363) );
   mBLabel.init( this, "mControllerLabels.mBLabel" );
   mBLabel.setText( LOC(25361) );
   mXLabel.init( this, "mControllerLabels.mXLabel" );
   mXLabel.setText( LOC(25362) );
   mYLabel.init( this, "mControllerLabels.mYLabel" );
   mYLabel.setText( LOC(25360) );

   mPausedLabel.init( this, "mPausedLabel" );

   mMenu.init( this, "" );
   mMenu.setWrap(true);
   mItem0.init( this, "mMenuItem0" );
   mItem1.init( this, "mMenuItem1" );
   mItem2.init( this, "mMenuItem2" );
   mItem3.init( this, "mMenuItem3" );
   mItem4.init( this, "mMenuItem4" );
   mItem5.init( this, "mMenuItem5" );
   mItem6.init( this, "mMenuItem6" );
  
   mButtonBar.init( this, "mButtonBar" );
   mButtonBar.setButtonStates( BUIButtonBarControl::cFlashButtonA, BUIButtonBarControl::cFlashButtonB );
   mButtonBar.setButtonTexts( gDatabase.getLocStringFromID(24064), gDatabase.getLocStringFromID(24065) );

   mPauseItemIndex = -1;

   mbPaused = false;
   mbSelectingDevice = false;

   mpOptionsMenu->initScreen( "art\\ui\\flash\\pregame\\optionsMenu\\optionsMenu.gfx", cFlashAssetCategoryInGame, "art\\ui\\flash\\pregame\\optionsMenu\\optionsMenuData.xml" );
   mpOptionsMenu->setHandler( this );

   mpSkullPicker->initScreen( "art\\ui\\flash\\hud\\hud_skullpicker\\hud_skullpicker.gfx", cFlashAssetCategoryInGame, "art\\ui\\flash\\hud\\hud_skullpicker\\hud_skullpicker.xml" );
   mpSkullPicker->setHandler( this );

   mpControllerScreen->initScreen( "art\\ui\\flash\\hud\\controller\\UIControllerScreen.gfx", cFlashAssetCategoryInGame, "art\\ui\\flash\\hud\\controller\\UIControllerScreen.xml" );
   mpControllerScreen->setHandler( this );
   
   return true;
}

//============================================================================
//============================================================================
void BUIGameMenu::deinit( void )
{
   mpOptionsMenu->deinit();
   mpSkullPicker->deinit();
   mpControllerScreen->deinit();
   __super::deinit();
}

//============================================================================
//============================================================================
bool BUIGameMenu::handleUIControlEvent( BUIControlEvent& event )
{
   BASSERT( mpUser );

   if( event.getID() != BUIButtonControl::ePress )
      return false;

   BUIButtonControl* pButton = (BUIButtonControl*)event.getControl();

   switch( event.getControl()->getControlID() )
   {
      case ePause:
         gModeManager.getModeGame()->setPaused(!gModeManager.getModeGame()->getPaused(), true, mpUser->getPlayerID());
         break;

      case eResume:
         mpHandler->handleUIScreenResult( this, eResult_Resume );
         break;

      case eSave:
      {
         //gGame.getUIGlobals()->showWaitDialog(BUString());
         XCONTENTDEVICEID deviceID = mpUser->getDefaultDevice();
         if (deviceID != XCONTENTDEVICE_ANY)
         {
            XDEVICE_DATA deviceData;
            ZeroMemory(&deviceData, sizeof(XDEVICE_DATA));
            if (XContentGetDeviceData(deviceID, &deviceData) != ERROR_SUCCESS)
            {
               mpUser->setDefaultDevice(XCONTENTDEVICE_ANY);
               deviceID = XCONTENTDEVICE_ANY;
            }
         }
         if( deviceID == XCONTENTDEVICE_ANY )
         {
            mbSelectingDevice = true;
            gUserManager.showDeviceSelector( mpUser, this, eSave, 0, true );
         }
         else
         {
            // Warn if existing save will be overwritten.
            if (gSaveGame.getCampaignSaveExists())
               gGame.getUIGlobals()->showYornBox(this, gDatabase.getLocStringFromID(25724), BUIGlobals::cDialogButtonsOKCancel, eSave);
            else
            {
               // ajl 11/19/08 - Show the device selector if there isn't enough space on the storage device.
               XDEVICE_DATA deviceData;
               if (XContentGetDeviceData(deviceID, &deviceData) == ERROR_SUCCESS && deviceData.ulDeviceFreeBytes < BSaveGame::MAX_SIZE_IN_BYTES)
               {
                  mbSelectingDevice = true;
                  gUserManager.showDeviceSelector( mpUser, this, eSave, 0, true );
               }
               else
                  gSaveGame.saveGame("campaign");
               //mpHandler->handleUIScreenResult( this, eResult_Resume );
            }
         }
         break;
      }

      case eLoad:
         gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(25441), BUIGlobals::cDialogButtonsOKCancel, eLoad );
         break;

      case eResign:
         gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(24050), BUIGlobals::cDialogButtonsOKCancel, eResign );
         break;

      case eRestart:
         gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(24049), BUIGlobals::cDialogButtonsOKCancel, eRestart );
         break;

      case eOptions:
         mpOptionsMenu->setUser( mpUser );
         mpOptionsMenu->setVisible( true );
         mpOptionsMenu->enter();
         break;

      case eControllerScreen:
         mpControllerScreen->setVisible(true);
         mpControllerScreen->enter();
         break;

      case eSkullPicker:
         mpSkullPicker->setVisible( true );
         mpSkullPicker->enter();
         break;

      case eButtonHelp:
         {
            mpUser->setOption_ShowButtonHelp( !mpUser->getOption_ShowButtonHelp() );

            BUString BHString = L"BUTTON HELP\t(";
            BHString += mpUser->getOption_ShowButtonHelp() ? L"ON)" : L"OFF)";
            pButton->setText( BHString );
         }
         break;

      case eResourceBoost:
         mpUser->setOption_ResourceBoost( true );
         break;

      case eFogOfWar:
         {
            mpUser->setOption_FogOfWarEnabled( !mpUser->getOption_FogOfWarEnabled() );
            
            BUString FOWString = L"FOG OF WAR\t\t(";
            FOWString += mpUser->getOption_FogOfWarEnabled() ? L"ON)" : L"OFF)";
            pButton->setText( FOWString );
         }
         break;

      case eUnlockView:
         gRecordGame.toggleViewLock();
         mpHandler->handleUIScreenResult( this, eResult_Resume );
         break;

      case eSaveAndQuit:
         gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(25449), BUIGlobals::cDialogButtonsOKCancel, eSaveAndQuit );
         break;

      case eDevSave:
         gSaveGame.saveGame("");
         mpHandler->handleUIScreenResult( this, eResult_Resume );
         break;

      case eDevLoad:
         gSaveGame.loadGame("");
         mpHandler->handleUIScreenResult( this, eResult_Resume );
         break;

      default:
         return false;
   }
   return true;
}

//============================================================================
//============================================================================
bool BUIGameMenu::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (mbSelectingDevice)
      return false;

   if( command == "cancel" )
   {
      mpHandler->handleUIScreenResult( this, eResult_Resume );
      return true;
   }
   return false;
}

//============================================================================
//============================================================================
bool BUIGameMenu::handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail )
{
   if (mbSelectingDevice)
      return false;

   if( mpOptionsMenu->getVisible() )
      return mpOptionsMenu->handleInput( port, event, controlType, detail );
   else if (mpSkullPicker->getVisible())
      return mpSkullPicker->handleInput( port, event, controlType, detail );
   else if (mpControllerScreen->getVisible())
      return mpControllerScreen->handleInput(port, event, controlType, detail);

   bool bResult = false;
   
   if( !bResult )
   {
      BUIControl* pSelectedItem = mMenu.getSelectedControl();
      if( pSelectedItem )
      {
         bResult = pSelectedItem->handleInput( port, event, controlType, detail );
      }
   }

   if( !bResult )
   {
      bResult = mMenu.handleInput( port, event, controlType, detail );
   }
   
   if( !bResult )
   {
      bResult = __super::handleInput( port, event, controlType, detail );
   }

   return true;
}

//============================================================================
//============================================================================
void BUIGameMenu::enter( void )
{
   __super::enter();

   mpUser = gUIManager->getCurrentUser();

   
   if (!gLiveSystem->isMultiplayerGameActive())
   {
      gModeManager.getModeGame()->setPaused(true);
      mPausedLabel.setText( gDatabase.getLocStringFromID(24691) );
   }
   
   populateMenu();
   populateDifficulty();
   updateGameTime(true);
}

//============================================================================
//============================================================================
void BUIGameMenu::refreshScreen()
{
   // make sure we update if something might have changed for us.
   populateMenu();  
}

//============================================================================
//============================================================================
void BUIGameMenu::update( float dt )
{
   bool bGamePaused = gModeManager.getModeGame()->getPaused();
   
   if( mpOptionsMenu->getVisible() )   
      mpOptionsMenu->update( dt );
   else if( mpSkullPicker->getVisible() )
      mpSkullPicker->update( dt );
   else if (mpControllerScreen->getVisible())
      mpControllerScreen->update(dt);
   else 
   {
      updateGameTime(); // always do this.

      // only do this when things change.
      if( mPauseItemIndex >= 0 && mbPaused != bGamePaused ) // If the paused state has changed
      {

         // Update the pause item
         mItems[mPauseItemIndex]->setText( bGamePaused ? gDatabase.getLocStringFromID(24692) : gDatabase.getLocStringFromID(25037) );
         mbPaused = bGamePaused;
      }
   }
}

//============================================================================
//============================================================================
void BUIGameMenu::render( void )
{
   if( mpOptionsMenu->getVisible() )
      mpOptionsMenu->render();
   else if (mpSkullPicker->getVisible())
      mpSkullPicker->render();
   else if (mpControllerScreen->getVisible())
      mpControllerScreen->render();
   else
   {
      __super::render();
   }
}

//============================================================================
//============================================================================
void BUIGameMenu::leave( void )
{
   if (!gLiveSystem->isMultiplayerGameActive())
      gModeManager.getModeGame()->setPaused(false);

   __super::leave();
}
//============================================================================
//============================================================================
bool BUIGameMenu::getVisible( void )
{
   return __super::getVisible() || ( mpOptionsMenu && mpOptionsMenu->getVisible() || 
                                   ( mpSkullPicker && mpSkullPicker->getVisible() ||
                                   ( mpControllerScreen && mpControllerScreen->getVisible() )) );
}

//============================================================================
//============================================================================
void BUIGameMenu::yornResult(uint result, DWORD userContext, int port)
{
   if( getVisible() && result == BUIGlobals::cDialogResultOK )
   {
      switch( userContext )
      {
         case eResign:
            mpHandler->handleUIScreenResult( this, eResult_Resign );
            break;

         case eRestart:
            mpHandler->handleUIScreenResult( this, eResult_Restart );
            break;

         case eLoad:
            {
               if (gSaveGame.getCampaignSaveExists(NULL))
               {
                  BGameSettings gameSettings;
                  mSaveGameNode = -1;
                  if (gSaveGame.getCampaignSaveExists(&gameSettings))
                  {
                     BCampaign * pCampaign = gCampaignManager.getCampaign(0);
                     BASSERT(pCampaign);
                     if (pCampaign)
                     {
                        BString mapName;
                        gameSettings.getString(BGameSettings::cMapName, mapName);

                        BCampaignNode* pNode;
                        if (mapName == "NewCampaign")
                        {
                           long nodeID = 0;
                           pNode = pCampaign->getNode(nodeID);
                           while (pNode && pNode->getFlag(BCampaignNode::cCinematic))
                           {
                              nodeID++;
                              pNode = pCampaign->getNode(nodeID);
                           }
                        }
                        else
                           pNode = pCampaign->getNode(mapName);

                        if (pNode)
                           mSaveGameNode=pNode->getID();
                     }
                     if (mSaveGameNode != -1)
                        playCampaign(mSaveGameNode, true);
                  }
               }
            }
            break;

         case eSave:
            gSaveGame.saveGame( "campaign", false );
            break;

         case eSaveAndQuit:
            if( mpUser->getDefaultDevice() == XCONTENTDEVICE_ANY )
            {
               mbSelectingDevice = true;
               gUserManager.showDeviceSelector( mpUser, this, eSaveAndQuit, 0, true );
            }
            else
            {
               gSaveGame.saveGame("campaign", true);
               //mpHandler->handleUIScreenResult( this, eResult_Resume );
            }
            break;
      }
   }
}

//============================================================================
//============================================================================
void BUIGameMenu::handleUIScreenResult( BUIScreen* pScreen, long result )
{
   if( pScreen == mpOptionsMenu )
   {
      mpOptionsMenu->leave();
      mpOptionsMenu->setVisible( false );
   }
   else if (pScreen == mpSkullPicker)
   {
      mpSkullPicker->leave();
      mpSkullPicker->setVisible( false );
   }
   else if (pScreen == mpControllerScreen)
   {
      mpControllerScreen->leave();
      mpControllerScreen->setVisible(false);
   }
   
}

//============================================================================
//============================================================================
void BUIGameMenu::notify( DWORD eventID, void* pTask )
{
   if( !pTask )
      return;

   BSelectDeviceAsyncTask* pSelectDeviceTask = reinterpret_cast<BSelectDeviceAsyncTask*>( pTask );
   
   XCONTENTDEVICEID deviceID = pSelectDeviceTask->getDeviceID();

   mbSelectingDevice = false;

   if( deviceID == XCONTENTDEVICE_ANY )
   {
      // No device selected
      // Show a dialog or something
      //gGame.getUIGlobals()->setWaitDialogVisible(false);
      return;
   }

   // Warn if existing save will be overwritten.
   if (gSaveGame.getCampaignSaveExists())
   {
      gGame.getUIGlobals()->showYornBox(this, gDatabase.getLocStringFromID(25724), BUIGlobals::cDialogButtonsOKCancel, eSave);
      return;
   }

   // ajl 12/04/08 - Verify there is enough space available to save the game.
   XDEVICE_DATA deviceData;
   if (XContentGetDeviceData(deviceID, &deviceData) == ERROR_SUCCESS && deviceData.ulDeviceFreeBytes < BSaveGame::MAX_SIZE_IN_BYTES)
   {
      gGame.getUIGlobals()->showYornBox(this, gDatabase.getLocStringFromID(26002), BUIGlobals::cDialogButtonsOK);
      return;
   }

   // Save the game to the selected device
   gSaveGame.saveGame( "campaign", eventID == eSaveAndQuit );
   //mpHandler->handleUIScreenResult( this, eResult_Resume );
}

//============================================================================
//============================================================================
void BUIGameMenu::addMenuItem( int index, int id, const BUString& text)
{
   if (index >= NUM_ITEMS)
      return;
   mItems[index]->show();
   mItems[index]->setControlID( id );
   mItems[index]->setText( text );
   mItems[index]->unfocus();
   mMenu.addControl( mItems[index] );
}

//============================================================================
//============================================================================
void BUIGameMenu::populateDifficulty()
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
void BUIGameMenu::populateMenu( void )
{
   disableSound();

   populateDifficulty();

   bool inMatchmaking = false;
   if (gLiveSystem && gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->isMatchmadeGame())
      inMatchmaking=true;

   // MENU
   mMenu.clearControls();
   int itemIndex = 0;

   // PAUSE / UNPAUSE / RESUME
   if (gLiveSystem->isMultiplayerGameActive() && !inMatchmaking)
   {
      long numPlayers = gWorld->getNumberPlayers();
      long numHumans = 0;

      for (long player = 1; player < numPlayers; player++)
      {
//-- FIXING PREFIX BUG ID 1535
         const BPlayer* pPlayer = gWorld->getPlayer(player);
//--
         if (pPlayer && pPlayer->isHuman())
            numHumans++;
      }

      if (numHumans > 1)
      {
         // PAUSE / UNPAUSE
         mPauseItemIndex = itemIndex++;
         addMenuItem( mPauseItemIndex, ePause, (gModeManager.getModeGame()->getPaused() ? gDatabase.getLocStringFromID(24692) : gDatabase.getLocStringFromID(25037)));
      }
      else
      {
         // RESUME
         addMenuItem( itemIndex++, eResume, gDatabase.getLocStringFromID(25041));
      }
   }
   else
   {
      // RESUME
      addMenuItem( itemIndex++, eResume, gDatabase.getLocStringFromID(25041) );
   }

   // UNLOCK/LOCK VIEW
   if (gRecordGame.isPlaying())
   {
      if (gRecordGame.isViewLocked())
         addMenuItem( itemIndex++, eUnlockView, L"UNLOCK VIEW" );
      else
         addMenuItem( itemIndex++, eUnlockView, L"LOCK VIEW" );
   }

   if( gConfig.isDefined( "CampaignRestartOption" ) )
   {
      long gameType = -1;
      gDatabase.getGameSettings()->getLong(BGameSettings::cGameType, gameType);

      // RESTART
      if( gameType != BGameSettings::cGameTypeSkirmish )
      {
         addMenuItem( itemIndex++, eRestart, gDatabase.getLocStringFromID(25038));
      }
   }

   // CAMPAIGN SAVE AND LOAD
   BGameSettings* pSettings = gDatabase.getGameSettings();
   long gameType = -1;
   pSettings->getLong(BGameSettings::cGameType, gameType);

#ifndef BUILD_FINAL
   if (gConfig.isDefined("DevSave"))
   {
      addMenuItem( itemIndex++, eDevSave, "SAVE (DEV)");
      if (!gLiveSystem->isMultiplayerGameActive())
         addMenuItem( itemIndex++, eDevLoad, "LOAD (DEV)");
   }
   else
#endif
   {
      if (gameType == BGameSettings::cGameTypeCampaign && !gLiveSystem->isMultiplayerGameActive() && !gConfig.isDefined(cConfigDemo) && !gConfig.isDefined(cConfigDemo2))
      {
         if (gUserManager.getPrimaryUser()->isSignedIn())
         {
            // SAVE
            addMenuItem( itemIndex++, eSave, gDatabase.getLocStringFromID(25040));

            // LOAD
            if (gSaveGame.getSavedCampaign())
            {
               BUString name;
               if (gSaveGame.getCampaignSaveExists(NULL, &name))
               {
                  BUString menuItemText;
                  menuItemText.locFormat(L"%s: %s", gDatabase.getLocStringFromID(25039).getPtr(), name.getPtr());
                  addMenuItem( itemIndex++, (gSaveGame.getSaveFileCorrupt() ? eLoadCorrupt : eLoad), menuItemText );
               }
            }
         }
      }
   }

   // OPTIONS
   addMenuItem( itemIndex++, eOptions, gDatabase.getLocStringFromID(24935));

   addMenuItem( itemIndex++, eControllerScreen, gDatabase.getLocStringFromID(25699));

   // SKULL PICKER
   if (!gRecordGame.isPlaying() && !inMatchmaking)
      addMenuItem( itemIndex++, eSkullPicker, gDatabase.getLocStringFromID(24988));

   // RESIGN
   addMenuItem( itemIndex++, eResign, gDatabase.getLocStringFromID(25036));
   
/*
   if (gameType == BGameSettings::cGameTypeCampaign && !gLiveSystem->isMultiplayerGameActive() && gUserManager.getPrimaryUser()->isSignedIn())
   {
      // SAVE AND QUIT
      addMenuItem( itemIndex++, eSaveAndQuit, gDatabase.getLocStringFromID(25451));
   }
*/

   for( ; itemIndex < NUM_ITEMS; ++itemIndex )
   {
      mItems[itemIndex]->setText(L"");
      mItems[itemIndex]->hide();
   }

   mMenu.setIndex( 0 );

   enableSound();
}

//============================================================================
//============================================================================
/*
void BUIGameMenu::populatePlayer( BPlayer* player, BUIGamerTagControl& playerControl )
{
   if (!player)
      return;

   BUser* user=gUserManager.getUserByPlayerID(player->getID());

   BString temp;
   BUString tempU;

   // Basic player info
   playerControl.setGamerTag(player->getName());
   temp.format("img://gamerPic:%I64x", player->getXUID());
   playerControl.setGamerPic(temp);

   int controllerPort=-1;
   if (user)
      controllerPort=user->getPort();
   playerControl.setPort(controllerPort);

   // fixme - we may want to update this on an update call.
   tempU.locFormat(gDatabase.getLocStringFromID(25325).getPtr(), gScoreManager.getBaseScore(player->getID() ) );
   playerControl.setTitleText(tempU);

   playerControl.setXuid(player->getXUID());

}
*/

//============================================================================
//============================================================================
void BUIGameMenu::updateGameTime( bool force )
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

//============================================================================
//============================================================================
void BUIGameMenu::playCampaign(int startingNode /* = -1*/, bool useSaveIfAvailable /* = -1 */)
{
   BUser* pUser=gUserManager.getPrimaryUser();
   BUserProfile* pProfile = pUser->getProfile();

   // error checking
   if (!pUser || !pProfile)
      return;

   // Start the Campaign in Single Player Mode
   // determine the last campaign mode the player was in
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (!pCampaign)
      return;

   pCampaign->setPlayContinuous(true);

   if( startingNode != -1 )
      pCampaign->setCurrentNodeID(startingNode);

   if( pCampaign->getCurrentNodeID() == -1 )
      pCampaign->setCurrentNodeID(0);

   gSaveGame.loadGame("campaign", true);
}

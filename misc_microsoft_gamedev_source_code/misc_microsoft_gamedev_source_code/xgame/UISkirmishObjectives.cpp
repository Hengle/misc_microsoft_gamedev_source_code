//============================================================================
// UISkirmishObjectives.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UISkirmishObjectives.h"
#include "UIObjectiveControl.h"

#include "database.h"
#include "player.h"
#include "usermanager.h"
#include "user.h"
#include "objectivemanager.h"
#include "world.h"
#include "flashminimap.h"
#include "uiwatermark.h"
#include "uimanager.h"
#include "render.h"
#include "econfigenum.h"
#include "team.h"
#include "LiveSystem.h"
#include "modemanager.h"
#include "modegame.h"
#include "uimanager.h"
#include "scoremanager.h"
#include "gamesettings.h"
#include "userprofilemanager.h"
#include "humanPlayerAITrackingData.h"

//============================================================================
//============================================================================
BUISkirmishObjectives::BUISkirmishObjectives( void ) :
mLastGameTimeUpdate(0)
{
}

//============================================================================
//============================================================================
BUISkirmishObjectives::~BUISkirmishObjectives( void )
{
   reset();
}

//============================================================================
//============================================================================
bool BUISkirmishObjectives::reset()
{
/*
   BUIObjectiveControl* c = NULL;

   // primary
   for (int i=0; i<mObjectiveItems.getNumber(); i++)
   {
      c = mObjectiveItems[i];
      delete c;
      mObjectiveItems[i]=NULL;
   }
   mObjectiveItems.clear();
*/

   return true;
}

//============================================================================
//============================================================================
bool BUISkirmishObjectives::init( const char* filename, const char* datafile )
{
   return BUIScreen::init(filename, datafile);
}


//============================================================================
//============================================================================
bool BUISkirmishObjectives::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   // grab the location of the minimap for 4x3 and 16x9
   int numChildren = dataNode.getNumberChildren();
   for (int i = 0; i < numChildren; ++i)
   {
      BXMLNode node = dataNode.getChild(i);
      if (node.getName().compare("MinimapLocation") == 0)
      {
         BSimString value;
         if (!node.getAttribValueAsString("type", value))
            continue;

         int index = -1;
         if (value=="16x9")
            index = cMinimapLocation16x9;
         else if (value=="4x3")
            index = cMinimapLocation4x3;

         if (index<0)
            continue;

         for (int j=0; j<node.getNumberChildren(); j++)
         {
            BXMLNode child(node.getChild(j));
            const BPackedString name(child.getName());

            if(name==B("Minimap"))                                    // Controls
            {               
               float mapCenterXScalar = 1.0f;
               float mapCenterYScalar = 1.0f;
               float mapWidthScalar   = 1.0f;
               float mapHeightScalar  = 1.0f;

               child.getAttribValueAsFloat("x", mapCenterXScalar);
               child.getAttribValueAsFloat("y", mapCenterYScalar);
               child.getAttribValueAsFloat("w", mapWidthScalar);
               child.getAttribValueAsFloat("h", mapHeightScalar);

               mMinimapLocation[index].mMapCenterX = mapCenterXScalar * BD3D::mD3DPP.BackBufferWidth;
               mMinimapLocation[index].mMapCenterY = mapCenterYScalar * BD3D::mD3DPP.BackBufferHeight;
               mMinimapLocation[index].mMapW = mapWidthScalar * BD3D::mD3DPP.BackBufferWidth;
               mMinimapLocation[index].mMapH = mapHeightScalar * BD3D::mD3DPP.BackBufferHeight;


            }
            else if (name==B("MinimapFlash"))
            {
               float mapFlashXScalar = 1.0f;
               float mapFlashYScalar = 1.0f;
               float mapFlashWidthScalar = 1.0f;
               float mapFlashHeightScalar = 1.0f;

               child.getAttribValueAsFloat("x", mapFlashXScalar);
               child.getAttribValueAsFloat("y", mapFlashYScalar);
               child.getAttribValueAsFloat("w", mapFlashWidthScalar);
               child.getAttribValueAsFloat("h", mapFlashHeightScalar);

               mMinimapLocation[index].mMapFlashX = mapFlashXScalar * BD3D::mD3DPP.BackBufferWidth;
               mMinimapLocation[index].mMapFlashY = mapFlashYScalar * BD3D::mD3DPP.BackBufferHeight;
               mMinimapLocation[index].mMapFlashW = mapFlashWidthScalar * BD3D::mD3DPP.BackBufferWidth;
               mMinimapLocation[index].mMapFlashH = mapFlashHeightScalar * BD3D::mD3DPP.BackBufferHeight;               
            }
         }
      }
   }

   mTitle.init(this, "mTitle");

   mDescription.init(this, "mDescriptionLabel");
   mDescription.setText( gDatabase.getLocStringFromID(25677));

   mTeam1.init(this, "mTeam1");
   mTeam2.init(this, "mTeam2");

   mTeam1.setText(gDatabase.getLocStringFromID(25296));
   mTeam2.setText(gDatabase.getLocStringFromID(25297));

   // initialize all the components for our screen
   mPlayerList.init( this, "", 0, NULL );

/*
   // initialize all the components for our screen
   mObjectiveList.init( this, "mObjectiveList", -1, NULL );

   // init the labels.
   mObjectiveTitle.init(this, "mObjectiveTitle", -1, NULL );

   mObjectiveTitle.setText(gDatabase.getLocStringFromID(25339));
*/

   initPlayers(dataNode);

   mHelpText.init(this, "mHelpText", -1, NULL);

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", -1, NULL);

   mGameTime.init(this, "mGameTime");

   mDifficulty.init(this, "mDifficulty");

   return true;
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::updatePlayerStatus()
{
   for (int i=0; i<cMaxSkirmishPlayers; i++)
   {
      const BPlayer * pPlayer = gWorld->getPlayer(mPlayers[i].getPlayerID());
      
      if (!pPlayer)
         continue;

      mPlayers[i].setStatus( pPlayer->getPlayerState() );
   }
}


//============================================================================
//============================================================================
void BUISkirmishObjectives::populatePlayer(BPlayer* player, BUIGamerTagLongControl& playerControl)
{
   if (!player)
      return;

//-- FIXING PREFIX BUG ID 1347
   const BUser* user=gUserManager.getUserByPlayerID(player->getID());
//--

   BString temp;
   BUString tempU;

   // Basic player info
   playerControl.setGamerTag(player->getLocalisedDisplayName());
   temp.format("img://gamerPic:%I64x", player->getXUID());
   playerControl.setGamerPic(temp);

   int controllerPort=-1;
   if (user)
      controllerPort=user->getPort();
   playerControl.setPort(controllerPort);

   // leader info
//-- FIXING PREFIX BUG ID 1348
   const BLeader* leader=player->getLeader();
//--
   if (leader)
      playerControl.setLeaderPic(leader->getUIBackgroundImage());

   tempU.locFormat(gDatabase.getLocStringFromID(25325).getPtr(), gScoreManager.getBaseScore(player->getID() ) );
   playerControl.setTitleText(tempU);

   DWORD playerColor = gWorld->getPlayerColor(player->getID(), BWorld::cPlayerColorContextUI);
   playerControl.setPlayerColor(playerColor);

   playerControl.setPlayerID(player->getID());

   BProfileRank rank(player->getRank());
   playerControl.setRank(rank.mRank);

   playerControl.setStatus( player->getPlayerState() );

   // misc stuff.
   // fixme - add this stuff in.
/*
   playerControl.setAIDifficulty(int difficulty);
   playerControl.setHost(bool bHost);
   playerControl.setPing(int ping);
*/
}



//============================================================================
//============================================================================
void BUISkirmishObjectives::populatePlayers()
{
   // turn off all the players
   BString temp;
   for (int i=0; i<cMaxSkirmishPlayers; i++)
      mPlayers[i].hide();

   // fill in and show each one we should
   int slot=0;
   for (int i=1; i<gWorld->getNumberTeams(); i++)
   {
      if (slot>0)
         slot=3;        // hack to start the next player on the 2nd team.

      BTeam * pTeam = gWorld->getTeam(i);
      if (!pTeam)
         continue;

      // get the player
      for (int p=0; p< pTeam->getNumberPlayers(); p++)
      {
         BPlayer * pPlayer = gWorld->getPlayer(pTeam->getPlayerID(p));
         if (!pPlayer)
            continue;

         // skip players that are set up in the scenario.
         if (pPlayer->isNPC())
            continue;

         populatePlayer(pPlayer, mPlayers[slot]);
         mPlayers[slot].show();
         slot++;
      }
   }
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::populateMapName()
{
   BSimString mapName;
   const BGameSettings* pSettings = gDatabase.getGameSettings();

   // get the map name
   pSettings->getString(BGameSettings::cMapName, mapName);

   // get the map definition for this map
   const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapName.getPtr());
   if (pMap!=NULL)
   {
      mTitle.setText(pMap->getMapName());
      mHelpText.setText(pMap->getMapDescription());
      updateRScrollButton();
   }
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::updateRScrollButton()
{
   invokeActionScript( "updateRScrollButton");
}


//============================================================================
//============================================================================
void BUISkirmishObjectives::populateScreen()
{
   reset();

   // Set the title of the screen
   populateMapName();

   populateDifficulty();

   populatePlayers();

   updatePlayerStatus();

   //populateObjectives();

   showMinimap();

//   showSelectedObjective();

   // set the focus on the first player
   mPlayerList.focus();
   mPlayerList.setIndex(0);

   // turn off focus on anything else.
/*
   mObjectiveList.unfocus();
   mObjectiveTitle.unfocus();
*/

   displayButtons();
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::initPlayers(BXMLNode dataNode)
{

   BXMLNode controlsNode;
   bool foundNode=dataNode.getChild( "UIGamerTagControl", &controlsNode );
   if (!foundNode)
   {
      BASSERTM(false, "UISkirmishPostgameScreen:: Unable to initialize player control input handlers.");
   }

   BString temp;
   for (int i=0; i<cMaxSkirmishPlayers; i++)
   {
      temp.format("mPlayer%d", i);
      

      if (foundNode)
         mPlayers[i].init(this, temp.getPtr(), -1, &controlsNode);
      else
         mPlayers[i].init(this, temp.getPtr());

      mPlayers[i].hide();

      mPlayerList.addControl(&mPlayers[i]);
   }
}

//============================================================================
//============================================================================
/*
bool BUISkirmishObjectives::addObjective(const WCHAR* menuText, int listID, int index, int controlID, int type, int status)
{
   BSimString controlPath;
   controlPath.format("mObjectiveList.mObjective%d", index);
   BUString text;
   text.set(menuText);

   BUIObjectiveControl *c = new BUIObjectiveControl();

   c->init( this, controlPath.getPtr(), controlID);
   c->setText(text);
   c->setType(type);
   c->setStatus(status);
   if (type != BUIObjectiveControl::cObjectiveTypeTitle)
      c->enable();
   else
      c->disable();
   c->unfocus(true);
   c->show(true);

   mObjectiveList.addControl(c);
   mObjectiveItems.add(c);

   return true;
}
*/




//============================================================================
//============================================================================
/*
void BUISkirmishObjectives::populateObjectives()
{
   long focusIndex = -1;
   mObjectiveList.clearControls();
   mObjectiveList.reset();
   mObjectiveList.setMaxItems(cMaxObjectives);
   mObjectiveList.setViewSize(4);
   mObjectiveItems.clear();

//-- FIXING PREFIX BUG ID 1350
   const BUser* pUser=gUserManager.getPrimaryUser();
//--
   if (!pUser)
      return;

   long playerID = pUser->getPlayerID();

   if (!gWorld)
      return;

   // get the objective manager
   BObjectiveManager* pObjectiveManager = gWorld->getObjectiveManager();
   if( !pObjectiveManager )
      return;

   // fill out the list
   int slotIndex = 0;

   // split the objectives apart
   for( int i = 0; i < pObjectiveManager->getNumberObjectives(); i++ )
   {
      BObjective * pObj = pObjectiveManager->getObjective(i);
      if (!pObj)
         continue;

      // is this for me?
      if ( !pObj->isPlayer(playerID) )
         continue;

      // is this visible yet?
      if ( !pObj->isDiscovered() )
         continue;

      int controlID = i;
      int type = BUIObjectiveControl::cObjectiveTypePrimary;
      int status = BUIObjectiveControl::cObjectiveStatusActive;

      // check status
      if (pObj->isCompleted())
         status = BUIObjectiveControl::cObjectiveStatusCompleted;

      // check type
      if (!pObj->isRequired())
         type = BUIObjectiveControl::cObjectiveTypeSecondary;

      if (focusIndex==-1)
         focusIndex=slotIndex;
      addObjective(pObj->getDescription().getPtr(), 1, slotIndex++, controlID, type, status);
   }


   // disable the rest of the controls
   for (int i=slotIndex; i<cMaxObjectives; i++)
   {
      addObjective(L"", 1, slotIndex, -1, 0, 0);
      BUIControl* c = mObjectiveList.getControl(slotIndex);
      if (c)
      {
         c->hide(true);
         c->disable();
      }
      slotIndex++;
   }

   mObjectiveList.setIndex(focusIndex);
   mObjectiveList.unfocus();
   mObjectiveTitle.unfocus();

}*/


//============================================================================
//============================================================================
bool BUISkirmishObjectives::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   // Controls
/*
   if (!handled && mObjectiveList.isFocused())
      handled = mObjectiveList.handleInput(port, event, controlType, detail);
*/
   
   if (!handled )
      handled = mHelpText.handleInput(port, event, controlType, detail);
   
   if (!handled && mPlayerList.isFocused())
      handled = mPlayerList.handleInput(port, event, controlType, detail);
   
   
   // Screen
   if (!handled)
      handled = BUIScreen::handleInput(port, event, controlType, detail);
   
   return true;
}

//============================================================================
//============================================================================
bool BUISkirmishObjectives::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
/*
      if (command=="right")
      {
         // if on player list, go to the objective list
         if (getPlayerWithFocus()>=0)
         {
            // turn on the focus on the objective list.
            mObjectiveList.focus(true);
            mObjectiveList.setIndex(0);
            mObjectiveTitle.focus(true);
   
            // remove the focus from the player list
            mPlayerList.unfocus();
            clearPlayerFocus();
   
            displayButtons();
            return true;
         }
      }
      else if (command=="left")
      {
         // if on objective list, go to player list
         if ( mObjectiveList.isFocused())
         {
            mObjectiveList.unfocus();
            mObjectiveTitle.unfocus();
   
            mPlayerList.focus();
            mPlayerList.setIndex(0);
            displayButtons();
            return true;
         }
      }
      else */
   if (command=="cancel")
   {
      gUI.playCancelSound();
      mpHandler->handleUIScreenResult( this, 0 );
      return true;
   }

   return false;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUISkirmishObjectives::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   if (event.getID()==BUIScrollingListControl::eItemSelected)
   {
      //showSelectedObjective();
   }
   else if (event.getID()==BUIListControl::eSelectionChanged)
   {
      displayButtons();
   }
   else if (control->getControlTypeID() == UIGamerTagControlID)
   {
      if (event.getString()=="gamerTag")
      {
         //if (gLiveSystem->isMultiplayerLiveGameActive())
         if (gDatabase.getGameSettingsNetworkType() == BGameSettings::cNetworkTypeLive)
         {
            // show the gamertag
            // need the port of the user that initiated the gamercard request
            BUser* pUser = gUserManager.getUserByPort(gUserManager.getPrimaryUser()->getPort());
            if (pUser)
            {
               int playerControlIndex = getPlayerWithFocus();
               if (playerControlIndex>=0 && playerControlIndex < cMaxSkirmishPlayers)
               {
                  BPlayer* pPlayer = gWorld->getPlayer(mPlayers[playerControlIndex].getPlayerID());
                  if (pPlayer)
                  {
                     if (pPlayer->isHuman() && pPlayer->getXUID() != INVALID_XUID && !gUserManager.isGuestXuid(pPlayer->getXUID()))
                        XShowGamerCardUI(pUser->getPort(), pPlayer->getXUID());
                  }
               }
            }
         }
      }
   }




   bool handled = false;

   return handled;
}

//============================================================================
//============================================================================
/*
void BUISkirmishObjectives::showSelectedObjective()
{
   BObjectiveManager* pObjectiveManager = gWorld->getObjectiveManager();
   if( !pObjectiveManager )
      return;

//-- FIXING PREFIX BUG ID 1353
   const BUIObjectiveControl* c = NULL;
//--
   if (mObjectiveList.isFocused())
      c = (BUIObjectiveControl*)mObjectiveList.getSelectedControl();

   if (!c)
      return;

   BObjective * pObj = pObjectiveManager->getObjective(c->getControlID());
   if (!pObj)
      return;;

   // populate the help text with the larger description text
   if (pObj->getHint().length()!=0)
      mHelpText.setText(pObj->getHint());
   else 
      mHelpText.setText(pObj->getDescription());
}
*/



//============================================================================
//============================================================================
bool BUISkirmishObjectives::displayButtons()
{
   uint b0=BUIButtonBarControl::cFlashButtonOff;
   uint b1=BUIButtonBarControl::cFlashButtonOff;
   uint b2=BUIButtonBarControl::cFlashButtonOff;
   uint b3=BUIButtonBarControl::cFlashButtonOff;
   uint b4=BUIButtonBarControl::cFlashButtonOff;

   // change the string IDs and button faces as needed

   // A - accept
   // B - back to main menu
   BUString s0;
   BUString s1;
   BUString s2;
   BUString s3;
   BUString s4;

   if (getPlayerWithFocus()>=0)
   {
      BPlayer * player = gWorld->getPlayer(mPlayers[getPlayerWithFocus()].getPlayerID());

      // if we have a human and we are in MP and not in lan mode.
      //if (gLiveSystem->isMultiplayerLiveGameActive() && player && player->isHuman())
      if ( (gDatabase.getGameSettingsNetworkType() == BGameSettings::cNetworkTypeLive) && player && player->isHuman() )
      {
         // determine if this player is a human or not
         b0 = BUIButtonBarControl::cFlashButtonA;
         s0.set(gDatabase.getLocStringFromID(25045));     // GAMERTAG
      }
   }

   b1=BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(23440));        // BACK

/*
   b4=BUIButtonBarControl::cFlashButtonudRStick;
   s4.set(gDatabase.getLocStringFromID(24803));        // SCROLL
*/


   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

   return true;
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::clearPlayerFocus()
{
   for (int i=0; i<cMaxSkirmishPlayers; i++)
   {
      mPlayers[i].unfocus();
   }
}


//============================================================================
//============================================================================
int BUISkirmishObjectives::getPlayerWithFocus()
{
   for (int i=0; i<cMaxSkirmishPlayers; i++)
   {
      if (mPlayers[i].isFocused())
         return i;
   }

   return -1;
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::setVisible( bool visible )
{
   if (getVisible() && !visible)
      restoreMinimap();

   BUIScreen::setVisible(visible);
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::enter( void )
{
   if (!gLiveSystem->isMultiplayerGameActive())
      gModeManager.getModeGame()->setPaused(true);

   gUIManager->setMinimapVisible(true);
   populateScreen();

   gUI.playConfirmSound();
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::leave( void )
{
   gUIManager->setMinimapVisible(false);

   if (!gLiveSystem->isMultiplayerGameActive())
      gModeManager.getModeGame()->setPaused(false);
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::update(float elapsedTime)
{
   if (!getVisible())
      return;

   // don't update every time (try 100ms)
   DWORD gameTime=gWorld->getGametime();
   if ( (gameTime - mLastGameTimeUpdate) < 100 )
      return;

   mLastGameTimeUpdate=gameTime;

   updatePlayerStatus();

   updateGameTime();
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::populateDifficulty()
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
void BUISkirmishObjectives::updateGameTime()
{
   // update the time label
   if (!gWorld)
      return;

   DWORD gameTime=gWorld->getGametime();
   DWORD s=gameTime/1000;
   DWORD m=s/60;
   s-=m*60;
   DWORD h=m/60;
   m-=h*60;

   BUString t;
   if(h>0)
      t.locFormat(L"%02d:%02d:%02d", h, m, s);
   else
      t.locFormat(L"%02d:%02d", m, s);

   mGameTime.setText(t);
}


//============================================================================
//============================================================================
void BUISkirmishObjectives::restoreMinimap()
{
   BFlashMinimap* pMinimap = gUIManager->getMinimap();
   if (!pMinimap)
      return;

   pMinimap->repositionMap(mOriginalLocation.mMapCenterX, mOriginalLocation.mMapCenterY, mOriginalLocation.mMapW, mOriginalLocation.mMapH);
   pMinimap->setDimension(mOriginalLocation.mMapFlashX, mOriginalLocation.mMapFlashY, mOriginalLocation.mMapFlashW, mOriginalLocation.mMapFlashH);

   if (gConfig.isDefined(cConfigAlpha))
      gWatermarkUI.setVisible(true);
}

//============================================================================
//============================================================================
void BUISkirmishObjectives::showMinimap()
{
   // fix up the positions of the minimap
   BFlashMinimap* pMinimap = gUIManager->getMinimap();

   if (!pMinimap)
      return;

   pMinimap->setVisible(true);
   mOriginalLocation.mMapCenterX = pMinimap->getMapViewCenterX();
   mOriginalLocation.mMapCenterY = pMinimap->getMapViewCenterY();
   mOriginalLocation.mMapW = pMinimap->getMapViewWidth();
   mOriginalLocation.mMapH = pMinimap->getMapViewHeight();

   mOriginalLocation.mMapFlashX = pMinimap->getFlashMovieX();
   mOriginalLocation.mMapFlashY = pMinimap->getFlashMovieY();
   mOriginalLocation.mMapFlashW = pMinimap->getFlashMovieW();
   mOriginalLocation.mMapFlashH = pMinimap->getFlashMovieH();

   if (gConfig.isDefined("LeaveMiniMapAlone"))
      return;

   int index = cMinimapLocation16x9;
   if (gRender.getAspectRatioMode()==BRender::cAspectRatioMode4x3)
      index = cMinimapLocation4x3;

   pMinimap->repositionMap(mMinimapLocation[index].mMapCenterX, mMinimapLocation[index].mMapCenterY, 
      mMinimapLocation[index].mMapW, mMinimapLocation[index].mMapH);


   pMinimap->setDimension(mMinimapLocation[index].mMapFlashX, mMinimapLocation[index].mMapFlashY,
      mMinimapLocation[index].mMapFlashW, mMinimapLocation[index].mMapFlashH);

   if (gConfig.isDefined(cConfigAlpha))
      gWatermarkUI.setVisible(false);
}

//============================================================================
// UIObjectiveScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UICampaignObjectives.h"
#include "UIObjectiveControl.h"

#include "database.h"
#include "player.h"
#include "team.h"
#include "usermanager.h"
#include "user.h"
#include "objectivemanager.h"
#include "world.h"
#include "flashminimap.h"
#include "uiwatermark.h"
#include "uimanager.h"
#include "render.h"
#include "econfigenum.h"
#include "scoremanager.h"
#include "LiveSystem.h"
#include "modemanager.h"
#include "modegame.h"
#include "uiwidgets.h"
#include "gamesettings.h"
#include "campaignmanager.h"
#include "soundmanager.h"
#include "userprofilemanager.h"

//============================================================================
//============================================================================
BUICampaignObjectives::BUICampaignObjectives( void ) : 
mLastGameTimeUpdate(0),
mpLastObjectiveControlWithFocus(NULL),
mpLastObjectiveLabelWithFocus(NULL),
mLastViewChangeTime(0)
{
}

//============================================================================
//============================================================================
BUICampaignObjectives::~BUICampaignObjectives( void )
{
   reset();
}

//============================================================================
//============================================================================
bool BUICampaignObjectives::reset()
{

   mpLastObjectiveControlWithFocus=NULL;
   mpLastObjectiveLabelWithFocus=NULL;

   BUIObjectiveControl* c = NULL;

   // primary
   for (int i=0; i<mPrimaryObjectiveItems.getNumber(); i++)
   {
      c = mPrimaryObjectiveItems[i];
      delete c;
      mPrimaryObjectiveItems[i]=NULL;
   }
   mPrimaryObjectiveItems.clear();

   // secondary
   for (int i=0; i<mOptionalObjectiveItems.getNumber(); i++)
   {
      c = mOptionalObjectiveItems[i];
      delete c;
      mOptionalObjectiveItems[i]=NULL;
   }
   mOptionalObjectiveItems.clear();

   // completed
   for (int i=0; i<mCompletedObjectiveItems.getNumber(); i++)
   {
      c = mCompletedObjectiveItems[i];
      delete c;
      mCompletedObjectiveItems[i]=NULL;
   }
   mCompletedObjectiveItems.clear();

   return true;
}

//============================================================================
//============================================================================
bool BUICampaignObjectives::init( const char* filename, const char* datafile )
{
   return BUIScreen::init(filename, datafile);
}


//============================================================================
//============================================================================
bool BUICampaignObjectives::init(BXMLNode dataNode)
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

   // initialize all the components for our screen
   mObjectiveList1.init( this, "mObjectiveList1", cControlIDList1, NULL );
   mObjectiveList2.init( this, "mObjectiveList2", cControlIDList2, NULL );
   mObjectiveList3.init( this, "mObjectiveList3", cControlIDList3, NULL );

   // init the labels.
   mLabelPrimaryObjectives.init(this, "mPrimaryObjectiveTitle", -1, NULL );
   mLabelOptionalObjectives.init(this, "mOptionalObjectiveTitle", -1, NULL );
   mLabelCompletedObjectives.init(this, "mCompletedObjectiveTitle", -1, NULL );

   BUString temp;
   temp.set(gDatabase.getLocStringFromID(25077));//L"PRIMARY");
   mLabelPrimaryObjectives.setText(temp);

   temp.set(gDatabase.getLocStringFromID(25078));//L"OPTIONAL");
   mLabelOptionalObjectives.setText(temp);
   
   temp.set(gDatabase.getLocStringFromID(25079));//L"COMPLETED");
   mLabelCompletedObjectives.setText(temp);

   mDescription.init(this, "mDescriptionLabel");
   mDescription.setText(gDatabase.getLocStringFromID(25677));     // Description

   mDifficulty.init(this, "mDifficulty");

   mTitle.init(this, "mTitle");

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

   mGameTime.init(this, "mGameTime");
   //mGameTime.hide();

   initPlayers(dataNode);

   mHelpText.init(this, "mHelpText", -1, NULL);

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", -1, NULL);

   // initImageViewer();

   return true;
}

//============================================================================
//============================================================================
void BUICampaignObjectives::populatePlayer(BPlayer* player, BUIGamerTagLongControl& playerControl)
{
   if (!player)
      return;

//-- FIXING PREFIX BUG ID 1342
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
//-- FIXING PREFIX BUG ID 1343
   const BLeader* leader=player->getLeader();
//--
   if (leader)
      playerControl.setLeaderPic(leader->getUIBackgroundImage());

   // fixme - we may want to update this on an update call.
   tempU.locFormat(gDatabase.getLocStringFromID(25325).getPtr(), gScoreManager.getBaseScore(player->getID() ) );
   playerControl.setTitleText(tempU);

   playerControl.setXuid(player->getXUID());

   BProfileRank rank(player->getRank());
   playerControl.setRank(rank.mRank);

   // misc stuff.
   // fixme - add this stuff in.
/*

   playerControl.setRank(int rank);
   playerControl.setAIDifficulty(int difficulty);
   playerControl.setHost(bool bHost);
   playerControl.setPing(int ping);
   playerControl.setStatus(int status);
*/

}

//============================================================================
//============================================================================
void BUICampaignObjectives::populatePlayers()
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser)
      populatePlayer(pUser->getPlayer(), mPlayer1);


   if (gWorld->getFlagCoop())
   {
      BTeamID teamID = pUser->getPlayer()->getTeamID();      // find the coop player
      BTeam * team = gWorld->getTeam(teamID);

      BPlayer * pCoopPlayer=NULL;
      for (int i=0; i<team->getNumberPlayers(); i++)
      {
         pCoopPlayer = gWorld->getPlayer(team->getPlayerID(i));

         // if we found a player and it's not the primary user, then assume it's the coop player
         // [10/16/2008 xemu] added check here to look for human players, since this was triggering on the wrong player in SCN03 
         if (pCoopPlayer && (pCoopPlayer->getID() != pUser->getPlayer()->getID()) && pCoopPlayer->isHuman())
            break;

         pCoopPlayer=NULL;    // just in case we didn't find a coop player
      }

      if (pCoopPlayer)
      {
         populatePlayer(pCoopPlayer, mPlayer2);
         mPlayer2.show();
      }
      else
      {
         mPlayer2.hide();
      }
   }
   else
   {
      mPlayer2.hide();
   }

}

//============================================================================
//============================================================================
void BUICampaignObjectives::populateImage()
{

}

//============================================================================
//============================================================================
void BUICampaignObjectives::showSelectedObjective()
{
   BObjectiveManager* pObjectiveManager = gWorld->getObjectiveManager();
   if( !pObjectiveManager )
      return;

//-- FIXING PREFIX BUG ID 1344
   const BUIObjectiveControl* c = NULL;
//--
   if (mObjectiveList1.isFocused())
      c = (BUIObjectiveControl*)mObjectiveList1.getSelectedControl();
   else if (mObjectiveList2.isFocused())
      c = (BUIObjectiveControl*)mObjectiveList2.getSelectedControl();
   else if (mObjectiveList3.isFocused())
      c = (BUIObjectiveControl*)mObjectiveList3.getSelectedControl();

   if (!c)
      return;
   
   BObjective * pObj = pObjectiveManager->getObjective(c->getControlID());
   if (!pObj)
      return;;

   BUString fullDescriptionText;



   // populate the help text with the larger description text
   if (pObj->getHint().length()!=0)
   {
      if (pObj->getScore() > 0)
      {
         //fullDescriptionText.format(L"Completion Score: %d<br>%s", pObj->getScore(), pObj->getHint().getPtr());
         fullDescriptionText.format(gDatabase.getLocStringFromID(25951), pObj->getScore(), pObj->getHint().getPtr());
      }
      else
      {
         fullDescriptionText.set(pObj->getHint().getPtr());
      }
   }
   else 
   {
      if (pObj->getScore() > 0)
      {
         fullDescriptionText.format(gDatabase.getLocStringFromID(25951), pObj->getScore(), pObj->getDescription().getPtr());
      }
      else
      {
         fullDescriptionText.set(pObj->getDescription().getPtr());
      }
   }

   mHelpText.setText(fullDescriptionText);

   updateRScrollButton();
}

//============================================================================
//============================================================================
void BUICampaignObjectives::updateRScrollButton()
{
   invokeActionScript( "updateRScrollButton");
}



//============================================================================
//============================================================================
void BUICampaignObjectives::populateScreen()
{
   reset();

   updateGameTime();

   mPlayer1.unfocus();
   mPlayer2.unfocus();
   populatePlayers();

   populateDifficulty();

   populateObjectives();

   populateImage();

   showMinimap();

   selectFirstObjective();

   showSelectedObjective();

   displayButtons();
}

//============================================================================
//============================================================================
void BUICampaignObjectives::populateDifficulty()
{
   BPlayer* pPlayer = gWorld->getPlayer(1);        // any way to make this safer?
   if (pPlayer && pPlayer->isHuman())
   {
      BDifficultyType playerDifficulty = pPlayer->getDifficultyType();
      mDifficulty.update(playerDifficulty);
/*
      mDifficulty.setText(gDatabase.getDifficultyStringByType(playerDifficulty));
*/
   }
}

//============================================================================
//    fixme - we need to get the auto updating of the user control working
//          for when users sign in or sign out
//============================================================================
void BUICampaignObjectives::initPlayers(BXMLNode dataNode)
{
   int numChildren = dataNode.getNumberChildren();
   for (int i = 0; i < numChildren; ++i)
   {
      BXMLNode node = dataNode.getChild(i);
      if (node.getName().compare("ControlList") == 0)
      {
         // Loop through the children of "ControlList"
         for (int j=0; j<node.getNumberChildren(); j++)
         {
            BXMLNode child(node.getChild(j));
            const BPackedString name(child.getName());

            if(name==B("UIGamerTagControl"))
            {
               mPlayer1.init(this, "mPlayer1", cControlIDPlayer1, &child);
               mPlayer2.init(this, "mPlayer2", cControlIDPlayer2, &child);
               mPlayer2.hide();
               return;;
            }
         }
      }
   }

   // if we didn't init in the loop above then we fall through to here.
   mPlayer1.init(this, "mPlayer1", cControlIDPlayer1, NULL);
   mPlayer2.init(this, "mPlayer2", cControlIDPlayer2, NULL);
   mPlayer2.hide();
}

//============================================================================
//============================================================================
void BUICampaignObjectives::initImageViewer()
{
   mImageViewer.init(this, "mImageViewer", 0, NULL);

   mImageViewer.setAutoTransition(true);
   mImageViewer.setViewDuration(9000.0f);
   mImageViewer.setImageSize(700, 350);

   mImageViewer.start();
   
}

//============================================================================
//============================================================================
bool BUICampaignObjectives::addObjective(const WCHAR* menuText, int listID, int index, int controlID, int type, int status)
{
   BSimString controlPath;
   controlPath.format("mObjectiveList%d.mObjective%d", listID, index);
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

   switch (listID)
   {
      case 1:
         mObjectiveList1.addControl(c);
         mPrimaryObjectiveItems.add(c);
         break;
      case 2:
         mObjectiveList2.addControl(c);
         mOptionalObjectiveItems.add(c);
         break;
      case 3:
         mObjectiveList3.addControl(c);
         mCompletedObjectiveItems.add(c);
         break;
   }

   return true;
}



//============================================================================
//============================================================================
void BUICampaignObjectives::populateObjectives()
{
   long focusIndex = -1;
   mObjectiveList1.clearControls();
   mObjectiveList1.reset();
   mPrimaryObjectiveItems.clear();
   mObjectiveList1.setMaxItems(cMaxObjectives);
   mObjectiveList1.setViewSize(2);


   mObjectiveList2.clearControls();
   mObjectiveList2.reset();
   mOptionalObjectiveItems.clear();
   mObjectiveList2.setMaxItems(cMaxObjectives);
   mObjectiveList2.setViewSize(4);

   mObjectiveList3.clearControls();
   mObjectiveList3.reset();
   mCompletedObjectiveItems.clear();
   mObjectiveList3.setMaxItems(cMaxObjectives);
   mObjectiveList3.setViewSize(4);

//-- FIXING PREFIX BUG ID 1345
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

   // split the objectives out
   BSmallDynamicSimArray<int> primaryObjectives;
   BSmallDynamicSimArray<int> optionalObjectives;
   BSmallDynamicSimArray<int> completedObjectives;

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

      if (pObj->isCompleted())         // is it done?
         completedObjectives.add(i);      // add the index
      else if (pObj->isRequired())     // not done, is it primary
         primaryObjectives.add(i);        // add the index
      else                             // not done, and it's optional
         optionalObjectives.add(i);       // add the index
   }

   // fill out the list
   int slotIndex = 0;

   // add a title, fixme - set it up properly
   // addObjective(L"PRIMARY OBJECTIVES", slotIndex++, -1, BUIObjectiveControl::cObjectiveTypeTitle, BUIObjectiveControl::cObjectiveStatusActive);

   // add the primary objectives
   for (int i=0; i<primaryObjectives.getNumber(); i++)
   {
      BObjective * pObj = pObjectiveManager->getObjective(primaryObjectives[i]);
      if (!pObj)
         continue;
      if (focusIndex==-1)
         focusIndex=slotIndex;
      addObjective(pObj->getDescription().getPtr(), 1, slotIndex++, primaryObjectives[i], BUIObjectiveControl::cObjectiveTypePrimary, BUIObjectiveControl::cObjectiveStatusActive);
   }

   for (int i=slotIndex; i<cMaxObjectives; i++)
   {
      addObjective(L"", 1, slotIndex, -1, 0, 0);
      BUIControl* c = mObjectiveList1.getControl(slotIndex);
      if (c)
      {
         c->hide(true);
         c->disable();
      }
      slotIndex++;
   }

   slotIndex = 0;

   // add a title, fixme - set it up properly
   // addObjective(L"OPTIONAL OBJECTIVES", slotIndex++, -1, BUIObjectiveControl::cObjectiveTypeTitle, BUIObjectiveControl::cObjectiveStatusActive);

   // add the primary objectives
   for (int i=0; i<optionalObjectives.getNumber(); i++)
   {
      BObjective * pObj = pObjectiveManager->getObjective(optionalObjectives[i]);
      if (!pObj)
         continue;
      addObjective(pObj->getDescription().getPtr(), 2, slotIndex++, optionalObjectives[i], BUIObjectiveControl::cObjectiveTypeSecondary, BUIObjectiveControl::cObjectiveStatusActive);
   }

   for (int i=slotIndex; i<cMaxObjectives; i++)
   {
      addObjective(L"", 2, slotIndex, -1, 0, 0);
      BUIControl* c = mObjectiveList2.getControl(slotIndex);
      if (c)
      {
         c->hide(true);
         c->disable();
      }
      slotIndex++;
   }

   slotIndex=0;

   // add a title, fixme - set it up properly
   // addObjective(L"COMPLETED OBJECTIVES", slotIndex++, -1, BUIObjectiveControl::cObjectiveTypeTitle, BUIObjectiveControl::cObjectiveStatusActive);

   // add the primary objectives
   for (int i=0; i<completedObjectives.getNumber(); i++)
   {
      BObjective * pObj = pObjectiveManager->getObjective(completedObjectives[i]);
      if (!pObj)
         continue;
      int type = BUIObjectiveControl::cObjectiveTypePrimary;
      if (!pObj->isRequired())
         type = BUIObjectiveControl::cObjectiveTypeSecondary;

      int status = BUIObjectiveControl::cObjectiveStatusCompleted;
      if (!pObj->isCompleted())
         status = BUIObjectiveControl::cObjectiveStatusActive;

      addObjective(pObj->getDescription().getPtr(), 3, slotIndex++, completedObjectives[i], type, status);
   }

   // disable the rest of the controls
   for (int i=slotIndex; i<cMaxObjectives; i++)
   {
      addObjective(L"", 3, slotIndex, -1, 0, 0);
      BUIControl* c = mObjectiveList3.getControl(slotIndex);
      if (c)
      {
         c->hide(true);
         c->disable();
      }
      slotIndex++;
   }

   mObjectiveList1.setIndex(focusIndex);
   mObjectiveList1.focus();
   mLabelPrimaryObjectives.focus();

   mObjectiveList2.unfocus();
   mObjectiveList2.setIndex(-1);
   mLabelOptionalObjectives.unfocus();

   mObjectiveList3.unfocus();
   mObjectiveList3.setIndex(-1);
   mLabelCompletedObjectives.unfocus();
}

//============================================================================
//============================================================================
bool BUICampaignObjectives::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   //Dirty E3 hack to stop the double up/down tap when you switch between modes - eric
   if (((controlType==cStickLeftUp) ||
      (controlType==cStickLeftDown) ||
      (controlType==cDpadUp) ||
      (controlType==cDpadDown)) && 
      (timeGetTime()-mLastViewChangeTime < 200))
   {
      return true;
   }

   // Controls
   if (!handled && mObjectiveList1.isFocused())
      handled = mObjectiveList1.handleInput(port, event, controlType, detail);

   if (!handled && mObjectiveList2.isFocused())
      handled = mObjectiveList2.handleInput(port, event, controlType, detail);
   
   if (!handled && mObjectiveList3.isFocused())
      handled = mObjectiveList3.handleInput(port, event, controlType, detail);
   
   if (!handled && mPlayer1.isFocused())
      handled = mPlayer1.handleInput(port, event, controlType, detail);
   
   if (!handled && mPlayer2.isFocused())
      handled = mPlayer2.handleInput(port, event, controlType, detail);
   
   if (!handled)
   handled = mHelpText.handleInput(port, event, controlType, detail);
   
   // Screen
   if (!handled)
      handled = BUIScreen::handleInput(port, event, controlType, detail);
   
   return true;
}

//============================================================================
//============================================================================
bool BUICampaignObjectives::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="left")
   {
      // if on player list, go to the objective list
      if (mPlayer1.isFocused() || mPlayer2.isFocused())
      {
         mPlayer1.unfocus();
         mPlayer2.unfocus();
         if (mpLastObjectiveControlWithFocus)
         {
            mpLastObjectiveControlWithFocus->focus(true);
            mpLastObjectiveLabelWithFocus->focus(true);
         }

         gSoundManager.playCue( "play_ui_menu_horizonal" );
         displayButtons();
         return true;
      }
   }
   else if (command=="right")
   {
      // if on objective list, go to player list
      if ( mObjectiveList1.isFocused() || 
            mObjectiveList2.isFocused() || 
            mObjectiveList3.isFocused() )
      {
         mpLastObjectiveControlWithFocus=NULL;
         mpLastObjectiveLabelWithFocus=NULL;
         if ( mObjectiveList1.isFocused())
         {
            mpLastObjectiveControlWithFocus=&mObjectiveList1;
            mpLastObjectiveLabelWithFocus=&mLabelPrimaryObjectives;
         }
         else if ( mObjectiveList2.isFocused())
         {
            mpLastObjectiveControlWithFocus=&mObjectiveList2;
            mpLastObjectiveLabelWithFocus=&mLabelOptionalObjectives;
         }
         else if ( mObjectiveList3.isFocused())
         {
            mpLastObjectiveControlWithFocus=&mObjectiveList3;
            mpLastObjectiveLabelWithFocus=&mLabelCompletedObjectives;
         }

         if (mpLastObjectiveControlWithFocus)
            mpLastObjectiveControlWithFocus->unfocus();

         if (mpLastObjectiveLabelWithFocus)
            mpLastObjectiveLabelWithFocus->unfocus();

         mPlayer1.focus();
         displayButtons();
         gSoundManager.playCue( "play_ui_menu_horizonal" );
         return true;
      }
   }
   else if (command=="prev")
   {
      // if on player list, go up
      if (mPlayer2.isFocused())
      {
         mPlayer2.unfocus();
         mPlayer1.focus();
         displayButtons();
         return true;
      }
   }
   else if (command=="next")
   {
      // if on player list go down
      if (mPlayer1.isFocused() && mPlayer2.isShown())
      {
         mPlayer1.unfocus();
         mPlayer2.focus();
         displayButtons();
         return true;
      }
   }
   else if (command=="cancel")
   {
      gUI.playCancelSound();
      mpHandler->handleUIScreenResult( this, 0 );
      return true;
   }
   return false;
}

//==============================================================================
//==============================================================================
void BUICampaignObjectives::selectFirstObjective()
{
   BUIControl* c=NULL;

   // try first objective list
   
   if (mObjectiveList1.getNumVisibleControls()>0)
   {
      // remove focus from objective list 2 (if it has focus)
      mObjectiveList2.unfocus();
      mLabelOptionalObjectives.unfocus();
      c = mObjectiveList2.getSelectedControl();
      if (c)
         c->unfocus();

      mObjectiveList3.unfocus();
      mLabelCompletedObjectives.unfocus();
      c = mObjectiveList3.getSelectedControl();
      if (c)
         c->unfocus();


      mObjectiveList1.focus();
      mLabelPrimaryObjectives.focus();
      c = mObjectiveList1.getControl(0);
      if (c)
         c->focus();
   }
   else if (mObjectiveList2.getNumVisibleControls()>0)
   {
      // remove focus from objective list 2 (if it has focus)
      mObjectiveList1.unfocus();
      mLabelPrimaryObjectives.unfocus();
      c = mObjectiveList1.getSelectedControl();
      if (c)
         c->unfocus();

      mObjectiveList3.unfocus();
      mLabelCompletedObjectives.unfocus();
      c = mObjectiveList3.getSelectedControl();
      if (c)
         c->unfocus();


      mObjectiveList2.focus();
      mLabelOptionalObjectives.focus();
      c = mObjectiveList2.getControl(0);
      if (c)
         c->focus();
   }
   else if (mObjectiveList3.getNumVisibleControls()>0)
   {
      // remove focus from objective list 2 (if it has focus)
      mObjectiveList1.unfocus();
      mLabelPrimaryObjectives.unfocus();
      c = mObjectiveList1.getSelectedControl();
      if (c)
         c->unfocus();

      mObjectiveList2.unfocus();
      mLabelOptionalObjectives.unfocus();
      c = mObjectiveList2.getSelectedControl();
      if (c)
         c->unfocus();


      mObjectiveList3.focus();
      mLabelCompletedObjectives.focus();
      c = mObjectiveList3.getControl(0);
      if (c)
         c->focus();
   }

}
// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUICampaignObjectives::handleUIControlEvent( BUIControlEvent& event )
{
   // mSecondaryMenu.show();

   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   bool handled = false;

   BUIControl* c=NULL;

   switch (event.getID())
   {
      case BUIScrollingListControl::eItemSelected:
      {
         showSelectedObjective();
         break;
      }

      case BUIScrollingListControl::eListBottomEvent:
      {
         // try to move the focus to the objective list below this one.
         switch(control->getControlID())
         {
         case cControlIDList1:
               
               if ( (mObjectiveList2.getNumVisibleControls()>0) ||
                    (mObjectiveList3.getNumVisibleControls()>0) )
               {
                  mObjectiveList1.unfocus();
                  mLabelPrimaryObjectives.unfocus();
                  c = mObjectiveList1.getSelectedControl();
                  if (c)
                     c->unfocus();

                  c=NULL;
                  if (mObjectiveList2.getNumVisibleControls()>0)
                  {
                     mObjectiveList2.focus();
                     mLabelOptionalObjectives.focus();
                     c = mObjectiveList2.getControl(0);
                  }
                  else
                  {
                     mObjectiveList3.focus();
                     mLabelCompletedObjectives.focus();
                     c = mObjectiveList3.getControl(0);
                  }
                  if (c)
                     c->focus();

                  showSelectedObjective();
                  mLastViewChangeTime = timeGetTime();
               }
               break;
         case cControlIDList2:
            if (mObjectiveList3.getNumVisibleControls()>0)
            {
               mObjectiveList2.unfocus();
               mLabelOptionalObjectives.unfocus();
               c = mObjectiveList2.getSelectedControl();
               if (c)
                  c->unfocus();
               mObjectiveList3.focus();
               mLabelCompletedObjectives.focus();
               c = mObjectiveList3.getControl(0);
               if (c)
                  c->focus();
               showSelectedObjective();
               mLastViewChangeTime = timeGetTime();
            }
               break;
         case cControlIDList3:
               break;
         }
         break;
      }

      case BUIScrollingListControl::eListTopEvent:
      {
         // try to move the focus to the objective list above this one.
         switch(control->getControlID())
         {
         case cControlIDList1:
            break;
         case cControlIDList2:
            if (mObjectiveList1.getNumVisibleControls()>0)
            {
               mObjectiveList2.unfocus();
               mLabelOptionalObjectives.unfocus();
               c = mObjectiveList2.getSelectedControl();
               if (c)
                  c->unfocus();
               mObjectiveList1.focus();
               mLabelPrimaryObjectives.focus();
               c = mObjectiveList1.getControl(mObjectiveList1.getLastVisible());
               if (c)
                  c->focus();
               showSelectedObjective();
               mLastViewChangeTime = timeGetTime();
            }
            break;
         case cControlIDList3:
            if ( (mObjectiveList2.getNumVisibleControls()>0) ||
                 (mObjectiveList1.getNumVisibleControls()>0) )
            {
               mObjectiveList3.unfocus();
               mLabelCompletedObjectives.unfocus();
               c = mObjectiveList3.getSelectedControl();
               if (c)
                  c->unfocus();

               c=NULL;
               if (mObjectiveList2.getNumVisibleControls()>0)
               {
                  mObjectiveList2.focus();
                  mLabelOptionalObjectives.focus();
                  c = mObjectiveList2.getControl(mObjectiveList2.getLastVisible());
               }
               else
               {
                  mObjectiveList1.focus();
                  mLabelPrimaryObjectives.focus();
                  c = mObjectiveList1.getControl(mObjectiveList1.getLastVisible());
               }
               if (c)
                  c->focus();
               showSelectedObjective();
               mLastViewChangeTime = timeGetTime();
            }
            break;
         }
         break;
      }

      case BUIControl::eStringEvent:
      {
         switch (control->getControlID())
         {
            case cControlIDPlayer1:
               if (event.getString() != "showGamerTag" )
                  break;
               // show the gamer tag for this player
               // if (gLiveSystem->isMultiplayerLiveGameActive())
               if (gDatabase.getGameSettingsNetworkType() == BGameSettings::cNetworkTypeLive)
                  XShowGamerCardUI(gUserManager.getPrimaryUser()->getPort(), mPlayer1.getXuid());
               handled=true;
               break;

            case cControlIDPlayer2:
               if (event.getString() != "showGamerTag")
                  break;
               // show the gamer tag for this player
               // if (gLiveSystem->isMultiplayerLiveGameActive())
               if (gDatabase.getGameSettingsNetworkType() == BGameSettings::cNetworkTypeLive)
                  XShowGamerCardUI(gUserManager.getPrimaryUser()->getPort(), mPlayer2.getXuid());
               handled=true;
               break;
         }
         break;
      }
   }

   return handled;
}



//============================================================================
//============================================================================
bool BUICampaignObjectives::displayButtons()
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


   if (mPlayer1.isFocused() || mPlayer2.isFocused())
   {
      // if we have a human and we are in MP and not in lan mode.
      //if (gLiveSystem->isMultiplayerLiveGameActive())
      if (gDatabase.getGameSettingsNetworkType() == BGameSettings::cNetworkTypeLive)
      {
         // The player is human by virtue that we are on live.
         b0 = BUIButtonBarControl::cFlashButtonA;
         s0.set(gDatabase.getLocStringFromID(25045));     // GAMERTAG
      }
   }

   b1=BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(25081));//L"BACK");

   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

   return true;
}

//============================================================================
//============================================================================
void BUICampaignObjectives::restoreMinimap()
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
void BUICampaignObjectives::showMinimap()
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


//============================================================================
//============================================================================
void BUICampaignObjectives::setVisible( bool visible )
{
   if (getVisible() && !visible)
      restoreMinimap();

   BUIScreen::setVisible(visible);
}

//============================================================================
//============================================================================
void BUICampaignObjectives::updateGameTime()
{
   // update the time label
   if (!gWorld)
      return;

   // don't update every time (try 100ms)
   DWORD gameTime=gWorld->getGametime();
   if ( (gameTime - mLastGameTimeUpdate) < 100 )
      return;

   mLastGameTimeUpdate=gameTime;

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

/*
   BUString temp;
   BPlayer* pPlayer = gWorld->getPlayer(1);        // any way to make this safer?
   if (pPlayer && pPlayer->isHuman())
   {
      BDifficultyType playerDifficulty = pPlayer->getDifficultyType();

      temp.format(L"%s   %s", gDatabase.getDifficultyStringByType(playerDifficulty).getPtr(), t.getPtr());
   }
   mDifficulty.setText(temp);
*/
}

//============================================================================
//============================================================================
void BUICampaignObjectives::update(float elapsedTime)
{
   if (!getVisible())
      return;

   updateGameTime();
}

//============================================================================
//============================================================================
void BUICampaignObjectives::enter( void )
{
   gUIManager->hideIngameObjectives(false);
   gUIManager->setMinimapVisible(true);
   if (!gLiveSystem->isMultiplayerGameActive())
      gModeManager.getModeGame()->setPaused(true);

   populateScreen();

   gUI.playConfirmSound();
}

//============================================================================
//============================================================================
void BUICampaignObjectives::leave( void )
{
   gUIManager->setWidgetsVisible( true );
   gUIManager->setMinimapVisible(false);
   if (!gLiveSystem->isMultiplayerGameActive())
      gModeManager.getModeGame()->setPaused(false);

   gUIManager->getMinimap()->setVisible(false);
}
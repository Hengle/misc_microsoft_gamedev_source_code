//============================================================================
// BUICpgSummaryPanelControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UICpgSummaryPanelControl.h"

#include "player.h"
#include "usermanager.h"
#include "user.h"
#include "world.h"
#include "objectivemanager.h"
#include "scoremanager.h"
#include "team.h"
#include "soundmanager.h"
#include "userprofilemanager.h"
#include "gamesettings.h"

//============================================================================
//============================================================================
BUICpgSummaryPanelControl::BUICpgSummaryPanelControl( void ) : 
mpLastObjectiveControlWithFocus(NULL),
mpLastObjectiveLabelWithFocus(NULL),
mLastViewChangeTime(0),
mpSelectedControl(NULL)
{
}

//============================================================================
//============================================================================
BUICpgSummaryPanelControl::~BUICpgSummaryPanelControl( void )
{
   // make sure we clean up.
   releaseControls();
}


//============================================================================
//============================================================================
bool BUICpgSummaryPanelControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
{
   bool result = false;
   bool initDone = false;

   // search the initData xml for the node we want to initialize with
   if (initData)
   {
      int numChildren = initData->getNumberChildren();
      for (int i = 0; i < numChildren; ++i)
      {
         BXMLNode node = initData->getChild(i);
         if (node.getName().compare("ControlList") == 0)
         {
            // Loop through the children of "ControlList"
            for (int j=0; j<node.getNumberChildren(); j++)
            {
               BXMLNode child(node.getChild(j));
               const BPackedString name(child.getName());

               if(name==B("BUICpgSummaryPanelControls"))
               {
                  initDone = true;
                  result = __super::init( parent, controlPath, controlID, &child );
                  break;
               }
            }
         }
         if (initDone)
            break;
      }
   }

   // if we didn't find our control set, then init with default.
   if (!initDone)
      result = __super::init( parent, controlPath, controlID, NULL );

      // Init the status label.
   mMissionStatusText.init(this, mScriptPrefix+"mMissionStatusText");

   // objective description title
   mDescription.init(this, mScriptPrefix+"mDescriptionLabel");
   mDescription.setText(gDatabase.getLocStringFromID(25677));     // Description

   // objective help text
   mHelpText.init(this, mScriptPrefix+"mHelpText", -1, NULL);



   // initialize our controls here
   mPlayer1.init(this, mScriptPrefix+"mPlayer1", -1, NULL);
   mPlayer2.init(this, mScriptPrefix+"mPlayer2", -1, NULL);
   mPlayer2.hide();

   mObjectiveList1Label.init(this, mScriptPrefix+"mIncompleteObjectiveTitle", -1, NULL);
   mObjectiveList1.init(this, mScriptPrefix+"mObjectiveList1", cControlIDList1, NULL);

   mObjectiveList2Label.init(this, mScriptPrefix+"mCompletedObjectiveTitle", -1, NULL);
   mObjectiveList2.init(this, mScriptPrefix+"mObjectiveList2", cControlIDList2, NULL);

   return result;
}

//============================================================================
//============================================================================
void BUICpgSummaryPanelControl::show( bool force /*= false*/ )
{
   if( mpSelectedControl )
      mpSelectedControl->focus( true );
   else
      selectFirstObjective();

   __super::show( force );
}

//============================================================================
//============================================================================
void BUICpgSummaryPanelControl::hide( bool force /*= false*/ )
{
   mpSelectedControl = NULL;

   if( mPlayer1.isFocused() )
      mpSelectedControl = &mPlayer1;
   else if( mPlayer2.isFocused() )
      mpSelectedControl = &mPlayer2;
   else if( mObjectiveList1.isFocused() )
      mpSelectedControl = &mObjectiveList1;
   else if( mObjectiveList2.isFocused() )
      mpSelectedControl = &mObjectiveList2;

   if( mpSelectedControl )
      mpSelectedControl->unfocus();

   __super::hide( force );
}

//============================================================================
//============================================================================
bool BUICpgSummaryPanelControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   //Dirty E3 hack to stop the double up/down tap when you switch between modes - eric
   if (((controlType==cStickLeftUp) ||
      (controlType==cStickLeftDown) ||
      (controlType==cDpadUp) ||
      (controlType==cDpadDown)) && 
      (timeGetTime()-mLastViewChangeTime < 200))
   {
      return true;
   }

   bool handled = false;

   if (mObjectiveList1.isFocused())
      handled = mObjectiveList1.handleInput(port, event, controlType, detail);
   if (handled)
      return true;

   if (mObjectiveList2.isFocused())
      handled = mObjectiveList2.handleInput(port, event, controlType, detail);
   if (handled)
      return true;

   if (!handled)
      handled = mHelpText.handleInput(port, event, controlType, detail);

   // Screen
   handled = BUIPanelControl::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   return handled;
}

//============================================================================
//============================================================================
bool BUICpgSummaryPanelControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
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

         // send a focus changed event up the change
         gSoundManager.playCue( "play_ui_menu_horizonal" );
         return true;
      }
   }
   else if (command=="right")
   {
      // if on objective list, go to player list
      if ( mObjectiveList1.isFocused() || 
           mObjectiveList2.isFocused() )
      {
         mpLastObjectiveControlWithFocus=NULL;
         mpLastObjectiveLabelWithFocus=NULL;
         if ( mObjectiveList1.isFocused())
         {
            mpLastObjectiveControlWithFocus=&mObjectiveList1;
            mpLastObjectiveLabelWithFocus=&mObjectiveList1Label;
         }
         else if ( mObjectiveList2.isFocused())
         {
            mpLastObjectiveControlWithFocus=&mObjectiveList2;
            mpLastObjectiveLabelWithFocus=&mObjectiveList2Label;
         }

         if (mpLastObjectiveControlWithFocus)
            mpLastObjectiveControlWithFocus->unfocus();

         if (mpLastObjectiveLabelWithFocus)
            mpLastObjectiveLabelWithFocus->unfocus();

         mPlayer1.focus();
         // send a focus changed event up the change
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
         // send a focus changed event up the change
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
         // send a focus changed event up the change
         return true;
      }
   }
   return false;
}

//==============================================================================
//==============================================================================
void BUICpgSummaryPanelControl::selectFirstObjective()
{
   BUIControl* c=NULL;

   // try first objective list
   if (mObjectiveList2.getNumVisibleControls()>0)
   {
      // remove focus from objective list 2 (if it has focus)
      mObjectiveList1.unfocus();
      mObjectiveList1Label.unfocus();
      c = mObjectiveList1.getSelectedControl();
      if (c)
         c->unfocus();

      mObjectiveList2.focus();
      mObjectiveList2Label.focus();
      c = mObjectiveList2.getControl(0);
      if (c)
         c->focus();
   }
   else if (mObjectiveList1.getNumVisibleControls()>0)
   {
      // remove focus from objective list 2 (if it has focus)
      mObjectiveList2.unfocus();
      mObjectiveList2Label.unfocus();
      c = mObjectiveList2.getSelectedControl();
      if (c)
         c->unfocus();

      mObjectiveList1.focus();
      mObjectiveList1Label.focus();
      c = mObjectiveList1.getControl(0);
      if (c)
         c->focus();
   }
}


// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUICpgSummaryPanelControl::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   BUIControl* c=NULL;

   switch (event.getID())
   {
      case BUIScrollingListControl::eItemSelected:
      {
         showSelectedObjective();
         return true;
      }

      case BUIScrollingListControl::eListBottomEvent:
      {
         // try to move the focus to the objective list below this one.
         switch(control->getControlID())
         {
         case cControlIDList2:

            if (mObjectiveList1.getNumVisibleControls()>0)
            {
               mObjectiveList2.unfocus();
               mObjectiveList2Label.unfocus();
               c = mObjectiveList2.getSelectedControl();
               if (c)
                  c->unfocus();

               c=NULL;
               if (mObjectiveList1.getNumVisibleControls()>0)
               {
                  mObjectiveList1.focus();
                  mObjectiveList1Label.focus();
                  c = mObjectiveList1.getControl(0);
               }
               if (c)
                  c->focus();

               showSelectedObjective();

               mLastViewChangeTime = timeGetTime();
               return true;
            }
            break;
         }
      }
      break;

      case BUIScrollingListControl::eListTopEvent:
      {
         // try to move the focus to the objective list above this one.
         switch(control->getControlID())
         {
            case cControlIDList1:
               if (mObjectiveList2.getNumVisibleControls()>0)
               {
                  mObjectiveList1.unfocus();
                  mObjectiveList1Label.unfocus();
                  c = mObjectiveList1.getSelectedControl();
                  if (c)
                     c->unfocus();
                  mObjectiveList2.focus();
                  mObjectiveList2Label.focus();
                  c = mObjectiveList2.getControl(mObjectiveList2.getLastVisible());
                  if (c)
                     c->focus();

                  showSelectedObjective();

                  mLastViewChangeTime = timeGetTime();
                  return true;
               }
               break;
         }
      }
      break;
   }

   return __super::handleUIControlEvent( event );
}

//============================================================================
//============================================================================
bool BUICpgSummaryPanelControl::populate()
{
   // basically set focus on the incomplete campaign objectives and go from there
   mPlayer1.unfocus();
   mPlayer2.unfocus();

   BUString text;
   text.set(gDatabase.getLocStringFromID(25069));//L"INCOMPLETE OBJECTIVES");
   mObjectiveList1Label.setText(text);

   text.set(gDatabase.getLocStringFromID(25070));//L"COMPLETE OBJECTIVES");
   mObjectiveList2Label.setText(text);

   populateMissionResult();

   populatePlayers();

   populateObjectives();

   selectFirstObjective();

   showSelectedObjective();

   return true;
}

//============================================================================
//============================================================================
void BUICpgSummaryPanelControl::populateMissionResult()
{
   BString keyFrame;
   keyFrame.set("");

   long gameType = -1;
   BGameSettings* pSettings = gDatabase.getGameSettings();
   BASSERT( pSettings );
   if( pSettings )
      pSettings->getLong( BGameSettings::cGameType, gameType );

   BPlayer* pPlayer = NULL;
   if( gameType == BGameSettings::cGameTypeCampaign )
      pPlayer = gWorld->getPlayer(1);        // any way to make this safer?
   else
   {
      BUser* pUser = gUIManager->getCurrentUser();
      BASSERT( pUser );
      pPlayer = pUser->getPlayer();
   }

   if (pPlayer && (pPlayer->isHuman() || gameType == BGameSettings::cGameTypeScenario)) // The player may be an "AI" in the Advanced Tutorial
   {
      keyFrame = pPlayer->getCiv()->getCivName();

      if (gUIManager->getScenarioResult() == BPlayer::cPlayerStateWon)
      {
         // Mission Complete
         mMissionStatusText.setText(gDatabase.getLocStringFromID(24007));
         keyFrame += " Win";
      }
      else
      {
         // Mission Failed.
         mMissionStatusText.setText(gDatabase.getLocStringFromID(24006));
         keyFrame += " Lose";
      }
   }

   if (keyFrame.length() > 0)
   {
      GFxValue value;
      value.SetString(keyFrame.getPtr());

      invokeActionScript("mEmblem.gotoAndStop", &value, 1);
   }
}

//============================================================================
//============================================================================
void BUICpgSummaryPanelControl::populatePlayer(BPlayer* player, BUIGamerTagLongControl& playerControl)
{
   if (!player)
      return;

//-- FIXING PREFIX BUG ID 1537
   const BUser* user=gUserManager.getUserByPlayerID(player->getID());
//--

   BString temp;
   BUString tempU;

   // Basic player info
   playerControl.setGamerTag(player->getLocalisedDisplayName());
   temp.format("img://gamerPic:%I64x", player->getXUID());
   playerControl.setGamerPic(temp);

   const BLeader* leader=player->getLeader();
   if (leader)
      playerControl.setLeaderPic(leader->getUIBackgroundImage());

   int controllerPort=-1;
   if (user)
      controllerPort=user->getPort();
   playerControl.setPort(controllerPort);

   // leader info
   /*
   temp.set("");
   playerControl.setLeaderPic(temp);
   */

   tempU.locFormat(gDatabase.getLocStringFromID(25325).getPtr(), gScoreManager.getFinalScore(player->getID()) );
   playerControl.setTitleText(tempU);

   BProfileRank rank(player->getRank());
   playerControl.setRank(rank.mRank);

   playerControl.setXuid( player->getXUID() );

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
void BUICpgSummaryPanelControl::populatePlayers()
{
/*
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser)
      populatePlayer(pUser->getPlayer(), mPlayer1);
*/
   long gameType = -1;
   BGameSettings* pSettings = gDatabase.getGameSettings();
   BASSERT( pSettings );
   if( pSettings )
      pSettings->getLong( BGameSettings::cGameType, gameType );

   BPlayer * player = NULL;

   if( gameType == BGameSettings::cGameTypeCampaign )
      player = gWorld->getPlayer(1);
   else
   {
      BUser* pUser = gUIManager->getCurrentUser();
      BASSERT( pUser );
      player = pUser->getPlayer();
   }

   BASSERT(player);
   if (!player)
      return;

   populatePlayer(player, mPlayer1);


   // do we have a coop player?
   BPlayer * coopPlayer = getCoopPlayer(player);
   if (coopPlayer)
   {
      mPlayer2.show();
      populatePlayer(coopPlayer, mPlayer2);
   }
   else
   {
      mPlayer2.hide();
   }

}

//============================================================================
//============================================================================
BPlayer * BUICpgSummaryPanelControl::getCoopPlayer(BPlayer* player)
{
   BASSERT(player);
   if (!player)
      return NULL;

   BPlayer * pCoopPlayer=NULL;
   if (gWorld->getFlagCoop())
   {
      BTeamID teamID = player->getTeamID();      // find the coop player
      BTeam * team = gWorld->getTeam(teamID);

      for (int i=0; i<team->getNumberPlayers(); i++)
      {
         pCoopPlayer = gWorld->getPlayer(team->getPlayerID(i));

         // if we found a player and it's not the primary user, then assume it's the coop player
         if (pCoopPlayer && (pCoopPlayer->getID() != player->getID()) && pCoopPlayer->isHuman())
            break;

         pCoopPlayer=NULL;    // just in case we didn't find a coop player
      }
   }

   return pCoopPlayer;
}


//============================================================================
//============================================================================
void BUICpgSummaryPanelControl::populateObjectives()
{
   releaseControls();

   long focusIndex = -1;
   mObjectiveList1.clearControls();
   mObjectiveList1.reset();
   mObjectiveList1Items.clear();
   mObjectiveList1.setMaxItems(cMaxObjectives);
   mObjectiveList1.setViewSize(8);

   mObjectiveList2.clearControls();
   mObjectiveList2.reset();
   mObjectiveList2Items.clear();
   mObjectiveList2.setMaxItems(cMaxObjectives);
   mObjectiveList2.setViewSize(4);

//-- FIXING PREFIX BUG ID 1538
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
   BSmallDynamicSimArray<int> completeObjectives;
   BSmallDynamicSimArray<int> incompleteObjectives;

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
         completeObjectives.add(i);      // add the index
      else
         incompleteObjectives.add(i);      // add the index
   }

   // fill out the list
   int slotIndex = 0;

   // add the complete objectives
   for (int i=0; i<completeObjectives.getNumber(); i++)
   {
      BObjective * pObj = pObjectiveManager->getObjective(completeObjectives[i]);
      if (!pObj)
         continue;
      int type = BUIObjectiveControl::cObjectiveTypePrimary;
      if (!pObj->isRequired())
         type = BUIObjectiveControl::cObjectiveTypeSecondary;
      addObjective(pObj->getDescription().getPtr(), 2, slotIndex++, completeObjectives[i], type, BUIObjectiveControl::cObjectiveStatusCompleted);
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

   slotIndex = 0;

   // add the incomplete objectives
   for (int i=0; i<incompleteObjectives.getNumber(); i++)
   {
      BObjective * pObj = pObjectiveManager->getObjective(incompleteObjectives[i]);
      if (!pObj)
         continue;
      if (focusIndex==-1)
         focusIndex=slotIndex;
      int type = BUIObjectiveControl::cObjectiveTypePrimary;
      if (!pObj->isRequired())
         type = BUIObjectiveControl::cObjectiveTypeSecondary;
      addObjective(pObj->getDescription().getPtr(), 1, slotIndex++, incompleteObjectives[i], type, BUIObjectiveControl::cObjectiveStatusFailed);
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

   slotIndex=0;


   mObjectiveList2.setIndex(focusIndex);
   mObjectiveList2.focus();
   mObjectiveList2Label.focus();

   mObjectiveList1.unfocus();
   mObjectiveList1.setIndex(-1);
   mObjectiveList1Label.unfocus();
}

//============================================================================
//============================================================================
void BUICpgSummaryPanelControl::releaseControls()
{
   for (int i=0; i<mControls.getNumber(); i++)
   {
      delete mControls[i];
      mControls[i]=NULL;
   }
   mControls.clear();
}

//============================================================================
//============================================================================
bool BUICpgSummaryPanelControl::addObjective(const WCHAR* menuText, int listID, int index, int controlID, int type, int status)
{
   BSimString controlPath;
   controlPath.format("%smObjectiveList%d.mObjective%d", mScriptPrefix.getPtr(), listID, index);
   BUString text;
   text.set(menuText);

   BUIObjectiveControl *c = new BUIObjectiveControl();
   mControls.add(c); // add for easy cleanup later.

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
         mObjectiveList1Items.add(c);
         break;
      case 2:
         mObjectiveList2.addControl(c);
         mObjectiveList2Items.add(c);
         break;
   }
   return true;
}

//============================================================================
//============================================================================
void BUICpgSummaryPanelControl::showSelectedObjective()
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
void BUICpgSummaryPanelControl::updateRScrollButton()
{
   getMovie()->invokeActionScript( "updateRScrollButton", NULL, 0);

   // this method is on the base movie and not in my control.
   // invokeActionScript( "updateRScrollButton");
}

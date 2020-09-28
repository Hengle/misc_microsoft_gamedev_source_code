#ifdef 
#endif
//============================================================================
// UISkirmishPostGameScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UISkirmishPostGameScreen.h"

#include "configsgame.h"
#include "UITabControl.h"
#include "UIPanelControl.h"
#include "UILabelControl.h"
#include "UIGridCellControl.h"
#include "user.h"
#include "modemanager.h"
#include "render.h"
#include "FontSystem2.h"
#include "protosquad.h"
#include "prototech.h"

#include "scoremanager.h"
#include "statsManager.h"
#include "world.h"
#include "uimanager.h"
#include "LiveSystem.h"
#include "userprofilemanager.h"
#include "gamesettings.h"

//============================================================================
//============================================================================
BUISkirmishPostGameScreen::BUISkirmishPostGameScreen( void ) : 
   mShowingStats(false),
   mStatsViewed(false),
   mpGridControlScore(NULL),
   mpGridControlMilitary(NULL),
   mpGridControlEconomy(NULL),
   mpGridControlTimeline(NULL),
   mCurrentTimeline(cStatsTimelineScore),
   mGraphFadeTime(0.0f),
   mLastUpdateGameTime(0)
{
   for (int i=0; i<cStatsTimelineCount; i++)
   {
      mTimelineHandles[i]=cInvalidGraphHandle;
   }
}

//============================================================================
//============================================================================
BUISkirmishPostGameScreen::~BUISkirmishPostGameScreen( void )
{
   for( int i = 0; i < mControls.getNumber(); ++i )
   {
      delete mControls[i];
   }
   mControls.clear();

   if( gConfig.isDefined( "ShowPostGameTimeline" ) )
   {
      for (int i=0; i<cStatsTimelineCount; i++)
      {
         if (mTimelineHandles[i]!=cInvalidGraphHandle)
         {
            gGraphManager.destroyGraph(mTimelineHandles[i]);
            mTimelineHandles[i]=cInvalidGraphHandle;
         }
      }
   }
}

//============================================================================
//============================================================================
bool BUISkirmishPostGameScreen::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   // grab the location of the minimap for 4x3 and 16x9
   int numChildren = dataNode.getNumberChildren();
   for (int i = 0; i < numChildren; ++i)
   {
      BXMLNode node = dataNode.getChild(i);
      if (node.getName().compare("TimelineLocation") == 0)
      {
         BSimString value;
         if (!node.getAttribValueAsString("type", value))
            continue;

         int index = -1;
         if (value=="16x9")
            index = cTimelineLocation16x9;
         else if (value=="4x3")
            index = cTimelineLocation4x3;

         if (index<0)
            continue;

         node.getAttribValueAsFloat("x", mTimelinePostions[index].mX);
         node.getAttribValueAsFloat("y", mTimelinePostions[index].mY);
         node.getAttribValueAsFloat("w", mTimelinePostions[index].mWidth);
         node.getAttribValueAsFloat("h", mTimelinePostions[index].mHeight);
      }
   }

   BXMLNode controlsNode;
   if (dataNode.getChild( "UIScrollableCallout", &controlsNode ) )
      mCallout.init(this, "mCallout", -1, &controlsNode);
   else
   {
      BASSERTM(false, "UISkirmishPostgameScreen:: Unable to initialize mCallout control input handler.");
      mCallout.init(this, "mCallout");    // use default
   }
   mCallout.hide(true);

   // initialize all the components for our screen
   mTabControl.init(this, "");      // no visual, so leave the path empty.

   initPlayers(dataNode);
   initPanels();
   initGrids();

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   mTeam1Label.init( this, "mPlayerContainer.mTeam1Label" );
   mTeam2Label.init( this, "mPlayerContainer.mTeam2Label" );

   mColumnHeader.init(this, "mColumnHeader");

/*
   // SRL - pulled - phx-16437
   mServiceRecord.init( "art\\ui\\flash\\pregame\\servicerecord\\uiservicerecord.gfx", "art\\ui\\flash\\pregame\\servicerecord\\uiservicerecord.xml" );
   mServiceRecord.setHandler( this );
*/
   
   return true;
}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::initPlayers(BXMLNode dataNode)
{
   BXMLNode controlsNode;
   bool foundNode=dataNode.getChild( "UIGamerTagControl", &controlsNode );
   if (!foundNode)
   {
      BASSERTM(false, "UISkirmishPostgameScreen:: Unable to initialize player control input handlers.");
   }

   BString temp;
   for (int i=0; i<cMaxPlayers; i++)
   {
      temp.format("mPlayerContainer.mPlayer%d", i);
      if (foundNode)
         mPlayers[i].init(this, temp.getPtr(), -1, &controlsNode);
      else
         mPlayers[i].init(this, temp.getPtr());
   }
}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::initPanels()
{
   BUIPanelControl* panel=NULL;

   // add the tabs
   panel = new BUIPanelControl();
   panel->init(this, "mTabScore");
   addTab("mTabLabelScore",      panel,   gDatabase.getLocStringFromID(25052));//L"Score");

   panel = new BUIPanelControl();
   panel->init(this, "mTabMilitary");
   addTab("mTabLabelMilitary",   panel,   gDatabase.getLocStringFromID(25053));//L"Military");

   panel = new BUIPanelControl();
   panel->init(this, "mTabEconomy");
   addTab("mTabLabelEconomy",    panel,   gDatabase.getLocStringFromID(25054));//L"Economy");

   if( gConfig.isDefined( "ShowPostGameTimeline" ) )
   {
      panel = new BUIPanelControl();
      panel->init(this, "mTabTimeline");
      addTab("mTabLabelTimeline",   panel,   gDatabase.getLocStringFromID(25055));//L"Timelines");
   }
   else
   {
      GFxValue val;
      val.SetBoolean( false );
      getMovie()->setVariable( "mTabLabelTimeline._visible", val );
   }
}

void initSingleGrid(BUIGridControl* pGrid);


//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::initGrids()
{
   // init the score tab
   mpGridControlScore = new BUIGridControl(cGridRows, cGridColumns-1);
   mpGridControlScore->init(this, "", -1, NULL);
   mControls.add(mpGridControlScore);
   initSingleGrid(mpGridControlScore, "mTabScore");

   // init the military tab
   mpGridControlMilitary = new BUIGridControl(cGridRows, cGridColumns);
   mpGridControlMilitary->init(this, "", -1, NULL);
   mControls.add(mpGridControlMilitary);
   initSingleGrid(mpGridControlMilitary, "mTabMilitary");

   // init the economy tab
   mpGridControlEconomy = new BUIGridControl(cGridRows, cGridColumns);
   mpGridControlEconomy->init(this, "", -1, NULL);
   mControls.add(mpGridControlEconomy);
   initSingleGrid(mpGridControlEconomy, "mTabEconomy");

   // init the timeline tab (one column, same number of rows)
   if( gConfig.isDefined( "ShowPostGameTimeline" ) )
   {
      mpGridControlTimeline = new BUIGridControl(cGridRows, 1);
      mpGridControlTimeline->init(this, "", -1, NULL);
      mControls.add(mpGridControlTimeline);
      initSingleGrid(mpGridControlTimeline, "mTabTimeline");
   }
   
}


//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::initSingleGrid(BUIGridControl* pGrid, const char* pCellPrefix)
{
   if (!pGrid)
      return;

   // set up the players in the grid control
   for (int row = 0; row<pGrid->getRowCount(); row++)
      pGrid->setControl(&mPlayers[row], row, 0);

   BString temp;
   BUIGridCellControl* pGridCell;

   // set up the grid controls for each of the grids
   for (int row=0; row<pGrid->getRowCount(); row++)
   {
      for (int col=1; col<pGrid->getColumnCount(); col++)
      {
         // add the cell
         temp.format("%s.mCell%d%d", pCellPrefix, row, col);
         pGridCell=new BUIGridCellControl();
         pGridCell->init(this, temp.getPtr(), -1, NULL);

         // add it 
         pGrid->setControl(pGridCell, row, col);
         mControls.add(pGridCell);
      }
   }
}

//==============================================================================
// BUISkirmishPostGameScreen::update
//==============================================================================
void BUISkirmishPostGameScreen::update(float elapsedTime)
{   
   if (mTabControl.getActiveTab()==cSkirmishPostgameTabTimeline)
   {
      mGraphFadeTime+=elapsedTime;
      gGraphManager.updateRenderThread(mGraphFadeTime);
   }
}


//==============================================================================
//==============================================================================
void BUISkirmishPostGameScreen::render()
{
/*
// SRL - pulled - phx-16437
   if( mServiceRecord.getVisible() )
      mServiceRecord.render();
   else
   {
*/
      __super::render();

      if (mTabControl.getActiveTab()==cSkirmishPostgameTabTimeline)
      {
         renderTimelineTab();
      }
/*
   }
*/
}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::populate()
{
   // allow us to play with this outside of the game mode.
   if (gModeManager.getModeType() == BModeManager::cModeGame)
   {
      // fill in the players
      populatePlayers();
      populateScoreTab();
      populateMilitaryTab();
      populateEconomyTab();
      
      if( gConfig.isDefined( "ShowPostGameTimeline" ) )
         populateTimelineTab();
   }

   // fill in the player
   mTabControl.setActiveTab(0);

   // focus on the first cell;
   mpGridControlScore->setSelectedCell(0,1);
   mpGridControlMilitary->setSelectedCell(0,1);
   mpGridControlEconomy->setSelectedCell(0,1);
   if( gConfig.isDefined( "ShowPostGameTimeline" ) )
      mpGridControlTimeline->setSelectedCell(0, 0);      // first player

   fixupPlayerFocus();

   checkColumnTitle();

   displayButtons();
}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::populatePlayers()
{
   int count = gStatsManager.getNumberPlayers();

   BDynamicArray<BPlayer*> alphaTeam;
   BDynamicArray<BPlayer*> bravoTeam;
   BDynamicArray<BPlayer*>* teams[2];

   BPlayer* pPlayer = NULL;
   int winningTeamID=0;
   for (long i=1; i<=6 && i < count; i++)        // start at 1, skip Gaia
   {
      BStatsPlayer* pPlayerStats = gStatsManager.getStatsPlayer(i);
      if (!pPlayerStats)
         continue;

      if (pPlayerStats->getPlayerType() == BPlayer::cPlayerTypeNPC)
         continue;

      pPlayer = pPlayerStats->getPlayer();
      if (pPlayer)
      {
         if (winningTeamID == 0 && pPlayerStats->isWon() && gWorld->getFlagGameOver())
            winningTeamID = pPlayer->getTeamID();

         if (pPlayer->getTeamID() == 1)
            alphaTeam.add(pPlayer);
         else
            bravoTeam.add(pPlayer);
      }
   }

   /*
   http://esbug01/browse/PHX-2421

   TCR # 076   SA Stats Reporting for Competitive Sessions
   Requirement Games that support the concept of competitive gameplay for both ranked and standard sessions must report a 
               relative score before the end of the session. Games must rank the player in relation to the number of players 
               in the session with the highest ranking going to the winner.
   Remarks     Games can report the ranking by setting X_PROPERTY_RELATIVE_SCORE and X_PROPERTY_SESSION_TEAM for each gamer 
               in the session before calling the XSessionEnd() function. To set these properties, games use the XSessionsWriteStats() function.

   Needed to pass cert based on our Alpha.
   
   From the cert team:

   The TCR is referring to what the user’s see and how the stats are being reported. While on the backend it is calling the necessary
   APIs for posting to the leaderboards it is failing to report to the gamer a relative score and a rank. While in-game it would be
   acceptable to not show a score but at the end of the match/session the game needs to inform the player of the score and rank, whether
   it is an individual player or team based scoring system.

   Turns out that to follow the letter of the TCR – it’s 100% about complying on the backend.
   To comply with the official test case of the TCR, the tester does indeed verify the UI in-game...

   This is to "guarantee a pass in compliance, and a pass in functional" certification testing.
   */

   const BUString& winnerStr = gDatabase.getLocStringFromID(25999);
   const BUString& loserStr = gDatabase.getLocStringFromID(26000);
   const BUString& alphaStr = gDatabase.getLocStringFromID(25296);
   const BUString& bravoStr = gDatabase.getLocStringFromID(25297);

   switch( winningTeamID )
   {
      case 0:  // No Winner yet
      {
         mTeam1Label.setText( alphaStr );
         mTeam2Label.setText( bravoStr );
         teams[0] = &alphaTeam;
         teams[1] = &bravoTeam;
         break;
      }

      case 1:  // Alpha Team won
      {
         mTeam1Label.setText( winnerStr );
         mTeam2Label.setText( loserStr );
         teams[0] = &alphaTeam;
         teams[1] = &bravoTeam;
         break;
      }

      case 2:  // Bravo Team won
      {
         mTeam1Label.setText( winnerStr );
         mTeam2Label.setText( loserStr );
         teams[0] = &bravoTeam;
         teams[1] = &alphaTeam;
         break;
      }
   }
   
   // now actually go through and put the players on the screen
   for (uint i=0, slot=0; i < 2; ++i)
   {
      for (uint j=0; j < 3; ++j, ++slot)
      {
         if (j < teams[i]->getSize())
         {
            populatePlayer(teams[i]->get(j), mPlayers[slot]);
            mPlayers[slot].show();
         }
         else
         {
            mPlayers[slot].hide();
            mPlayers[slot].setPlayerID(-1);
         }
      }
   }
}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::populatePlayer(BPlayer* pPlayer, BUIGamerTagControl& playerControl)
{
   if (!pPlayer)
      return;

   playerControl.setPlayerID(pPlayer->getID());

   BUser* pUser = gUserManager.getUserByPlayerID(pPlayer->getID());

   BString temp;
   BUString tempU;

   // Basic player info
   playerControl.setGamerTag(pPlayer->getLocalisedDisplayName());
   temp.format("img://gamerPic:%I64x", pPlayer->getXUID());
   playerControl.setGamerPic(temp);
   
   if( pPlayer->getCiv() )
      playerControl.setCiv( pPlayer->getCiv()->getCivName() );

   if( pPlayer->getLeader() )
   {
      BLeader* pLeader = pPlayer->getLeader();
      BUString leaderName = gDatabase.getLocStringFromIndex(pLeader->mNameIndex).getPtr();
      playerControl.setTitleText( leaderName );
   }

   int controllerPort=-1;
   if (pUser)
      controllerPort = pUser->getPort();
   playerControl.setPort(controllerPort);

   playerControl.setPlayerID(pPlayer->getID());

   DWORD playerColor = gWorld->getPlayerColor(pPlayer->getID(), BWorld::cPlayerColorContextUI);
   playerControl.setPlayerColor(playerColor);

   BProfileRank rank(pPlayer->getRank());
   playerControl.setRank(rank.mRank);

   playerControl.setStatus(pPlayer->getPlayerState());
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
void BUISkirmishPostGameScreen::populateScoreTab()
{
   BUIGridCellControl* control=NULL;
   BUString cell;

   BSmallDynamicSimArray<BScorePlayer> players;

   gScoreManager.queryPlayers(players);

   uint count = players.getSize();

   for (int i=0; i<cMaxPlayers; i++ )
   {
      if (mPlayers[i].getPlayerID()>=0)
      {
         long playerID = mPlayers[i].getPlayerID();

//-- FIXING PREFIX BUG ID 1762
         const BScorePlayer* pScorePlayer = NULL;
//--

         for (uint j=0; j < count; ++j)
         {
            if (players[j].mPlayerID == playerID)
            {
               pScorePlayer = &players[j];
               break;
            }
         }

         if (pScorePlayer == NULL)
         {
            hideRow(mpGridControlScore, i);
            continue;
         }

         // populate this row of data for the score
         // cell 1 - Base Score
         control = static_cast<BUIGridCellControl*>(mpGridControlScore->getControl(i, 1));
         if (control)
         {
            cell.locFormat(L"%d", pScorePlayer->mBaseScore);
            control->setText(cell);
         }

         // cell 2 - Bonus
         control = static_cast<BUIGridCellControl*>(mpGridControlScore->getControl(i, 2));
         if (control)
         {
            //11/17/2008 - We use a custom function to format float values so that we can 
            //use different decimal separators in localized builds.  This was bug 12464
            wchar_t floatTemp[20];

            // calc score bonus
            float scoreBonus = (1.0f + pScorePlayer->mCombatBonus + pScorePlayer->mTimeBonus);                 // factor in combat and time bonuses
            if(pScorePlayer->mFinalBaseScore > 0)
               scoreBonus += (long)((float)pScorePlayer->mFinalSkullBonus / pScorePlayer->mFinalBaseScore);    // calculate and factor in skull bonus
            scoreBonus *= (1.0f + pScorePlayer->mMPBonus);                                                     // increase by multiplayer bonus

            // format and set
            locFormatFloat(floatTemp, 20, scoreBonus, 2, gDatabase.getLocStringFromID(25996));
            cell.locFormat(L"%s X", floatTemp);
            control->setText(cell);
         }

         // cell 3 - Total 
         control = static_cast<BUIGridCellControl*>(mpGridControlScore->getControl(i, 3));
         if (control)
         {
            cell.locFormat(L"%d", pScorePlayer->mFinalScore);
            control->setText(cell);
         }

         // cell 4 - Place
         control = static_cast<BUIGridCellControl*>(mpGridControlScore->getControl(i, 4));
         if (control)
         {
            // PLACE is 1st, 2nd, 3rd, etc...
            // PLACE string IDs are 24708-24713
            if (!gWorld->getFlagGameOver())
               cell.empty();
            else if (pScorePlayer->mFinalScore == 0)
               cell.locFormat(L"-");
            else if (j >= 0 && j < cMaxPlayers)
               cell = gDatabase.getLocStringFromID(24708+j);
            else
               cell.empty();
            control->setText(cell);
         }

         // XXX Are we creating awards for skirmish games?
         // cell 5 - Number of awards earned
/*
         control = static_cast<BUIGridCellControl*>(mpGridControlScore->getControl(i, 5));
         if (control)
         {
            cell.format(L"0");
            control->setText(cell);
         }
*/
      }
      else
      {
         hideRow(mpGridControlScore, i);
      }
   }
}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::hideRow(BUIGridControl* pGridControl, int row)
{
   BUIControl* cell = NULL;
   // turn this row of data off
   for (int c=1; c<pGridControl->getColumnCount(); c++)
   {
      cell = pGridControl->getControl(row, c);
      if (cell)
         cell->hide();
   }
}


//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::populateMilitaryTab()
{
   BUIGridCellControl* control=NULL;
   for (int i=0; i<cMaxPlayers; i++ )
   {
      if (mPlayers[i].getPlayerID()>=0)
      {
         BStatsPlayer* pPlayerStats = gStatsManager.getStatsPlayer(mPlayers[i].getPlayerID());
         if (!pPlayerStats)
            continue;

         BPlayer* pPlayer = pPlayerStats->getPlayer();

         BStatRecorderTotal* pStatsMilitaryTotal = reinterpret_cast<BStatRecorderTotal*>(pPlayerStats->getStatsRecorder("MilitarySquadTotals2"));
         BStatRecorderTotal* pStatsBuildingsTotal = reinterpret_cast<BStatRecorderTotal*>(pPlayerStats->getStatsRecorder("BuildingTotals2"));

         // squad data
         int squadsBuilt = pStatsMilitaryTotal->getStatTotal().mBuilt;
         int squadsLost = pStatsMilitaryTotal->getStatTotal().mLost;
         int squadsKilled = pStatsMilitaryTotal->getStatTotal().mDestroyed;

         // building data
         int buildingsBuilt = pStatsBuildingsTotal->getStatTotal().mBuilt;
         int buildingsLost = pStatsBuildingsTotal->getStatTotal().mLost;
         int buildingKilled = pStatsBuildingsTotal->getStatTotal().mDestroyed;

         // callout strings
         BUString calloutSquadsBuilt;
         BUString calloutSquadsDestroyed;
         calloutSquadsBuilt.set(L"");
         calloutSquadsDestroyed.set(L"");
         BUString temp;

         const BStatRecorder::BStatTotalHashMap& squads = pStatsMilitaryTotal->getTotalHashMap();
         for (BStatRecorder::BStatTotalHashMap::const_iterator it = squads.begin(); it != squads.end(); ++it)
         {
            const BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(it->first);

            // This is where we have issues with squads like ODSTs, Hawks, Grizzlies and what happens when they're jacked
            // the stats system doesn't track each individual unit, it only rolls up on a protoID basis.
            //
            // If we use the display name of the BProtoSquad from the player, we'll get the appropriate upgraded squad name (i.e. Hawk)
            // but if they destroy another player's Hornet then it will show them as destroying a Hawk.
            //
            if (pProtoSquad)
            {
               BUString name;
               pProtoSquad->getStatsName(name);

               if ((it->second.mBuilt>0) || (it->second.mLost>0))
               {
                  temp.locFormat(L"%s: %d (%d)<br>", name.getPtr(), it->second.mBuilt, it->second.mLost);
                  calloutSquadsBuilt.append(temp.getPtr());
               }

               if (it->second.mDestroyed > 0)
               {
                  temp.locFormat(L"%s: %d<br>", name.getPtr(), it->second.mDestroyed);
                  calloutSquadsDestroyed.append(temp.getPtr());
               }
            }
         }

         BUString cell;
         // cell 1
         control = static_cast<BUIGridCellControl*>(mpGridControlMilitary->getControl(i, 1));
         if (control)
         {
            cell.locFormat(L"%d (%d)", squadsBuilt, squadsLost);
            control->setText(cell);
            if (calloutSquadsBuilt.length() > 0)
            {
               control->setDetailIndicator(true);
               control->setCalloutTitle(gDatabase.getLocStringFromID(25587));
               control->setCalloutText(calloutSquadsBuilt);
            }
         }

         // cell 2
         control = static_cast<BUIGridCellControl*>(mpGridControlMilitary->getControl(i, 2));
         if (control)
         {
            cell.locFormat(L"%d", squadsKilled);
            control->setText(cell);
            if (calloutSquadsDestroyed.length()>0)
            {
               control->setDetailIndicator(true);
               control->setCalloutTitle(gDatabase.getLocStringFromID(25588));
               control->setCalloutText(calloutSquadsDestroyed);
            }
         }

         BUString calloutBuildingsBuilt;
         BUString calloutBuildingsDestroyed;
         calloutBuildingsBuilt.set(L"");
         calloutBuildingsDestroyed.set(L"");

         const BStatRecorder::BStatTotalHashMap& buildings = pStatsBuildingsTotal->getTotalHashMap();
         for (BStatRecorder::BStatTotalHashMap::const_iterator it = buildings.begin(); it != buildings.end(); ++it)
         {
//-- FIXING PREFIX BUG ID 1764
            const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(it->first);
//--

            if (pProtoObject)
            {
               BUString name;
               pProtoObject->getStatsName(name);

               if ( (it->second.mBuilt>0) || (it->second.mLost>0))
               {
                  temp.locFormat(L"%s: %d (%d)<br>", name.getPtr(), it->second.mBuilt, it->second.mLost);
                  calloutBuildingsBuilt.append(temp.getPtr());
               }
               if (it->second.mDestroyed > 0)
               {
                  temp.locFormat(L"%s: %d<br>", name.getPtr(), it->second.mDestroyed);
                  calloutBuildingsDestroyed.append(temp.getPtr());
               }
            }
         }

         // cell 3
         control = static_cast<BUIGridCellControl*>(mpGridControlMilitary->getControl(i, 3));
         if (control)
         {
            cell.locFormat(L"%d (%d)", buildingsBuilt, buildingsLost);
            control->setText(cell);
            if (calloutBuildingsBuilt.length()>0)
            {
               control->setDetailIndicator(true);
               control->setCalloutTitle(gDatabase.getLocStringFromID(25589));
               control->setCalloutText(calloutBuildingsBuilt);
            }
         }

         // cell 4
         control = static_cast<BUIGridCellControl*>(mpGridControlMilitary->getControl(i, 4));
         if (control)
         {
            cell.locFormat(L"%d", buildingKilled);
            control->setText(cell);
            if (calloutBuildingsDestroyed.length()>0)
            {
               control->setDetailIndicator(true);
               control->setCalloutTitle(gDatabase.getLocStringFromID(25590));
               control->setCalloutText(calloutBuildingsDestroyed);
            }
         }

         // cell 5 - combat effectiveness
         control = static_cast<BUIGridCellControl*>(mpGridControlMilitary->getControl(i, 5));
         if (control)
         {
            cell.locFormat(L"%d", static_cast<int>(gScoreManager.getCombatBonusPercent(pPlayer->getID()) + 0.5f));
            control->setText(cell);
         }
      }
      else
      {
         hideRow(mpGridControlMilitary, i);
      }
   }

}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::populateEconomyTab()
{
   BUIGridCellControl* control=NULL;
   for (int i=0; i<cMaxPlayers; i++ )
   {
      if (mPlayers[i].getPlayerID()>=0)
      {
         BStatsPlayer* pPlayerStats = gStatsManager.getStatsPlayer(mPlayers[i].getPlayerID());
         if (!pPlayerStats)
            continue;

         BUString techsResearchedCallout;
         techsResearchedCallout.set(L"");

         BStatRecorderTotal* pStatsTechTotal = reinterpret_cast<BStatRecorderTotal*>(pPlayerStats->getStatsRecorder("ResearchTotals"));
         const BStatRecorder::BStatTotalHashMap& techs = pStatsTechTotal->getTotalHashMap();
         for (BStatRecorder::BStatTotalHashMap::const_iterator it = techs.begin(); it != techs.end(); ++it)
         {
//-- FIXING PREFIX BUG ID 1767
            const BProtoTech* pt = gDatabase.getProtoTech(it->first);
//--
            if (pt)
            {
               // [10/27/2008 xemu] filter out "hidden" techs
               if (!pt->getHiddenFromStats())
               {
                  BUString name;
                  pt->getDisplayName(name);

                  if (it->second.mResearched>0)
                  {
                     BUString temp;
                     temp.locFormat(L"%s: %d<br>", name.getPtr(), it->second.mResearched);
                     techsResearchedCallout.append(temp.getPtr());
                  }
               }
            }
         }

         // supplies gathered, crates gathered, max bases, max tech level (graphic), techs researched
         long suppliesID = gDatabase.getRIDSupplies();
         long powerID = gDatabase.getRIDPower();
         int suppliesGathered = (int)ceil(pPlayerStats->getTotalResources().get(suppliesID));
         int cratesGathered = (int)ceil(pPlayerStats->getGatheredResources().get(suppliesID));
         int maxPower = (int)ceil(pPlayerStats->getMaxResources().get(powerID));

//-- FIXING PREFIX BUG ID 1769
         const BStatRecorderEvent* pStatsBaseEvents = reinterpret_cast<BStatRecorderEvent*>(pPlayerStats->getStatsRecorder("BaseEvents"));
//--
         uint maxBases = 0;
         if (pStatsBaseEvents)
            maxBases = pStatsBaseEvents->getStatTotal().mBuilt;

         int techsResearched = pStatsTechTotal->getStatTotal().mResearched;

         BUString cell;

         // cell 1 - supplies gathered
         control = static_cast<BUIGridCellControl*>(mpGridControlEconomy->getControl(i, 1));
         if (control)
         {
            cell.locFormat(L"%d", suppliesGathered);
            control->setText(cell);
         }

         // cell 2 - crates gathered
         control = static_cast<BUIGridCellControl*>(mpGridControlEconomy->getControl(i, 2));
         if (control)
         {
            cell.locFormat(L"%d", cratesGathered);
            control->setText(cell);
         }

         // cell 3 - max bases
         control = static_cast<BUIGridCellControl*>(mpGridControlEconomy->getControl(i, 3));
         if (control)
         {
            cell.locFormat(L"%u", maxBases);
            control->setText(cell);
         }

         // cell 4 - max tech level
         control = static_cast<BUIGridCellControl*>(mpGridControlEconomy->getControl(i, 4));
         if (control)
         {
            cell.locFormat(L"%d", maxPower);
            control->setText(cell);
         }

         // cell 5 - techs researched
         control = static_cast<BUIGridCellControl*>(mpGridControlEconomy->getControl(i, 5));
         if (control)
         {
            cell.locFormat(L"%d", techsResearched);
            control->setText(cell);

            if (techsResearchedCallout.length()>0)
            {
               control->setDetailIndicator(true);
               control->setCalloutTitle(gDatabase.getLocStringFromID(25596));
               control->setCalloutText(techsResearchedCallout);
            }
         }
      }
      else
      {
         hideRow(mpGridControlEconomy, i);
      }
   }
}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::populateTimelineTab()
{
   uint8 resourcesUsed=0;
   uint32 minX;
   uint32 maxX;
   float minY;
   float maxY;
   
   for (int t=0; t<cStatsTimelineCount; t++)
   {
      // generate the timelines
      BGraphHandle hGraph = gGraphManager.createGraph();
      if (hGraph == cInvalidGraphHandle)
         continue;

      mTimelineHandles[t]=hGraph;

      BGraphAttribs* pAttribs = gGraphManager.getGraph(hGraph);
      if (pAttribs)
      {
         int index = cTimelineLocation16x9;
         if (gRender.getAspectRatioMode()==BRender::cAspectRatioMode4x3)
            index = cTimelineLocation4x3;

         float x = mTimelinePostions[index].mX;
         float y = mTimelinePostions[index].mY;
         float z = 0.0f;
         float width  = mTimelinePostions[index].mWidth;
         float height = mTimelinePostions[index].mHeight;

         bool bUseDots = true;
         if (bUseDots)
            pAttribs->setType(BGraphAttribs::cLineGraphDots);
         else
            pAttribs->setType(BGraphAttribs::cLineGraphNormal);

         pAttribs->setPosition(BVec3(x, y, z));
         pAttribs->setSizeX(width);
         pAttribs->setSizeY(height);
         pAttribs->setBackgroundColor(cDWORDPurple);            
         pAttribs->setEnabled(false);

         pAttribs->setHorizontalSmallTickSizeX(2.0f);
         pAttribs->setHorizontalSmallTickSizeY(5.0f);
         pAttribs->setHorizontalLargeTickSizeX(2.0f);
         pAttribs->setHorizontalLargeTickSizeY(10.0f);
         pAttribs->setHorizontalSmallTickInterval(5);
         pAttribs->setHorizontalLargeTickInterval(25);
      }
   }

   BDynamicArray<float> supplyPoints;
   BDynamicArray<float> powerPoints;
   BDynamicArray<float> basePoints;
   BDynamicArray<float> scorePoints;
   BDynamicArray<float> popPoints;   
   long suppliesID = gDatabase.getRIDSupplies();
   long powerID = gDatabase.getRIDPower();
   BSmallDynamicSimArray<BStatRecorder::BStatResources> resourcePoints;
   BSmallDynamicSimArray<BStatRecorder::BStatEventFloat> popData;
   BSmallDynamicSimArray<BStatRecorder::BStatEventUInt32> baseData;
   BSmallDynamicSimArray<BStatRecorder::BStatEventInt32> scoreData;


   for (int i=0; i<cMaxPlayers; i++ )
   {
      if (mPlayers[i].getPlayerID()>=0)
      {
         BStatsPlayer* pPlayerStats = gStatsManager.getStatsPlayer(mPlayers[i].getPlayerID());
         if (!pPlayerStats)
            continue;

         BColor playerColor = cColorWhite;
         BPlayer* pPlayer = pPlayerStats->getPlayer();
         if (pPlayer)
         {
            // playerColor = gDatabase.getPlayerColor(pPlayer->getColorIndex()).mColor1;
            playerColor = gWorld->getPlayerColor(pPlayer->getID(), BWorld::cPlayerColorContextUI);
         }

#ifndef BUILD_FINAL
         if (gConfig.isDefined(cConfigUseGraphTestData))
         {
            static int  sampleCount = 15;
            for (int k = 0; k < cStatsTimelineCount; ++k)
            {
               BDynamicArray<float> mPoints;
               for (int j = 0; j < sampleCount; j++)
               {
                  float value = getRandRangeFloat(cUnsyncedRand, 0.0f, 200.0f);
                  mPoints.add(value);
               }

               addTimeline(k, mPoints, playerColor.asDWORD(), mPlayers[i].getPlayerID());
            }
            continue;
         }
#endif

         resourcePoints.clear();
         pPlayerStats->queryResourceGraph(resourcesUsed, minX, maxX, minY, maxY, resourcePoints);

         supplyPoints.clear();
         powerPoints.clear();
         for (int p=0; p<resourcePoints.getNumber(); p++)
         {
            float supplies = resourcePoints[p].mCost.get(suppliesID);
            float power = resourcePoints[p].mCost.get(powerID);
            supplyPoints.add( supplies );
            powerPoints.add( power );
         }

         popData.clear();
         pPlayerStats->queryPopGraph(minX, maxX, minY, maxY, popData);

         popPoints.clear();
         for (int d=0; d<popData.getNumber(); d++)
         {
            float pop = popData[d].mValue;
            popPoints.add(pop);
         }

         baseData.clear();
         pPlayerStats->queryBaseGraph(minX, maxX, minY, maxY, baseData);

         basePoints.clear();
         for (int d=0; d<baseData.getNumber(); d++)
         {
            uint32 count = baseData[d].mValue;
            basePoints.add((float)count);
         }

         scoreData.clear();
         pPlayerStats->queryScoreGraph(minX, maxX, minY, maxY, scoreData);

         scorePoints.clear();
         for (int d=0; d<scoreData.getNumber(); d++)
         {
            int32 score = scoreData[d].mValue;
            scorePoints.add(static_cast<float>(score));
         }
         
         addTimeline(cStatsTimelineScore,     scorePoints,  playerColor.asDWORD(), mPlayers[i].getPlayerID());
         addTimeline(cStatsTimelineSupplies,  supplyPoints, playerColor.asDWORD(), mPlayers[i].getPlayerID());
         addTimeline(cStatsTimelineTechLevel, powerPoints,  playerColor.asDWORD(), mPlayers[i].getPlayerID());
         addTimeline(cStatsTimelinePop,       popPoints,    playerColor.asDWORD(), mPlayers[i].getPlayerID());
         addTimeline(cStatsTimelineBases,     basePoints,   playerColor.asDWORD(), mPlayers[i].getPlayerID());
      }
   }

   // normalize the graphs now
   for (int t=0; t<cStatsTimelineCount; t++)
   {
      BGraphAttribs* pAttribs = gGraphManager.getGraph(mTimelineHandles[t]);
      if (pAttribs)
         pAttribs->normalize();
   }
}


//============================================================================
//============================================================================
bool BUISkirmishPostGameScreen::addTab(const char * labelName, BUIPanelControl *panel, const WCHAR* labelText)
{
   BUILabelControl* label = NULL;

   label = new BUILabelControl();
   label->init(this, labelName);
   BUString labelTextString;
   labelTextString.set(labelText);
   label->setText(labelTextString);
   mControls.add(panel);
   mControls.add(label);
   mTabControl.addTab(label, panel);

   return true;
}


//============================================================================
//============================================================================
bool BUISkirmishPostGameScreen::displayButtons()
{
   uint b0=BUIButtonBarControl::cFlashButtonOff;
   uint b1=BUIButtonBarControl::cFlashButtonOff;
   uint b2=BUIButtonBarControl::cFlashButtonOff;
   uint b3=BUIButtonBarControl::cFlashButtonOff;
   uint b4=BUIButtonBarControl::cFlashButtonOff;

   // change the string IDs and button faces as needed

   // A - continue
   // B - Quit
   // Y - Restart
   BUString s0;
   BUString s1;
   BUString s2;
   BUString s3;
   BUString s4;

   // Get our current selected cell
//-- FIXING PREFIX BUG ID 1772
   const BUIControl* c = getSelectedCell();
//--

   if (mCallout.isShown())
   {
      // if the callout is up, then A is close
      b0 = BUIButtonBarControl::cFlashButtonB;
      s0.set(gDatabase.getLocStringFromID(25044)); // CLOSE
      mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
      mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);
      return true;
   }

   // A Button
   if (!mShowingStats)
   {
      b0 = BUIButtonBarControl::cFlashButtonA;
      s0.set(gDatabase.getLocStringFromID(25043));//STATS

      b1=BUIButtonBarControl::cFlashButtonB;
      s1.set(gDatabase.getLocStringFromID(25042));//QUIT
      mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
      mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);
      return true;
   }

   if (c)
   {
      switch (c->getControlTypeID())
      {
         case UIGamerTagControlID:
            {
               // if (gLiveSystem->isMultiplayerLiveGameActive())
               if (gDatabase.getGameSettingsNetworkType() == BGameSettings::cNetworkTypeLive)
               {
                  int playerControlIndex = getPlayerWithFocus();
                  if (playerControlIndex >= 0)
                  {
//-- FIXING PREFIX BUG ID 1771
                     const BPlayer* pPlayer = gWorld->getPlayer(mPlayers[playerControlIndex].getPlayerID());
//--
                     if (pPlayer->isHuman() && pPlayer->getXUID() != INVALID_XUID && !gUserManager.isGuestXuid(pPlayer->getXUID()))
                     {
                        b0 = BUIButtonBarControl::cFlashButtonA;
                        s0.set(gDatabase.getLocStringFromID(25045));//GAMERTAG

/*
                        // also need checks for whether the user is Live enabled or local since we can only pull up service records for Live enabled players
                        b2 = BUIButtonBarControl::cFlashButtonX;
                        // XXX loc
                        s2.set(gDatabase.getLocStringFromID(25046)); //L"SERVICE");
*/

                        // submit a review
                        b3 = BUIButtonBarControl::cFlashButtonY;
                        s3.set(gDatabase.getLocStringFromID(25646));
                     }
                  }
               }
            }
            break;
         case UIGridCellControlID:
            {
               const BUIGridCellControl* cell = static_cast<const BUIGridCellControl*>(c);
               if (cell && cell->getDetailIndicator())
               {
                  b0 = BUIButtonBarControl::cFlashButtonA;
                  s0.set(gDatabase.getLocStringFromID(25047));//L"DETAILS");
               }
            }
            break;
      }
   }

   // B Button
   b1 = BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(25048));//L"GAME");

   // Y Button
/*
   if (mTabControl.getActiveTab()==cSkirmishPostgameTabTimeline)
   {
      b3 = BUIButtonBarControl::cFlashButtonY;
      s3.set(gDatabase.getLocStringFromID(25049));//L"TIMELINE");
   }

   if (mTabControl.getActiveTab() > 0)
   {
      b3 = BUIButtonBarControl::cFlashButtonLeftButton;
      s3.set(gDatabase.getLocStringFromID(25050));//L"PREVIOUS");
   }

   if (mTabControl.getActiveTab() < mTabControl.getNumberTabs() - 1)
   {
      b4 = BUIButtonBarControl::cFlashButtonRightButton;
      s4.set(gDatabase.getLocStringFromID(25051));//L"NEXT");
   }
*/

   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

   return true;
}

//============================================================================
//============================================================================
bool BUISkirmishPostGameScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {
      gUI.playCancelSound();
      mpHandler->handleUIScreenResult( this, eResult_Back );
      return true;
   }
   else if (command=="timelineUp")
   {
      // change timeline
      if (mTabControl.getActiveTab()!=cSkirmishPostgameTabTimeline)
         return true;

      mCurrentTimeline--;
      if (mCurrentTimeline<0)
         mCurrentTimeline=cStatsTimelineCount-1;

      showTimeline();
      displayButtons();
      return true;
   }
   else if ( (command=="toggleTimeline") || (command=="timelineDown") )
   {
      // change timeline
      if (mTabControl.getActiveTab()!=cSkirmishPostgameTabTimeline)
         return true;

      mCurrentTimeline++;
      if (mCurrentTimeline>=cStatsTimelineCount)
         mCurrentTimeline=0;

      showTimeline();
      displayButtons();
      return true;
   }
   return false;
}


//============================================================================
//============================================================================
bool BUISkirmishPostGameScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
/*
   // SRL - pulled - phx-16437
   if( mServiceRecord.getVisible() )
      return mServiceRecord.handleInput( port, event, controlType, detail );
*/

   bool handled = false;

   if (!handled && mCallout.isShown())
      handled = mCallout.handleInput(port, event, controlType, detail);

   if (!handled)
      handled = mTabControl.handleInput(port, event, controlType, detail);
   
   if (!handled)
   {
      switch (mTabControl.getActiveTab())
      {
      case cSkirmishPostgameTabScore:
         handled = mpGridControlScore->handleInput(port, event, controlType, detail);
         break;
      case cSkirmishPostgameTabMilitary:
         handled = mpGridControlMilitary->handleInput(port, event, controlType, detail);
         break;
      case cSkirmishPostgameTabEconomy:
         handled = mpGridControlEconomy->handleInput(port, event, controlType, detail);
         break;
      case cSkirmishPostgameTabTimeline:
         handled = mpGridControlTimeline->handleInput(port, event, controlType, detail);
         break;
      }
   }

   if (!handled)
      handled = BUIScreen::handleInput(port, event, controlType, detail);
   
   return true;
}


// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUISkirmishPostGameScreen::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   switch ( control->getControlTypeID() )
   {
      case UIGridControlID:
         if (event.getID() == BUIGridControl::eSelectionChanged)
         {
            if (mTabControl.getActiveTab()==cSkirmishPostgameTabTimeline)
               showTimeline(false);

            checkColumnTitle();

            mCallout.hide();
            displayButtons();
         }
         break;
      case UITabControlID:
         if (event.getID() == BUITabControl::eTabChanged)
         {
            fixupPlayerFocus();
            checkColumnTitle();
         }
         if (mTabControl.getActiveTab()==cSkirmishPostgameTabTimeline)
            showTimeline();

         mCallout.hide();
         displayButtons();
         break;
      case UIGridCellControlID:
         if (event.getID() == BUIGridCellControl::ePress)
         {
//-- FIXING PREFIX BUG ID 1773
            const BUIGridCellControl* cell = static_cast<BUIGridCellControl*>(control);
//--
            if (cell && cell->getDetailIndicator())
            {
               // turn on our callout for this cell if we have one.
               mCallout.setText(cell->getCalloutText());
               mCallout.setTitle(cell->getCalloutTitle());
               mCallout.show();
               gUI.playConfirmSound();
            }
         }
         displayButtons();
         break;
      case UIScrollableCalloutControlID:
         if (event.getID() == BUIScrollableCalloutControl::eCancel)
         {
            mCallout.hide();
            gUI.playCancelSound();
         }
         displayButtons();
         break;
      case UIGamerTagControlID:
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
                  if (playerControlIndex>=0 && playerControlIndex < cMaxPlayers)
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
/*
         // SRL - pulled - phx-16437
         else if (event.getString()=="serviceRecord")
         {
            if (gLiveSystem->isMultiplayerLiveGameActive())
            // if (gLiveSystem->getMPSession() && !gLiveSystem->getMPSession()->isInLANMode() )
            {
               BUIGamerTagControl* pGamerTagControl = (BUIGamerTagControl*)event.getControl();
               // show the service record
               mServiceRecord.populateFromUser( gUserManager.getUserByPlayerID( pGamerTagControl->getPlayerID() ) );
               mServiceRecord.enter();
               mServiceRecord.setVisible(true);
            }
         }
*/
         else if (event.getString()=="sessionReview")
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
                  if (playerControlIndex>=0 && playerControlIndex < cMaxPlayers)
                  {
                     BPlayer* pPlayer = gWorld->getPlayer(mPlayers[playerControlIndex].getPlayerID());
                     if (pPlayer)
                     {
                        if (pPlayer->isHuman() && pPlayer->getXUID() != INVALID_XUID && !gUserManager.isGuestXuid(pPlayer->getXUID()))
                           XShowPlayerReviewUI(pUser->getPort(), pPlayer->getXUID());
                     }
                  }
               }
            }
         }
         displayButtons();
         break;
   }

   return true;
}

//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::handleUIScreenResult( BUIScreen* pScreen, long result )
{
/*
// SRL - pulled - phx-16437
   if( pScreen == &mServiceRecord )
   {
      mServiceRecord.leave();
      mServiceRecord.setVisible(false);
   }
*/
}

//==============================================================================
//==============================================================================
void BUISkirmishPostGameScreen::checkColumnTitle()
{
   // if the selected cell is not in col0, then clear focus from players
   // if the selected cell is in col 0, then set focus on the player

   int row=-1;
   int col=-1;

   BUString columnHeader;

   switch (mTabControl.getActiveTab())
   {
   case cSkirmishPostgameTabScore:
      mpGridControlScore->getSelectedCell(row, col);
      if (mpGridControlScore->isValidCoordinate(row, col) && col!=0)
      {
         switch(col)
         {
            case 1:
               
               columnHeader.set(gDatabase.getLocStringFromID(25898).getPtr());
               break;
            case 2:
               columnHeader.set(gDatabase.getLocStringFromID(25899).getPtr());
               break;
            case 3:
               columnHeader.set(gDatabase.getLocStringFromID(25900).getPtr());
               break;
            case 4:
               columnHeader.set(gDatabase.getLocStringFromID(25901).getPtr());
               break;
         }
      }
      break;
   case cSkirmishPostgameTabMilitary:
      mpGridControlMilitary->getSelectedCell(row, col);
      if (mpGridControlMilitary->isValidCoordinate(row, col) && col!=0)
      {
         switch(col)
         {
         case 1:
            columnHeader.set(gDatabase.getLocStringFromID(25902).getPtr());
            break;
         case 2:
            columnHeader.set(gDatabase.getLocStringFromID(25903).getPtr());
            break;
         case 3:
            columnHeader.set(gDatabase.getLocStringFromID(25904).getPtr());
            break;
         case 4:
            columnHeader.set(gDatabase.getLocStringFromID(25905).getPtr());
            break;
         case 5:
            columnHeader.set(gDatabase.getLocStringFromID(25906).getPtr());
            break;
         }
      }
      break;
   case cSkirmishPostgameTabEconomy:
      mpGridControlEconomy->getSelectedCell(row, col);
      if (mpGridControlEconomy->isValidCoordinate(row, col) && col!=0)
      {
         switch(col)
         {
         case 1:
            columnHeader.set(gDatabase.getLocStringFromID(25907).getPtr());
            break;
         case 2:
            columnHeader.set(gDatabase.getLocStringFromID(25908).getPtr());
            break;
         case 3:
            columnHeader.set(gDatabase.getLocStringFromID(25909).getPtr());
            break;
         case 4:
            columnHeader.set(gDatabase.getLocStringFromID(25910).getPtr());
            break;
         case 5:
            columnHeader.set(gDatabase.getLocStringFromID(25911).getPtr());
            break;
         }
      }
      break;
/*
   case cSkirmishPostgameTabTimeline:
      mpGridControlTimeline->getSelectedCell(row, col);
      if (mpGridControlTimeline->isValidCoordinate(row, col) && col!=0)
      {
      }
      break;
*/
   }

   mColumnHeader.setText(columnHeader);

}

//==============================================================================
// Since we don't instantiate the player controls on each tab, we need to adjust
// the player focus as we switch from tab to tab. 
// 
// The alternative might be to always set the focus on a specific control as 
// you switch tabs.
//==============================================================================
void BUISkirmishPostGameScreen::fixupPlayerFocus()
{
   // if the selected cell is not in col0, then clear focus from players
   // if the selected cell is in col 0, then set focus on the player

   int playerWithFocus=-1;
   int row=-1;
   int col=-1;

   if (mCallout.isShown())
      mCallout.hide();

   switch (mTabControl.getActiveTab())
   {
   case cSkirmishPostgameTabScore:
      mpGridControlScore->getSelectedCell(row, col);
      if (mpGridControlScore->isValidCoordinate(row, col) && col==0)
      {
         playerWithFocus=row;
      }
      break;
   case cSkirmishPostgameTabMilitary:
      mpGridControlMilitary->getSelectedCell(row, col);
      if (mpGridControlMilitary->isValidCoordinate(row, col) && col==0)
      {
         playerWithFocus=row;
      }
      break;
   case cSkirmishPostgameTabEconomy:
      mpGridControlEconomy->getSelectedCell(row, col);
      if (mpGridControlEconomy->isValidCoordinate(row, col) && col==0)
      {
         playerWithFocus=row;
      }
      break;
   case cSkirmishPostgameTabTimeline:
      mpGridControlTimeline->getSelectedCell(row, col);
      if (mpGridControlTimeline->isValidCoordinate(row, col) && col==0)
      {
         playerWithFocus=row;
      }
      break;
   }

   // adjust player focus.
   for (int i=0; i<cMaxPlayers; i++)
   {
      if (i==playerWithFocus)
         mPlayers[i].focus();
      else
         mPlayers[i].unfocus();
   }

}

//==============================================================================
// BUIPostGame::setStatsVisible
//==============================================================================
void BUISkirmishPostGameScreen::setStatsVisible(bool bVisible)
{
   if (mShowingStats == bVisible)
      return;

   mShowingStats = bVisible;
   GFxValue values[1];
   values[0].SetBoolean(bVisible);

   mpMovie->invokeActionScript("setStatsVisible", values, 1);

   if (bVisible)
   {
      // this is kind of a hack, but it will get andy going for right now.
      /*
      What we need to do is hide all of the controls in a movie clip so we can turn off
      that one movie clip and all the children will turn off/on keeping proper state.
      */
      int selectedTab = mTabControl.getActiveTab();
      for (int i=0; i<mTabControl.getNumberTabs(); i++)
      {
         BUIControl* pane = mTabControl.getPane(i);
         if (pane)
         {
            // force the UI to sync to this.
            if (i==selectedTab)
               pane->show(true);
            else
               pane->hide(true);
         }
      }
   }
}

//==============================================================================
// BUIPostGame::showTimeline
//==============================================================================
void BUISkirmishPostGameScreen::showTimeline(bool animate)
{
   if (animate)
      mGraphFadeTime=0.0f;

   for (int i=0; i<cStatsTimelineCount; i++)
   {
      BGraphAttribs* pAttribs = gGraphManager.getGraph(mTimelineHandles[i]);
      if (pAttribs)
      {
         // show the correct timeline
         if (mCurrentTimeline==i)
         {
            long selectedPlayerID = -1;
            if (mpGridControlTimeline)
            {
//-- FIXING PREFIX BUG ID 1774
               const BUIGamerTagControl* gamerTag = (BUIGamerTagControl*)mpGridControlTimeline->getSelectedControl();
//--
               if (gamerTag)
                  selectedPlayerID = gamerTag->getPlayerID();
            }

            int numLines = pAttribs->getTimelineCount();
            for (int t=0; t<numLines; t++)
            {
               BGraphTimeline * timeline = pAttribs->getTimeline(t);
               if (timeline)
               {
                  timeline->mColor = gWorld->getPlayerColor(timeline->mPlayerID, BWorld::cPlayerColorContextUI);
                  timeline->mbEnableHighlight = false;
                  timeline->mbIsSelected = false;

                  float interval = 5.0f;
                  float sizeX = 7.0f;
                  float sizeY = 7.0f;
                  timeline->mDotInterval = interval;
                  timeline->mSizeX = sizeX;
                  timeline->mSizeY = sizeY;
                  if (timeline->mPlayerID == selectedPlayerID)
                  {
                     timeline->mbEnableHighlight = true;
                     timeline->mbIsSelected = true;
                  }
               }
            }

            pAttribs->setEnabled(true);
         }
         else
            pAttribs->setEnabled(false);
      }
   }
}

//==============================================================================
// BUIPostGame::addTimeline
//==============================================================================
void BUISkirmishPostGameScreen::addTimeline(int id, BDynamicArray<float>& points, DWORD playerColor, int playerNumber)
{
   // don't add a blank timeline
   if (points.getNumber()<=0)
      return;

   BGraphAttribs* pAttribs = gGraphManager.getGraph(mTimelineHandles[id]);
   if (pAttribs)
   {
      pAttribs->addTimeline(points, playerColor, playerNumber, 2.0f);   
   }
}

//==============================================================================
// BUIPostGame::renderTimelineTab
//==============================================================================
void BUISkirmishPostGameScreen::renderTimelineTab()
{
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BUISkirmishPostGameScreen::workerRenderMovie));

   // render what timeline it is
   BSimString timeline;
   switch (mCurrentTimeline)
   {
   case cStatsTimelineScore:
      timeline.set("Score");
      break;
   case cStatsTimelineBases:
      timeline.set("Bases");
      break;
   case cStatsTimelineTechLevel:
      timeline.set("Tech Level");
      break;
   case cStatsTimelineSupplies:
      timeline.set("Supplies");
      break;
   case cStatsTimelinePop:
      timeline.set("Pop");
      break;
   }

   BHandle fontHandle=gFontManager.getFontDenmark14();
   gFontManager.setFont( fontHandle );

   // get our bounding values
   float yh=gFontManager.getLineHeight();

   //float sx=gUI.mfSafeX1;
   //float sy=gUI.mfSafeY1;
   //   float by=gUI.mfSafeY2-yh;
   //float ex=gUI.mfSafeX2;

   float x = 400.0f;
   float y = 600.0f;
   if (gRender.getAspectRatioMode()==BRender::cAspectRatioMode4x3)
   {
      x = 100.0f;
      y = 110.0f;
   }

   // Headers
   gFontManager.drawText(fontHandle, x, y-yh, timeline.getPtr());
}

//==============================================================================
//==============================================================================
void BUISkirmishPostGameScreen::workerRenderMovie(void* pData)
{
   ASSERT_RENDER_THREAD
      gGraphManager.renderGraphs();
}


//==============================================================================
//==============================================================================
BUIControl* BUISkirmishPostGameScreen::getSelectedCell()
{
   switch (mTabControl.getActiveTab())
   {
      case cSkirmishPostgameTabScore:
         if (mpGridControlScore)
            return mpGridControlScore->getSelectedControl();
         break;
      case cSkirmishPostgameTabMilitary:
         if (mpGridControlMilitary)
            return mpGridControlMilitary->getSelectedControl();
         break;
      case cSkirmishPostgameTabEconomy:
         if (mpGridControlEconomy)
            return mpGridControlEconomy->getSelectedControl();
         break;
      case cSkirmishPostgameTabTimeline:
         if (mpGridControlTimeline)
            return mpGridControlTimeline->getSelectedControl();
         break;
   }
   return NULL;
}

//============================================================================
//============================================================================
int BUISkirmishPostGameScreen::getPlayerWithFocus()
{
   for (int i=0; i<cMaxPlayers; i++)
   {
      if (mPlayers[i].isFocused())
         return i;
   }

   return -1;
}


//============================================================================
//============================================================================
void BUISkirmishPostGameScreen::enter( void )
{
   if (!getStatsViewed() || (gWorld && gWorld->getFlagGameOver() && gWorld->getGametime() != mLastUpdateGameTime))
   {
      mLastUpdateGameTime = (gWorld ? gWorld->getGametime() : 0);
      populate();
      setStatsViewed(true);
   }

   gUIManager->setMinimapVisible(false);
   setStatsVisible(true);
   displayButtons();
}

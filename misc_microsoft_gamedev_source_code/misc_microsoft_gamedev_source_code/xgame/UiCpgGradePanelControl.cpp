//============================================================================
// UICpgGradePanelControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UICpgGradePanelControl.h"

#include "player.h"
#include "usermanager.h"
#include "user.h"
#include "scoremanager.h"
#include "database.h"
#include "world.h"
#include "team.h"
#include "userprofilemanager.h"

//============================================================================
//============================================================================
BUICpgGradePanelControl::BUICpgGradePanelControl( void )
{
}

//============================================================================
//============================================================================
BUICpgGradePanelControl::~BUICpgGradePanelControl( void )
{
}


//============================================================================
//============================================================================
bool BUICpgGradePanelControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
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

               if(name==B("BUICpgGradePanelControl"))
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

   initLabels();

   mMissionStatusText.init(this, mScriptPrefix+"mMissionStatusText");
   mScoreRangeLabels.init(this, mScriptPrefix+"mScoreRangeLabels");
   mScoreRangeValues.init(this, mScriptPrefix+"mScoreRangeValues");

   mMedalImage.init(this, mScriptPrefix+"mMedalImage");

   mPlayerList.init( this, "" );
   mPlayerList.setAlignment( BUIListControl::eHorizontal );

   // initialize our controls here
   mPlayer[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mPlayer", -1, NULL);
   mPlayer[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mPlayer", -1, NULL);

   // Init the text fields

   return result;
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::initLabels()
{
   mParTimeLabel.init(this, mScriptPrefix+"mParTimeLabel");
   mParTimeText.init(this, mScriptPrefix+"mParTimeText");

   mFinalScoreLabel.init(this, mScriptPrefix+"mFinalScoreLabel");
   mFinalScoreText.init(this, mScriptPrefix+"mFinalScoreText");

   mTitle.init(this, mScriptPrefix+"mTitle");

   // Score Labels
   mBaseScoreObjectiveLabel[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mLabelLine0");
   mBaseScoreObjectiveLabel[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mLabelLine0");

   mCombatBonusLabel[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mLabelLine1");
   mCombatBonusLabel[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mLabelLine1");

   mTimeBonusLabel[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mLabelLine2");
   mTimeBonusLabel[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mLabelLine2");

   mSkullsBonusLabel[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mLabelLine3");
   mSkullsBonusLabel[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mLabelLine3");

   mTotalScoreLabel[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mLabelLine4");
   mTotalScoreLabel[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mLabelLine4");


   // Score Fields
   mBaseScoreObjective[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mFieldLine0");
   mBaseScoreObjective[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mFieldLine0");

   mCombatBonusText[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mFieldLine1");
   mCombatBonusText[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mFieldLine1");

   mTimeBonusText[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mFieldLine2");
   mTimeBonusText[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mFieldLine2");

   mSkullsBonusText[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mFieldLine3");
   mSkullsBonusText[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mFieldLine3");

   mTotalScoreText[cPrimaryCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox1.mFieldLine4");
   mTotalScoreText[cCoopCampaignPlayer].init(this, mScriptPrefix+"mPlayerBox2.mFieldLine4");
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::show( bool force /*=false*/ )
{
   mPlayerList.focus( true );
   __super::show( force );
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::hide( bool force /*=false*/ )
{
   mPlayerList.unfocus();
   __super::hide( force );
}

//============================================================================
//============================================================================
bool BUICpgGradePanelControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   if( mPlayerList.getControlCount() > 0 )
   {
      handled = mPlayerList.handleInput( port, event, controlType, detail );
      if( handled )
         return handled;
   }

   // Screen
   handled = BUIPanelControl::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   return handled;
}

//============================================================================
//============================================================================
bool BUICpgGradePanelControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
if (command=="prev")
   {
      // if on player list, go up
      if (mPlayer[cCoopCampaignPlayer].isFocused())
      {
         mPlayer[cCoopCampaignPlayer].unfocus();
         mPlayer[cPrimaryCampaignPlayer].focus();
         // send a focus changed event up the change
         return true;
      }
   }
   else if (command=="next")
   {
      // if on player list go down
      if (mPlayer[cPrimaryCampaignPlayer].isFocused() && mPlayer[cCoopCampaignPlayer].isShown())
      {
         mPlayer[cPrimaryCampaignPlayer].unfocus();
         mPlayer[cCoopCampaignPlayer].focus();
         // send a focus changed event up the change
         return true;
      }
   }
   return false;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUICpgGradePanelControl::handleUIControlEvent( BUIControlEvent& event )
{
   return __super::handleUIControlEvent( event );
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::populateLabels()
{
   BUString temp;

   temp.set(gDatabase.getLocStringFromID(25674)); // oBJECTIVE SCORE
   mBaseScoreObjectiveLabel[cPrimaryCampaignPlayer].setText(temp);
   mBaseScoreObjectiveLabel[cCoopCampaignPlayer].setText(temp);

   temp.set(gDatabase.getLocStringFromID(25072));//"Combat Bonus");
   mCombatBonusLabel[cPrimaryCampaignPlayer].setText(temp);
   mCombatBonusLabel[cCoopCampaignPlayer].setText(temp);

   temp.set(gDatabase.getLocStringFromID(25073));//"Time Bonus");
   mTimeBonusLabel[cPrimaryCampaignPlayer].setText(temp);
   mTimeBonusLabel[cCoopCampaignPlayer].setText(temp);

   temp.set(gDatabase.getLocStringFromID(25074));//"Skulls Bonus");
   mSkullsBonusLabel[cPrimaryCampaignPlayer].setText(temp);
   mSkullsBonusLabel[cCoopCampaignPlayer].setText(temp);

   temp.set(gDatabase.getLocStringFromID(25075));//"Total Score");
   mTotalScoreLabel[cPrimaryCampaignPlayer].setText(temp);
   mTotalScoreLabel[cCoopCampaignPlayer].setText(temp);

   temp.set(gDatabase.getLocStringFromID(25076));//"Par Time");
   mParTimeLabel.setText(temp);

   temp.set(gDatabase.getLocStringFromID(24951));//"SCORE");
   mFinalScoreLabel.setText(temp);

}


//============================================================================
//============================================================================
void BUICpgGradePanelControl::populateParTime()
{
   // populate par time
   BUString parTimeMin;
   BUString parTimeMax;
   BUString parTimeText;

   formatTime(gScoreManager.getScenarioData().mMissionMinParTime, parTimeMin);
   formatTime(gScoreManager.getScenarioData().mMissionMaxParTime, parTimeMax);

   parTimeText.locFormat(L"%s - %s", parTimeMin.getPtr(), parTimeMax.getPtr());

   mParTimeText.setText(parTimeText);
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::hidePlayer2()
{
   GFxValue value;
   value.SetString("off");

   invokeActionScript("mPlayerBox2.gotoAndStop", &value, 1);
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::movePlayer1ToCenter()
{
   GFxValue value;
   value.SetString("right");

   invokeActionScript("mPlayerBox1.gotoAndStop", &value, 1);
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::populateMissionResult()
{
   BString keyFrame;
   keyFrame.set("");

   BPlayer* pPlayer = gWorld->getPlayer(1);        // any way to make this safer?
   if (pPlayer && pPlayer->isHuman())
   {
      if (pPlayer->getPlayerState() == BPlayer::cPlayerStateWon)
      {
         // Mission Complete
         mMissionStatusText.setText(gDatabase.getLocStringFromID(24007));
         keyFrame.set("UNSC Win");
      }
      else
      {
         // Mission Failed.
         mMissionStatusText.setText(gDatabase.getLocStringFromID(24006));
         keyFrame.set("UNSC Lose");
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
bool BUICpgGradePanelControl::populate()
{
   if (!gWorld->getFlagCoop())
   {
      hidePlayer2();
      movePlayer1ToCenter();
   }

   populateMedal();

   populateLabels();

   populateMissionResult();

   populateParTime();
   
   // basically set focus on the incomplete campaign objectives and go from there
   mPlayer[cPrimaryCampaignPlayer].unfocus();
   mPlayer[cCoopCampaignPlayer].unfocus();

   populatePlayers();

   return true;
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::formatTime(DWORD time, BUString& timeString)
{
   DWORD s=time/1000;
   DWORD m=s/60;
   s-=m*60;
   DWORD h=m/60;
   m-=h*60;

   if(h>0)
      timeString.locFormat(L"%02d:%02d:%02d", h, m, s);
   else
      timeString.locFormat(L"%02d:%02d", m, s);
}


//============================================================================
//============================================================================
void BUICpgGradePanelControl::populateScore(BPlayer* player, int playerIndex)
{
   if (!player)
      return;

   BUString temp;

   long objScore = 0, combatScore = 0, baseScore = 0, combatBonus = 0; 
   long timeBonus = 0, skullBonus = 0, finalScore = 0, finalGrade = 0;
   float combatBonusPercent = 0.0f, timeBonusPercent = 0.0f;

   bool success = gScoreManager.getFinalScoreValues(player->getID(), objScore, combatScore, baseScore, combatBonus, combatBonusPercent, 
                               timeBonus, timeBonusPercent, skullBonus, finalScore, finalGrade);
   BASSERT(success);

   // Objective Score
   temp.locFormat(L"%d", objScore);
   mBaseScoreObjective[playerIndex].setText(temp);

   // Military Score
   // SRL 10/22/08 - phx-16040
   // temp.locFormat(L"%d", combatScore);
/*
   temp.set(L"");
   mBaseScoreMilitary[playerIndex].setText(temp);
*/

   // Base Score Total
/*
   temp.locFormat(L"%d", baseScore);
   mBaseScoreTotal[playerIndex].setText(temp);
*/

   wchar_t floatTemp[20];

   // Combat Bonus
   //11/17/2008 - We use a custom function to format float values so that we can 
   //use different decimal separators in localized builds.  This was bug 12464
   locFormatFloat(floatTemp, 20, combatBonusPercent, 2, gDatabase.getLocStringFromID(25996));
   temp.locFormat(L"%d (%s x)", combatBonus, floatTemp);
   mCombatBonusText[playerIndex].setText(temp);

   // Time Bonus
   //11/17/2008 - We use a custom function to format float values so that we can 
   //use different decimal separators in localized builds.  This was bug 12464
   locFormatFloat(floatTemp, 20, timeBonusPercent, 2, gDatabase.getLocStringFromID(25996));
   temp.locFormat(L"%d (%s x)", timeBonus, floatTemp);
   mTimeBonusText[playerIndex].setText(temp);

   // Skulls Bonus
   temp.locFormat(L"%d", skullBonus);
   mSkullsBonusText[playerIndex].setText(temp);

   // total score
   temp.locFormat(L"%d", finalScore);
   mTotalScoreText[playerIndex].setText(temp);
}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::populatePlayer(BPlayer* player, int playerIndex)
{
   if (!player)
      return;

   BUser* user=gUserManager.getUserByPlayerID(player->getID());

   BString temp;
   BUString tempU;

   // Basic player info
   mPlayer[playerIndex].setGamerTag(player->getLocalisedDisplayName());
   temp.format("img://gamerPic:%I64x", player->getXUID());
   mPlayer[playerIndex].setGamerPic(temp);

   const BLeader* leader=player->getLeader();
   if (leader)
      mPlayer[playerIndex].setLeaderPic(leader->getUIBackgroundImage());

   int controllerPort=-1;
   if (user)
      controllerPort=user->getPort();
   mPlayer[playerIndex].setPort(controllerPort);

   // leader info
   /*
   temp.set("");
   playerControl.setLeaderPic(temp);
   */

   tempU.locFormat(gDatabase.getLocStringFromID(25325).getPtr(), gScoreManager.getFinalScore(player->getID()) );
   mPlayer[playerIndex].setTitleText(tempU);

   mPlayer[playerIndex].setXuid( player->getXUID() );

   BProfileRank rank(player->getRank());
   mPlayer[playerIndex].setRank(rank.mRank);

   // misc stuff.
   // fixme - add this stuff in.
   /*

   playerControl.setAIDifficulty(int difficulty);
   playerControl.setHost(bool bHost);
   playerControl.setPing(int ping);
   playerControl.setStatus(int status);
   */

}

//============================================================================
//============================================================================
void BUICpgGradePanelControl::populateMedal()
{
   BPlayer * player = gWorld->getPlayer(1);
   BASSERT(player);
   if (!player)
      return;

   long difficulty = DifficultyType::cNormal;
   BPlayer* pPlayer = gWorld->getPlayer(1);        // any way to make this safer?
   if (pPlayer && pPlayer->isHuman())
   {
      difficulty = pPlayer->getDifficultyType();
   }

   // populate the grade (for now use the primary user's grade.
   int grade = gScoreManager.getFinalGrade(player->getID());

   long gradeGold = gScoreManager.getScoreRequiredForGrade(BScoreManager::cGradeGold);
   long gradeSilver = gScoreManager.getScoreRequiredForGrade(BScoreManager::cGradeSilver);
   long gradeBronze = gScoreManager.getScoreRequiredForGrade(BScoreManager::cGradeBronze);
   long gradeTin = gScoreManager.getScoreRequiredForGrade(BScoreManager::cGradeTin);

   // build the grade range string
   mScoreRangeLabels.setText(gDatabase.getLocStringFromID(25944));
   BUString temp;
   temp.locFormat(gDatabase.getLocStringFromID(25897).getPtr(), gradeGold, gradeSilver, (gradeGold-1), gradeBronze, (gradeSilver-1), gradeTin, (gradeBronze-1));
   mScoreRangeValues.setText(temp);


   BString medalImage;
   switch (grade)
   {
      case 1: // medalText.set("GLD"); break;
         {
            switch (difficulty)
            {
            case DifficultyType::cEasy:
               medalImage.set("easy_gold");
               break;
            case DifficultyType::cNormal:
               medalImage.set("normal_gold");
               break;
            case DifficultyType::cHard:
               medalImage.set("heroic_gold");
               break;
            case DifficultyType::cLegendary:
               medalImage.set("legendary_gold");
               break;
            }
         }
         break;
      case 2: // medalText.set("SIL"); break;
         {
            switch (difficulty)
            {
            case DifficultyType::cEasy:
               medalImage.set("easy_silver");
               break;
            case DifficultyType::cNormal:
               medalImage.set("normal_silver");
               break;
            case DifficultyType::cHard:
               medalImage.set("heroic_silver");
               break;
            case DifficultyType::cLegendary:
               medalImage.set("legendary_silver");
               break;
            }
         }
         break;
   case 3: // medalText.set("BRZ"); break;
      {
         switch (difficulty)
         {
         case DifficultyType::cEasy:
            medalImage.set("easy_bronze");
            break;
         case DifficultyType::cNormal:
            medalImage.set("normal_bronze");
            break;
         case DifficultyType::cHard:
            medalImage.set("heroic_bronze");
            break;
         case DifficultyType::cLegendary:
            medalImage.set("legendary_bronze");
            break;
         }
      }
      break;
   case 4: // medalText.set("BRZ"); break; // [8/15/2008 xemu] some old data still has a 4th medal type
      {
         switch (difficulty)
         {
         case DifficultyType::cEasy:
            medalImage.set("easy_tin");
            break;
         case DifficultyType::cNormal:
            medalImage.set("normal_tin");
            break;
         case DifficultyType::cHard:
            medalImage.set("heroic_tin");
            break;
         case DifficultyType::cLegendary:
            medalImage.set("legendary_tin");
            break;
         }
      }
      break;
   }

   if (medalImage.length() > 0)
   {
      mMedalImage.clearImages();
      BString temp;
      temp.format("img://art\\ui\\flash\\shared\\textures\\medals\\%s.ddx", medalImage.getPtr());
      mMedalImage.addImage(temp.getPtr());
      mMedalImage.start();

   }


}


//============================================================================
//============================================================================
void BUICpgGradePanelControl::populatePlayers()
{
   mPlayerList.clearControls();

   // By definition, the primary player (not primary user) is player 1.

   BPlayer * player = gWorld->getPlayer(1);
   BASSERT(player);
   if (!player)
      return;

   BUString temp;
   if (player)
   {
      //temp.format(L"%S", pUser->getPlayer()->getName().getPtr());
      //mPlayerLabel[cPrimaryCampaignPlayer].setText(temp);
   }

   populatePlayer(player, cPrimaryCampaignPlayer);
   populateScore(player, cPrimaryCampaignPlayer);

   long finalScore = gScoreManager.getFinalScore(player->getID());

   BPlayer * coopPlayer = getCoopPlayer(player);

   // do we have a coop player?
   if (coopPlayer)
   {
      populatePlayer(coopPlayer, cCoopCampaignPlayer);
      populateScore(coopPlayer, cCoopCampaignPlayer);
      finalScore += gScoreManager.getFinalScore(coopPlayer->getID());

      mPlayerList.addControl( &(mPlayer[cPrimaryCampaignPlayer]) );
      mPlayerList.addControl( &(mPlayer[cCoopCampaignPlayer]) );
      mPlayerList.setIndex( 0 );
   }

   // fill in the final score value
   temp.format(L"%d", finalScore);
   mFinalScoreText.setText(temp);
}


//============================================================================
//============================================================================
BPlayer * BUICpgGradePanelControl::getCoopPlayer(BPlayer* player)
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
         if (pCoopPlayer && (pCoopPlayer->getID() != player->getID()) && pCoopPlayer->isHuman() )
            break;

         pCoopPlayer=NULL;    // just in case we didn't find a coop player
      }
   }

   return pCoopPlayer;
}


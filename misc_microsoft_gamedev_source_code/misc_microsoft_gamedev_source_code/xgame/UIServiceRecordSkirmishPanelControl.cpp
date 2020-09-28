//============================================================================
// BUIServiceRecordSkirmishPanelControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "uiservicerecordskirmishpanelcontrol.h"

#include "player.h"
#include "usermanager.h"
#include "user.h"
#include "scoremanager.h"
#include "gamedirectories.h"
#include "gamemode.h"
#include "userprofilemanager.h"


//============================================================================
//============================================================================
BUIServiceRecordSkirmishPanelControl::BUIServiceRecordSkirmishPanelControl( void ) :
   mXuid(INVALID_XUID),
   mLevel(0),
   mQueryTimerSet(false)
{
   eventReceiverInit(cThreadIndexSim);
}

//============================================================================
//============================================================================
BUIServiceRecordSkirmishPanelControl::~BUIServiceRecordSkirmishPanelControl( void )
{
   if (mQueryTimerSet)
   {
      mQueryTimerSet = false;
      mQueryTimer.cancel();
      gEventDispatcher.deregisterHandle(mQueryTimer.getHandle(), cThreadIndexSim);
   }

   eventReceiverDeinit();
}

//============================================================================
//============================================================================
bool BUIServiceRecordSkirmishPanelControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, const BXMLNode* initData )
{
   bool result = __super::init( parent, controlPath, controlID, NULL );   

   if (result)
   {
      // Init the text fields
      result = mScrollList.init(this, mScriptPrefix+"mList", -1, NULL);
   }

   return result;
}

//============================================================================
//============================================================================
bool BUIServiceRecordSkirmishPanelControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = mScrollList.handleInput(port, event, controlType, detail);
   if (handled)
      return true;

   // Screen
   handled = BUIPanelControl::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   return handled;
}

//============================================================================
//============================================================================
bool BUIServiceRecordSkirmishPanelControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{   
   return false;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUIServiceRecordSkirmishPanelControl::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   bool handled = false;

   return handled;

}

//============================================================================
//============================================================================
bool BUIServiceRecordSkirmishPanelControl::populateFromUser(BUser* pUser)
{   
   if (!pUser)
      return false;

   BUString dataString1;
   BUString dataString2;

   populateSkirmishRecord(pUser, dataString1, dataString2);
   updateData(dataString1, dataString2);

   if (pUser->isLiveEnabled())
   {
      mXuid = pUser->getXuid();

      mLevel = gLSPManager.queryServiceRecord(mXuid);

      if (pUser->getSigninState() == eXUserSigninState_SignedInToLive && pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
         gLSPManager.updateServiceRecord(pUser->getXuid());

      if (!mQueryTimerSet)
      {
         mQueryTimerSet = true;

         BEvent handleEvent;
         handleEvent.clear();
         handleEvent.mFromHandle = mEventHandle;
         handleEvent.mToHandle = mEventHandle;
         handleEvent.mEventClass = cQueryEventTimer;
         gEventDispatcher.registerHandleWithEvent(mQueryTimer.getHandle(), handleEvent);

         mQueryTimer.set(2500);
      }
   }

   return true;
}

//============================================================================
//============================================================================
void BUIServiceRecordSkirmishPanelControl::updateData(const BUString& dataString1, const BUString& dataString2)
{
   mScrollList.updateTextBegin();

   int index = 0;
   int copyCount = dataString1.length();
   const int maxCharsCopied = 1024;

   BUString tempStr;   
   BUString debugString1;
   BUString debugString2;
   while (copyCount > 0)
   {
      int amountToCopy = copyCount;
      if (amountToCopy > maxCharsCopied)
         amountToCopy = maxCharsCopied;
      
      tempStr.copy(dataString1, amountToCopy, index);
      mScrollList.appendTextColumn0(tempStr);

      debugString1.append(tempStr);

      index+= amountToCopy;
      copyCount -= amountToCopy;
   }
      
   index = 0;
   copyCount = dataString2.length();
   while (copyCount > 0)
   {
      int amountToCopy = copyCount;
      if (amountToCopy > maxCharsCopied)
         amountToCopy = maxCharsCopied;
      
      tempStr.copy(dataString2, amountToCopy, index);
      mScrollList.appendTextColumn1(tempStr);

      debugString2.append(tempStr);

      index+= amountToCopy;
      copyCount -= amountToCopy;
   }


   mScrollList.updateTextEnd();
}

//============================================================================
//============================================================================
void BUIServiceRecordSkirmishPanelControl::populateSkirmishRecord(BUser* pUser, BUString& dataString1, BUString& dataString2)
{
   if (!pUser)
      return;

   BUserProfile* pProfile = const_cast<BUserProfile*>(pUser->getProfile());
   if (!pProfile)
      return;
   
   BUString tempStr;
   const int numLeaders = 6;
   int leaderNameID[numLeaders];
   leaderNameID[0] = 7000; // "Captain Cutter"
   leaderNameID[1] = 7002; // "Sergeant Forge"
   leaderNameID[2] = 7004; // "Professor Anders"
   leaderNameID[3] = 7010; // "Arbiter"
   leaderNameID[4] = 7012; // "Brute Chieftain"
   leaderNameID[5] = 7014; // "Prophet of Regret"

   dataString1.append(gDatabase.getLocStringFromID(25638)); // "Rank:"

   // Recruit
   // Recruit (Next rank at 10 wins)
   // Recruit Level 50
   // Recruit Level 50 (Next rank at 10 wins)
   // 
   // 25647 == Recruit
   // 25648 == Lieutenant
   // 25649 == Captain
   // 25650 == Major
   // 25651 == Commander
   // 25652 == Colonel
   // 25653 == Brigadier
   // 25654 == General
   //
   // 25656 == Unranked
   //
   // 25655 == %1!s! Level %2!d!
   // 25665 == (Next rank at %1!d! wins)
   //
   if (pProfile->getRank().mRank == 0)
      tempStr = gDatabase.getLocStringFromID(25656);
   else if (pProfile->getRank().mServerUpdated)
      tempStr.locFormat(gDatabase.getLocStringFromID(25655), gDatabase.getLocStringFromID(25647 + pProfile->getRank().mRank - 1).getPtr(), pProfile->getRank().mLevel);
   else
      tempStr = gDatabase.getLocStringFromID(25647 + pProfile->getRank().mRank - 1);

   appendIndention(dataString1, 1);
   dataString1.append(tempStr); 
   appendLineBreak(dataString1);

   // 25325 // Score: %1!d!
   tempStr.locFormat(gDatabase.getLocStringFromID(25325), pProfile->mMatchmakingScore);
   dataString1.append(tempStr);
   appendLineBreak(dataString1);

   appendLineBreak(dataString1);

   if (pProfile->getNextRankAt() > 0)
   {
      tempStr.locFormat(gDatabase.getLocStringFromID(25665), pProfile->getNextRankAt());
      dataString2.append(tempStr);
   }

   appendLineBreak(dataString2);
   appendLineBreak(dataString2);
   appendLineBreak(dataString2);

   // Games played and win%
   int totalGamesPlayed[2];
   int totalGamesWon[2];
   float winPercent[2];

   totalGamesPlayed[0] = 0;
   totalGamesPlayed[1] = 0;
   totalGamesWon[0] = 0;
   totalGamesWon[1] = 0;
   for(int i=0; i<2; i++)
   {
      for(int mode=0; mode<pProfile->mNumModes; mode++)
      {
         totalGamesPlayed[i] += pProfile->mSkirmishModes[i][mode].games;
         totalGamesWon[i] += pProfile->mSkirmishModes[i][mode].wins;
      }
   }

   for (int j=1; j>=0; j--)
   {
      winPercent[j] = 0.0f;
      if (totalGamesPlayed[j] > 0)
      {
         winPercent[j] = 100.0f * static_cast<float>(totalGamesWon[j]) / static_cast<float>(totalGamesPlayed[j]);
      }
   }

   dataString1.append(gDatabase.getLocStringFromID(25666)); // "Games Played:"
   appendLineBreak(dataString1);
   appendLineBreak(dataString2);

/*
   appendIndention(dataString1, 1);
   tempStr.locFormat(L"%d   ", totalGamesPlayed[1]);
   dataString1.append(tempStr);
   dataString1.append(gDatabase.getLocStringFromID(25668)); // "PUBLIC SKIRMISH GAMES"
   appendLineBreak(dataString1);
   dataString2.append(gDatabase.getLocStringFromID(25667)); // "Win Percentage:"
   tempStr.locFormat(L" %.2f   ", winPercent[1]);
   dataString2.append(tempStr);
   appendLineBreak(dataString2);
*/

//   appendIndention(dataString1, 1);
   tempStr.locFormat(L"%d   ", totalGamesPlayed[0]);
   dataString1.append(tempStr);
   dataString1.append(gDatabase.getLocStringFromID(25639)); // "PRIVATE SKIRMISH GAMES"
   appendLineBreak(dataString1);
   dataString2.append(gDatabase.getLocStringFromID(25667)); // "Win Percentage:"
   tempStr.locFormat(L" %.2f   ", winPercent[0]);
   dataString2.append(tempStr);
   appendLineBreak(dataString2);

/*
   appendLineBreak(dataString1);
   appendLineBreak(dataString2);
*/

   int j = 0;
   int k = 0;
   const BScenarioList& scenarioList = gDatabase.getScenarioList();
   int numSkirmishMaps = scenarioList.getMapCount();
   BUString typeStr[2];
//   typeStr[1] = gDatabase.getLocStringFromID(25668); // "PUBLIC SKIRMISH GAMES"
   typeStr[0] = gDatabase.getLocStringFromID(25639); // "PRIVATE SKIRMISH GAMES"

//   for (k=1; k>=0; k--)
   for (k=0; k>=0; k--)  //Only display the private games (k=0) for now.  Loop left in, incase this changes back
   {
      // Skirmish Stats (by Game Mode)
      dataString1.append(typeStr[k]);
      appendLineBreak(dataString1);
      appendLineBreak(dataString2);

      appendIndention(dataString1, 1);
      dataString1.append(gDatabase.getLocStringFromID(25642)); // "Selected Leader"
      appendLineBreak(dataString1);
      appendLineBreak(dataString2);

      j=0;
      for (i=0; i<gDatabase.getNumberLeaders(); i++)
      {
         BLeader *leader = gDatabase.getLeader(i);
         if( leader->mTest || (leader->mLeaderCivID != gDatabase.getCivID("UNSC") && leader->mLeaderCivID != gDatabase.getCivID("Covenant") ) )
            continue;

         BWinCount* winCount = pProfile->getLeaderWinsByIndex(k, i);

         appendIndention(dataString1, 2);
         dataString1.append(gDatabase.getLocStringFromID(leaderNameID[j++]));
         appendLineBreak(dataString1);
         dataString2.append(gDatabase.getLocStringFromID(25644)); // "Played:"
         tempStr.locFormat(L" %d   ", winCount->games);
         dataString2.append(tempStr);
         dataString2.append(gDatabase.getLocStringFromID(25645)); // "Won:"
         tempStr.locFormat(L" %d", winCount->wins);
         dataString2.append(tempStr);
         appendLineBreak(dataString2);
      }

      appendLineBreak(dataString1);
      appendLineBreak(dataString2);

      appendIndention(dataString1, 1);
      dataString1.append(gDatabase.getLocStringFromID(25643)); // "Selected Map"
      appendLineBreak(dataString1);
      appendLineBreak(dataString2);

      int mapIndex=0;
      for (i=0; i<numSkirmishMaps; i++)
      {
         const BScenarioMap* pScenario = scenarioList.getMapInfo(i);
         if (pScenario == NULL || pScenario->getType() != BScenarioMap::cScenarioTypeFinal || pScenario->getMapNameStringID() == 0)
            continue;

         BWinCount* winCount = pProfile->getMapWinsByIndex(k, i);

         appendIndention(dataString1, 2);
         dataString1.append(pScenario->getMapName()); // Map name
         appendLineBreak(dataString1);
         dataString2.append(gDatabase.getLocStringFromID(25644)); // "Played:"
         tempStr.locFormat(L" %d   ", winCount->games);
         dataString2.append(tempStr);
         dataString2.append(gDatabase.getLocStringFromID(25645)); // "Won:"
         tempStr.locFormat(L" %d", winCount->wins);
         dataString2.append(tempStr);
         appendLineBreak(dataString2);

         mapIndex++;
      }

      appendLineBreak(dataString1);
      appendLineBreak(dataString2);

      appendIndention(dataString1, 1);
      dataString1.append(gDatabase.getLocStringFromID(25669)); // "Game Mode:"
      appendLineBreak(dataString1);
      appendLineBreak(dataString2);

      int modeIndex=0;
      for (j=0; j<SR_MAX_NUM_MODES; j++)  //Modes are ranked and unranked
      {
         BGameMode* gameMode = gDatabase.getGameModeByID(j);
         if (!gameMode)
            continue;

         BWinCount* winCount = pProfile->getModeWinsByIndex(k, j);

         appendIndention(dataString1, 2);
         dataString1.append(gameMode->getDisplayName()); // Game Mode Name
         appendLineBreak(dataString1);
         dataString2.append(gDatabase.getLocStringFromID(25644)); // "Played:"
         tempStr.locFormat(L" %d   ", winCount->games);
         dataString2.append(tempStr);
         dataString2.append(gDatabase.getLocStringFromID(25645)); // "Won:"
         tempStr.locFormat(L" %d", winCount->wins);
         dataString2.append(tempStr);
         appendLineBreak(dataString2);

         modeIndex++;
      }

      appendLineBreak(dataString1);
      appendLineBreak(dataString2);
   }
}

//============================================================================
//============================================================================
void BUIServiceRecordSkirmishPanelControl::appendLineBreak(BUString& s)
{
   s.append(L"<br/>");
}

//============================================================================
//============================================================================
void BUIServiceRecordSkirmishPanelControl::appendIndention(BUString& s, uint num)
{
   for (uint i = 0; i<num; i++)
      s.append(L"   ");
}

//==============================================================================
// 
//==============================================================================
bool BUIServiceRecordSkirmishPanelControl::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cQueryEventTimer:
      {
         if (mXuid != INVALID_XUID)
         {
            uint level = gLSPManager.queryServiceRecord(mXuid);
            if (level != mLevel)
            {
               BUser* pUser = gUserManager.getPrimaryUser();
               if (pUser != NULL && pUser->getXuid() == mXuid)
               {
                  mLevel = level;

                  populateFromUser(pUser);
               }

               // shutoff the timer
               // if the level on the client and the server are the same, then the timer will fire
               // indefinitely but only while we're on the screen
               if (mQueryTimerSet)
               {
                  mQueryTimerSet = false;
                  mQueryTimer.cancel();
                  gEventDispatcher.deregisterHandle(mQueryTimer.getHandle(), cThreadIndexSim);
               }
            }
         }
      }
   }
   return false;
}
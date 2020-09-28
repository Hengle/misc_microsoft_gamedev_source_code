//============================================================================
// BUIServiceRecordCampaignPanelControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "uiservicerecordcampaignpanelcontrol.h"

#include "player.h"
#include "usermanager.h"
#include "user.h"
#include "userprofilemanager.h"
#include "scoremanager.h"
#include "skullmanager.h"
#include "campaignmanager.h"
#include "campaignprogress.h"
#include "configsgame.h"
#include "achievementmanager.h"
#include "userachievements.h"

//============================================================================
//============================================================================
BUIServiceRecordCampaignPanelControl::BUIServiceRecordCampaignPanelControl( void )
{
}

//============================================================================
//============================================================================
BUIServiceRecordCampaignPanelControl::~BUIServiceRecordCampaignPanelControl( void )
{
}


//============================================================================
//============================================================================
bool BUIServiceRecordCampaignPanelControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, const BXMLNode* initData )
{
   bool result = __super::init( parent, controlPath, controlID, NULL );
   if (result)
   {
      result = mScrollList.init(this, mScriptPrefix+"mList", -1, NULL);
   }
      
   return result;
}

//============================================================================
//============================================================================
bool BUIServiceRecordCampaignPanelControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
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
bool BUIServiceRecordCampaignPanelControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{   
   return false;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUIServiceRecordCampaignPanelControl::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   bool handled = false;

   return handled;

}

//============================================================================
//============================================================================
bool BUIServiceRecordCampaignPanelControl::populateFromUser(BUser* pUser)
{
   if (!pUser)
      return false;

   BUString dataString1;
   BUString dataString2;

   populateMissionData(pUser, dataString1, dataString2);

   updateData(dataString1, dataString2);   
   return true;
}

//============================================================================
//============================================================================
void BUIServiceRecordCampaignPanelControl::updateData(const BUString& dataString1, const BUString& dataString2)
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
void BUIServiceRecordCampaignPanelControl::populateMissionData(BUser* pUser, BUString& dataString1, BUString& dataString2)
{   
   if (!pUser)
      return;

   BUserProfile* pProfile = const_cast<BUserProfile*>(pUser->getProfile());
   if (!pProfile)
      return;

   int campaignCount = gCampaignManager.getNumberCampaigns();
   BCampaign* pCampaign = NULL;

   BUString tempStr;
   for (int i = 0; i < campaignCount; ++i)
   {
      pCampaign = gCampaignManager.getCampaign(i);
      if (!pCampaign)
         continue;

      bool bDisplayedPickups = false;

      int  missionCompletedCount = 0;
      // Set up both solo and co-op 
      for (int mode = 0; mode < 2; ++mode) // 0 = solo, 1 = coop
      {
         int numLevels = 0;
         int levelsCompleted[DifficultyType::cNumberStandardDifficultyTypes];
         for (int k = 0; k < DifficultyType::cNumberStandardDifficultyTypes; ++k)
            levelsCompleted[k] = 0;

         BCampaignNode* pTestNode = NULL;
         for (int j=1; j<pCampaign->getNumberNodes(); ++j)
         {
            pTestNode = pCampaign->getNode(j);
            if (!pTestNode)
               continue;  

            if (pTestNode->getFlag(BCampaignNode::cCinematic))
               continue;

            numLevels++;
            for (int k = 0; k < DifficultyType::cNumberStandardDifficultyTypes; ++k)
            {
               if (pProfile->mCampaignProgress.isScenarioCompleted(pTestNode->getID(), mode, k))
               {
                  levelsCompleted[k]++;
               }
            }
         }

         // Add the display name for the campaign - if necessary
         dataString1.append(gDatabase.getLocStringFromID(pCampaign->getDisplayNameStringID()));
         appendIndention(dataString1, 2);
         dataString1.append(L"(");
         if (mode == 0)
            dataString1.append(gDatabase.getLocStringFromID(25134)); // "Solo"
         else
            dataString1.append(gDatabase.getLocStringFromID(25133)); // "Co-op"
         dataString1.append(L")");

         // Include the Lifetime Campaign Score
         uint32 careerScore = pProfile->mCampaignProgress.getLifetimeCampaignScore(mode);
         dataString2.append(gDatabase.getLocStringFromID(25627)); // "Lifetime Score:"
         tempStr.locFormat(L" %d", careerScore);
         dataString2.append(tempStr); 
         appendLineBreak(dataString1);         
         appendLineBreak(dataString2);

         // General Level completion and career scores
         for (int k = 0; k < DifficultyType::cNumberStandardDifficultyTypes; ++k)
         {
            appendIndention(dataString1, 1);
            if (levelsCompleted[k] < 10)
               appendIndention(dataString1, 1); // align spacing
            tempStr.locFormat(L"%d/%d", levelsCompleted[k], numLevels);
            dataString1.append(tempStr); 
            appendIndention(dataString1, 1);
            dataString1.append(gDatabase.getDifficultyStringByType(k));
            appendLineBreak(dataString1);

            uint32 score = pProfile->mCampaignProgress.getLifetimeScore(mode, k);
            dataString2.append(gDatabase.getLocStringFromID(25627)); // "Lifetime Score:"
            tempStr.locFormat(L" %d", score);
            dataString2.append(tempStr);
            appendLineBreak(dataString2);
         }
         appendLineBreak(dataString1);  // Extra space
         appendLineBreak(dataString2);

         if (!bDisplayedPickups)
         {
            // Count Skulls and Black Boxes
            int skullsFound = 0;
            int blackBoxesFound = 0;
            BCampaignNode* pNode = NULL;
            bool bSkullOrBoxFound[50];
            for (int j=1; j<pCampaign->getNumberNodes(); ++j)
            {
               pNode = pCampaign->getNode(j);
               if (!pNode)
                  continue;  

               if (pNode->getFlag(BCampaignNode::cCinematic))
                  continue;

               // Black boxes and skulls found
               bool bSkullUnlocked = gCollectiblesManager.hasSkullBeenCollected(pUser, pProfile->mCampaignProgress.scenarioNodeIDToProgressIndex(j));
               bool bBlackBoxUnlocked = gCollectiblesManager.hasBlackBoxBeenCollected(pUser, pProfile->mCampaignProgress.scenarioNodeIDToProgressIndex(j));

               if (bSkullUnlocked) skullsFound++;
               if (bBlackBoxUnlocked) blackBoxesFound++;
               if (bSkullUnlocked || bBlackBoxUnlocked)
                  bSkullOrBoxFound[j] = true;
               else
                  bSkullOrBoxFound[j] = false;
            }

            appendIndention(dataString1, 1);
            if (skullsFound < 10)
               appendIndention(dataString1, 1); // align spacing
            tempStr.locFormat(L"%d/%d", skullsFound, numLevels);
            dataString1.append(tempStr); 
            appendIndention(dataString1, 1);
            dataString1.append(gDatabase.getLocStringFromID(25670)); // "Skulls Found"
            appendLineBreak(dataString1);
            appendLineBreak(dataString2);

            appendIndention(dataString1, 1);
            if (blackBoxesFound < 10)
               appendIndention(dataString1, 1); // align spacing
            tempStr.locFormat(L"%d/%d", blackBoxesFound, numLevels);
            dataString1.append(tempStr); 
            appendIndention(dataString1, 1);
            dataString1.append(gDatabase.getLocStringFromID(25671)); // "Black Boxes Found"
            appendLineBreak(dataString1);
            appendLineBreak(dataString2);

            // Go through each mission in this campaign and show Skulls and Black Boxes found 
            for (int j=1; j<pCampaign->getNumberNodes(); ++j)
            {
               pNode = pCampaign->getNode(j);
               if (!pNode)
                  continue;  

               if (pNode->getFlag(BCampaignNode::cCinematic))
                  continue;

               if (bSkullOrBoxFound[j] || gConfig.isDefined(cConfigServiceRecordShowAll))
               {
                  appendIndention(dataString1, 6);
                  dataString1.append(gDatabase.getLocStringFromID(pNode->getDisplayNameStringID()));

                  // Black boxes and skulls found
                  bool bSkullUnlocked = gCollectiblesManager.hasSkullBeenCollected(pUser, pProfile->mCampaignProgress.scenarioNodeIDToProgressIndex(j));
                  bool bBlackBoxUnlocked = gCollectiblesManager.hasBlackBoxBeenCollected(pUser, pProfile->mCampaignProgress.scenarioNodeIDToProgressIndex(j));

                  dataString2.append(gDatabase.getLocStringFromID(25628)); // "Skull:"
                  dataString2.append(L" ");
                  if (bSkullUnlocked)
                     dataString2.append(gDatabase.getLocStringFromID(25118)); // "Yes"
                  else
                     dataString2.append(gDatabase.getLocStringFromID(25119)); // "No"

                  appendIndention(dataString2, 2);
                  dataString2.append(gDatabase.getLocStringFromID(25629)); // "Black Box:"
                  dataString2.append(L" ");
                  if (bBlackBoxUnlocked)
                     dataString2.append(gDatabase.getLocStringFromID(25118)); // "Yes"
                  else
                     dataString2.append(gDatabase.getLocStringFromID(25119)); // "No"

                  appendLineBreak(dataString1);
                  appendLineBreak(dataString2);
               }
            }
            bDisplayedPickups = true;

            appendLineBreak(dataString1);  // Extra space
            appendLineBreak(dataString2);
         }

         BCampaignNode* pNode = NULL;
         // Go through each mission in this campaign difficulty 
         for (int j=1; j<pCampaign->getNumberNodes(); ++j)
         {
            pNode = pCampaign->getNode(j);
            if (!pNode)
               continue;  

            if (pNode->getFlag(BCampaignNode::cCinematic))
               continue;

            //-- don't display any data for scenarios that are locked
            if (!gConfig.isDefined(cConfigServiceRecordShowAll))
            {
               if (!pProfile->mCampaignProgress.isScenarioUnlocked(pNode->getID()))
                  continue;

               bool bSkip = true;
               for (int difficulty = 0; difficulty < DifficultyType::cNumberStandardDifficultyTypes; ++difficulty)
               {
                  if (pProfile->mCampaignProgress.isScenarioCompleted(pNode->getID(), mode, difficulty))
                     bSkip = false;
               }

               if (bSkip)
                  continue;
            }

            // Level Title
            appendIndention(dataString1, 1);
            dataString1.append(gDatabase.getLocStringFromID(pNode->getDisplayNameStringID()));

            appendLineBreak(dataString1);
            appendLineBreak(dataString2);

            for (int k = 0; k < DifficultyType::cNumberStandardDifficultyTypes; ++k)
            {
               if (!gConfig.isDefined(cConfigServiceRecordShowAll))
               {
                  if (!pProfile->mCampaignProgress.isScenarioCompleted(pNode->getID(), mode, k))
                     continue;
               }

               appendIndention(dataString1, 2);
               dataString1.append(gDatabase.getDifficultyStringByType(k));
               uint32 score = pProfile->mCampaignProgress.getScenarioScore(pNode->getID(), mode, k);
               long   medalDisplayStringID = pProfile->mCampaignProgress.getScenarioMedalStringID(pNode->getID(), mode, k);   

               tempStr.locFormat(L"    (%s)", gDatabase.getLocStringFromID(medalDisplayStringID).getPtr());
               dataString1.append(tempStr);

               dataString2.append(gDatabase.getLocStringFromID(25633)); // "Score:"
               tempStr.locFormat(L" %d", score);
               dataString2.append(tempStr);

               appendIndention(dataString2, 2);
               bool bParMet = pProfile->mCampaignProgress.isScenarioParBeaten(j, mode, k);
               dataString2.append(gDatabase.getLocStringFromID(25631)); // "Par:"
               dataString2.append(L" ");
               if (bParMet)
                  dataString2.append(gDatabase.getLocStringFromID(25118)); // "Yes"
               else
                  dataString2.append(gDatabase.getLocStringFromID(25119)); // "No"

               appendLineBreak(dataString1);
               appendLineBreak(dataString2);
            }

            missionCompletedCount ++;
         }

         appendLineBreak(dataString1);
         appendLineBreak(dataString2);
      }
      if (missionCompletedCount > 0)
      {
         // add a line break after each for readability
         appendLineBreak(dataString1);
         appendLineBreak(dataString2);
      }
   }
}

//============================================================================
//============================================================================
void BUIServiceRecordCampaignPanelControl::appendLineBreak(BUString& s)
{
   s.append(L"<br/>");
}

//============================================================================
//============================================================================
void BUIServiceRecordCampaignPanelControl::appendIndention(BUString& s, uint num)
{
   for (uint i = 0; i<num; i++)
      s.append(L"   ");
}
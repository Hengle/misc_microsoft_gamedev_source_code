//==============================================================================
// campaignProgress.cpp
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "campaignProgress.h"
#include "campaignmanager.h"
#include "stream\dynamicStream.h"
#include "compressedStream.h"
#include "user.h"
#include "usermanager.h"
#include "userprofilemanager.h"
#include "scoremanager.h"
#include "uiticker.h"
#include "configsgame.h"

#ifndef BUILD_FINAL
#include "debugTextDisplay.h"
#endif

// Constants

// Globals


//============================================================================
//============================================================================
BCampaignProgress::BCampaignProgress()
{
   clear();
}

//============================================================================
//============================================================================
BCampaignProgress::~BCampaignProgress()
{
}


//============================================================================
//============================================================================
void BCampaignProgress::clear()
{
   for(int i=0; i<CAMPAIGN_MAX_SCENARIOS; i++)
   {
      for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
      {
         for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
         {
            mScenarioScores[i][j][k] = 0;
            mScenarioGrades[i][j][k] = 0;
            mScenarioTimes[i][j][k] = 0;
            mScenarioParMet[i][j][k] = false;

            mLifetimeScore[j][k] = 0;
         }
      }
      mScenarioUnlocked[i] = false;
      mScenarioNodeID[i] = -999;
   }

   mLifetimeCampaignScore = 0;

   mCurrentCampaignNode = -1;
   mHighestUnlockedScenario = 0;
   mLowestUnbeatenScenario = 0;
   mbNodeIDTableConstructed = false;
   mSessionID = 0;
   mNumberScenarios = 0;

   mCinematicProgressMappings.clear();

   // [7/16/2008 xemu] added this to clear to ensure that the starting state before you ever do anything is correct.
   // [7/16/2008 xemu] may need to just call ComputeVariables() here? 
   mScenarioUnlocked[0] = true; 

}

//============================================================================
//============================================================================
void BCampaignProgress::enterScore( uint scenarioID, uint mode, uint difficulty, long playerID, uint score, uint grade, uint time, uint partime )
{
   int index = scenarioNodeIDToProgressIndex(scenarioID);
   if(index < 0)
      return;

   //time should be in seconds

   bool changeMade = false;

   if( score > mScenarioScores[index][mode][difficulty] )
   {
      if( mScenarioScores[index][mode][difficulty] > 0 )
         gScoreManager.reportCampaignScoreImproved(playerID);

      mScenarioScores[index][mode][difficulty] = score;
      mScenarioGrades[index][mode][difficulty] = (uint8)grade;
      changeMade = true;
   }

   if( (mScenarioScores[index][mode][difficulty] > 0) && ((time < mScenarioTimes[index][mode][difficulty]) || (mScenarioTimes[index][mode][difficulty])==0) )
   {
      if(time > SCENARIO_MAX_TIME)
         time = SCENARIO_MAX_TIME;

      mScenarioTimes[index][mode][difficulty] = (uint16)time;
      if( time <= partime )
         mScenarioParMet[index][mode][difficulty] = true;

      changeMade = true;
   }

   mLifetimeCampaignScore += score;
   mLifetimeScore[mode][difficulty] += score;

   if(changeMade)
      computeVariables();
}

//============================================================================
//============================================================================
bool BCampaignProgress::writeToStream( BDynamicStream &stream )
{
   if(stream.writeObj(mCurrentCampaignNode) == false)
   return(false);

   uint8 numScenarios = CAMPAIGN_MAX_SCENARIOS;
   if(stream.writeObj(numScenarios) == false)
   return(false);

   uint8 numModes = CAMPAIGN_MAX_MODES;
   if(stream.writeObj(numModes) == false)
   return(false);

   uint8 numDifficulties = CAMPAIGN_DIFFICULTY_LEVELS;
   if(stream.writeObj(numDifficulties) == false)
   return(false);

   for(int i=0; i<CAMPAIGN_MAX_SCENARIOS; i++)
   {
      for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
      {
         for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
         {
            //24 bits - top 4 for the grade, next 20 for the score (4b + 8b + 8b)
            uint8 byte1 = ((mScenarioGrades[i][j][k] & 0xF) << 4) | uint8((mScenarioScores[i][j][k] >> 16) & 0xF);
            uint8 byte2 = uint8((mScenarioScores[i][j][k] >> 8) & 0xFF);
            uint8 byte3 = uint8((mScenarioScores[i][j][k]) & 0xFF);
            if(stream.writeObj(byte1) == false)
            return(false);
            if(stream.writeObj(byte2) == false)
            return(false);
            if(stream.writeObj(byte3) == false)
            return(false);
            //16 bits - top bit for whether par is met, 15 lower bits for time in seconds (max ~9 hrs)
            uint16 dbyte = (mScenarioTimes[i][j][k] & 0x7FFF);
            if( mScenarioParMet[i][j][k] == true )
               dbyte = dbyte | 0x8000;
            if(stream.writeObj(dbyte) == false)
            return(false);
         }
      }
   }

   if(stream.writeObj(mLifetimeCampaignScore) == false)
   return(false);

   if(stream.writeObj(mSessionID) == false)
   return(false);
   
   for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
   {
      for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
      {
            if(stream.writeObj(mLifetimeScore[j][k]) == false)
            return(false);
      }
   }

   return(true);
}

//============================================================================
//============================================================================
bool BCampaignProgress::readFromStream( BInflateStream* pInfStream, uint8 version )
{
   uint8 numScenarios = CAMPAIGN_MAX_SCENARIOS;
   uint8 numModes = CAMPAIGN_MAX_MODES;
   uint8 numDifficulties = CAMPAIGN_DIFFICULTY_LEVELS;

   *pInfStream >> mCurrentCampaignNode;
   *pInfStream >> numScenarios;
   *pInfStream >> numModes;
   *pInfStream >> numDifficulties;

   BASSERT(numScenarios<=CAMPAIGN_MAX_SCENARIOS);
   BASSERT(numModes<=CAMPAIGN_MAX_MODES);
   BASSERT(numDifficulties<=CAMPAIGN_DIFFICULTY_LEVELS);

   for(int i=0; i<numScenarios; i++)
   {
      for(int j=0; j<numModes; j++)
      {
         for(int k=0; k<numDifficulties; k++)
         {
            //24 bits - top 4 for the grade, next 20 for the score (4b + 8b + 8b)
            uint8 byte1 = 0;
            uint8 byte2 = 0;
            uint8 byte3 = 0;

            *pInfStream >> byte1;
            *pInfStream >> byte2;
            *pInfStream >> byte3;

            mScenarioGrades[i][j][k] = (byte1 & 0xF0)>>4;
            mScenarioScores[i][j][k] = (uint32(byte1 & 0x0F)<<16) | (uint32(byte2)<<8) | (uint32(byte3));

            uint16 dbyte = 0;
            *pInfStream >> dbyte;

            mScenarioTimes[i][j][k] = (dbyte & 0x7FFF);
            mScenarioParMet[i][j][k] = ((dbyte & 0x8000) != 0);
         }
      }
   }

   *pInfStream >> mLifetimeCampaignScore;

   if (version >= 16)
      *pInfStream >> mSessionID;
   
   if (version >= 17)
   {
      for(int j=0; j<numModes; j++)
      {
         for(int k=0; k<numDifficulties; k++)
         {
            *pInfStream >> mLifetimeScore[j][k];
         }
      }
   }

   computeVariables();

   return(true);
}

//============================================================================
//============================================================================
void BCampaignProgress::computeVariables()
{
   mScenarioUnlocked[0] = true;  //First Scenario is always unlocked.

   mLowestUnbeatenScenario = CAMPAIGN_MAX_SCENARIOS;

   for(int i=0; i<CAMPAIGN_MAX_SCENARIOS; i++)
   {
      bool scenarioCompleted = false;
      for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
      {
         for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
         {
            //This assumes that you do not get a saved score unless you beat the scenario.
            if(mScenarioScores[i][j][k] > 0)
            {
               scenarioCompleted = true;
               break;
            }
         }
      }

      if( scenarioCompleted )
      {
         mScenarioUnlocked[i] = true;
         if( (uint)i < CAMPAIGN_MAX_SCENARIOS-1 )
            mScenarioUnlocked[i+1] = true;
      }
      else
      {
         if( (uint)i < mLowestUnbeatenScenario )
            mLowestUnbeatenScenario = i;
      }
   }

   mHighestUnlockedScenario = 0;
   for(int i=CAMPAIGN_MAX_SCENARIOS-1; i>=0; i--)
   {
      if( mScenarioUnlocked[i] )
      {
         mHighestUnlockedScenario = i;
         break;
      }
   }
   
}

//============================================================================
//============================================================================
void BCampaignProgress::constructNodeIDTable()
{
   BCampaign *campaign = gCampaignManager.getCampaign(0);
   BASSERT(campaign);

   int index = 0;
   for(int i=0; i<campaign->getNumberNodes() && index<CAMPAIGN_MAX_SCENARIOS; i++)
   {
      if( campaign->getNode(i)->getFlag(BCampaignNode::cCinematic) )
      {
         // [7/17/2008 xemu] cinematic nodes, create a new mapping element
         BCinematicProgressMapping progressMapping;
         progressMapping.mCinematicNodeID = i;
         // [7/17/2008 xemu] note we want to use index here before it is incremented, since cinematics unlock along with their associated scenario 
         progressMapping.mProgressIndex = index;

         mCinematicProgressMappings.add(progressMapping);
         continue;
      }

      //We do not save any information for the tutorial nodes
      if( campaign->getNode(i)->getFlag(BCampaignNode::cTutorial) )
      {
         continue;
      }

      // [7/17/2008 xemu] if not a cinematic node, implied to be a scenario
      mScenarioNodeID[index] = i;
      index++;
   }
   mNumberScenarios = index;
   mbNodeIDTableConstructed = true;
}

//============================================================================
//============================================================================
void BCampaignProgress::setCurrentCampaignNode(long nodeID)
{ 
   mCurrentCampaignNode = nodeID; 
   int index = scenarioNodeIDToProgressIndex(nodeID);
   if(index>0)
      mScenarioUnlocked[index] = true;
}

//============================================================================
//============================================================================
long BCampaignProgress::scenarioNodeIDToProgressIndex(long nodeID)
{ 
   if( !mbNodeIDTableConstructed )
      constructNodeIDTable();

   for(int i=0; i<CAMPAIGN_MAX_SCENARIOS; i++)
   {
      if(mScenarioNodeID[i] == nodeID)
         return i;
   }
   return(-1);
}

//============================================================================
//============================================================================
long BCampaignProgress::progressIndexToScenarioNodeID(long index)
{ 
   if( !mbNodeIDTableConstructed )
      constructNodeIDTable();

   if(index < CAMPAIGN_MAX_SCENARIOS)
      return mScenarioNodeID[index];
   else
      return(-1);
}

//============================================================================
//============================================================================
long BCampaignProgress::cinematicNodeIDToProgressIndex(long nodeID)
{ 
   // [7/17/2008 xemu] we need the node ID tables just like in the scenario version 
   if( !mbNodeIDTableConstructed )
      constructNodeIDTable();

   // [7/17/2008 xemu] ok, so here we go through and we are actually looking for the "progress index" that matches
   int i;
   for (i=0; i < mCinematicProgressMappings.getNumber(); i++)
   {
      if (mCinematicProgressMappings[i].mCinematicNodeID == nodeID)
         return mCinematicProgressMappings[i].mProgressIndex;
   }

   // [7/17/2008 xemu] no match, so return failure value 
   return(-1);
}

//============================================================================
//============================================================================
bool BCampaignProgress::isScenarioUnlocked(long nodeID)
{
#ifndef BUILD_FINAL
   if (gConfig.isDefined("unlockAllCampaignMissions"))
      return true;
#endif

   if (gConfig.isDefined(cConfigDemo2))
   {
      if (nodeID <= 7)
         return true;
      else
         return false;
   }

   long progressIndex = scenarioNodeIDToProgressIndex(nodeID);

   // [7/16/2008 xemu] if we have no progress node, not sure what the right default is, but for now lets say anything unspecified is unlocked. 
   if (progressIndex == -1)
      return(true);

   BASSERT(progressIndex < CAMPAIGN_MAX_SCENARIOS);
   bool retval = mScenarioUnlocked[progressIndex];
   return(retval);
}

//============================================================================
//============================================================================
bool BCampaignProgress::isCinematicUnlocked(long nodeID)
{
   // [7/17/2008 xemu] find the mapping of cinematic to related scenario unlock
   long progressIndex = cinematicNodeIDToProgressIndex(nodeID);

   // [7/16/2008 xemu] if we have no progress node, not sure what the right default is, but for now lets say anything unspecified is unlocked. 
   if (progressIndex == -1)
      return(true);

   BASSERT(progressIndex < CAMPAIGN_MAX_SCENARIOS);
   bool retval = mScenarioUnlocked[progressIndex];
   return(retval);
}

//============================================================================
//============================================================================
BCampaignProgress *BCampaignProgress::getCampaignProgress(bool *pShowAll)
{
   bool showAll = gConfig.isDefined("unlockAllCampaignMissions") || gConfig.isDefined(cConfigDemo2);

   BUser * const user = gUserManager.getPrimaryUser();
   BASSERT(user);
   BUserProfile *pProfile = NULL;

   // [7/28/2008 xemu] removed check for being signed in to deal better with the default profile 
   if (user != NULL) 
   {
      pProfile = user->getProfile();
   }

   BCampaignProgress *pCampaignProgress = NULL;
   if (pProfile == NULL)
   {
      showAll = true;
   } 
   else
   {
      pCampaignProgress = &pProfile->mCampaignProgress;
   }

   if (pShowAll != NULL)
      *pShowAll = showAll;
   // [7/28/2008 xemu] ok, right now you can actually get in this state, and upstream code is now robust enough to handle it.  But it's not properly
   //    designed right now (it's oriented towards dev condition) so putting in an assert to make sure we deal with it.
   BASSERT(pCampaignProgress);
   return pCampaignProgress;
}


//============================================================================
//============================================================================
long BCampaignProgress::getUnlockedMissionCount() const
{
   long i;
   long count = 0;

   bool showAll = gConfig.isDefined("unlockAllCampaignMissions");
   if (showAll)
      return(CAMPAIGN_MAX_SCENARIOS);

   if (gConfig.isDefined(cConfigDemo2))
      return 8;

   for (i=0; i < CAMPAIGN_MAX_SCENARIOS; i++)
   {
      if (mScenarioUnlocked[i])
         count++;
   }

   return(count);
}

//============================================================================
//============================================================================
long BCampaignProgress::getUnlockedMovieCount() const
{
   bool showAll = gConfig.isDefined("unlockAllCampaignMissions");
   long cMaxCinematics = 17;  // ugh
   if (showAll)
      return(cMaxCinematics);

   if (gConfig.isDefined(cConfigDemo2))
      return 5;

   long count = 0;
   long i;
   for (i=0; i < mCinematicProgressMappings.getNumber(); i++)
   {
      long progressIndex = mCinematicProgressMappings[i].mProgressIndex;
      if (mScenarioUnlocked[progressIndex])
         count++;
   }
   return(count);
}

//============================================================================
//============================================================================
uint32 BCampaignProgress::getScenarioScore(uint scenarioID, uint mode, uint difficulty) 
{
   int index = scenarioNodeIDToProgressIndex(scenarioID);
   if(index < 0)
      return 0;

   BASSERT((index < CAMPAIGN_MAX_SCENARIOS) && (index >= 0)); 
   BASSERT((mode < CAMPAIGN_MAX_MODES) && (mode >= 0)); 
   BASSERT((difficulty < CAMPAIGN_DIFFICULTY_LEVELS) && (difficulty >= 0)); 
   return (mScenarioScores[index][mode][difficulty]); 
}

//============================================================================
//============================================================================
uint32 BCampaignProgress::getScenarioMedal(uint scenarioID, uint mode, uint difficulty) 
{
   int index = scenarioNodeIDToProgressIndex(scenarioID);
   if(index < 0)
      return 0;

   BASSERT((index < CAMPAIGN_MAX_SCENARIOS) && (index >= 0)); 
   BASSERT((mode < CAMPAIGN_MAX_MODES) && (mode >= 0)); 
   BASSERT((difficulty < CAMPAIGN_DIFFICULTY_LEVELS) && (difficulty >= 0)); 
   return (mScenarioGrades[index][mode][difficulty]); 
}

//============================================================================
//============================================================================
long BCampaignProgress::getScenarioMedalStringID(uint scenarioID, uint mode, uint difficulty) 
{
   uint32 grade = getScenarioMedal(scenarioID, mode, difficulty);
   return gScoreManager.getStringIDForGrade(grade);
}

//============================================================================
//============================================================================
bool BCampaignProgress::getScenarioParMet(uint scenarioID, int mode, uint difficulty) 
{
   int index = scenarioNodeIDToProgressIndex(scenarioID);
   if(index < 0)
      return false;

   BASSERT((index < CAMPAIGN_MAX_SCENARIOS) && (index >= 0)); 
   BASSERT((mode == -1) || ((mode < CAMPAIGN_MAX_MODES) && (mode >= 0)));
   BASSERT((difficulty < CAMPAIGN_DIFFICULTY_LEVELS) && (difficulty >= 0)); 
   // [10/23/2008 xemu] mode -1 means beat in either mode
   if (mode == -1)
   {
      if (mScenarioParMet[index][0][difficulty])
         return(true);
      else 
         return (mScenarioParMet[index][1][difficulty]);
   }
   else
      return (mScenarioParMet[index][mode][difficulty]); 
}

//============================================================================
//============================================================================
uint32 BCampaignProgress::getScenarioTime(uint scenarioID, uint mode, uint difficulty) 
{
   int index = scenarioNodeIDToProgressIndex(scenarioID);
   if(index < 0)
      return false;

   BASSERT((index < CAMPAIGN_MAX_SCENARIOS) && (index >= 0)); 
   BASSERT((mode < CAMPAIGN_MAX_MODES) && (mode >= 0)); 
   BASSERT((difficulty < CAMPAIGN_DIFFICULTY_LEVELS) && (difficulty >= 0)); 
   return (mScenarioTimes[index][mode][difficulty]); 
}

//============================================================================
//============================================================================
bool BCampaignProgress::isScenarioCompleted(uint scenarioID, int mode, int difficulty) 
{
   uint modeStart = 0;
   uint modeStop = CAMPAIGN_MAX_MODES-1;
   uint diffStart = 0;
   uint diffStop = CAMPAIGN_DIFFICULTY_LEVELS-1;

   // [10/15/2008 xemu] mode -1 means "any mode"
   if (mode == -1)
   {
      modeStart = 0;
      modeStop = CAMPAIGN_MAX_MODES - 1;
   }
   else if ( (mode >= 0) && (mode < CAMPAIGN_MAX_MODES) )  //specific mode specified
      modeStart = modeStop = mode;

   if( (difficulty >= 0) && (difficulty < CAMPAIGN_DIFFICULTY_LEVELS) )  //specific difficulty specified
      diffStart = diffStop = difficulty;

   for(uint j=modeStart; j<=modeStop; j++)
   {
      for(uint k=diffStart; k<=diffStop; k++)
      {
         if( getScenarioScore(scenarioID, j, k) > 0 )
            return true;
      }
   }
   return false;
}

//============================================================================
//============================================================================
bool BCampaignProgress::isScenarioCompleted(uint scenarioID, int mode, int diffStart, int diffStop) 
{
   BASSERT(diffStart <= diffStop);
   BASSERT(diffStart >= 0);
   BASSERT(diffStop <= CAMPAIGN_DIFFICULTY_LEVELS - 1);

   uint modeStart = 0;
   uint modeStop = CAMPAIGN_MAX_MODES-1;

   // [10/15/2008 xemu] mode -1 means "any mode"
   if (mode == -1)
   {
      modeStart = 0;
      modeStop = CAMPAIGN_MAX_MODES - 1;
   }
   else if ( (mode >= 0) && (mode < CAMPAIGN_MAX_MODES) )  //specific mode specified
      modeStart = modeStop = mode;

   for(uint j=modeStart; j<=modeStop; j++)
   {
      for(uint k=diffStart; k<=diffStop; k++)
      {
         if( getScenarioScore(scenarioID, j, k) > 0 )
            return true;
      }
   }
   return false;
}

//============================================================================
//============================================================================
bool BCampaignProgress::isScenarioCompletedByProgressIndex(uint index, int mode, int difficulty) 
{
   uint modeStart = 0;
   uint modeStop = CAMPAIGN_MAX_MODES-1;
   uint diffStart = 0;
   uint diffStop = CAMPAIGN_DIFFICULTY_LEVELS-1;

   // [10/15/2008 xemu] mode -1 means "any mode"
   if (mode == -1)
   {
      modeStart = 0;
      modeStop = CAMPAIGN_MAX_MODES - 1;
   }
   else if ( (mode >= 0) && (mode < CAMPAIGN_MAX_MODES) )  //specific mode specified
      modeStart = modeStop = mode;

   if( (difficulty >= 0) && (difficulty < CAMPAIGN_DIFFICULTY_LEVELS) )  //specific difficulty specified
      diffStart = diffStop = difficulty;

   for(uint j=modeStart; j<=modeStop; j++)
   {
      for(uint k=diffStart; k<=diffStop; k++)
      {
         if( mScenarioScores[index][j][k] > 0 )
            return true;
      }
   }
   return false;
}

//============================================================================
//============================================================================
bool BCampaignProgress::isScenarioParBeaten(uint scenarioID, int mode, int difficulty) 
{
   uint modeStart = 0;
   uint modeStop = CAMPAIGN_MAX_MODES-1;
   uint diffStart = 0;
   uint diffStop = CAMPAIGN_DIFFICULTY_LEVELS-1;

   // [10/15/2008 xemu] mode -1 means "any mode"
   if (mode == -1)
   {
      modeStart = 0;
      modeStop = CAMPAIGN_MAX_MODES - 1;
   }
   else if ( (mode >= 0) && (mode < CAMPAIGN_MAX_MODES) )  //specific mode specified
      modeStart = modeStop = mode;

   if( (difficulty >= 0) && (difficulty < CAMPAIGN_DIFFICULTY_LEVELS) )  //specific difficulty specified
      diffStart = diffStop = difficulty;

   for(uint j=modeStart; j<=modeStop; j++)
   {
      for(uint k=diffStart; k<=diffStop; k++)
      {
         if( getScenarioParMet(scenarioID, j, k) )
            return true;
      }
   }
   return false;
}

//============================================================================
//============================================================================
bool BCampaignProgress::isScenarioParBeaten(uint scenarioID, int mode, int diffStart, int diffStop) 
{
   BASSERT(diffStart <= diffStop);
   BASSERT(diffStart >= 0);
   BASSERT(diffStop <= CAMPAIGN_DIFFICULTY_LEVELS - 1);

   uint modeStart = 0;
   uint modeStop = CAMPAIGN_MAX_MODES-1;

   // [10/15/2008 xemu] mode -1 means "any mode"
   if (mode == -1)
   {
      modeStart = 0;
      modeStop = CAMPAIGN_MAX_MODES - 1;
   }
   else if ( (mode >= 0) && (mode < CAMPAIGN_MAX_MODES) )  //specific mode specified
      modeStart = modeStop = mode;

   for(uint j=modeStart; j<=modeStop; j++)
   {
      for(uint k=diffStart; k<=diffStop; k++)
      {
         if( getScenarioParMet(scenarioID, j, k) )
            return true;
      }
   }
   return false;
}

//============================================================================
//============================================================================
bool BCampaignProgress::isScenarioMedalEarned(uint medalRequired, uint scenarioID, int mode, int difficulty) 
{
   uint modeStart = 0;
   uint modeStop = CAMPAIGN_MAX_MODES-1;
   uint diffStart = 0;
   uint diffStop = CAMPAIGN_DIFFICULTY_LEVELS-1;

   // [10/15/2008 xemu] mode -1 means "any mode"
   if (mode == -1)
   {
      modeStart = 0;
      modeStop = CAMPAIGN_MAX_MODES - 1;
   }
   else if ( (mode >= 0) && (mode < CAMPAIGN_MAX_MODES) )  //specific mode specified
      modeStart = modeStop = mode;

   if( (difficulty >= 0) && (difficulty < CAMPAIGN_DIFFICULTY_LEVELS) )  //specific difficulty specified
      diffStart = diffStop = difficulty;

   for(uint j=modeStart; j<=modeStop; j++)
   {
      for(uint k=diffStart; k<=diffStop; k++)
      {
         int medal = getScenarioMedal(scenarioID, j, k);
         if( (medal > 0) && (medal <= (int)medalRequired) )   //currently allowing for higher medals to qualify
            return true;
      }
   }
   return false;
}

//============================================================================
//============================================================================
bool BCampaignProgress::isScenarioMedalEarned(uint medalRequired, uint scenarioID, int mode, int diffStart, int diffStop)
{
   BASSERT(diffStart <= diffStop);
   BASSERT(diffStart >= 0);
   BASSERT(diffStop <= CAMPAIGN_DIFFICULTY_LEVELS - 1);

   uint modeStart = 0;
   uint modeStop = CAMPAIGN_MAX_MODES-1;

   // [10/15/2008 xemu] mode -1 means "any mode"
   if (mode == -1)
   {
      modeStart = 0;
      modeStop = CAMPAIGN_MAX_MODES - 1;
   }
   else if ( (mode >= 0) && (mode < CAMPAIGN_MAX_MODES) )  //specific mode specified
      modeStart = modeStop = mode;

   for(uint j=modeStart; j<=modeStop; j++)
   {
      for(uint k=diffStart; k<=diffStop; k++)
      {
         int medal = getScenarioMedal(scenarioID, j, k);
         if( (medal > 0) && (medal <= (int)medalRequired) )   //currently allowing for higher medals to qualify
            return true;
      }
   }
   return false;
}

//============================================================================
//============================================================================
void BCampaignProgress::setupTickerInfo() 
{
   BUString tempString = "";

   //Medals earned (Cooperative Easy: 4 gold, 3 silver, 5 bronze)
   {
      int medals[CAMPAIGN_MAX_MODES][CAMPAIGN_DIFFICULTY_LEVELS][3];

      for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
      {
         for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
         {
            medals[j][k][0] = medals[j][k][1] = medals[j][k][2] = 0;
         }
      }

      for(int i=0; i<CAMPAIGN_MAX_SCENARIOS; i++)
      {
         for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
         {
            for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
            {
               if(mScenarioGrades[i][j][k] == 1)
                  medals[j][k][0]++;
               else if(mScenarioGrades[i][j][k] == 2)
                  medals[j][k][1]++;
               else if(mScenarioGrades[i][j][k] == 3)
                  medals[j][k][2]++;
            }
         }
      }

      BUString modeString = "";
      BUString diffString = "";
      for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
      {
         if( j==0 )
            modeString.locFormat(gDatabase.getLocStringFromID(25134)); //solo
         else
            modeString.locFormat(gDatabase.getLocStringFromID(25133)); //ccop

         for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
         {
            if( k==0 )
               diffString.locFormat(gDatabase.getLocStringFromID(22255)); //easy
            else if( k==1 )
               diffString.locFormat(gDatabase.getLocStringFromID(22256)); //normal
            else if( k==2 )
               diffString.locFormat(gDatabase.getLocStringFromID(22257)); //heroic
            else if( k==3 )
               diffString.locFormat(gDatabase.getLocStringFromID(22258)); //legendary

            tempString.locFormat(gDatabase.getLocStringFromID(25703), modeString.getPtr(), diffString.getPtr(), medals[j][k][0], medals[j][k][1], medals[j][k][2]);
            BUITicker::addString(tempString, 4, -1, BUITicker::eMedalsEarned + (j*CAMPAIGN_DIFFICULTY_LEVELS)+k);
         }
      }
   }

   //Missions beaten in par time
   {
      int numMet = 0;
      for(int i=0; i<CAMPAIGN_MAX_SCENARIOS; i++)
      {
         bool parmet = false;
         for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
         {
            for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
            {
               if( mScenarioParMet[i][j][k] )
                  parmet = true;
            }
         }
         if( parmet )
            numMet++;
      }

      tempString.locFormat(gDatabase.getLocStringFromID(25704), numMet);
      BUITicker::addString(tempString, 4, -1, BUITicker::eNumParMet);
   }

   //Lifetime campaign score
   {
      tempString.locFormat(gDatabase.getLocStringFromID(25705), getLifetimeCampaignScore());
      BUITicker::addString(tempString, 4, -1, BUITicker::eLifetimeCampaignScore);
   }
}

//============================================================================
uint32 BCampaignProgress::getLifetimeCampaignScore(uint mode) const
{
   int d;
   int totalScore = 0;
   for (d = 0; d < CAMPAIGN_DIFFICULTY_LEVELS; d++)
   {
      totalScore = totalScore + mLifetimeScore[mode][d];
   }
   return(totalScore);
}

#ifndef BUILD_FINAL
//============================================================================
//============================================================================
void BCampaignProgress::dumpInfo() const
{
   gConsoleOutput.output(cMsgConsole, "------------------------------------------------------------------------");
   gConsoleOutput.output(cMsgConsole, "mCurrentCampaignNode = %d", mCurrentCampaignNode );
   for(int i=0; i<CAMPAIGN_MAX_SCENARIOS; i++)
   {
      gConsoleOutput.output(cMsgConsole, "Mission %d:", i );
      for(int j=0; j<CAMPAIGN_MAX_MODES; j++)
      {
         if( j==0 )
            gConsoleOutput.output(cMsgConsole, "..SinglePlayer" );
         else
            gConsoleOutput.output(cMsgConsole, "..Cooperative" );

         for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
         {
            int hrs =  mScenarioTimes[i][j][k] / 3600;
            int min = (mScenarioTimes[i][j][k] - (hrs * 3600)) / 60;
            int sec =  mScenarioTimes[i][j][k] - (hrs * 3600) - (min * 60);
            if (k==0)
               gConsoleOutput.output(cMsgConsole, "....Easy       Score: %5d   Grade: %d   Time: %2d:%2d:%2d  %s", mScenarioScores[i][j][k], mScenarioGrades[i][j][k], hrs, min, sec, mScenarioParMet[i][j][k] ? "(under par)" : "" );
            else if (k==1)
               gConsoleOutput.output(cMsgConsole, "....Normal     Score: %5d   Grade: %d   Time: %2d:%2d:%2d  %s", mScenarioScores[i][j][k], mScenarioGrades[i][j][k], hrs, min, sec, mScenarioParMet[i][j][k] ? "(under par)" : "" );
            else if (k==2)
               gConsoleOutput.output(cMsgConsole, "....Heroic     Score: %5d   Grade: %d   Time: %2d:%2d:%2d  %s", mScenarioScores[i][j][k], mScenarioGrades[i][j][k], hrs, min, sec, mScenarioParMet[i][j][k] ? "(under par)" : "" );
            else if (k==3)
               gConsoleOutput.output(cMsgConsole, "....Legendary  Score: %5d   Grade: %d   Time: %2d:%2d:%2d  %s", mScenarioScores[i][j][k], mScenarioGrades[i][j][k], hrs, min, sec, mScenarioParMet[i][j][k] ? "(under par)" : "" );
         }
      }
   }
   gConsoleOutput.output(cMsgConsole, "------------------------------------------------------------------------");
}


//============================================================================
//============================================================================
void BCampaignProgress::displayInfoInStats(BDebugTextDisplay& textDisplay, int page) const
{
   char diff0[] = "Easy  ";
   char diff1[] = "Normal";
   char diff2[] = "Heroic";
   char diff3[] = "Legend";

   char *diff = NULL;

   int start = 0;
   int stop = 8;
   if( page == 2 )
   {
      start = 8;
      stop = 15;
   }

   textDisplay.printf("mCurrentCampaignNode = %d", mCurrentCampaignNode );
   for(int i=start; i<stop; i++)
   {
      textDisplay.printf("%2d) Solo:     Score  Grade   Time                 Coop:     Score  Grade   Time", i+1 );
      for(int k=0; k<CAMPAIGN_DIFFICULTY_LEVELS; k++)
      {
         int hrs =  mScenarioTimes[i][0][k] / 3600;
         int min = (mScenarioTimes[i][0][k] - (hrs * 3600)) / 60;
         int sec =  mScenarioTimes[i][0][k] - (hrs * 3600) - (min * 60);

         int hrs2 =  mScenarioTimes[i][1][k] / 3600;
         int min2 = (mScenarioTimes[i][1][k] - (hrs2 * 3600)) / 60;
         int sec2 =  mScenarioTimes[i][1][k] - (hrs2 * 3600) - (min2 * 60);

         if (k==0)
            diff = diff0;
         else if (k==1)
            diff = diff1;
         else if (k==2)
            diff = diff2;
         else if (k==3)
            diff = diff3;

         textDisplay.printf("      %s  %5d    %d   %02d:%02d:%02d  %s          %s  %5d    %d   %02d:%02d:%02d  %s", diff, mScenarioScores[i][0][k], mScenarioGrades[i][0][k], hrs, min, sec, mScenarioParMet[i][0][k] ? "(par)" : "     ",
            diff, mScenarioScores[i][1][k], mScenarioGrades[i][1][k], hrs2, min2, sec2, mScenarioParMet[i][1][k] ? "(par)" : "     ");
      }
   }
}
#endif

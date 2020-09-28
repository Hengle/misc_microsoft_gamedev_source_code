//==============================================================================
// matchMakingHopperList.cpp
//
// Copyright (c) 2007-2008 Ensemble Studios
//==============================================================================
#include "Common.h"
#include "matchMakingHopperList.h"
#include "database.h"
#include "LiveSystem.h"

#include "xLastGenerated.h"

//==============================================================================
//
//==============================================================================
BMatchMakingHopper::BMatchMakingHopper() :
   mName(""),
   mListIndex(0),
   mPlayers(0),
   mPlayersPerTeam(0),
   mTeamCode(cBMatchMakingHopperTeamCodeCustomTeams),
   mRanked(false),
   mLocStringID(-1),
   //mRestrictedNATOk(false),
   mXLastGameModeIndex(0),
   //mEnabled(cBMatchMakingHopperEnabled),
   mGameModeIndex(0),
   mFastScan(cBMatchMakingFastScanEnabled),
   mHostPercent(cBMatchMakingHostingPercent),
   mHostTimeout(cBMatchMakingHostTimeout),
   mSearchTimeOut(cBMatchMakingSearchTimeOut),
   mFastScanSearchCount(cBMatchMakingFastScanSearchCount),
   mNormalSearchCount(cBMatchMakingNormalSearchCount),
   mHostDelayTimeBase(cBMatchMakingHostDelayTimeBase),
   mHostDelayTimeRandom(cBMatchMakingHostDelayTimeRandom),
   //mRestrictedNATOk(cBMatchMakingRestrictedNATOK),
   //mNOLSPModeEnabled(false),
   mUserCount(0),
   mAverageWait(0)
{
   mMapList.clear();
   mMinMatchQualityPerPass[0]=0.50;
   mMinMatchQualityPerPass[1]=0.25;
   mMinMatchQualityPerPass[2]=0.1;
   mMinMatchQualityPerPass[3]=0.0;
   mMinMatchQualityPerPass[4]=0.0;
}

//==============================================================================
//
//==============================================================================
BOOL BMatchMakingHopper::isValid(bool restrictedNATEnabled)
{
   /*
   if ((mEnabled) &&
       (!restrictedNATEnabled || mRestrictedNATOk) &&
       (mXLastGameModeIndex != CONTEXT_GAME_MODE_CUSTOM_GAME))            //HAX so that system ignores the custom game slot
   {
      return TRUE;
   }
   */
   return TRUE;
}

//==============================================================================
//
//==============================================================================
BOOL BMatchMakingHopper::loadFromXML(BXMLNode node)
{
   const BPackedString name(node.getName());
   BASSERT(name=="GS");
   BSimString gameModeStr, playersStr, teamsStr, rankedStr, xLastGameModeStr, restrictedNATOkStr, noLSPEnabledStr;
   const bool hasGameMode = node.getAttribValue("GM", &gameModeStr);
   const bool hasPlayers = node.getAttribValue("P", &playersStr);
   const bool hasTeams = node.getAttribValue("T", &teamsStr);
   const bool hasRanked = node.getAttribValue("R", &rankedStr);
   const bool hasXLastGameMode = node.getAttribValue("XGM", &xLastGameModeStr); 

   //const bool hasRestrictedNATOk = node.getAttribValue("RestrictedNATOk", &restrictedNATOkStr);   
   if ( !hasGameMode || !hasPlayers || !hasTeams || !hasRanked || !hasXLastGameMode )
   {
      return FALSE;
   }

   node.getAttribValue("Name", &mName);
   node.getAttribValueAsLong("_locID", mLocStringID);
   node.getAttribValueAsUInt8("FSC", mFastScanSearchCount);
   node.getAttribValueAsUInt8("NSC", mNormalSearchCount);
   node.getAttribValueAsUInt32("HDB", mHostDelayTimeBase);
   node.getAttribValueAsUInt32("HDR", mHostDelayTimeRandom);
   node.getAttribValueAsUInt32("HT",  mHostTimeout);
   mPlayers = (uint8)playersStr.asLong();
   mPlayersPerTeam = (uint8)mPlayers/2;         //Just calculated for now, might need to be stored in data if we support 2v2v2
   mGameModeIndex = (uint)gameModeStr.asLong();
   mXLastGameModeIndex = (uint)xLastGameModeStr.asLong();
   float mqValue = 0.0;
   if (node.getAttribValueAsFloat("MQ1", mqValue))
   {
      mMinMatchQualityPerPass[0] = mqValue;
   }
   if (node.getAttribValueAsFloat("MQ2", mqValue))
   {
      mMinMatchQualityPerPass[1] = mqValue;
   }
   if (node.getAttribValueAsFloat("MQ3", mqValue))
   {
      mMinMatchQualityPerPass[2] = mqValue;
   }
   if (node.getAttribValueAsFloat("MQ4", mqValue))
   {
      mMinMatchQualityPerPass[3] = mqValue;
   }
   if (node.getAttribValueAsFloat("MQ5", mqValue))
   {
      mMinMatchQualityPerPass[4] = mqValue;
   }

   if (rankedStr=="Y")
   {
      mRanked = true;
   }
   else
   {
      mRanked = false;
   }
   if (teamsStr=="C")
   {
      mTeamCode = cBMatchMakingHopperTeamCodeCustomTeams;
   }
   else if (teamsStr=="Y")
   {
      mTeamCode = cBMatchMakingHopperTeamCodeTeamsOnly;
   }
   else
   {
      mTeamCode = cBMatchMakingHopperTeamCodeNoTeams;
   }

   //Get the maplist    
   BXMLNode mapListNode(node.getChild((long)0));
   const BPackedString nodeName(mapListNode.getName());
   if(nodeName!="ml")
   {
      return FALSE;
   }
   long mapListNodeCount=mapListNode.getNumberChildren();
   if (mapListNodeCount==0)
   {
      return FALSE;
   }
   for (long i=0;i<mapListNodeCount;i++)
   {
      BXMLNode mapNode(mapListNode.getChild(i));
      const BPackedString nodeName(mapNode.getName());
      if(nodeName=="m")
      {
         BSimString mapNameStr;
         const bool hasMapName = mapNode.getAttribValue("n", &mapNameStr);
         if (hasMapName)
         {
            //TODO - validate map name?
            mMapList.add( mapNameStr );
         }
      }
   }
   return TRUE;
}

//==============================================================================
//
//==============================================================================
BUString BMatchMakingHopper::getLocString(bool withPopCount) const
{
   //Base string
   BUString result = gDatabase.getLocStringFromID(mLocStringID);
   if (!withPopCount)
   {
      return result;
   }

   BUString popString;
   BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
   if (!pHopperList)
   {
      return result;
   }

   //See if the pop display has been turned off
   if (pHopperList->getStaticData()->mLowPopulationLimit==0)
   {
      return result;
   }

   if (mUserCount<pHopperList->getStaticData()->mLowPopulationLimit)
   {
      popString = gDatabase.getLocStringFromID(25454);
   }
   else
   {
      popString.locFormat(gDatabase.getLocStringFromID(25453).getPtr(), mUserCount);
   }
   result += " ";
   result += popString;
   return result;
}

//==============================================================================
//
//==============================================================================
BMatchMakingHopperList::BMatchMakingHopperList() :
mLoaded(FALSE),
mFromDisk(FALSE),
mRestrictedNATMode(FALSE),
mUpdateTime(0)
{
   mHopperList.clear();
}

//==============================================================================
//
//==============================================================================
BOOL BMatchMakingHopperList::loadFromXML(BXMLNode node, bool fileFromDisk)
{
   long nodeCount=node.getNumberChildren();
   if (nodeCount==0)
   {
      //Nothing?
      BASSERT(false);
      return FALSE;
   }

   for (long i=0;i<nodeCount;i++)
   {
      BXMLNode childNode(node.getChild(i)); 
      BMatchMakingHopper newItem;
      const BPackedString name(childNode.getName());
      if(name=="GS")
      {
         //This is a hopper list row item
         if (newItem.loadFromXML(childNode)==TRUE)
         {
            //newItem.mListIndex = mHopperList.getNumber();
            newItem.mListIndex = newItem.mXLastGameModeIndex;
            mHopperList.add(newItem);
         }
         else
         {
            BASSERT(false);
            return FALSE;
         }
      }
      else
      {
         //This is global data (not per-hopper)
         if (name=="PerP")
         {
            childNode.getTextAsUInt16(mStaticData.mPreferredPing);
         } 
         else if (name=="MaxP")
         {
            childNode.getTextAsUInt16(mStaticData.mMaxPing);
         }
         else if (name=="MaxT")
         {
            childNode.getTextAsDWORD(mStaticData.mMaxMatchmakingTimeTotal);
         }
         else if (name=="LPL")
         {
            childNode.getTextAsUInt16(mStaticData.mLowPopulationLimit);
         }   
         else if (name=="MST")
         {
            childNode.getTextAsDWORD(mStaticData.mMaxTimeForASingleSearch);
         }
         else if (name=="NBTL")
         {
            childNode.getTextAsBool(mStaticData.mBadTargetListEnabled);
            //The value in the XML file is to DISABLE this option
            mStaticData.mBadTargetListEnabled = !mStaticData.mBadTargetListEnabled;
         }    
         else if (name=="MQOS")
         {
            childNode.getTextAsDWORD(mStaticData.mMaxQoSResponseTime);            
         }
         else if (name=="CPE")
         {
            childNode.getTextAsBool(mStaticData.mConnectionProxyEnabled);            
         }    
         else if (name=="NSUT")
         {
            childNode.getTextAsDWORD(mStaticData.mNetworkSocketUnresponsiveTimeout);            
         } 
         else if (name=="NJRT")
         {
            childNode.getTextAsDWORD(mStaticData.mNetworkJoinRequestTimeout);            
         } 
         else if (name=="NPRT")
         {
            childNode.getTextAsDWORD(mStaticData.mNetworkProxyRequestTimeout);            
         } 
         else if (name=="NPPT")
         {
            childNode.getTextAsDWORD(mStaticData.mNetworkProxyPingTimeout);            
         } 
      }
   }
   mLoaded = TRUE;
   mFromDisk = fileFromDisk;
   return TRUE;
}

//==============================================================================
//
//==============================================================================
BMatchMakingHopper* BMatchMakingHopperList::findHopperByID(uint index)
{
   if (!mLoaded)
      return NULL;

   if (index>=mHopperList.getSize())
      return NULL;

   return &mHopperList[index];

}

//==============================================================================
// Call this when you want a valid, default hopper
//==============================================================================
BMatchMakingHopper* BMatchMakingHopperList::findFirstValidHopper(BOOL teamOnly)
{
   if (!mLoaded)
      return NULL;

   for (uint i=0;i<mHopperList.getSize();i++)
   {
      //if (mHopperList[i].isValid(mRestrictedNATMode) == TRUE)
      //nat mode thing is deprecated
      if (mHopperList[i].mTeamCode!=BMatchMakingHopper::cBMatchMakingHopperTeamCodeCustomTeams)
      {
         if ((!teamOnly) || 
            (mHopperList[i].mTeamCode!=BMatchMakingHopper::cBMatchMakingHopperTeamCodeNoTeams))
         {
            return &mHopperList[i];
         }
      }
   }
   return NULL;
}

//==============================================================================
// Lets you walk up and down the valid hopper list
//==============================================================================
BMatchMakingHopper* BMatchMakingHopperList::getNextValidHopper(uint startingIndex, int direction)
{
   //TODO - look at depricating this
   if (!mLoaded)
      return NULL;

   if (direction>1)
      direction=1;
   if (direction<-1)
      direction=-1;
   uint index=startingIndex;
   if (index<0)
      index = 0;
   if (index>=mHopperList.getSize())
      index=mHopperList.getSize()-1;
  
   BMatchMakingHopper* returnHopper = findHopperByID(index);
   if ((returnHopper) && (returnHopper->isValid(mRestrictedNATMode)!=TRUE))
      returnHopper=NULL;

   for(;;)
   {
      index=index+direction;
      if (index<0)
         return returnHopper;
      if (index>=mHopperList.getSize())
         return returnHopper;
      BMatchMakingHopper* temp = findHopperByID(index);
      if ((temp) && (temp->isValid(mRestrictedNATMode)==TRUE))
         return temp;    
   }

   return returnHopper;
}

//==============================================================================
//
//==============================================================================
// NOTE: this is an UNFILTER search, it grabs the record even if it is not enabled/allowed etc
BMatchMakingHopper* BMatchMakingHopperList::findHopperByHopperIndex(uint hopperIndex)
{
   if (!mLoaded)
      return NULL;

   for (uint i=0;i<mHopperList.getSize();i++)
   {
      //if (mHopperList[i].mListIndex==hopperIndex)
      if (mHopperList[i].mXLastGameModeIndex==hopperIndex)
      {
         return &mHopperList[i];
      }
   }
   return NULL;   
}

//==============================================================================
// Finds you the first hopper that matches the game mode you pass it (starting with the index you pass it)
//==============================================================================
BMatchMakingHopper* BMatchMakingHopperList::findHopperByGameMode(uint gameModeIndex, uint startingSearchIndex)
{
   //DEPRICATED
   BASSERT(false);
   return NULL;

   /*
   if (!mLoaded)
      return NULL;

   if (startingSearchIndex>=mHopperList.getSize())
      return NULL;

   //Don't validate to find the custom index
   if (gameModeIndex==CONTEXT_GAME_MODE_CUSTOM_GAME)
   {
      for (uint i=startingSearchIndex;i<mHopperList.getSize();i++)
      {
         if ((mHopperList[i].mXLastGameModeIndex==gameModeIndex))
         {
            return &mHopperList[i];
         }
      }
      //We should never get here
      BASSERTM(false, "BMatchMakingHopperList::findHopperByGameMode - ERROR: Could not find the custom game index when requested for it");
   }
   else
   {
      for (uint i=startingSearchIndex;i<mHopperList.getSize();i++)
      {
         if (mHopperList[i].isValid(mRestrictedNATMode) == TRUE)
         {
            if (mHopperList[i].mXLastGameModeIndex==gameModeIndex)
            {
               return &mHopperList[i];
            }
         }
      }
   }
   return NULL;   
   */
}

//==============================================================================
//Returns the hopper for custom games - which is hidden from the list in matchmaking mode
//==============================================================================
BMatchMakingHopper* BMatchMakingHopperList::findCustomGameHopper()
{
   if (!mLoaded)
      return NULL;

  for (uint i=0;i<mHopperList.getSize();i++)
   {
      if (mHopperList[i].mXLastGameModeIndex==CONTEXT_GAME_MODE_PARTYHOPPER)
      {
         return &mHopperList[i];
      }
   }
   //We should never get here
   BASSERTM(false, "BMatchMakingHopperList::findCustomGameHopper - ERROR: Could not find the custom game index when requested for it");
   return NULL;
}

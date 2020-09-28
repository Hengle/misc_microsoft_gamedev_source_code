//==============================================================================
// matchMakingHopperList.h
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

#pragma once

// Used to track connection/game data for local Live sessions 

// xsystem
#include "xmlreader.h"
#include "xnetwork.h"

// Default values for various tuning parameters that are per hopper
const bool   cBMatchMakingFastScanEnabled    = TRUE;
const FLOAT  cBMatchMakingHostingPercent     = 0.25f;
const uint   cBMatchMakingHostTimeout        = 15 * 1000;
const uint   cBMatchMakingSearchTimeOut      = 45 * 1000;
const byte   cBMatchMakingFastScanSearchCount= 10;     //Number of records to request from Live when fast scan search is done
const byte   cBMatchMakingNormalSearchCount  = 20;     //Number of records to request from Live when full search is done
const uint32 cBMatchMakingHostDelayTimeBase  = 0;      //Minimum time to wait (ms) after a full search before going to host mode
const uint32 cBMatchMakingHostDelayTimeRandom= 5000;  //Rand max time to wait (ms) added to above value 
const uint32 cBMatchMakingMatchQualityEntries=5;

// Default values for various tuning parameters that are global
const uint   cBMatchMakingProcessTimeout    = 30*60 * 1000;         //Max time to wait for matchmaking
const uint16 cBMatchMakingPreferredPing     = 100;
const uint16 cBMatchMakingMaxPing           = 250;
const uint16 cBMatchMakingLowPopLimit       = 50;
const DWORD  cBMatchMakingMaxTimeForASingleSearch=45000;
const DWORD  cBMatchMakingMaxQoSResponseTime= 4000;

const uint  cBInvalidHopperIndex            = UINT_MAX;


class BMatchMakingHopper
{
   public:
      enum BMatchMakingHopperTeamCode
      {
         cBMatchMakingHopperTeamCodeCustomTeams,         //Only used for the CUSTOM game hopper
         cBMatchMakingHopperTeamCodeNoTeams,
         cBMatchMakingHopperTeamCodeTeamsOnly
      };

   BMatchMakingHopper();
   BOOL  loadFromXML(BXMLNode node);
   BOOL  isValid(bool restrictedNATEnabled);

   //Loaded from the hopper data (mpGameDataBase.xml or from the XML file from the LSP)
   BSimString                    mName;
   long                          mLocStringID;      
   bool                          mRanked;                //True if this is a ranked hopper
   //bool                          mRestrictedNATOk;       //True if it ok to play this hopper with a restricted NAT
   uint8                         mPlayers;               //Number of players in the game for this hopper
   uint8                         mPlayersPerTeam;        //Number of players per team for this hopper
   uint                          mGameModeIndex;         //Which internal game mode is this hopper for
   uint                          mXLastGameModeIndex;    //XLast index that this hopper specifically maps to - UNIQUE TO EACH HOPPER
   BMatchMakingHopperTeamCode    mTeamCode;              //Maps to one of the values above: teams allowed(optional), no teams, or teams only     
   BDynamicSimArray<BSimString>  mMapList;               //List of maps for this hopper
   BOOL                          mFastScan;              //If enabled, do a fastscan approach right when matchmaking is initiated
   float                         mHostPercent;           //If there is no fastscan, what % of the time does it go to host mode versus search mode initially
   uint                          mSearchTimeOut;         //Max time (ms) for any search to last (including query, QoS, and join attempts)
   //BOOL                          mEnabled;               //FALSE if this hopper is turned off
   //BOOL                          mNOLSPModeEnabled;      //TRUE if this hopper is auto-enabled during NO LSP mode
   uint8                         mFastScanSearchCount;   //Number of records to request from Live when fast scan search is done
   uint8                         mNormalSearchCount;     //Number of records to request from Live when full search is done
   uint32                        mHostDelayTimeBase;     //Minimum time to wait (ms) after a full search before going to host mode
   uint32                        mHostDelayTimeRandom;   //Rand max time to wait (ms) added to above value 
   uint32                        mHostTimeout;           //How long to stay in host mode before giving up (*number of current people in session)
   float                         mMinMatchQualityPerPass[cBMatchMakingMatchQualityEntries]; //Min match qualities per search iteration
   //Set from the dynamic data from the LSP (when available)
   DWORD                         mUserCount;             //How many people is the LSP reporting as in this hopper currently (data from LSP is uint16 that has been divided by 10)
   uint32                        mAverageWait;           //What is the average time for a game the LSP is reporting for this hopper
   //Calculated from data
   //This now just points to the mXLastGameModeIndex 
   uint                          mListIndex;             //So when you have a pointer to this entry, you can find out what its index in the list is
   //Helper method for getting the loc'd string with population count in it
   BUString                      getLocString(bool withPopCount = false) const;

};

class BMatchMakingStaticGlobalData
{
public:
   BMatchMakingStaticGlobalData():
      mPreferredPing(cBMatchMakingPreferredPing),
      mMaxPing(cBMatchMakingMaxPing),
      mBadTargetListEnabled(true),
      mConnectionProxyEnabled(true),
      mMaxMatchmakingTimeTotal(cBMatchMakingProcessTimeout),
      mMaxTimeForASingleSearch(cBMatchMakingMaxTimeForASingleSearch),
      mLowPopulationLimit(cBMatchMakingLowPopLimit),
      mMaxQoSResponseTime(cBMatchMakingMaxQoSResponseTime),
      mNetworkSocketUnresponsiveTimeout(XNetwork::cConnectionTimeout),
      mNetworkJoinRequestTimeout(XNetwork::cDefaultJoinRequestTimeout),
      mNetworkProxyRequestTimeout(XNetwork::cDefaultProxyRequestTimeout),
      mNetworkProxyPingTimeout(XNetwork::cDefaultProxyPingTimeout)
   {};
   uint16      mPreferredPing;
   uint16      mMaxPing;
   bool        mBadTargetListEnabled;
   bool        mConnectionProxyEnabled;
   DWORD       mMaxMatchmakingTimeTotal;
   DWORD       mMaxTimeForASingleSearch;
   uint16      mLowPopulationLimit;
   DWORD       mMaxQoSResponseTime;
   DWORD       mNetworkSocketUnresponsiveTimeout;
   DWORD       mNetworkJoinRequestTimeout;
   DWORD       mNetworkProxyRequestTimeout;
   DWORD       mNetworkProxyPingTimeout;
};

class BMatchMakingDynamicGlobalData
{
public:
   BMatchMakingDynamicGlobalData ():
      mGlobalUserCount(0) {};
   DWORD       mGlobalUserCount;
};



class BMatchMakingHopperList
{
public:
   BMatchMakingHopperList();
   BOOL  loadFromXML(BXMLNode node, bool fileFromDisk=false);
   void  clear() {mHopperList.clear();mLoaded=FALSE;};
   BOOL  isLoaded() {return mLoaded;};
   BOOL  wasLoadedFromDisk() {return mFromDisk;};
   int   getHopperCount() { return mHopperList.getNumber(); }
   DWORD getTimeSinceLastUpdate() {return timeGetTime()-mUpdateTime;};
   //Use this method to iterate through the list of hoppers via a local, contiguous count
   BMatchMakingHopper*  findHopperByID(uint index);
   //Use this method to find a particular hopper via its unique index
   BMatchMakingHopper*  findHopperByHopperIndex(uint hopperIndex);
   //Call this when you want a valid, default hopper
   BMatchMakingHopper*  findFirstValidHopper(BOOL teamOnly=FALSE);
   BMatchMakingHopper*  getNextValidHopper(uint startingIndex, int direction);
   BMatchMakingHopper*  findHopperByGameMode(uint gameModeIndex, uint startingSearchIndex=0);
   BMatchMakingHopper*  findCustomGameHopper();    //Returns the hopper for custom games - which is hidden from the list in matchmaking mode
   void                 setRestrictedNATMode(bool mode) {mRestrictedNATMode=mode;};
   //Config value queries
   const BMatchMakingStaticGlobalData*  getStaticData() {return &mStaticData;};
   BMatchMakingDynamicGlobalData*       getDynamicData() {return &mDynamicData;};
   /*
   uint16   getPreferredPing() {return mPreferredPing;};
   uint16   getMaxPing() {return mMaxPing;};
   BOOL     isTwoStageQOSEnabled() {return mTwoStageQOSEnabled;};
   DWORD    getMaxMatchmakingTimeAllowed() {return mMaxMatchmakingTimeTotal;};
   DWORD    getGlobalUserCount() {return mGlobalUserCount;};
   void     setPreferredPing(uint16 value) {mPreferredPing = value;};
   void     setMaxPing(uint16 value) {mMaxPing = value;};
   void     setTwoStageQOSEnabled(BOOL value) {mTwoStageQOSEnabled = value;};
   void     setMaxMatchmakingTimeAllowed(DWORD value) {mMaxMatchmakingTimeTotal = value;};
   void     setGlobalUserCount(DWORD value) {mGlobalUserCount=value; mUpdateTime=timeGetTime();};
   */

private:
   BOOL        mLoaded;
   BOOL        mFromDisk;
   bool        mRestrictedNATMode;
   BDynamicSimArray<BMatchMakingHopper>  mHopperList;
   DWORD       mUpdateTime;               //Timestamp of when the config was last updated

   BMatchMakingStaticGlobalData     mStaticData;
   BMatchMakingDynamicGlobalData    mDynamicData;
};

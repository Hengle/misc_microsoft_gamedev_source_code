//--------------------------------------------------------------------------------------
// liveSessionSearch.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//--------------------------------------------------------------------------------------

#pragma once

#include "LiveGameDescriptor.h"

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------
#define cBLiveSessionSearchMaxSearchResults       50
#define cBLiveSessionSearchMaxConnectionsPerQuery 5

//Forward dec
class BLiveMatchMaking;

//Used to query live for a list of sessions and maintain information about them
class BLiveSessionSearch
{
public:
   //Enums
   // valid session search states
   enum sessionSearchState
   {
      cSessionSearchStateNone,
      cSessionSearchStateCreateError,
      cSessionSearchStateResultsMissingError,
      cSessionSearchStateCreating,
      cSessionSearchStateWaitingForData,
      cSessionSearchStateWaitingForQoSRunning,
      cSessionSearchStateResultsReady,
      cSessionSearchStateResultsProcessed,
      cSessionSearchStateDeleting
   };

   //Structure used to send data to QoS requesting clients
   // NOTE: Max size of this structure is 512 bytes - this can be bumped up with XNetStartup if we need it bigger
   #pragma pack(push)
   #pragma pack(1)
   struct BQoSResponseData
   {
      DWORD    mCheckSum;
      uint8    mLanguageCode;
      //uint8    mVersionCode;
      uint8    mPublicSlots;
      uint8    mPublicSlotsOpen;
      //uint8    mClientCount;
      uint64   mNonce;
      //XNADDR   mClientXNAddrs[cBLiveSessionSearchMaxConnectionsPerQuery];
   };
   #pragma pack(pop)
   #define cBQoSResponseDataBaseSize sizeof(uint8)+sizeof(uint8)+sizeof(uint8)+sizeof(uint64)+sizeof(DWORD)   //+sizeof(uint8)+sizeof(uint8)

   BLiveSessionSearch(BLiveMatchMaking* matchmaker, uint xLastHopperIndex, uint maxResultsCount, bool ranked, WORD maxPing, WORD perfPing, uint8 languageCode=0, DWORD checkSum=0, float minMatchQuality=0, double sigma=0, double mu=0);
   ~BLiveSessionSearch();

   void update();
   sessionSearchState getResultsState() { return mSearchState; };
   void setResultsAsProcessed()         { mSearchState = cSessionSearchStateResultsProcessed; };
   bool areResultsReady()               { return (mSearchState == cSessionSearchStateResultsReady); };
   bool areResultsProcessed()           { return (mSearchState == cSessionSearchStateResultsProcessed); };  
   DWORD getNumberOfSearchResults();
   DWORD getNumberOfPossbileMatches()   { return mPossibleMatchCount;};
   DWORD getStartToPostQosTime()        { return mQOSLaunchTime;};
   BLiveGameDescriptor* getSearchResultRecord( DWORD index );
   WORD  getPerferredPing()             { return mPerferredPing;};

protected:
   void        sortResults();
   static BOOL CompareResultRows( const BLiveGameDescriptor& a, const BLiveGameDescriptor& b );

   BLiveGameDescriptor           mResultDescriptors[cBLiveSessionSearchMaxSearchResults]; // 384 * 50 == 19200
   BLiveMatchMaking*             mpMatchMaker;

   XNADDR                        mXnAddr[cBLiveSessionSearchMaxSearchResults];   // 36 * 50 == 1800
   XNKEY                         mXnKey[cBLiveSessionSearchMaxSearchResults];    // 16 * 50 == 800
   XNKID                         mXnKID[cBLiveSessionSearchMaxSearchResults];    // 8 * 50 == 400

   XOVERLAPPED                   mOverlapped;               //Overlapped task data
   XUSER_CONTEXT                 mContexts[2];
   XUSER_PROPERTY                mSearchProperties[2];
   sessionSearchState            mSearchState;
   PXSESSION_SEARCHRESULT_HEADER mpSearchResults;           //Match search results
   DWORD                         mResultCount;              //STAT that tracks how many results Live returned
   DWORD                         mGoodMatchCount;           //Record count after bad skill matches were removed
   DWORD                         mPossibleMatchCount;       //STAT that tracks how many games are valid (post Qos, post filter) targets
   DWORD                         mLiveQueryTime;            //STAT that tracks how long it took for Live to return results
   DWORD                         mStartToPostQosTime;       //STAT how long it took to go from start - til when all QoS was complete
   WORD                          mPerferredPing;            //These are static because the static sort method must access them.  There should only ever be ONE search running at once
   WORD                          mMaxPing;
   DWORD                         mInitialQueryLaunchTime;
   DWORD                         mQOSLaunchTime;
   DWORD                         mMaxResultCount;
   DWORD                         mRequestedResultCount;
   DWORD                         mHopperIndex;
   DWORD                         mCheckSum;
   double                        mClientSigma;
   double                        mClientMu;
   float                         mMinMatchQuality;
   XNQOS*                        mpQoSResults;
   uint8                         mLanguageCode;
   //uint8                         mVersionCode;
   bool                          mRanked : 1;
};

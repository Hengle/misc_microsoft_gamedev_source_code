//--------------------------------------------------------------------------------------
// liveSessionSearch.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//--------------------------------------------------------------------------------------
#include "Common.h"
#include "liveSessionSearch.h"
#include "XLastGenerated.h"
#include "LiveSystem.h"
#include "mpSession.h"
#include "liveMatchMaking.h"
#include "usermanager.h"
#include "user.h"

// For logging and configuration settings
#include "mpcommheaders.h"
#include "commlog.h"
#include "econfigenum.h"

#define cBLiveSessionSearchInitialQoSProbes   8
#define cBLiveSessionSearchSecondaryQoSProbes 8
#define cBLiveSessionSearchLiveResultsMaxTime 10000

//==============================================================================
//
//==============================================================================
BLiveSessionSearch::BLiveSessionSearch(BLiveMatchMaking* matchmaker, uint xLastHopperIndex, uint maxResultsCount, bool ranked, WORD maxPing, WORD perfPing, uint8 languageCode, DWORD checkSum, float minMatchQuality, double sigma, double mu):
   mResultCount(0),
   mGoodMatchCount(0),
   mMaxResultCount(0),
   mPossibleMatchCount(0),
   mRequestedResultCount(0),
   mSearchState(cSessionSearchStateCreating),
   mpSearchResults(NULL),
   mpQoSResults(NULL),
   mLiveQueryTime(0),
   mStartToPostQosTime(0),
   mLanguageCode(languageCode),
   //mVersionCode(versionCode),
   mCheckSum(checkSum),
   mQOSLaunchTime(0),
   mClientSigma(sigma),
   mClientMu(mu),
   mInitialQueryLaunchTime(0),
   mMinMatchQuality(minMatchQuality),
   mpMatchMaker(matchmaker),
   mHopperIndex(xLastHopperIndex),
   mRanked(ranked),
   mMaxPing(maxPing),
   mPerferredPing(perfPing)
{
   BASSERT(mpMatchMaker);

   BUser* user = gUserManager.getPrimaryUser();
   BASSERT(user);
   if (!user)
   {
      nlog(cMPMatchmakingCL, "BLiveSessionSearch::BLiveSessionSearch - error, No primary user to tie this search to");
      mSearchState=cSessionSearchStateCreateError;
      return;
   }

   nlog(cMPMatchmakingCL, "BLiveSessionSearch::BLiveSessionSearch - Hopper:%d - MaxResults:%d - Lang:%d - ChkSum:[0x%08X]", xLastHopperIndex, maxResultsCount, languageCode, checkSum);
   BASSERT(maxResultsCount<=cBLiveSessionSearchMaxSearchResults);
   mMaxResultCount = maxResultsCount;
   mRequestedResultCount = mMaxResultCount+mpMatchMaker->getBadTargetCount();
   if (mRequestedResultCount>50)
   {
      mRequestedResultCount=50;
   }
   if (mRequestedResultCount!=mMaxResultCount)
   {
      nlog(cMPMatchmakingCL, "BLiveSessionSearch::BLiveSessionSearch - Increasing requested results to %d because of size of bad targets list", mRequestedResultCount); 
   }

   DWORD cbResults = 0;
   DWORD ret;
   ZeroMemory( &mOverlapped, sizeof( mOverlapped ) );

   // Pass in 0 for cbResults to have the buffer size calculated for us by XTL
   ret = XSessionSearch(SESSION_MATCH_QUERY_HOPPERSEARCH,     // Procedure index
                        user->getPort(),                  // User index
                        mRequestedResultCount,                  // Maximum results
                        0,                                // Number of properties   (ignored)
                        0,                                // Number of contexts     (ignored)
                        NULL,                             // Properties             (ignored)
                        NULL,                             // Contexts               (ignored)
                        &cbResults,                       // Size of result buffer
                        NULL,                             // Pointer to results     (ignored)
                        NULL                              // Overlapped data structure
                        );

   if(( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbResults == 0 ))
   {
      //failed to get results buffer size 
      BASSERT(false);
      nlog(cMPMatchmakingCL, "BLiveSessionSearch::BLiveSessionSearch - error, XSessionSearch could not compute the buffer size we need");
      mSearchState = cSessionSearchStateCreateError;
      return;
   }

   mpSearchResults = (XSESSION_SEARCHRESULT_HEADER *)new BYTE[cbResults];

   if (!mpSearchResults)
   {
      //Failed to allocate memory for search results
      BASSERT(false);
      nlog(cMPMatchmakingCL, "BLiveSessionSearch::BLiveSessionSearch - error, could not alloc the memory we need for the search buffer");
      mSearchState = cSessionSearchStateCreateError;
      return;
   }

   static XUSER_CONTEXT mContexts[2];
   mContexts[0].dwContextId = X_CONTEXT_GAME_TYPE;
   if (mRanked == true)
   {
      mContexts[0].dwValue = X_CONTEXT_GAME_TYPE_RANKED; 
   }
   else
   {
      mContexts[0].dwValue = X_CONTEXT_GAME_TYPE_STANDARD;
   }
   mContexts[1].dwContextId = X_CONTEXT_GAME_MODE;
   mContexts[1].dwValue     = mHopperIndex;

   mSearchProperties[0].dwPropertyId = X_PROPERTY_GAMER_MU;
   mSearchProperties[0].value.type = XUSER_DATA_TYPE_DOUBLE;
   mSearchProperties[0].value.dblData = mClientMu;
   mSearchProperties[1].dwPropertyId = X_PROPERTY_GAMER_SIGMA;
   mSearchProperties[1].value.type = XUSER_DATA_TYPE_DOUBLE;
   mSearchProperties[1].value.dblData = mClientSigma;
  
   // Fire off the query
   ret = XSessionSearch(SESSION_MATCH_QUERY_HOPPERSEARCH, 
                        user->getPort(),
                        mRequestedResultCount,
                        2,
                        2,
                        mSearchProperties,
                        mContexts,
                        &cbResults,
                        mpSearchResults,
                        &mOverlapped
                        );

   if (ret != ERROR_IO_PENDING)
   {
      //OnlineMatchSearch failed 
      nlog(cMPMatchmakingCL, "BLiveSessionSearch::BLiveSessionSearch - error, XSessionSearch failed with code %d", ret);
      mSearchState = cSessionSearchStateCreateError;
      return;
   }
   mInitialQueryLaunchTime = timeGetTime();
   mSearchState = cSessionSearchStateWaitingForData;
}

//==============================================================================
// 
//==============================================================================
BLiveSessionSearch::~BLiveSessionSearch()
{
   nlog(cMPMatchmakingCL, "BLiveSessionSearch::~BLiveSessionSearch");
   
   if (!XHasOverlappedIoCompleted(&mOverlapped))
   {
      DWORD result = XCancelOverlapped( &mOverlapped );
      BASSERT (result == ERROR_SUCCESS);
      result;
   }

   if (mpQoSResults)
   {
      nlog(cMPMatchmakingCL, "BLiveSessionSearch::~BLiveSessionSearch - dumping active QoS results");
      XNetQosRelease(mpQoSResults);
      mpQoSResults = NULL;
   }

   if (mpSearchResults)
   {
      //todo - XNetQosResults each launched qos query from here that is active
      delete[] mpSearchResults;
      mpSearchResults = NULL;
   }

   mResultCount = 0;
   mSearchState = cSessionSearchStateDeleting;
}

//==============================================================================
// 
//==============================================================================
void BLiveSessionSearch::update()
{
   //Some checks to see if we should still be running updates
   if (!gLiveSystem->getMPSession() ||
       !gLiveSystem->getHopperList())
   {
      return;
   }

   //Process results based on our state
   switch (mSearchState) 
   {
      case(cSessionSearchStateWaitingForData):
         {
            //See if the Live search has completed and we have results
            if (XHasOverlappedIoCompleted(&mOverlapped))
            {
               //Record the query time
               mLiveQueryTime = timeGetTime() - mInitialQueryLaunchTime;

               //Check result codes
               nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Overlapped IO complete, checking results");
               if (!mpSearchResults)
               {
                  nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Warning:Search results are NULL, search has failed");
                  mSearchState = cSessionSearchStateResultsMissingError;
                  return;
               }

               HRESULT hr = XGetOverlappedExtendedError( &mOverlapped );
               if (FAILED(hr))
               {
                  //It failed!
                  nlog(cMPPartySystemCL, "BLiveSessionSearch::update - Overlapped reported complete but with ERROR with return code [0x%08X]", hr);
                  mSearchState = cSessionSearchStateResultsReady;
                  return;
               }

               //Parse the results into the internal data structure
               mResultCount = mpSearchResults->dwSearchResults;
               nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Results[%d/%d] ready, parsing them into the game descriptors (Bad target list size:[%d])", mResultCount, mRequestedResultCount, mpMatchMaker->getBadTargetCount()); 
               BASSERT(mResultCount <= cBLiveSessionSearchMaxSearchResults);     //Why'd it return more than we requested?
               BLiveGameDescriptor* gameRecord = NULL;
               DWORD mGoodMatchCount = 0;
               for (DWORD i=0;i<mResultCount;++i)
               {
                  //Fill in the game descriptor
                  gameRecord = &mResultDescriptors[i];
                  XSESSION_SEARCHRESULT& liveDataRecord = mpSearchResults->pResults[ i ];
                  gameRecord->setXnAddr(liveDataRecord.info.hostAddress);
                  gameRecord->setXNKID(liveDataRecord.info.sessionID);
                  gameRecord->setXNKEY(liveDataRecord.info.keyExchangeKey);
                  gameRecord->setRanked(mRanked);
                  gameRecord->setGameModeIndex(mHopperIndex);
                  gameRecord->setSlots( (uint8)liveDataRecord.dwFilledPublicSlots + (uint8)liveDataRecord.dwOpenPublicSlots );
                  gameRecord->setOpenSlots( (uint8)liveDataRecord.dwOpenPublicSlots );
                  //BCHAR_T hostGamertag[XUSER_NAME_SIZE];
                  //memset(hostGamertag, 0, sizeof(hostGamertag));
                  BSimString hostGamertag = "";
                  for (uint8 p=0;p<liveDataRecord.cProperties;p++)
                  {
//-- FIXING PREFIX BUG ID 1304
                     const PXUSER_PROPERTY prop = &liveDataRecord.pProperties[p];
//--
                     if (prop->dwPropertyId == X_PROPERTY_GAMER_HOSTNAME)
                     {
                        hostGamertag.set( prop->value.string.pwszData, prop->value.string.cbData);
                        gameRecord->setHostGamertag( hostGamertag );
                     }
                     else if (prop->dwPropertyId == X_PROPERTY_GAMER_RATING)
                     {
                        gameRecord->setHostRating(prop->value.fData);
                     }
                     else if (prop->dwPropertyId == X_PROPERTY_GAMER_MU)
                     {
                        gameRecord->setHostMu(prop->value.dblData);
                     }
                     else if (prop->dwPropertyId == X_PROPERTY_GAMER_SIGMA)
                     {
                        gameRecord->setHostSigma(prop->value.dblData);
                     }
                  }

                  BSimString name;
                  name.format( "Live game %i", i);
                  gameRecord->setName( name );

                  //Calculate the match quality for each
                  gameRecord->calcuateMatchQuality(mClientSigma, mClientMu);
                  
                  //OMG SPAM
                  BNetIPString strAddr(gameRecord->getXnAddr().inaOnline);
                  BNetIPString strAddrInternal(gameRecord->getXnAddr().ina);
                  BSimString key;
                  key.format("0x%08X%08X%08X%08X", *(DWORD*)&liveDataRecord.info.keyExchangeKey.ab[0],*(DWORD*)&liveDataRecord.info.keyExchangeKey.ab[4],*(DWORD*)&liveDataRecord.info.keyExchangeKey.ab[8],*(DWORD*)&liveDataRecord.info.keyExchangeKey.ab[12]);                  
                  nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Entry %d - %s - %s/%s - matchQ:%f Sig:%f Mu:%f Key:%s", i, gameRecord->getHostGamertag().getPtr(), strAddr.getPtr(), strAddrInternal.getPtr(), gameRecord->getMatchQuality(), gameRecord->getHostSigma(), gameRecord->getHostMu(), key.getPtr());
                  
                  //Filter out entry if it is in the bad target list
                  if (mpMatchMaker->isBadTarget(gameRecord->getXnAddr()))
                  {
                     nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - dropping entry because it is in bad target list");
                     gameRecord->setState( BLiveGameDescriptor::cStateComplete );
                     gameRecord->setResultState( BLiveGameDescriptor::cResultsBadTargetEntry);
                  }

                  //Filter out entry if it is outside the current match quality target
                  else if (gameRecord->getMatchQuality()< mMinMatchQuality)
                  {
                     nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - dropping entry due to match quality %f/%f", gameRecord->getMatchQuality(), mMinMatchQuality);
                     gameRecord->setState( BLiveGameDescriptor::cStateComplete );
                     gameRecord->setResultState( BLiveGameDescriptor::cResultsBadMatchQuality);
                     //Don't mark these as bad targets, when the skill level delta expands in future searches - we don't want to exclude them
                     //mpMatchMaker->addBadTarget(gameRecord->getXnAddr());
                  }
                  else
                  {
                     gameRecord->setState( BLiveGameDescriptor::cStateHostQoSRunning );
                     //Fill in a localQoS query structure
                     Utils::FastMemCpy( &mXnAddr[mGoodMatchCount], &liveDataRecord.info.hostAddress, sizeof(XNADDR) );
                     Utils::FastMemCpy( &mXnKID[mGoodMatchCount], &liveDataRecord.info.sessionID, sizeof(XNKID) );
                     Utils::FastMemCpy( &mXnKey[mGoodMatchCount], &liveDataRecord.info.keyExchangeKey, sizeof(XNKEY) );
                     gameRecord->setQoSSearchIndex(mGoodMatchCount);
                     mGoodMatchCount++;
                     if (mGoodMatchCount>=mMaxResultCount)
                     {
                        //We don't want to ever request QoS for more than what the max results were for the original configuration of this hopper
                        //(we added in the count of the bad targets list to the request earlier)
                        break;
                     }
                  }
               }

               if ((mResultCount==0) || (mGoodMatchCount==0))
               {
                  //No results
                  nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - No sessions match the search query");
                  mSearchState = cSessionSearchStateResultsReady;
               }
               else
               {
                  //Launch a QoS Query
                  nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Results ready, launching batched QoS against them");
                  const XNADDR* ptrsXNADDR[cBLiveSessionSearchMaxSearchResults];
                  const XNKID* ptrsXNKID[cBLiveSessionSearchMaxSearchResults];
                  const XNKEY* ptrsXNKEY[cBLiveSessionSearchMaxSearchResults];
                  for (uint i = 0; i < mGoodMatchCount; i++)
                  {
                     ptrsXNADDR[i] = &mXnAddr[i];
                     ptrsXNKID[i] = &mXnKID[i];
                     ptrsXNKEY[i] = &mXnKey[i];
                  }
                  int result = XNetQosLookup(mGoodMatchCount, ptrsXNADDR, ptrsXNKID, ptrsXNKEY, 0, NULL, 0, cBLiveSessionSearchInitialQoSProbes, 0, 0, NULL, &mpQoSResults );
                  if (result !=0 )
                  {
                     nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - qos launched failed with error %d", result);
                     mSearchState = cSessionSearchStateResultsMissingError;
                     return;
                  }
                  mQOSLaunchTime = timeGetTime();
                  mSearchState = cSessionSearchStateWaitingForQoSRunning;
               }
            }
            else
            {
               //Check if we have waited long enough on Live
               if ((timeGetTime() - mInitialQueryLaunchTime) > cBLiveSessionSearchLiveResultsMaxTime)
               {
                  nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - SessionSearch took too long, canceling");
                  mSearchState = cSessionSearchStateResultsReady;
               }
            }
            break;
         }
      case(cSessionSearchStateWaitingForQoSRunning) :
         {
            if (mpQoSResults)
            {
               //I have at least some results ready
               //Go through the results and move that data/state into the tracking structure
               uint hostReadyCount=0;
               mPossibleMatchCount=0;
               
               for (uint hostIndex=0;hostIndex<mpQoSResults->cxnqos;hostIndex++)
               {
                  //Get a pointer to the QoS results for this host
//-- FIXING PREFIX BUG ID 1305
                  const XNQOSINFO* hostQoS = &mpQoSResults->axnqosinfo[hostIndex];
//--
                  //Get a pointer to the descriptor for this host
                  //BLiveGameDescriptor* hostInfo = &mResultDescriptors[hostIndex];
                  BLiveGameDescriptor* hostInfo = NULL;
                  for (uint i=0;i<mResultCount;i++)
                  {
                     if (mResultDescriptors[i].getQoSSearchIndex()==hostIndex)
                     {
                        hostInfo = &mResultDescriptors[i];
                        break;
                     }
                  }
                  BASSERT(hostInfo);
                  //This should never happen - but lets check for it anyways
                  if (!hostInfo)
                  {
                     //Skip that entry
                     break;
                  }

                  //We are waiting on QoS results from this potential host - evaluate where we are
                  if ((hostInfo->getState() == BLiveGameDescriptor::cStateHostQoSRunning) &&
                     (hostQoS->bFlags & XNET_XNQOSINFO_COMPLETE))
                  {
                     //OMG MORE spam
                     BNetIPString strAddr(hostInfo->getXnAddr().inaOnline);
                     BNetIPString strAddrInternal(hostInfo->getXnAddr().ina);
                     nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Got QoS results for Entry %s - %s/%s - matchQ:%f", hostInfo->getHostGamertag().getPtr(), strAddr.getPtr(), strAddrInternal.getPtr(), hostInfo->getMatchQuality());

                     //System says it has completed its QoS attempt
                     if ((hostQoS->cProbesRecv != cBLiveSessionSearchInitialQoSProbes) ||
                         !(hostQoS->bFlags & XNET_XNQOSINFO_DATA_RECEIVED))
                     {
                        //But we either didn't get all our probes back or we never got the host packed-in data
                        // Either means this is not a good target host
                        nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Dropping target [%d - %s/%s] - QoS cant reach it", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr());
                        hostInfo->setState(BLiveGameDescriptor::cStateComplete);
                        hostInfo->setResultState(BLiveGameDescriptor::cResultsCouldNotContactHost);
                        hostInfo->setQOSHostTime(timeGetTime()-mQOSLaunchTime);
                        mpMatchMaker->addBadTarget(hostInfo->getXnAddr());
                     }
                     else
                     {
                        //We have results
                        BASSERT(hostQoS->pbData);
                        BQoSResponseData hostData;
                        if (hostQoS->cbData == 1 )
                        {
                           //If the host is up, but not ready to handle joins yet - just skip it but don't put on the bad target list
                           nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Dropping target [%d - %s/%s] - Host not ready yet", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr());
                           hostInfo->setState(BLiveGameDescriptor::cStateComplete);
                           hostInfo->setResultState(BLiveGameDescriptor::cResultsCouldNotContactHost);  
                        }
                        else if (hostQoS->cbData != sizeof(hostData) )
                        {
                           //Hmm - buffer overrun/underrun would have happened here, I'm going to assume that the data is bad - we want to skip this one
                           nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - host data buffer size is too large (%d, max is %d) - skipping it", hostQoS->cbData, sizeof(hostData) );
                           nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Dropping target [%d - %s/%s] - Bad buffer", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr());
                           hostInfo->setState(BLiveGameDescriptor::cStateComplete);
                           hostInfo->setResultState(BLiveGameDescriptor::cResultsBufferError);
                           mpMatchMaker->addBadTarget(hostInfo->getXnAddr());
                        }
                        else
                        {
                           //Copy/set all the data we know about this host target
                           Utils::FastMemCpy(&hostData, hostQoS->pbData, hostQoS->cbData);
                           hostInfo->setChecksum(hostData.mCheckSum);
                           hostInfo->setOpenSlots(hostData.mPublicSlotsOpen);
                           hostInfo->setSlots(hostData.mPublicSlots);
                           hostInfo->setNonce(hostData.mNonce);
                           hostInfo->setLanguageCode(hostData.mLanguageCode);
                           //hostInfo->setVersionCode(hostData.mVersionCode);
                           hostInfo->setQOSHostTime(timeGetTime()-mQOSLaunchTime);
                           hostInfo->setAvgPing(hostQoS->wRttMedInMsecs); 

                           //Check for various filters
                           bool filteredOut = false;

                           //Is the session reporting it is full?
                           if (!filteredOut) 
                           {
                              if (hostData.mPublicSlotsOpen == 0)
                              {
                                 //Yup - no public slots are open
                                 nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Dropping target [%d - %s/%s] - Game is full", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr());
                                 hostInfo->setState(BLiveGameDescriptor::cStateComplete);
                                 hostInfo->setResultState(BLiveGameDescriptor::cResultsFull);  
                                 //mpMatchMaker->addBadTarget(hostInfo->getXnAddr());
                                 filteredOut = true;
                              }
                           }
                           //Check the checksum
                           if (!filteredOut) 
                           {
                              if (mCheckSum != hostData.mCheckSum)
                              {
                                 nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Dropping target [%d - %s/%s] - Wrong checksum number ChkSum:[0x%08X]", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr(), hostData.mCheckSum);
                                 hostInfo->setResultState(BLiveGameDescriptor::cResultsVersionCodeMismatch);
                                 hostInfo->setState(BLiveGameDescriptor::cStateComplete);
                                 mpMatchMaker->addBadTarget(hostInfo->getXnAddr());
                                 filteredOut = true;
                              }
                           }
                           //Are we doing language code filtering
                           if (!filteredOut) 
                           {
                              if (mLanguageCode != hostData.mLanguageCode)
                              {
                                 nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Dropping target [%d - %s/%s] - Wrong langauge code [%d]", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr(), hostData.mLanguageCode);
                                 hostInfo->setResultState(BLiveGameDescriptor::cResultsLanguageCodeMismatch);
                                 hostInfo->setState(BLiveGameDescriptor::cStateComplete);
                                 mpMatchMaker->addBadTarget(hostInfo->getXnAddr());
                                 filteredOut = true;
                              }
                           } 
                           //Is the ping too high
                           if (!filteredOut)
                           {
                              if (hostQoS->wRttMedInMsecs > mMaxPing)
                              {
                                 nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Dropping target [%d - %s/%s] - Ping too high [%d]", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr(), hostQoS->wRttMedInMsecs);
                                 hostInfo->setResultState(BLiveGameDescriptor::cResultsPingAboveMaximum);
                                 hostInfo->setState(BLiveGameDescriptor::cStateComplete);
                                 mpMatchMaker->addBadTarget(hostInfo->getXnAddr());
                                 filteredOut = true;
                              }
                           }  

                           if (!filteredOut)
                           {
                              //This host record is complete
                              nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Valid target [%d - %s/%s] ping[%d]", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr(), hostQoS->wRttMedInMsecs);                           
                              hostInfo->setResultState(BLiveGameDescriptor::cResultsJoinable);  
                              hostInfo->setState(BLiveGameDescriptor::cStateComplete);    
                           }
                        }
                     }
                  }
                  
                  //Check for completeness
                  if (hostInfo->getState() >= BLiveGameDescriptor::cStateComplete)
                  {
                     //Lets count out how many are ready
                     hostReadyCount++;

                     //Note: this is just for debugging/logging, player never sees this string
                     BSimString resultStr("");
                     switch(hostInfo->getResultState())
                     {
                        case(BLiveGameDescriptor::cResultsNone):
                           {
                              resultStr.format("Result state NONE");
                              break;
                           }
                        case(BLiveGameDescriptor::cResultsFull):
                           {
                              resultStr.format("Game full (%d)",hostInfo->getAvgPing());
                              break;
                           }
                        case(BLiveGameDescriptor::cResultsCouldNotContactHost):
                           {
                              resultStr.format("Host not responding");
                              break;
                           }
                        case(BLiveGameDescriptor::cResultsCouldNotContactPeer):
                           {
                              resultStr.format("Peer not responding (%d)",hostInfo->getAvgPing());
                              break;
                           }
                        case(BLiveGameDescriptor::cResultsBufferError):
                           {
                              resultStr.format("Buffer error");
                              break;
                           }
                        case(BLiveGameDescriptor::cResultsClientQoSError):
                           {
                              resultStr.format("Client QoS Error");
                              break;
                           }
                        case(BLiveGameDescriptor::cResultsVersionCodeMismatch):
                           {
                              resultStr.format("Version code mismatch");
                              break;
                           }
                        case(BLiveGameDescriptor::cResultsLanguageCodeMismatch):
                           {
                              resultStr.format("Language code mismatch");
                              break;
                           }
                        case(BLiveGameDescriptor::cResultsPingAboveMaximum):
                           {
                              resultStr.format("Ping above max");
                              break;
                           }                           
                        case(BLiveGameDescriptor::cResultsJoinable):
                           {        
                              mPossibleMatchCount++;
                              BASSERT(mPossibleMatchCount<51);
                              BSimString t;
                              t.copy(hostInfo->getHostGamertag());
                              resultStr.format("Good %s, r:%4.2f slots:%d ping:%d", t.getPtr(), hostInfo->getHostRating(), hostInfo->getOpenSlots(), hostInfo->getAvgPing());
                              break;
                           }
                     }
                     hostInfo->setName( resultStr );
                     //nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - QoS result processed:%s", hostInfo->getName().getPtr() );
                  }                  
               }              

               //If every one is complete - mark the results as completely finished
               if (hostReadyCount == mpQoSResults->cxnqos)
               {
                  sortResults();
                  mStartToPostQosTime = timeGetTime() - mInitialQueryLaunchTime;
                  nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Post QoS results complete");
                  mSearchState = cSessionSearchStateResultsReady;
               }   
               else
               {
                  //Check if we have waited long enough for this QoS scan to complete
                  BASSERT(gLiveSystem->getHopperList());
                  if (gLiveSystem->getHopperList())
                  {
                     if ((timeGetTime() - mQOSLaunchTime) > gLiveSystem->getHopperList()->getStaticData()->mMaxQoSResponseTime)                  
                     {
                        nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - QoS took too long [%d ms], canceling", (timeGetTime() - mQOSLaunchTime));
                        //Kill the QoS if it is out there
                        if (mpQoSResults)
                        {
                           //Kill the initial QoS request
                           XNetQosRelease(mpQoSResults);
                           mpQoSResults = NULL;
                        }
                        //Mark any unfinished host entrys as complete - host not reachable
                        BLiveGameDescriptor* hostInfo = NULL;
                        for (uint i=0;i<mResultCount;i++)
                        {
                           hostInfo = &mResultDescriptors[i];
                           if (hostInfo)
                           {
                              if (hostInfo->getState()==BLiveGameDescriptor::cStateHostQoSRunning)
                              {
                                 hostInfo->setState(BLiveGameDescriptor::cStateComplete);
                                 hostInfo->setResultState(BLiveGameDescriptor::cResultsCouldNotContactHost);
                                 hostInfo->setQOSHostTime(timeGetTime()-mQOSLaunchTime);
                                 BNetIPString strAddr(hostInfo->getXnAddr().inaOnline);
                                 BNetIPString strAddrInternal(hostInfo->getXnAddr().ina);
                                 nlog(cMPMatchmakingCL, "BLiveSessionSearch::update - Dropping target [%d - %s/%s] - QoS timed out [%d ms]", hostIndex, strAddr.getPtr(), strAddrInternal.getPtr(), hostInfo->getQOSHostTime());
                                 mpMatchMaker->addBadTarget(hostInfo->getXnAddr());
                              }
                           }
                        }
                        //Sort what we have
                        sortResults();
                        //Mark search as complete
                        mSearchState = cSessionSearchStateResultsReady;
                     }
                  }
                  else
                  {
                     //There is no hopper list - just ditch out of here
                     //Mark search as complete
                     mSearchState = cSessionSearchStateResultsReady;
                  }
               }
            }
            break;
         }
   }
}

//==============================================================================
// 
//==============================================================================
DWORD BLiveSessionSearch::getNumberOfSearchResults(void)
{
   return mResultCount;
}

//==============================================================================
// 
//==============================================================================
BLiveGameDescriptor* BLiveSessionSearch::getSearchResultRecord( DWORD index )
{
    if ((( mSearchState == cSessionSearchStateResultsReady )  ||
        ( mSearchState == cSessionSearchStateResultsProcessed)) &&
        ( index <  mResultCount))
   {
      return &mResultDescriptors[index];
   }
   return NULL;
}

//==============================================================================
// 
//==============================================================================
void BLiveSessionSearch::sortResults()
{
#ifndef BUILD_FINAL
   nlog(cMPMatchmakingCL, "BLiveSessionSearch::sortResults - Pre-sort joinable target list");
   BSimString t;
   for (DWORD i=0;i<mResultCount;i++)
   {
      BLiveGameDescriptor* hostInfo = &mResultDescriptors[i];
      if (hostInfo->getResultState()==BLiveGameDescriptor::cResultsJoinable)
      {
         t.copy(hostInfo->getHostGamertag());
         nlog(cMPMatchmakingCL, "%d Target:%s, MQ:%f ping:%d", i, t.getPtr(), hostInfo->getMatchQuality(), hostInfo->getAvgPing());
      }
   }
#endif   

   //Special sort to order the results according to ping within a particular range
   //The issue is this - the search comes back in the preferred order (social versus ranked) according to what Live thinks is best
   //  To this ordering - we want to sort by ping any records that are over the preferred ping
   //So for example if you were looking at a ranked results you would see (from start to end):
   //   Under 100ms ping - sorted by original order (skill delta)
   //   Over 100ms ping - sorted by ping from lowest to highest
   std::sort( &mResultDescriptors[0], &mResultDescriptors[mResultCount-1], &CompareResultRows );


#ifndef BUILD_FINAL
   nlog(cMPMatchmakingCL, "BLiveSessionSearch::sortResults - Post-sort joinable target list");
   for (DWORD i=0;i<mResultCount;i++)
   {
      BLiveGameDescriptor* hostInfo = &mResultDescriptors[i];
      if (hostInfo->getResultState()==BLiveGameDescriptor::cResultsJoinable)
      {
         t.copy(hostInfo->getHostGamertag());
         nlog(cMPMatchmakingCL, "%d Target:%s, MQ:%f ping:%d", i, t.getPtr(), hostInfo->getMatchQuality(), hostInfo->getAvgPing());
      }
   }
#endif   

}

//==============================================================================
//
//==============================================================================
BOOL BLiveSessionSearch::CompareResultRows( const BLiveGameDescriptor& a, const BLiveGameDescriptor& b )
{
   //Sort order
   // mState = cResultsJoinable first
   // IF ping<perfPing
   //    by matchqual
   // Else
   //    by ping

   if (a.getResultState()==BLiveGameDescriptor::cResultsJoinable)
   {
      if (b.getResultState()!=BLiveGameDescriptor::cResultsJoinable)
      {
         return TRUE;
      }
   }
   else
   {
      if (b.getResultState()==BLiveGameDescriptor::cResultsJoinable)
      {
         return FALSE;
      }
   }

   if (!gLiveSystem->getMPSession() || !gLiveSystem->getMPSession()->getMatchmaker() || !gLiveSystem->getMPSession()->getMatchmaker()->getCurrentSessionSearch())
   {
      //Should never get hit
      return TRUE;
   }

   WORD perfPing = gLiveSystem->getMPSession()->getMatchmaker()->getCurrentSessionSearch()->getPerferredPing();
   
   if (a.getAvgPing()<=perfPing)
   {
      if (b.getAvgPing()<=perfPing)
      {
         //Sort by match quality
         return (a.getMatchQuality()<b.getMatchQuality());
      }
      else
      {
         return TRUE;
      }
   }
   else
   {
      if (b.getAvgPing()<=perfPing)
      {
         return FALSE;
      }
      else
      {
         //Sort by ping
         return (a.getAvgPing()<b.getAvgPing());
      }
   }
}
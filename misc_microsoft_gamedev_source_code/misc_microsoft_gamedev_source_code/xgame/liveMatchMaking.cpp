//==============================================================================
// liveMatchMaking.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#include "Common.h"
#include "liveMatchMaking.h"

#include "LiveSystem.h"
#include "XLastGenerated.h"            //For the TitleID
#include "mpSession.h"
#include "liveSession.h"
#include "mpGameSession.h"

#include "netPackets.h"
#include "lspManager.h"

#include "mpcommheaders.h"
#include "commlog.h"

//xSystem
#include "config.h"
#include "econfigenum.h"

//==============================================================================
// 
//==============================================================================
BLiveMatchMaking::BLiveMatchMaking(BMPSession* mpSession) :
   mState(cBLiveMatchMakingStateNone),
   mLastError(cBLiveMatchMakingErrorNone),
   mJoinAttemptInProgress(FALSE),
   mPartyFullyJoined(FALSE),
   mpLiveSessionSearch(NULL),
   mStartTime(0),
   mStateTimer(0),
   mSearchCount(0),
   mPrimaryJoinFailures(0),
   mClientJoinFailures(0),
   mLanguageCode(0),
   //mVersionCode(0),
   mSkillQueryResultsSize(0),
   mpSkillQueryResults(NULL),
   mCountdownDisplayDigit(0),
   mpMPSession(mpSession)
{
   BASSERT(mpSession);

#ifndef BUILD_FINAL
   long value=0;
   if (gConfig.get(cConfigMMFilterHackCode, &value))
   {
      mLanguageCode=(uint8)value;
      nlog(cMPMatchmakingCL, " BLiveMatchMaking::BLiveMatchMaking - config has specified a matchmaking hack code of %d, system will only matchmake againist other systems with the same code", mLanguageCode);
   }
#endif
   
   //Eventually we'll have version numbers, but for now just use a subset of the game checksum
   //mVersionCode = (uint8) mpMPSession->getCachedGameChecksum();
   mpSearchRequest = new BMPSessionMatchMakingData();
   XNetRandom( (BYTE*)&mPerfLogDataPacketCurrent.mMatchMakingNonce, sizeof(mPerfLogDataPacketCurrent.mMatchMakingNonce) );
   Utils::FastMemSet(mpSearchRequest, 0, sizeof(BMPSessionMatchMakingData));
   Utils::FastMemSet(&mSkillQueryOverlapped, 0, sizeof(XOVERLAPPED));
   Utils::FastMemSet(&mLastJoinTarget, 0, sizeof(XNADDR)); 
   for (int i=0;i<BPerfLogPacketV2::cPerfLogMaxXuids;i++)
      mMembers[i].mXuid = 0;
   mBadTargetList.clear();
   //HAX - lets add our local xnaddr to the bad targets list in slot 0
   XNADDR localXn;
   if (gLiveSystem->getLocalXnAddr(localXn))
   {
      addBadTarget(localXn);
   }
   mState = cBLiveMatchMakingStateReadyToStart;
}

//==============================================================================
// 
//==============================================================================
BLiveMatchMaking::~BLiveMatchMaking()
{
   if (!XHasOverlappedIoCompleted(&mSkillQueryOverlapped))
   {
      //It never completed
      XCancelOverlapped( &mSkillQueryOverlapped );
   }

   if (mpSkillQueryResults)
   {
      delete[] mpSkillQueryResults;
      mpSkillQueryResults = NULL;
   }

   if (mpSearchRequest)
   {
      delete (mpSearchRequest);
      mpSearchRequest = NULL;
   }

   if (mpLiveSessionSearch)
   {
      delete mpLiveSessionSearch;
      mpLiveSessionSearch = NULL;
   }

   setState(cBLiveMatchMakingStateDeleted);
}

//==============================================================================
// 
//==============================================================================
void BLiveMatchMaking::setState(BLiveMatchMakingState newState)
{
   //Safe way to set the state so that it never overrides a state of 'shutdown'
   if ((mState >= cBLiveMatchMakingStateError) &&
      (mState > newState))
   {
      //This will allow the state to be moved forward only once it is in shutting down mode (all modes after that one are steps towards being deleted)
      nlog(cMPMatchmakingCL, " BLiveMatchMaking::setState - Ignoring new state, I am shutting down or errored out");
      return;
   }
   mState = newState;
}

//==============================================================================
// 
//==============================================================================
uint BLiveMatchMaking::getMaxMembersPerTeamForCurrentSearch()
{
   if (mState==cBLiveMatchMakingEventCodeNone)
   {
      return 0;
   }

   if (!mpSearchRequest || !mpSearchRequest->mSelectedHopper)
   {
      return 0;
   }

   uint expectedPartySize = mpSearchRequest->mSelectedHopper->mPlayersPerTeam;
   if ( mpSearchRequest->mSelectedHopper->mTeamCode == BMatchMakingHopper::cBMatchMakingHopperTeamCodeNoTeams)
   {
      expectedPartySize = 1;
   }
   return (expectedPartySize);
}

//==============================================================================
// 
//==============================================================================
void BLiveMatchMaking::update()
{
   //Some states I am in are final - no need for more processing
   if (mState>=cBLiveMatchMakingStateComplete) 
   {
      return;
   }

   //Make sure we still have the interfaces to do our job
   if (!mpMPSession->getSessionInterface())
   {
      nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- ERROR: Aborted, no session/playerManager interface");
      stopWithError(cBLiveMatchMakingErrorNoInterface);
      return;
   }

   BASSERT(mpMPSession);
   BASSERT(mpSearchRequest);
   BASSERT(mpSearchRequest->mSelectedHopper);
   
   //We don't do any processing if we are waiting on mpSession to hook us up into a game
   if (mJoinAttemptInProgress && !mPartyFullyJoined)
   {
      //Removing this -it is WAY too spammy
      //nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Waiting on mpSession to hook entire party into a game - no more updating");
      return;
   }

   //Don't do any processing if there is a game that is launching
   if (gLiveSystem->getMPSession()->getGameSession() && 
       gLiveSystem->getMPSession()->getGameSession()->isGameRunning())
   {
      return;
   }

   //Lets check if this whole thing has just taken too long
   if ((timeGetTime() - mStartTime) > gLiveSystem->getHopperList()->getStaticData()->mMaxMatchmakingTimeTotal) 
      
   {
      nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Matchmaking process has taken too long, canceling [%d ms]", (timeGetTime() - mStartTime));
      stopWithError(cBLiveMatchMakingErrorTimedOut);
      return;
   }

   //Process based on state
   switch (mState)
   {
      case (cBLiveMatchMakingStatePreLaunch) :
      {
         //This is the pre-start wait state
         if (timeGetTime()- mStartTime > 1000)
         {
            //Ok - we now go into a pre-state which does the following things async:
            // - Queries the party session to get everyone to return their skill values for everyone for this hopper REMOVE
            // - Query live for the skill data for everyone in this hopper
            // - Queries the LSP for a new configData refresh
            // - Counts down from X seconds to zero
            // At the end of the countdown, it verifies:
            // - That it has the Live skill values (and calcs an average if needed)
            // - That it has new configData
            // - That the hopper is still valid against the new configData
            // Then it starts actual processing of matchmaking

            //Request the confifData refresh if older than X seconds
            // Let the normal TTL system determine this
            /*
            if ((timeGetTime() - gLiveSystem->getHopperList()->getTimeSinceLastUpdate()) > 5000)
            {
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Config data is old [%d ms], requesting refresh", (timeGetTime() - gLiveSystem->getHopperList()->getTimeSinceLastUpdate()));
               gLiveSystem->getMPSession()->remoteRequestESOConfigFile();
            }
            */

            //Get the XUIDs for everyone in the party session
            uint partySize = mpMPSession->getPartySession()->getPartyCount();
            uint expectedPartySize = mpSearchRequest->mSelectedHopper->mPlayersPerTeam;
            if ( mpSearchRequest->mSelectedHopper->mTeamCode == BMatchMakingHopper::cBMatchMakingHopperTeamCodeNoTeams)
            {
               expectedPartySize = 1;
            }
            BASSERT( partySize<=BPerfLogPacketV2::cPerfLogMaxXuids );
            if (partySize != expectedPartySize)
            {
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Wrong number of players for this hopper");
               setState(cBLiveMatchMakingStateError);
               mpMPSession->matchMakingFailed(cBLiveMatchMakingErrorWrongNumberOfPartyMembers);
               return;
            }

            Utils::FastMemSet(&mSkillQueryXuidList, 0, sizeof(XUID) * BPerfLogPacketV2::cPerfLogMaxXuids);
            mpMPSession->getPartySession()->getPartyXUIDs(mSkillQueryXuidList);
            //Setup a local record for each XUID
            for (uint i=0;i<partySize;i++)
            {
               mMembers[i].mXuid = mSkillQueryXuidList[i];
            }
            //Request the party session to send out a request to everyone to report in their skill values
            //mpMPSession->getPartySession()->partySendSkillRequest( (uint8)mpSearchRequest->mSelectedHopper->mListIndex );

            //Request the skill data from Live
            BASSERT(mpSkillQueryResults==NULL);
            Utils::FastMemSet(&mSkillQueryRequest[0], 0, sizeof(XUSER_STATS_SPEC));    //Current support is only for ONE query
            mSkillQueryRequest[0].dwNumColumnIds = 4;
            mSkillQueryResultsSize = 0;
            //We need to map around the fact that the indexes in the XLAST file are not contiguous for the matchmade hoppers :|
            if (mpSearchRequest->mSelectedHopper->mGameModeIndex>2)
            {
               //FUTURE DLC SUPPORT - lets us put all other game modes into 1 hopper
               BASSERT(mpSearchRequest->mSelectedHopper->mXLastGameModeIndex!=0);
               mSkillQueryRequest[0].dwViewId = STATS_VIEW_SKILL_RANKED_RANDOM_1V1 + (mpSearchRequest->mSelectedHopper->mXLastGameModeIndex % 5);
            }
            else
            {
               mSkillQueryRequest[0].dwViewId = STATS_VIEW_SKILL_RANKED_STANDARD_1V1 + mpSearchRequest->mSelectedHopper->mXLastGameModeIndex -1;
            }            
            mSkillQueryRequest[0].rgwColumnIds[0] = X_STATS_COLUMN_SKILL_SKILL;
            mSkillQueryRequest[0].rgwColumnIds[1] = X_STATS_COLUMN_SKILL_GAMESPLAYED;
            mSkillQueryRequest[0].rgwColumnIds[2] = X_STATS_COLUMN_SKILL_MU;
            mSkillQueryRequest[0].rgwColumnIds[3] = X_STATS_COLUMN_SKILL_SIGMA;

            DWORD titleID;
#ifdef TITLEID_HALO_WARS_ALPHA
            titleID = TITLEID_HALO_WARS_ALPHA;
#else
            titleID = TITLEID_HALO_WARS;
#endif
            DWORD ret = XUserReadStats( titleID, partySize, &mSkillQueryXuidList[0], 1, &mSkillQueryRequest[0], &mSkillQueryResultsSize,  mpSkillQueryResults, NULL );
            if ((ret != ERROR_INSUFFICIENT_BUFFER) ||
               (mSkillQueryResultsSize ==0))
            {
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- ERROR: Skill request failed on FIRST call [%d]", ret);
               setState(cBLiveMatchMakingStateError);
               mpMPSession->matchMakingFailed(cBLiveMatchMakingErrorSkillCallFailed1);
               return;
            }

            mpSkillQueryResults = (PXUSER_STATS_READ_RESULTS) new BYTE[mSkillQueryResultsSize];

            if (!mpSkillQueryResults)
            {
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- ERROR: Could not alloc for the skill query results");
               setState(cBLiveMatchMakingStateError);
               mpMPSession->matchMakingFailed(cBLiveMatchMakingErrorOutOfMemory);
               return;
            }

            Utils::FastMemSet(mpSkillQueryResults, 0, mSkillQueryResultsSize);
            Utils::FastMemSet(&mSkillQueryOverlapped, 0, sizeof(XOVERLAPPED));   
            ret = XUserReadStats( titleID, partySize, &mSkillQueryXuidList[0], 1, &mSkillQueryRequest[0], &mSkillQueryResultsSize,  mpSkillQueryResults, &mSkillQueryOverlapped );
            //ret = XUserReadStats( titleID, partySize, xuidList, 1, &mSkillQueryRequest[0], &mSkillQueryResultsSize,  mpSkillQueryResults, NULL);

            if (ret != ERROR_IO_PENDING)
            {
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- ERROR: Skill request failed on SECOND call [%d]", ret);
               setState(cBLiveMatchMakingStateError);
               mpMPSession->matchMakingFailed(cBLiveMatchMakingErrorSkillCallFailed2);
               return;
            }

            //mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeStartCountdown, 0, 0);
            setState(cBLiveMatchMakingStatePreLaunchQuerysActive);
            nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- going to cBLiveMatchMakingStatePreLaunchQuerysActive");
         }
      }
      case (cBLiveMatchMakingStatePreLaunchQuerysActive) :
         {
            //This is the pre-start wait state
            if (timeGetTime()- mStartTime > 4000)
            {
               setState(cBLiveMatchMakingStatePreLaunchQuerysWaiting);
            }
            else
            {
               //Update the message every second
               uint diff =  timeGetTime() - mStartTime;
               if (diff != 0)
               {
                  diff = 5 - (uint)((diff) / 1000);
               }            
               if (diff != mCountdownDisplayDigit)
               {
                  mCountdownDisplayDigit = diff;
                  mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeStartCountdown, mCountdownDisplayDigit, 0);
               }               
            } 
            break;
         }
      case (cBLiveMatchMakingStatePreLaunchQuerysWaiting):
      {
         //This is the pre-start wait state
         //if (timeGetTime()- mStartTime > 4000)
         {
            //Times up - lets see if we have everything to get going or not

            //Get the skill query results
            if (!XHasOverlappedIoCompleted(&mSkillQueryOverlapped))
            {
               //It has not completed
               //Have I waited long enough? (ten seconds)
               if (timeGetTime()- mStartTime > 11000)    
               {
                  XCancelOverlapped( &mSkillQueryOverlapped );
                  nlog(cMPPartySystemCL, "BLiveMatchMaking::update - Skill query never completed");
                  stopWithError(cBLiveMatchMakingErrorMissingSkillData);
                  return;
               }
               else
               {
                  //Lets wait a lil longer
                  return;
               }
            }

            //Call completed - check the results
            HRESULT hr = XGetOverlappedExtendedError( &mSkillQueryOverlapped );
            if (FAILED(hr))
            {
               //It failed
               nlog(cMPPartySystemCL, "BLiveMatchMaking::update - Skill query failed with return code [0x%08X]", hr);
               stopWithError(cBLiveMatchMakingErrorMissingSkillData);
               return;   
            }

            //Check that there are results
            if (!mpSkillQueryResults)
            {
               //Total failure
               nlog(cMPPartySystemCL, "BLiveMatchMaking::update - Skill query failed, there is no result data");
               stopWithError(cBLiveMatchMakingErrorMissingSkillData);
               return;   
            }

            //Check update on the config data
            //Let the TTL system handle this
            /*
            if ((timeGetTime() - gLiveSystem->getHopperList()->getTimeSinceLastUpdate()) > 11000)
            {
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- WARNING - Config data is old");
               //stopWithError(cBLiveMatchMakingErrorMissingSkillData);
               //return;
            }
            */

            //Validate the requested hopper
            /*
            if (mpSearchRequest->mSelectedHopper->mEnabled == FALSE)
            {
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Hopper index %i is NOT ENABLED", mpSearchRequest->mSelectedHopper);
               stopWithError(cBLiveMatchMakingErrorInvalidHopper);
               return;
            }
            */

            //Do we have the correct number of rows?
            if (mpSkillQueryResults->pViews[0].dwNumRows != mpMPSession->getPartySession()->getPartyCount())
            {
               //Nope
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Skill query only returned [%d] rows, party has [%d] members", mpSkillQueryResults->pViews[0].dwNumRows, mpMPSession->getPartySession()->getPartyCount());
               stopWithError(cBLiveMatchMakingErrorWrongNumberOfPartyMembers);
               return;
            }
                        
            //Loadup the perflog data that is the base (stuff that won't change as we cycle through the process)
            mPerfLogDataPacketCurrent.mEventCode = cBLiveMatchMakingEventCodeNone;
            mPerfLogDataPacketCurrent.mHopperIndex = (byte)mpSearchRequest->mSelectedHopper->mXLastGameModeIndex;
            uint resultCount = mpSkillQueryResults->pViews[0].dwNumRows;
            const DWORD cColumnCount = 4;
            for (uint i=0;i<resultCount;i++)
            {
               //Get the XUID
               mMembers[i].mXuid = mpSkillQueryResults->pViews[0].pRows[i].xuid;
               mPerfLogDataPacketCurrent.mXuids[i] = mMembers[i].mXuid;
               //Double check that the result is valid
               if (mMembers[i].mXuid==0)
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- ERROR: Skill query returned a row [%d] with NO Xuid", i);
                  stopWithError(cBLiveMatchMakingErrorMissingSkillData);
                  return;
               }
               //Get the query results from the columns
               if (mpSkillQueryResults->pViews[0].pRows[i].dwNumColumns != cColumnCount)
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- ERROR: Skill query only returned [%d] columns, we requested[%d]", mpSkillQueryResults->pViews[0].pRows[i].dwNumColumns, cColumnCount);                  
                  stopWithError(cBLiveMatchMakingErrorMissingSkillData);
                  return;
               }
               for (uint j=0;j<cColumnCount;j++)
               {
                  switch (mpSkillQueryResults->pViews[0].pRows[i].pColumns[j].wColumnId)
                  {
                     case (X_STATS_COLUMN_SKILL_SKILL):
                        {
                           mMembers[i].mRating = mpSkillQueryResults->pViews[0].pRows[i].pColumns[j].Value.i64Data;
                           mMemberSkillAverage.mRating += mMembers[i].mRating;
                           break;
                        }
                     case (X_STATS_COLUMN_SKILL_GAMESPLAYED):
                        {
                           mMembers[i].mGamesPlayed = mpSkillQueryResults->pViews[0].pRows[i].pColumns[j].Value.i64Data;
                           mMemberSkillAverage.mGamesPlayed += mMembers[i].mGamesPlayed;
                           break;
                        }
                     case (X_STATS_COLUMN_SKILL_MU):
                        {
                           mMembers[i].mMu = mpSkillQueryResults->pViews[0].pRows[i].pColumns[j].Value.dblData;
                           mMemberSkillAverage.mMu += mMembers[i].mMu;
                           break;
                        }
                     case (X_STATS_COLUMN_SKILL_SIGMA):
                        {
                           mMembers[i].mSigma = mpSkillQueryResults->pViews[0].pRows[i].pColumns[j].Value.dblData;
                           mMemberSkillAverage.mSigma += mMembers[i].mSigma * mMembers[i].mSigma;
                           break;
                        }
                  default:
                     {
                        //We didn't request this column - error
                        nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- ERROR: Skill query only returned column index [%d] that we did NOT request", mpSkillQueryResults->pViews[0].pRows[i].pColumns[j].wColumnId);
                        stopWithError(cBLiveMatchMakingErrorMissingSkillData);
                        return;
                     }
                  }
               }

               //Live doesn't return default initial values if they have never played a game - it returns zeros - check for this
               if ((mMembers[i].mMu==0) &&
                  (mMembers[i].mSigma==0))
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- No skill data for player, using defaults 1.0/3.0");
                  mMembers[i].mMu=3.0;
                  mMembers[i].mSigma=1.0;
                  mMemberSkillAverage.mMu += mMembers[i].mMu;
                  mMemberSkillAverage.mSigma += mMembers[i].mSigma * mMembers[i].mSigma;
               }
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Processed skill data for XUID [%I64u] - Rating:%d Gamesplayed:%d Sigma:%f Mu:%f",
                  mMembers[i].mXuid, mMembers[i].mRating, mMembers[i].mGamesPlayed, mMembers[i].mSigma, mMembers[i].mMu );
            }
            //Calculate averages
            mMemberSkillAverage.mXuid = 0;          
            mMemberSkillAverage.mSigma = sqrt( mMemberSkillAverage.mSigma / (DOUBLE) resultCount );
            mMemberSkillAverage.mMu = mMemberSkillAverage.mMu /(DOUBLE) resultCount;
            mMemberSkillAverage.mRating = (uint64)(mMemberSkillAverage.mRating / resultCount);
            mMemberSkillAverage.mGamesPlayed = (uint64)(mMemberSkillAverage.mGamesPlayed / resultCount);

            //Lets redo sigma/mu using the official stuff
            double origSigma[BPerfLogPacketV2::cPerfLogMaxXuids];
            double origMu[BPerfLogPacketV2::cPerfLogMaxXuids];
            double newSigma;
            double newMu;
            for (uint i=0;i<resultCount;i++)
            {
               origSigma[i] = mMembers[i].mSigma;
               origMu[i] = mMembers[i].mMu;
            }
            DWORD ret = XSessionCalculateSkill(resultCount, (double*)&origMu, (double*)&origSigma, &newMu, &newSigma);
            if (ret!=ERROR_SUCCESS)
            {
               BASSERTM(false, "BLiveMatchMaking::update() - XSessionCalculateSkill failed");
            }
            else
            {
               mMemberSkillAverage.mSigma = newSigma;
               mMemberSkillAverage.mMu = newMu;
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Skill average: Rating:%d Gamesplayed:%d Sigma:%f Mu:%f",
                  mMemberSkillAverage.mRating, mMemberSkillAverage.mGamesPlayed, mMemberSkillAverage.mSigma, mMemberSkillAverage.mMu );
            }
            
            //Show it in the UI
            uint skillValue = (uint)(mMemberSkillAverage.mRating);
            mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeShowSkillInfo, skillValue, 0);

            //Add averages to the perflog post
            mPerfLogDataPacketCurrent.mAverageGameCount = (uint16)mMemberSkillAverage.mGamesPlayed;
            //mPerfLogDataPacketBase.mAverageSigma = (float)mMemberSkillAverage.mSigma;
            //mPerfLogDataPacketBase.mAverageMu = (float)mMemberSkillAverage.mMu;
            BASSERT(mMemberSkillAverage.mRating<51);
            mPerfLogDataPacketCurrent.mAverageRating = (byte)mMemberSkillAverage.mRating;
            mPerfLogDataPacketCurrent.mEventCode = (byte)cBLiveMatchMakingEventCodeStarting;
            BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mTotalRunTime, timeGetTime() - mStartTime);
            //gLSPManager.postPerfData(&mPerfLogDataPacketCurrent);

            //Check for testing optional override
            if (gLiveSystem->getMPTestOptions( BLiveSystem::cLiveSystemTestHostOnly)==TRUE)
            {
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Starting in Host mode due to override");
               setState(cBLiveMatchMakingStateHosting);
               return;
            }

            //See what to do first - host or scan
            if (!mpSearchRequest->mSelectedHopper->mFastScan)
            {
               //No initial quick scan - see if we host or search first
               if (float(rand()/RAND_MAX) < mpSearchRequest->mSelectedHopper->mHostPercent)
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Starting in Host mode due to random");
                  setState(cBLiveMatchMakingStateHosting);
                  return;
               }
               else
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::start -- Starting in detailed search mode due to random");
                  setState(cBLiveMatchMakingStateDetailedSearch);
                  return;
               }
            }

            //Do fast scan
            setState(cBLiveMatchMakingStateFastSearch);
            nlog(cMPMatchmakingCL, "BLiveMatchMaking::start -- Starting in fast scan mode");
            return;
         }
        
         break;
      }

      case (cBLiveMatchMakingStatePreHostRandomWait):
        {
            //This is a wait state just to add some randomness to the cycle times
            //Probably only needed for development
           BASSERT(mJoinAttemptInProgress==FALSE);
            if (mStateTimer==0)
            {      
               mStateTimer = timeGetTime() + mpSearchRequest->mSelectedHopper->mHostDelayTimeBase;
               if (mpSearchRequest->mSelectedHopper->mHostDelayTimeRandom!=0)
               {
                  mStateTimer+=DWORD(rand() % mpSearchRequest->mSelectedHopper->mHostDelayTimeRandom);
               }
               BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mHostDelayTime, mStateTimer - timeGetTime());
               mCountdownDisplayDigit = 0;
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Started prehosting wait of time:%d", (mStateTimer - timeGetTime()));
            }
            else
            {
               if (timeGetTime() > mStateTimer)
               {
                  mStateTimer=0;
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Started prehosting wait time up, going to hosting state");
                  setState(cBLiveMatchMakingStateHosting);
               }
               else
               {
                  //Update the message every second
                  uint diff = mStateTimer - timeGetTime();
                  if (diff != 0)
                  {
                     diff = (uint)(diff / 1000);
                  }
                  if (diff != mCountdownDisplayDigit)
                  {
                     mCountdownDisplayDigit = diff;
                     mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeCycleDelay, mCountdownDisplayDigit, 0);
                  } 
               }
            }
            break;
        }

      case (cBLiveMatchMakingStateHosting) :
         {
            //I am currently in a hosting mode
            if (mStateTimer==0)
            {
               //Need to start hosting up
               //Is it clear to do so?
               if (!mpMPSession->isReadyToStartGameSession())
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Want to start a new host, but waiting on mpSession to go back to idle state");
                  return;
               }
               //TODO - change so that it uses the private slot count to reserve spaces for party members and any second local player
               if (!mpMPSession->hostStartupLive(mpSearchRequest->mSelectedHopper->mXLastGameModeIndex, mpSearchRequest->mSelectedHopper->mRanked, mpSearchRequest->mSelectedHopper->mPlayers, 0))
               {
                  //Hosting failed
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- ERROR - hostStartupLive reported immediate failure");
                  stopWithError(cBLiveMatchMakingErrorHostingFailed);
                  return;
               }
               mStateTimer = timeGetTime();
               mJoinAttemptInProgress = TRUE;
               mPartyFullyJoined = FALSE;
               BSimString temp = "Hosting initiated...";
               mpMPSession->getSessionInterface()->mpSessionEvent_ESOSearchStatusChanged(temp);
               mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeWaitingForMorePlayers, 0, 0);
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Started hosting, total time:%d", (timeGetTime()-mStartTime));
            }
            else
            {
               //Make sure we have not waited too long in this hosting state
               if (gLiveSystem->getMPTestOptions( BLiveSystem::cLiveSystemTestHostOnly)==TRUE)
               {
                  //With that flag set - we never time out
                  return;
               }
               BOOL quitHosting = FALSE;
               if ((mpSearchRequest->mSelectedHopper->mTeamCode == BMatchMakingHopper::cBMatchMakingHopperTeamCodeNoTeams) && 
                   (mpMPSession->getGameSessionPlayerCount()>2))
               {
                  //This is the "we got 3" rule.  So in a non-team, 2v2 or 3v3.  Once we have 3 people in there - wait the timeout time * the room size (max size)
                  //Obviously if someone drops - then we are back to normal rules
                  if ((timeGetTime() - mStateTimer) > (mpSearchRequest->mSelectedHopper->mHostTimeout * mpMPSession->getGameSessionPlayerCount() * 2))
                  {
                     nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Canceling hosting, %d connected players, waited max time, host time:%d total time:%d", mpMPSession->getGameSessionPlayerCount(), (timeGetTime()-mStateTimer), (timeGetTime()-mStartTime));
                     quitHosting = TRUE;
                  }
               }
               else if (mpMPSession->getGameSessionPlayerCount()>1)
               {
                  //We have connected players
                  if ((timeGetTime() - mStateTimer) > (mpSearchRequest->mSelectedHopper->mHostTimeout * mpMPSession->getGameSessionPlayerCount()))
                  {
                     nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Canceling hosting, %d connected players, host time:%d total time:%d", mpMPSession->getGameSessionPlayerCount(), (timeGetTime()-mStateTimer), (timeGetTime()-mStartTime));
                     quitHosting = TRUE;
                  }
               }
               else
               {
                  //No connected players
                  if ((timeGetTime() - mStateTimer) > mpSearchRequest->mSelectedHopper->mHostTimeout)
                  {
                     nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Canceling hosting, no connected players, host time:%d total time:%d", (timeGetTime()-mStateTimer), (timeGetTime()-mStartTime));
                     quitHosting = TRUE;
                  }
               }
               if (quitHosting)
               {
                  //Switch back to scanning
                  BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mHostTime, (timeGetTime() - mStateTimer));
                  setState(cBLiveMatchMakingStateDetailedSearch);
                  mStateTimer = 0;
                  mpMPSession->abortGameSession();          
               }
            }
            break;
         }

      case (cBLiveMatchMakingStateFastSearch) :
         {
            if (mStateTimer==0)
            {
               //Lets make sure any previous session interaction is complete
               if (!mpMPSession->isReadyToStartGameSession())
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Want to start searching, but waiting on mpSession to go back to idle state");
                  return;
               }
               //Need to start this fast search up
               if (mpLiveSessionSearch)
               {
                  delete mpLiveSessionSearch;
                  mpLiveSessionSearch = NULL;
               }
               mpLiveSessionSearch = new BLiveSessionSearch(this, 
                  mpSearchRequest->mSelectedHopper->mXLastGameModeIndex, 
                  mpSearchRequest->mSelectedHopper->mFastScanSearchCount, 
                  mpSearchRequest->mSelectedHopper->mRanked, 
                  gLiveSystem->getHopperList()->getStaticData()->mPreferredPing, 
                  gLiveSystem->getHopperList()->getStaticData()->mPreferredPing,
                  mLanguageCode, 
                  mpMPSession->getCachedGameChecksum(), 
                  mpSearchRequest->mSelectedHopper->mMinMatchQualityPerPass[0], 
                  mMemberSkillAverage.mSigma, 
                  mMemberSkillAverage.mMu);
               mStateTimer = timeGetTime();
               BSimString temp = "Fastscan initiated...";
               mpMPSession->getSessionInterface()->mpSessionEvent_ESOSearchStatusChanged(temp);  
               mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeFastScan, 0, 0);
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Started Fastscan, total time:%d", (timeGetTime()-mStartTime));
            }
            else 
            {
               if (mJoinAttemptInProgress)
               {
                  if (mPartyFullyJoined)
                  {
                     //We have joined a target and are waiting on them to start the game or to cancel it - game host has control at this point
                     return;
                  }
                  //Not everyone in the party has joined
                  //For now - lets wait on that connection logic to time out and continue us on
                  return;
               }

               //Lets make sure any previous session interaction is complete
               if (!mpMPSession->isReadyToStartGameSession())
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Want to continue searching, but waiting on mpSession to go back to idle state");
                  return;
               }

               //Check here if the search is stale (IE: Its been around for a 30+ seconds, if so all those targets are probably gone)
               if ((timeGetTime() - mStateTimer) > gLiveSystem->getHopperList()->getStaticData()->mMaxTimeForASingleSearch)
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- search data is stale [%d]ms, dropping it, moving to pre-host", (timeGetTime() - mStateTimer) );
                  BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mFastScanTime,(timeGetTime() - mStateTimer));
                  if (mpLiveSessionSearch)
                  {
                     delete mpLiveSessionSearch;
                     mpLiveSessionSearch = NULL;
                  }
                  mStateTimer = 0;
                  setState(cBLiveMatchMakingStatePreHostRandomWait);
               }
               else 
               {
                  //Just need to continue processing the search results
                  BASSERT(mpLiveSessionSearch);
                  mpLiveSessionSearch->update();

                  //But dont' act on the results if the game session isnt in a state where it can join
                  if (mpLiveSessionSearch->areResultsReady())
                  {
                     //All results are in - process them
                     BSimString temp = "Processing scan results...";
                     if (mPerfLogDataPacketCurrent.mFastScanData.mResultCount == 0)
                     {
                        //First time through here - store off some stats
                        mPerfLogDataPacketCurrent.mFastScanData.mResultCount = (byte)mpLiveSessionSearch->getNumberOfSearchResults();
                        mPerfLogDataPacketCurrent.mFastScanData.mPostQOSResultCount = (byte)mpLiveSessionSearch->getNumberOfPossbileMatches();
                        //BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mFastScanTime, mpLiveSessionSearch->getStartToPostQosTime());
                     }
                     mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeFastScan, mpLiveSessionSearch->getNumberOfSearchResults(), mpLiveSessionSearch->getNumberOfPossbileMatches());
                     mpMPSession->getSessionInterface()->mpSessionEvent_ESOSearchStatusChanged(temp );   
                     if (processHopperSearchResults()==FALSE)
                     {
                        //There are no further results to process                     
                        BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mFastScanTime,(timeGetTime() - mStateTimer));
                        delete mpLiveSessionSearch;
                        mpLiveSessionSearch = NULL;
                        mStateTimer = 0;
                        setState(cBLiveMatchMakingStatePreHostRandomWait);
                     }
                  }
               }
            }
            break;
         }

      case(cBLiveMatchMakingStateDetailedSearch) :
         {
            if (mStateTimer==0)
            {
               //Lets make sure any previous session interaction is complete
               if (!mpMPSession->isReadyToStartGameSession())
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Want to do detailed searching, but waiting on mpSession to go back to idle state");
                  return;
               }

               //Need to start this fast search up
               if (mpLiveSessionSearch)
               {
                  delete mpLiveSessionSearch;
                  mpLiveSessionSearch = NULL;
               }
               DWORD passIndex = mSearchCount+1;
               if (passIndex>=cBMatchMakingMatchQualityEntries)
                  passIndex=cBMatchMakingMatchQualityEntries-1;
               mpLiveSessionSearch = new BLiveSessionSearch(this, 
                  mpSearchRequest->mSelectedHopper->mXLastGameModeIndex, 
                  mpSearchRequest->mSelectedHopper->mNormalSearchCount, 
                  mpSearchRequest->mSelectedHopper->mRanked, 
                  gLiveSystem->getHopperList()->getStaticData()->mMaxPing,
                  gLiveSystem->getHopperList()->getStaticData()->mPreferredPing,
                  mLanguageCode, 
                  mpMPSession->getCachedGameChecksum(), 
                  mpSearchRequest->mSelectedHopper->mMinMatchQualityPerPass[passIndex], 
                  mMemberSkillAverage.mSigma, 
                  mMemberSkillAverage.mMu);
               mStateTimer = timeGetTime();
               BSimString temp = "Full scan initiated...";
               mpMPSession->getSessionInterface()->mpSessionEvent_ESOSearchStatusChanged(temp);  
               mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeNormalScan, 0, 0);
               nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Started detailed scan, total time:%d", (timeGetTime()-mStartTime));
            }
            else
            {
               if (mJoinAttemptInProgress)
               {
                  if (mPartyFullyJoined)
                  {
                     //We have joined a target and are waiting on them to start the game or to cancel it - game host has control at this point
                     return;
                  }
                  //Not everyone in the party has joined
                  //For now - lets wait on that connection logic to time out and continue us on
                  return;
               }

               //Lets make sure any previous session interaction is complete
               if (!mpMPSession->isReadyToStartGameSession())
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Want to do detailed searching, but waiting on mpSession to go back to idle state");
                  return;
               }

               //Check here if the search is stale (IE: Its been around for a 30+ seconds, if so all those targets are probalby gone)
               if ((timeGetTime() - mStateTimer) > gLiveSystem->getHopperList()->getStaticData()->mMaxTimeForASingleSearch)
               {
                  nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- search data is stale [%d]ms, dropping it, moving to pre-host", (timeGetTime() - mStateTimer) );
                  BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mSearchTime,(timeGetTime() - mStateTimer));
                  if (mpLiveSessionSearch)
                  {
                     delete mpLiveSessionSearch;
                     mpLiveSessionSearch = NULL;
                  }
                  mStateTimer = 0;
                  mSearchCount++;
                  setState(cBLiveMatchMakingStateEndOfCycle);
               }
               else 
               {
                  //Just need to continue processing the search results
                  BASSERT(mpLiveSessionSearch);
                  mpLiveSessionSearch->update();
                  if (mpLiveSessionSearch->areResultsReady())
                  {
                     //All results are in - process them
                     BSimString temp = "Processing scan results...";
                     if (mPerfLogDataPacketCurrent.mSearchData.mResultCount == 0)
                     {
                        //First time through here - store off some stats
                        mPerfLogDataPacketCurrent.mSearchData.mResultCount = (byte)mpLiveSessionSearch->getNumberOfSearchResults();
                        mPerfLogDataPacketCurrent.mSearchData.mPostQOSResultCount = (byte)mpLiveSessionSearch->getNumberOfPossbileMatches();
                        //BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mSearchTime, mpLiveSessionSearch->getStartToPostQosTime());
                     }
                     BASSERT(mpLiveSessionSearch->getNumberOfPossbileMatches()<=mpLiveSessionSearch->getNumberOfSearchResults());
                     mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeNormalScan, mpLiveSessionSearch->getNumberOfSearchResults(), mpLiveSessionSearch->getNumberOfPossbileMatches());
                     mpMPSession->getSessionInterface()->mpSessionEvent_ESOSearchStatusChanged(temp );   
                     if (processHopperSearchResults()==FALSE)
                     {
                        //There are no further results to process
                        delete mpLiveSessionSearch;
                        mpLiveSessionSearch = NULL;
                        BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mSearchTime, (timeGetTime() - mStateTimer));
                        mStateTimer = 0;
                        mSearchCount++;
                        setState(cBLiveMatchMakingStateEndOfCycle);
                     }
                  }
               }
            }
            break;
         }
      case(cBLiveMatchMakingStateEndOfCycle) :
         {
            //Dummy state that posts perflog up and starts back at the beginning of the search cycle
            BPerfLogPacketV2::setTimeFromMSTime(&mPerfLogDataPacketCurrent.mTotalRunTime, timeGetTime() - mStartTime);
            mPerfLogDataPacketCurrent.mEventCode = cBLiveMatchMakingEventCodeEndOfCycle;
            mPerfLogDataPacketCurrent.mCycleCount++;
            //gLSPManager.postPerfData(&mPerfLogDataPacketCurrent);
            //Utils::FastMemCpy( &mPerfLogDataPacketCurrent, &mPerfLogDataPacketBase, sizeof(BPerfLogPacket));
            setState(cBLiveMatchMakingStatePreHostRandomWait);
            break;
         }
   }
}

//==============================================================================
// Returns TRUE if there are still results left to process after this pass
//==============================================================================
BOOL BLiveMatchMaking::processHopperSearchResults()
{   
   //But dont' act on the results if the game session isn't in a state where it can join
   if (!mpMPSession->isReadyToStartGameSession())
   {
      nlog(cMPMatchmakingCL, "BLiveMatchMaking::update -- Want to look at maybe joining a game, but waiting on mpSession to go back to idle state");
      return TRUE;      //Indicate to the caller that there may be more results to process since I didnt actually get to checking the list
   }

   for (uint i=0;i<mpLiveSessionSearch->getNumberOfSearchResults();i++)
   {
      BLiveGameDescriptor* result = mpLiveSessionSearch->getSearchResultRecord(i);
      if ((result) &&
          (result->getState() == BLiveGameDescriptor::cStateComplete) &&
          (result->getResultState() == BLiveGameDescriptor::cResultsJoinable))
      {
         //Lets try to join that session
         if (mState==cBLiveMatchMakingStateFastSearch)
         {
            mPerfLogDataPacketCurrent.mFastScanData.mConnectionAttempts++;
         }
         else
         {
            mPerfLogDataPacketCurrent.mSearchData.mConnectionAttempts++;
         }
         mJoinAttemptInProgress = TRUE;
         mPartyFullyJoined = FALSE;
         BSimString temp = "Attempting join...";
         mpMPSession->getSessionInterface()->mpSessionEvent_ESOSearchStatusChanged(temp);  
         mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeAttemptingJoin, 0, 0);
         result->setState(BLiveGameDescriptor::cStateProcessed);
         Utils::FastMemCpy(&mLastJoinTarget, &result->getXnAddr(), sizeof(XNADDR)); 
         //addBadTarget(result->getXnAddr());
         //Go ahead and record the metrics for why we picked this
         mPerfLogDataPacketCurrent.mLaunchedMatchSkillAverage = (byte)((result->getHostRating() + mMemberSkillAverage.mRating)/2);
         mPerfLogDataPacketCurrent.mLaunchedMatchRatingQuality = (byte)(result->getMatchQuality()*100);
         if (mpMPSession->joinLiveGame(result))
         {
            nlog(cMPMatchmakingCL, "BLiveMatchMaking::processHopperSearchResults -- join live game launched with match quality of [%d]percent, skill avg[%d]",mPerfLogDataPacketCurrent.mLaunchedMatchRatingQuality, mPerfLogDataPacketCurrent.mLaunchedMatchSkillAverage);
            return TRUE;         
         }      
      }
   }

   //Nothing left to process
   nlog(cMPMatchmakingCL, "BLiveMatchMaking::processHopperSearchResults -- Current hopper search has no results left to process");
   return FALSE;
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveMatchMaking::start( BMPSessionMatchMakingData* configData )
{
   if (!mpMPSession->getSessionInterface())
   {
      nlog(cMPMatchmakingCL, "BLiveMatchMaking::start -- ERROR: Aborted, no session/playerManager interface");
      setState(cBLiveMatchMakingStateError);
      mpMPSession->matchMakingFailed(cBLiveMatchMakingErrorNoInterface);
      return FALSE;
   }

   //Sanity checks
   BASSERT(mpMPSession);
   BASSERT(gLiveSystem->getHopperList());
   BASSERT(configData);

   //Member variable copies
   Utils::FastMemCpy(mpSearchRequest, configData, sizeof(BMPSessionMatchMakingData));
   mStartTime = timeGetTime();
   if (!mpSearchRequest->mSelectedHopper)
   {
      nlog(cMPMatchmakingCL, "BLiveMatchMaking::start -- No hopper defined");
      setState(cBLiveMatchMakingStateError);
      mpMPSession->matchMakingFailed(cBLiveMatchMakingErrorInvalidHopper);
      return FALSE;
   }

   //update UI
   mCountdownDisplayDigit = 5;
   mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeStartCountdown, mCountdownDisplayDigit, 0);

   setState(cBLiveMatchMakingStatePreLaunch);
   nlog(cMPMatchmakingCL, "BLiveMatchMaking::start -- Starting countdown to begin");

   return TRUE;
}

//==============================================================================
// 
//==============================================================================
void BLiveMatchMaking::resume()
{
   nlog(cMPMatchmakingCL, "BLiveMatchMaking::resume -- continuing matchmaking process");
   if (!mJoinAttemptInProgress)
   {
      nlog(cMPMatchmakingCL, "BLiveMatchMaking::resume -- Ignoring request - I did not have a game session in progress currently");
      return;
   }
   if (!mPartyFullyJoined)
   {
      addBadTarget(mLastJoinTarget);      
   }
   mJoinAttemptInProgress=FALSE;
   mPartyFullyJoined=FALSE;
   if (mState==cBLiveMatchMakingStateHosting)
   {
      //If we were hosting - move on
      mStateTimer = 0;
      setState(cBLiveMatchMakingStateDetailedSearch);
   }
}

//==============================================================================
// This is called by mpsession letting us know that everyone in our party has joined the correct target
//==============================================================================
void BLiveMatchMaking::allPartyMembersJoinedTarget()
{
   nlog(cMPMatchmakingCL, "BLiveMatchMaking::allPartyMembersJoinedTarget -- Woot");
   mpMPSession->getPartySession()->partySendMatchmakingStatusInfo( cBLiveMatchMakingStatusCodeWaitingForMorePlayers, 0, 0);
   mPartyFullyJoined = TRUE;
   //If this is a party game - then tell Live to change our posted skill ratings to reflect everyone in our party
   uint partySize = mpMPSession->getPartySession()->getPartyCount();
   if ((partySize>1) &&
       (mpMPSession->getGameSession()) &&
       (mpMPSession->getGameSession()->getLiveSession()))
   {
      mpMPSession->getGameSession()->getLiveSession()->modifySkill(partySize, (XUID*)&mSkillQueryXuidList);
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveMatchMaking::stopWithError(BLiveMatchMakingErrorCode reason)
{
   BPerfLogPacketV2::setTimeFromMSTime(&mPerfLogDataPacketCurrent.mTotalRunTime, (timeGetTime() - mStartTime));
   mPerfLogDataPacketCurrent.mEventCode = (byte)reason;
   gLSPManager.postPerfData(&mPerfLogDataPacketCurrent);
   mPartyFullyJoined = FALSE;
   setState(cBLiveMatchMakingStateError);
   mLastError = reason;
   mpMPSession->matchMakingFailed(reason);
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveMatchMaking::abort()
{
   if (mJoinAttemptInProgress)
   {
      mpMPSession->abortGameSession();
      mJoinAttemptInProgress=FALSE;
   }
   stopWithError(cBLiveMatchMakingErrorAbortRequested);

   return TRUE;
}

//==============================================================================
// 
//==============================================================================
void BLiveMatchMaking::matchMadeGameLaunched()
{
   nlog(cMPMatchmakingCL, "BLiveMatchMaking::matchMadeGameLaunched -- Game has launched, matchmaking done");
   BPerfLogPacketV2::setTimeFromMSTime(&mPerfLogDataPacketCurrent.mTotalRunTime, (timeGetTime() - mStartTime));
   mPerfLogDataPacketCurrent.mEventCode = cBLiveMatchMakingEventCodeGameFound;
   if (mpMPSession->getGameSession() && mpMPSession->getGameSession()->getLiveSession())
   {
      mPerfLogDataPacketCurrent.mLaunchedMatchHostNonce = mpMPSession->getGameSession()->getLiveSession()->getNonce();
   }
   //Update the time bucket based on the mode we are in with the game launched
   switch (mState)
   {
      case (cBLiveMatchMakingStateFastSearch):
         {
            BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mFastScanTime,(timeGetTime() - mStateTimer));
            break;
         }
      case (cBLiveMatchMakingStateDetailedSearch):
         {
            BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mSearchTime,(timeGetTime() - mStateTimer));
            break;
         }
      case (cBLiveMatchMakingStateHosting):
         {
            BPerfLogPacketV2::addTimeFromMSTime(&mPerfLogDataPacketCurrent.mHostTime, (timeGetTime() - mStateTimer));
            break;
         }
   }   
   if (mpMPSession->getGameSession() && mpMPSession->getGameSession()->getSession())
   {
      uint32 avgPing = mpMPSession->getGameSession()->getSession()->getAveragePing();
      if (avgPing>UINT16_MAX)
         mPerfLogDataPacketCurrent.mLaunchedMatchPingAverage = UINT16_MAX;
      else
         mPerfLogDataPacketCurrent.mLaunchedMatchPingAverage = (uint16)avgPing;
   }

   gLSPManager.postPerfData(&mPerfLogDataPacketCurrent);
   setState(cBLiveMatchMakingStateComplete);
}

//==============================================================================
// 
//==============================================================================
void BLiveMatchMaking::addBadTarget(XNADDR xnaddr)
{
   if (gLiveSystem->getHopperList()->getStaticData()->mBadTargetListEnabled)
   {
      mBadTargetList.add(xnaddr);
   }
   else
   {
      nlog(cMPMatchmakingCL, "BLiveMatchMaking::addBadTarget -- Ignoring, bad target list deactivated"); 
   }
}

//==============================================================================
// 
//==============================================================================
bool BLiveMatchMaking::isBadTarget(XNADDR xnaddr)
{
   if (!gLiveSystem->getHopperList()->getStaticData()->mBadTargetListEnabled)
   {
      return false;
   }

   byte* checkMe = (byte*)&xnaddr;
   for (int i=0; i<mBadTargetList.getNumber(); i++)
   {
      byte* badOne = (byte*)&mBadTargetList[i];
      bool matched=true;
      for (uint k=0;k<sizeof(XNADDR);k++)
      {
         if (badOne[k]!=checkMe[k])
         {
            matched=false;
            break;
         }
      }
      if (matched)
      {
         if (i==0)
            nlog(cMPMatchmakingCL, "BLiveMatchMaking::isBadTarget - ERROR OMG I FOUND MYSELF IN THE SEARCH");
         return true;
      }
   }

   return false;
}



//==============================================================================
// 
//==============================================================================
BLiveMatchMakingMember::BLiveMatchMakingMember() :
mXuid(0),
mMu(0.0f),
mSigma(0.0f),
mRating(0),
mGamesPlayed(0)
{
}

/*
//==============================================================================
// 
//==============================================================================
void BLiveMatchMaking::requestedConnectionFailed()
{
   //Continue on matchmaking
   nlog(cMPMatchmakingCL, "BLiveMatchMaking::requestedConnectionFailed -- Continuing on in matchmaking");
   BASSERT(mJoinAttemptInProgress==TRUE);
   mJoinAttemptInProgress=FALSE;
   if (mState==cBLiveMatchMakingStateHosting)
   {
      //Move to the next step if we the ones hosting
      setState(cBLiveMatchMakingStateDetailedSearch);
      mStateTimer=0;
      
      //

   }
}
*/

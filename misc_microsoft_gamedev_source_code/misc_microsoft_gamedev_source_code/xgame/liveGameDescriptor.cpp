//==============================================================================
// liveGameDescriptor.cpp
//
// Copyright (c) 2006, Ensemble Studios
//==============================================================================

// Includes
#include "Common.h"
#include "liveGameDescriptor.h"

// For logging and configuration settings
#include "mpcommheaders.h"
#include "commlog.h"
#include "econfigenum.h"

//==============================================================================
// 
//==============================================================================
BLiveGameDescriptor::BLiveGameDescriptor() :
   mState(cStateNone),
   mResultState(cResultsNone),
   mChecksum(0),
   mLocal(false),
   mGameType(0),
   mClientCount(0),
   mpQoSResults(NULL),
   mOpenSlots(0),
   mSlots(0),
   mRanked(false),
   mGameModeIndex(0),
   mHostRating(0.0f),
   mHostSigma(0),
   mHostMu(0),
   mMatchQuality(0),
   mNonce(0),
   mLanguageCode(0),
   mVersionCode(0),
   mQOSHostTime(0),
   mQOSClientTime(0),
   mQOSSearchIndex(0),
   mTimeTrack(0),
   mAveragePing(0)
{
   Utils::FastMemSet(&mXnAddr, 0, sizeof(mXnAddr));
   Utils::FastMemSet(&mXnKID, 0, sizeof(mXnKID));
   Utils::FastMemSet(&mXnKey, 0, sizeof(mXnKey));
   mUpdateTime = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
BLiveGameDescriptor::~BLiveGameDescriptor()
{
   if (mpQoSResults)
   {
      XNetQosRelease( mpQoSResults );
      mpQoSResults = NULL;
   }
}

//==============================================================================
// 
//==============================================================================
//Returns true if this descriptor has been around longer than the expireTime (in ms)
bool BLiveGameDescriptor::hasExpired(DWORD expireTime)
{
   return ( (timeGetTime() - mUpdateTime) > expireTime);
}

//==============================================================================
// 
//==============================================================================
//Given a list of client XNADDRs - run QoS against all of them
void BLiveGameDescriptor::launchClientQoS(uint clientCount, XNADDR* clientXNADDRArray)
{
   BASSERT(false);
   //DEPRICATED!!!
   /*
   BASSERT(clientCount>0);

   mTimeTrack = timeGetTime();
   mClientCount = (uint8)clientCount;
   if (clientCount> cBLiveGameDescriptorMaxClients)
   {
      //Too many clients there - that host must be full
      setResultState(cResultsFull);
      setState(cStateComplete);
      return;
   }

   //Copy the client targets in
   Utils::FastMemCpy(mClientXnAddr, clientXNADDRArray, sizeof(XNADDR)*clientCount); 
   for (uint i=0;i<clientCount;i++)
   {
      Utils::FastMemCpy(mClientXnKID, &mXnKID, sizeof(XNKID)); 
      Utils::FastMemCpy(mClientXnKey, &mXnKey, sizeof(XNKEY)); 
   }

   //Launch off the QoS
   const XNADDR* ptrsXNADDR[cBLiveGameDescriptorMaxClients];
   const XNKID* ptrsXNKID[cBLiveGameDescriptorMaxClients];
   const XNKEY* ptrsXNKEY[cBLiveGameDescriptorMaxClients];
   for (uint i = 0; i < clientCount; i++)
   {
      ptrsXNADDR[i] = &mClientXnAddr[i];
      ptrsXNKID[i] = &mClientXnKID[i];
      ptrsXNKEY[i] = &mClientXnKey[i];
   }

   int result = XNetQosLookup(clientCount, ptrsXNADDR, ptrsXNKID, ptrsXNKEY, 0, NULL, 0, 8, 0, 0, NULL, &mpQoSResults );
   if (result !=0 )
   {
      setResultState(cResultsClientQoSError);
      setState(cStateComplete);
      return;
   }

   //Set our state so we check for those results
   setState(cStateClientQoSRunning);
   */
}

//==============================================================================
//Kill QoS for this record if it is running
//==============================================================================
void BLiveGameDescriptor::cancelQoS()
{
   //Lets do a little check here so we can set out results state correctly
   if (mState == cStateHostQoSRunning)
   {
      setResultState(cResultsCouldNotContactHost);
   }
   else if (mState == cStateClientQoSRunning)
   {
      setResultState(cResultsCouldNotContactPeer);
   }

   mQOSClientTime = timeGetTime() - mTimeTrack;

   //Clean up
   if (mpQoSResults)
   {
      XNetQosRelease( mpQoSResults );
      mpQoSResults = NULL;
   }
   setState(cStateComplete);
}

//==============================================================================
//
//==============================================================================
void BLiveGameDescriptor::processQoS()
{  
   BASSERT(mState == cStateClientQoSRunning);
   if (!mpQoSResults)
   {
      //Check for timeout
      //This is where a timeout check would go, however - liveSessionSearch (which calls this processQoS method)
      //  is already running an overall timeout check for QoS so this one would be redundant
      //mResultState = cResultsCouldNotContactPeer;
      //mState = cStateComplete;
   }
   else
   {
      //We have results - are they complete?
      if (mpQoSResults->cxnqosPending == 0)
      {
         //All probes done - lets check the results
         mQOSClientTime = timeGetTime() - mTimeTrack;
         uint connectedCount = 0;
         for (uint i=0;i<mpQoSResults->cxnqos;i++)
         {
            if (mpQoSResults->axnqosinfo[i].bFlags & XNET_XNQOSINFO_COMPLETE)
            {
               connectedCount++;
               mAveragePing = max(mAveragePing, mpQoSResults->axnqosinfo[i].wRttMedInMsecs);
            }
         }
         if (connectedCount == mClientCount)
         {
            mResultState = cResultsJoinable;
         }
         else
         {
            mResultState = cResultsCouldNotContactPeer;
         }
         mState = cStateComplete;
      }
   }  
}

//==============================================================================
//
//==============================================================================
void BLiveGameDescriptor::setName(const BSimString& name)
{
   mName = name;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
const BSimString& BLiveGameDescriptor::getName() const
{
   return mName;
}

//==============================================================================
//
//==============================================================================
void BLiveGameDescriptor::setHostGamertag(const BSimString& gamertag)
{
   mHostGamertag = gamertag;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
const BSimString& BLiveGameDescriptor::getHostGamertag() const
{
   return mHostGamertag;
}

//==============================================================================
//
//==============================================================================
void BLiveGameDescriptor::setLocal(bool local)
{
   mLocal = local;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
bool BLiveGameDescriptor::getLocal() const
{
   return mLocal;
}

//==============================================================================
//
//==============================================================================
void BLiveGameDescriptor::setChecksum(DWORD checksum)
{
   mChecksum = checksum;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
DWORD BLiveGameDescriptor::getChecksum() const
{
   return mChecksum;
}

//==============================================================================
// 
//==============================================================================
void BLiveGameDescriptor::setXNKID(const XNKID& newKid)
{
   mXnKID = newKid;
   mUpdateTime = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLiveGameDescriptor::setXNKEY(const XNKEY& newKey)
{
   mXnKey = newKey;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
void BLiveGameDescriptor::setXnAddr(const XNADDR& xnaddr)
{
   mXnAddr = xnaddr;
   //memcpy( &mXnAddr, copyThis, sizeof(XNADDR));
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
WORD BLiveGameDescriptor::getPort() const
{
   return mXnAddr.wPortOnline;
}

//==============================================================================
// 
//==============================================================================
const IN_ADDR& BLiveGameDescriptor::getAddress() const
{
   return mXnAddr.ina;
}

//==============================================================================
// 
//==============================================================================
const IN_ADDR& BLiveGameDescriptor::getTranslatedAddress() const
{
   return mXnAddr.inaOnline;
}

void BLiveGameDescriptor::calcuateMatchQuality(double targetSigma, double targetMu )
{
   //Stolen from the live sessions sample

   const int clientMulipicity = 1;
   const int hostMulipicity = 1;
   // used internally in Xbox 360 Live leaderboards for the PERFORMANCE VARIATION
   const DOUBLE dBetaXbox360Live = 0.5;
   // used internally in Xbox 360 Live leaderboards for the LEARNING FACTOR
   //const DOUBLE dTauXbox360Live = 0.01;

   DOUBLE dClientMu = targetMu * ( DOUBLE )clientMulipicity;
   DOUBLE dClientSigma2 = targetSigma * targetSigma * ( DOUBLE )clientMulipicity;
   DOUBLE dHostMu = mHostMu * ( DOUBLE )hostMulipicity;
   DOUBLE dHostSigma2 = mHostSigma * mHostSigma * ( DOUBLE )hostMulipicity;
   DOUBLE dTotalBeta2 = dBetaXbox360Live * dBetaXbox360Live * ( DOUBLE )( clientMulipicity + hostMulipicity );
   DOUBLE dMuDifference = dClientMu - dHostMu;
   DOUBLE dClientWithPerfectHostVariance = dTotalBeta2 + dClientSigma2;
   DOUBLE dClientWithCurrentHostVariance = dClientWithPerfectHostVariance + dHostSigma2;

   mMatchQuality =  exp( -0.5 * dMuDifference * dMuDifference / dClientWithCurrentHostVariance ) *
      sqrt( dClientWithPerfectHostVariance / dClientWithCurrentHostVariance );
}


//==============================================================================
// liveVoice.cpp
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================

#include "Common.h"
#include "liveVoice.h"

#include "liveSystem.h"
#include "mpcommheaders.h"
#include "commlog.h"
#include "econfigenum.h"

//==============================================================================
// 
//==============================================================================
BVoicePlayer::BVoicePlayer() :
   mXuid(0),
   mControllerID(XUSER_MAX_COUNT),
   mClientID(UINT32_MAX),
   mChannelCode(BVoice::cNone),
   mSessionID(0),
   mHeadset(FALSE),
   mIsLocal(false),
   mIsMuted(false)
{
   IGNORE_RETURN(Utils::FastMemSet(&mTalkerList, 0, sizeof(XUID)*XNetwork::cMaxClients));
   IGNORE_RETURN(Utils::FastMemSet(&mSessionMute, 0, sizeof(mSessionMute)));
   IGNORE_RETURN(Utils::FastMemSet(&mAddr, 0, sizeof(SOCKADDR_IN)));
}

//==============================================================================
// 
//==============================================================================
BVoicePlayer::BVoicePlayer(const BVoicePlayer& source)
{
   *this = source;
}

//==============================================================================
// 
//==============================================================================
BVoicePlayer& BVoicePlayer::operator=(const BVoicePlayer& source)
{
   if (this == &source)
      return *this;

   IGNORE_RETURN(Utils::FastMemCpy(&mTalkerList, source.mTalkerList, sizeof(XUID)*XNetwork::cMaxClients));
   IGNORE_RETURN(Utils::FastMemCpy(&mSessionMute, source.mSessionMute, sizeof(mSessionMute)));
   IGNORE_RETURN(Utils::FastMemCpy(&mAddr, &source.mAddr, sizeof(SOCKADDR_IN)));

   mXuid = source.mXuid;
   mControllerID = source.mControllerID;
   mClientID = source.mClientID;
   mChannelCode = source.mChannelCode;
   mSessionID = source.mSessionID;
   mHeadset = source.mHeadset;
   mIsLocal = source.mIsLocal;
   mIsMuted = source.mIsMuted;

   return *this;
}

//==============================================================================
// 
//==============================================================================
void BVoicePlayer::init(uint clientID, uint controllerID, XUID xuid, const SOCKADDR_IN& addr, uint sessionId)
{
   mControllerID = XUSER_MAX_COUNT;
   mClientID = clientID;
   mXuid = xuid;
   mSessionID = sessionId;
   mAddr = addr;

   // remote talkers will use XUSER_MAX_COUNT as their controllerID, it will be up to us
   // to determine their location

   for (uint i=0; i < XUSER_MAX_COUNT; ++i)
   {
      XUID tempXuid;
      if ((XUserGetXUID(i, &tempXuid) == ERROR_SUCCESS) && (xuid == tempXuid))
      {
         mControllerID = i;
         mIsLocal = true;
         break;
      }
   }

   if (controllerID < XUSER_MAX_COUNT && mIsLocal == false)
   {
      mControllerID = controllerID;
      mIsLocal = true;
   }
}

//==============================================================================
// 
//==============================================================================
BLiveVoice::BLiveVoice() :
   mpXHVEngine(NULL),
   mQueuedData(&gNetworkHeap),
   mVoiceEventHandle(cInvalidEventReceiverHandle),
   mSessionEventHandle(cInvalidEventReceiverHandle),
   mSessionIndex(0),
   mXNotifyHandle(INVALID_HANDLE_VALUE),
   mVoiceSampleInterval(cDefaultVoiceInterval),
   mTalkerThrottle(0),
   mQueuedThrottle(0),
   mVoiceTimerSet(false)
{
   for (uint i=0; i < BVoice::cMaxSessions*cMaxClients; i++)
      mHeadsetPresent[i] = 0;

   IGNORE_RETURN(Utils::FastMemSet(mHeadsetPresentInternal, 0, sizeof(mHeadsetPresentInternal)));

   IGNORE_RETURN(Utils::FastMemSet(mSpeakersEnabled, 0, sizeof(mSpeakersEnabled)));

   for (uint i=0; i < XUSER_MAX_COUNT; i++)
   {
      mPortXuidMap[i] = 0;
      mProfileMute[i] = 1;
   }

   IGNORE_RETURN(Utils::FastMemSet(mHasVoice, 0, sizeof(mHasVoice)));

   IGNORE_RETURN(Utils::FastMemSet(mChatBuffer, 0, sizeof(mChatBuffer)));
   IGNORE_RETURN(Utils::FastMemSet(mLocalDataSize, 0, sizeof(mLocalDataSize)));

   IGNORE_RETURN(Utils::FastMemSet(mReadProfileRequest, 0, sizeof(mReadProfileRequest)));

   for (uint i=0; i < BVoice::cMaxSessions*cMaxClients; i++)
      mIsTalking[i] = 0;

   IGNORE_RETURN(Utils::FastMemSet(mIsTalkingInternal, 0, sizeof(mIsTalkingInternal)));

   for (uint i=0; i < BVoice::cMaxSessions; i++)
   {
      for (uint j=0; j < XUSER_MAX_COUNT; j++)
      {
         for (uint k=0; k < cMaxClients; k++)
         {
            mMuteList[i][j][k] = 0;
         }
      }
   }

#ifndef BUILD_FINAL
   long voiceSampleInterval = 0;
   if (gConfig.get(cConfigVoiceSampleInterval, &voiceSampleInterval))
      mVoiceSampleInterval = static_cast<DWORD>(voiceSampleInterval);
#endif

   registerMPCommHeaders();

   nlog(cMPVoiceCL, "BLiveVoice created");
}

//==============================================================================
// 
//==============================================================================
BLiveVoice::~BLiveVoice() 
{
   // cleanup
   shutdown();
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::initSession(BVoice::Session session, BEventReceiverHandle eventHandle, bool set)
{
   if (mVoiceEventHandle == cInvalidEventReceiverHandle)
      return;

   BDEBUG_ASSERT(eventHandle != cInvalidEventReceiverHandle);
   if (eventHandle == cInvalidEventReceiverHandle)
      return;

   // send a message to the voice thread to switch sessions
   gEventDispatcher.send(eventHandle, mVoiceEventHandle, cVoiceEventInitSession, session, set);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setSession(BVoice::Session session)
{
   if (mVoiceEventHandle == cInvalidEventReceiverHandle)
      return;

   // send a message to the voice thread to switch sessions
   gEventDispatcher.send(cInvalidEventReceiverHandle, mVoiceEventHandle, cVoiceEventSetSession, session);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setChannel(BVoice::Session session, XUID xuid, BVoice::Channel channel)
{
   if (mVoiceEventHandle == cInvalidEventReceiverHandle)
      return;

   BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
   pVoiceRequest->init(xuid, channel);
   gEventDispatcher.send(cInvalidEventReceiverHandle, mVoiceEventHandle, cVoiceEventSetChannel, session, 0, pVoiceRequest);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setAllChannels(BVoice::Session session, BVoice::Channel channel)
{
   if (mVoiceEventHandle == cInvalidEventReceiverHandle)
      return;

   // send a message to the voice thread to switch sessions
   gEventDispatcher.send(cInvalidEventReceiverHandle, mVoiceEventHandle, cVoiceEventSetChannel, session, channel);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::migrateSessionIfAble(BVoice::Session fromSession, BVoice::Session toSession, BVoice::Channel channel)
{
   if (mVoiceEventHandle == cInvalidEventReceiverHandle)
      return;

   // send a message to the voice thread to switch sessions
   gEventDispatcher.send(cInvalidEventReceiverHandle, mVoiceEventHandle, cVoiceEventMigrateSession, (toSession << 16) | fromSession, channel);
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveVoice::isTalking(BVoice::Session session, uint clientID)
{
   if (session >= BVoice::cMaxSessions)
      return FALSE;
   if (clientID >= cMaxClients)
      return FALSE;

   return mIsTalking[(session * cMaxClients) + clientID];
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveVoice::isHeadsetPresent(BVoice::Session session, uint clientID)
{
   if (session >= BVoice::cMaxSessions)
      return FALSE;
   if (clientID >= cMaxClients)
      return FALSE;

   return mHeadsetPresent[(session * cMaxClients) + clientID];
}

//==============================================================================
// Checks the mute status of the given clientID/xuid pair for the given
//    session for the specific controller
//==============================================================================
BOOL BLiveVoice::isMuted(BVoice::Session session, uint controllerID, uint clientID, XUID xuid, BOOL& tcr90Mute, BOOL& sessionMute)
{
   tcr90Mute = FALSE;
   sessionMute = FALSE;

   if (controllerID >= XUSER_MAX_COUNT)
      return TRUE;

   if (clientID >= cMaxClients)
      return TRUE;

   if (xuid == 0)
      return TRUE;

   // don't attempt to query for myself because it will say that I'm muted
   if (mPortXuidMap[controllerID] == xuid)
      return static_cast<BOOL>(mProfileMute[controllerID]);

   BOOL retval = TRUE;
   BOOL result = TRUE;

   uint rc = XUserMuteListQuery(controllerID, xuid, &result);
   if (rc == ERROR_NOT_LOGGED_ON)
      return FALSE;
   retval = ((rc != ERROR_SUCCESS) || result);

   // if we're muted from the blade, don't bother with the TCR #90 checks
   if (retval)
      return TRUE;

   if (clientID >= XNetwork::cMaxClients)
      return retval;

   if (session >= BVoice::cMaxSessions)
      return TRUE;

   retval = mMuteList[session][controllerID][clientID];

   tcr90Mute = (retval & 0xFFFF);
   sessionMute = (retval >> 16);

   return (tcr90Mute || sessionMute);
}

//==============================================================================
// This is a temporary mute for the session
//==============================================================================
void BLiveVoice::mutePlayer(BVoice::Session session, uint controllerID, XUID xuid, BOOL mute)
{
   if (session >= BVoice::cMaxSessions)
      return;

   if (controllerID >= XUSER_MAX_COUNT)
      return;

   if (mVoiceEventHandle == cInvalidEventReceiverHandle)
      return;

   // send a message to the voice thread to mute the given player
   BVoiceSessionMuteRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceSessionMuteRequestPayload, gNetworkHeap);
   pVoiceRequest->init(xuid, mute);
   gEventDispatcher.send(cInvalidEventReceiverHandle, mVoiceEventHandle, cVoiceEventMutePlayer, session, controllerID, pVoiceRequest);
}

BLiveVoice* BLiveVoice::mpInstance = NULL;
uint BLiveVoice::mRefCount = 0;

//==============================================================================
// 
//==============================================================================
BLiveVoice* BLiveVoice::getInstance()
{
   if (mpInstance)
      return mpInstance;

   mpInstance = HEAP_NEW(BLiveVoice, gNetworkHeap);
   mpInstance->init();
   mpInstance->addRef();

   return mpInstance;
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::addRef()
{
   ++BLiveVoice::mRefCount;
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::release()
{
   if (BLiveVoice::mRefCount == 0 || --BLiveVoice::mRefCount == 0)
   {
      shutdown();
      HEAP_DELETE(BLiveVoice::mpInstance, gNetworkHeap);
      BLiveVoice::mpInstance = NULL;
      BLiveVoice::mRefCount = 0;
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::init()
{
   if (mpXHVEngine)
      return;

   HRESULT hr;

   // Initialize XHV
   XHV_INIT_PARAMS params = { 0 };

   // Set the maximum number of talkers
   params.dwMaxLocalTalkers  = cMaxLocalTalkers; // only supporting a maximum of 2 local players
   params.dwMaxRemoteTalkers = cMaxRemoteTalkers;

   // Set the processing modes
   XHV_PROCESSING_MODE LocalModes[]  = { XHV_LOOPBACK_MODE, XHV_VOICECHAT_MODE };
   XHV_PROCESSING_MODE RemoteModes[] = { XHV_VOICECHAT_MODE };

   params.localTalkerEnabledModes       = LocalModes;
   params.remoteTalkerEnabledModes      = RemoteModes;
   params.dwNumLocalTalkerEnabledModes  = ARRAYSIZE(LocalModes);
   params.dwNumRemoteTalkerEnabledModes = ARRAYSIZE(RemoteModes);

   HANDLE hWorkerThread = INVALID_HANDLE_VALUE;

   // Initialize XHV
   hr = XHVCreateEngine(&params, &hWorkerThread, &mpXHVEngine);

   if (FAILED(hr))
   {
      nlog(cMPVoiceCL, "**ERROR** BLiveVoice::init error %i trying to startup XHV engine", hr);
      BDEBUG_ASSERTM(SUCCEEDED(hr), "BLiveVoice::initialize - XHVCreateEngine failed");
      return;
   }

   // move the XHV to core 0 hw thread 1
   XSetThreadProcessor(hWorkerThread, 5);

   BEventReceiverHandle voiceEventHandle = gEventDispatcher.addClient(this, BVoice::cThreadIndexVoice);
   __lwsync();
   InterlockedExchange64((LONG64*)&mVoiceEventHandle, voiceEventHandle);

   mVoiceTimerSet = true;
   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mToHandle = mVoiceEventHandle;
   handleEvent.mEventClass = cVoiceEventTimer;
   gEventDispatcher.registerHandleWithEvent(mVoiceTimer.getHandle(), handleEvent);

   // create a notification listener so we can get profile/mute list changes
   mXNotifyHandle = XNotifyCreateListener(XNOTIFY_SYSTEM | XNOTIFY_FRIENDS);
   if (mXNotifyHandle != INVALID_HANDLE_VALUE)
   {
      handleEvent.clear();
      handleEvent.mToHandle = mVoiceEventHandle;
      handleEvent.mEventClass = cVoiceEventXNotify;
      gEventDispatcher.registerHandleWithEvent(mXNotifyHandle, handleEvent);
   }

   // update/flush our connections at 10Hz
   mVoiceTimer.set(mVoiceSampleInterval);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::shutdown()
{
   if (mVoiceTimerSet)
   {
      mVoiceTimer.cancel();
      mVoiceTimerSet = false;
      gEventDispatcher.deregisterHandle(mVoiceTimer.getHandle(), BVoice::cThreadIndexVoice);
   }

   if (mVoiceEventHandle != cInvalidEventReceiverHandle)
   {
      if (mXNotifyHandle != INVALID_HANDLE_VALUE)
         gEventDispatcher.deregisterHandle(mXNotifyHandle, BVoice::cThreadIndexVoice);

      gEventDispatcher.send(cInvalidEventReceiverHandle, mVoiceEventHandle, cVoiceEventDeinit, 0, 0, NULL, BEventDispatcher::cSendWaitForDelivery);

      gEventDispatcher.removeClientDeferred(mVoiceEventHandle, true);
   }

   mVoiceEventHandle = cInvalidEventReceiverHandle;

   if (mXNotifyHandle != INVALID_HANDLE_VALUE)
   {
      CloseHandle(mXNotifyHandle);
      mXNotifyHandle = INVALID_HANDLE_VALUE;
   }

   //if (mpXHVEngine)
   //{
   //   mpXHVEngine->Release();
   //   mpXHVEngine = NULL;
   //}
}

//==============================================================================
// 
//==============================================================================
bool BLiveVoice::submitVoice(BVoiceBuffer& data)
{
   if (mpXHVEngine == NULL)
      return true;

   BASSERT(data.mIndex < BVoiceBuffer::cBufSize);
   if (data.mIndex >= BVoiceBuffer::cBufSize)
      return true;

   DWORD sentSize = data.mSize - data.mIndex;
   if (sentSize == 0)
      return true;
   HRESULT hr = mpXHVEngine->SubmitIncomingChatData(data.mXuid, data.mBuf + data.mIndex, &sentSize);
   if (hr == S_FALSE && sentSize < (data.mSize - data.mIndex))
   {
      data.mIndex = data.mIndex + static_cast<uint16>(sentSize & MAXWORD);
      nlogt(cMPVoiceCL, "**WARNING** BLiveVoice::submitVoice failed[0x%08X] or output buffer full, queueing %d bytes", hr, (data.mSize - data.mIndex));
      return false;
   }

   return true;
}

//==============================================================================
// Called by the network/mp layer when it has a voice data packet ready to process
//==============================================================================
void BLiveVoice::processIncomingVoice(BVoiceBuffer& data)
{
   if (mpXHVEngine == NULL)
      return;

   if (mQueuedData.getSize() > 0)
   {
      BHandle hItem;
      BVoiceBuffer* pData = mQueuedData.getHead(hItem);
      while (pData)
      {
         if (submitVoice(*pData))
         {
            HEAP_DELETE(pData, gNetworkHeap);
            pData = mQueuedData.removeAndGetNext(hItem);
         }
         else
         {
            // queue up the given data and return
            BVoiceBuffer* pTemp = HEAP_NEW(BVoiceBuffer, gNetworkHeap);
            IGNORE_RETURN(Utils::FastMemCpy(pTemp, &data, sizeof(BVoiceBuffer)));
            mQueuedData.addToTail(pTemp);
            return;
         }
      }
   }

   // if I reach this point, my queue is either empty or I've cleared out my queue and can attempt at submitting the new buffer
   if (!submitVoice(data))
   {
      // queue up the given data and return
      BVoiceBuffer* pTemp = HEAP_NEW(BVoiceBuffer, gNetworkHeap);
      IGNORE_RETURN(Utils::FastMemCpy(pTemp, &data, sizeof(BVoiceBuffer)));
      mQueuedData.addToTail(pTemp);
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processVoiceQueue()
{
   if (mpXHVEngine == NULL)
      return;
   if (mQueuedData.getSize() == 0)
      return;

   BHandle hItem;
   BVoiceBuffer* pData = mQueuedData.getHead(hItem);
   while (pData)
   {
      if (!submitVoice(*pData))
         return;

      HEAP_DELETE(pData, gNetworkHeap);

      pData = mQueuedData.removeAndGetNext(hItem);
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::update()
{
   if (!mpXHVEngine)
      return;

   // Count how many bytes of data we have available
   WORD voiceBytes = 0;

   // Get the users that have voice data available
   DWORD voiceFlags = mpXHVEngine->GetDataReadyFlags();

   // Look for any new incoming data
   for (uint i=0; i < XUSER_MAX_COUNT; i++)
   {
      if (mHasVoice[i])
      {
         if (voiceFlags & (1 << i))
         {
            // Buffer the received voice data
            DWORD numPackets;
            DWORD bytes;

            bytes = BLiveVoice::cChatBufferSize - mLocalDataSize[i];

            if (mpXHVEngine->GetLocalChatData(i, mChatBuffer[i] + mLocalDataSize[i], &bytes, &numPackets) == S_OK)
               mLocalDataSize[i] += ((WORD)bytes) & MAXWORD;
         }

         // Keep a running count of voice bytes ready to go
         voiceBytes += mLocalDataSize[i] & MAXWORD;
      }
   }

   // or if any buffer is more than 70% full
   if (mSessionEventHandle != cInvalidEventReceiverHandle)
   {
      if (voiceBytes > 0)
      {
         for (uint i=0; i < XUSER_MAX_COUNT; i++)
         {
            if (mLocalDataSize[i])
            {
               BVoiceBuffer* pBuf = mAllocator.alloc();
               if (pBuf != NULL)
               {
                  pBuf->init(mChatBuffer[i], mLocalDataSize[i], mPortXuidMap[i]);
                  gEventDispatcher.send(cInvalidEventReceiverHandle, mSessionEventHandle, cNetEventVoiceBroadcast, (uint)pBuf);
               }
               mLocalDataSize[i] = 0;
            }
         }
      }
   }

   if (++mQueuedThrottle > 3)
   {
      mQueuedThrottle = 0;

      processVoiceQueue();
   }

   // determine talkers, but only every 5 updates (500 ms)
   if (++mTalkerThrottle > 5)
   {
      mTalkerThrottle = 0;

      checkHeadsets();

      queryProfiles();

      // dump all the settings + timestamp
      //uint now = timeGetTime();
      //for (uint i=0; i < BVoice::cMaxSessions; ++i)
      //{
      //   BVoiceSession& session = mSessions[i];
      //   nlogt(cMPVoiceCL, "UPDATE -- now[%u] sessionID[%d] eventHandle[%I64u] playerCount[%d]", now, i, session.mEventHandle, session.mPlayers.getSize());
      //   for (int j=session.mPlayers.getSize()-1; j >= 0; --j)
      //   {
      //      BVoicePlayer& player = session.mPlayers[j];
      //      nlogt(cMPVoiceCL, "          xuid[%I64u] controllerID[%d] clientID[%d] session[%d] channel[%d] headset[%d] local[%d] muted[%d]",
      //         player.mXuid, player.mControllerID, player.mClientID, player.mSessionID, player.mChannelCode, player.mHeadset, player.mIsLocal, player.mIsMuted);
      //      for (uint k=0; k < XNetwork::cMaxClients; ++k)
      //      {
      //         nlogt(cMPVoiceCL, "             talkerList[%d] == %I64u", k, player.mTalkerList[k]);
      //      }
      //      for (uint k=0; k < XUSER_MAX_COUNT; ++k)
      //      {
      //         nlogt(cMPVoiceCL, "             sessionMute[%d] == %d", k, player.mSessionMute[k]);
      //      }
      //   }
      //}
   }

   for (uint i=0; i < cMaxClients; ++i)
   {
      BOOL isTalking = FALSE;

//-- FIXING PREFIX BUG ID 4153
      const BVoicePlayerInternal& player = mPlayers[i];
//--

      if (player.mXuid != 0)
      {
         if (player.mIsLocal)
         {
            if (player.mControllerID < XUSER_MAX_COUNT)
               isTalking = mpXHVEngine->IsLocalTalking(player.mControllerID);
         }
         else
            isTalking = mpXHVEngine->IsRemoteTalking(player.mXuid);

         //setTalking(mSessionIndex, player.mClientID, isTalking);
         setTalking(mSessionIndex, player.getXuid(), isTalking);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::mutePlayer(uint localControllerID, XUID xuid, bool mute)
{
   if (!mpXHVEngine)
      return;

   BASSERT(xuid);

   if (mute)
   {
      mpXHVEngine->SetPlaybackPriority(xuid, localControllerID, XHV_PLAYBACK_PRIORITY_NEVER);
      nlogt(cMPVoiceCL, "BLiveVoice::mutePlayer - Setting xuid[%I64u] to MUTED in the XHVEngine", xuid);
   }
   else
   {
      mpXHVEngine->SetPlaybackPriority(xuid, localControllerID, XHV_PLAYBACK_PRIORITY_MAX);
      nlogt(cMPVoiceCL, "BLiveVoice::mutePlayer - Setting xuid[%I64u] as clear to talk in the XHVEngine", xuid);
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::checkHeadsets()
{
   for (uint i=0; i < XUSER_MAX_COUNT; ++i)
   {
      if (mHasVoice[i])
      {
         BOOL headset = mpXHVEngine->IsHeadsetPresent(i);
         setHeadset(i, headset);
      }
      else
      {
         setHeadset(i, FALSE);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::addLocal(BVoicePlayerInternal& player)
{
   if (!mpXHVEngine)
      return;

   HRESULT hr;

   BASSERT(player.getXuid());

   if (player.getControllerID() >= XUSER_MAX_COUNT)
      return;

   if (!mHasVoice[player.getControllerID()])
   {
      hr = mpXHVEngine->RegisterLocalTalker(player.getControllerID()); 
      if (FAILED(hr))
      {
         nlogt(cMPVoiceCL, "**ERROR** BLiveVoice::addLocal could not register local player %ud, error 0x%08x", player.getControllerID(), hr);
         BDEBUG_ASSERTM(SUCCEEDED(hr), "BLiveVoice::addLocal - could not register local player");
         return;
      }

      player.setXHV(true);

      hr = mpXHVEngine->StartLocalProcessingModes(player.getControllerID(), &XHV_VOICECHAT_MODE, 1);

      if (SUCCEEDED(hr))
      {
         hr = mpXHVEngine->StopLocalProcessingModes(player.getControllerID(), &XHV_LOOPBACK_MODE, 1);
      }

      if (FAILED(hr))
      {
         nlogt(cMPVoiceCL, "**ERROR** BLiveVoice::addLocal could not register local player %ud, error 0x%08x", player.getControllerID(), hr);
         BDEBUG_ASSERTM(SUCCEEDED(hr), "BLiveVoice::addLocal - could not register local player");
         return;
      }

      __lwsync();
      InterlockedExchange64((LONG64*)&mPortXuidMap[player.getControllerID()], player.getXuid());

      mHasVoice[player.getControllerID()] = TRUE;

      mLocalDataSize[player.getControllerID()] = 0;

      mProfileDirty[player.getControllerID()] = TRUE;

      // default to having speakers enabled and allow us to determine the truth later
      mSpeakersEnabled[player.getControllerID()] = TRUE;
      // default the profile mute status to true, we'll reverse it later if that's not the case
      Sync::InterlockedExchangeExport(&mProfileMute[player.getControllerID()], 1);

      checkHeadsets();

      queryProfiles();
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::removeLocal(BVoicePlayerInternal& player)
{
   if (!mpXHVEngine)
      return;

   if (player.getControllerID() >= XUSER_MAX_COUNT)
      return;

   if (mHasVoice[player.getControllerID()])
   {
      HRESULT hr = mpXHVEngine->UnregisterLocalTalker(player.getControllerID());
      if (hr != S_OK)
      {
         nlogt(cMPVoiceCL, "**ERROR** BLiveVoice::removeLocal error %i calling UnregisterLocalTalker", hr);
         BDEBUG_ASSERTM((hr == S_OK), "BLiveVoice::removeLocal - UnregisterLocalTalker failed");
      }

      player.setXHV(false);

      __lwsync();
      InterlockedExchange64((LONG64*)&mPortXuidMap[player.getControllerID()], 0);

      mHasVoice[player.getControllerID()] = FALSE;

      setHeadset(player.getControllerID(), FALSE);

      for (uint i=0; i < BVoice::cMaxSessions; i++)
      {
         for (uint k=0; k < cMaxClients; k++)
         {
            Sync::InterlockedExchangeExport(&mMuteList[i][player.getControllerID()][k], 0);
         }
      }

      // need to cancel any outstanding overlapped operations
      mProfileDirty[player.getControllerID()] = FALSE;
      if (mReadProfileRequest[player.getControllerID()])
      {
         mReadProfileRequest[player.getControllerID()]->cancel();
         HEAP_DELETE(mReadProfileRequest[player.getControllerID()], gNetworkHeap);
         mReadProfileRequest[player.getControllerID()] = NULL;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::addRemote(BVoicePlayerInternal& player)
{
   if (!mpXHVEngine)
      return;

   HRESULT hr;

   BASSERT(player.getXuid());

   hr = mpXHVEngine->RegisterRemoteTalker(player.getXuid(), NULL, NULL, NULL);
   if (hr != S_OK)
   {
      nlogt(cMPVoiceCL, "**ERROR** BLiveVoice::addRemote error %i calling RegisterRemoteTalker", hr);
      BDEBUG_ASSERTM((hr == S_OK), "BLiveVoice::addRemote - RegisterRemoteTalker failed");
      return;
   }

   player.setXHV(true);

   hr = mpXHVEngine->StartRemoteProcessingModes(player.getXuid(), &XHV_VOICECHAT_MODE, 1);
   if (hr != S_OK)
   {
      nlogt(cMPVoiceCL, "**ERROR** BLiveVoice::addRemote error %i calling StartRemoteProcessingModes for xuid[%I64u]", hr, player.getXuid());
      BDEBUG_ASSERTM((hr == S_OK), "BLiveVoice::addRemote - StartRemoteProcessingModes failed");

      hr = mpXHVEngine->UnregisterRemoteTalker(player.getXuid());
      if (hr != S_OK)
      {
         //Should never hit this
         nlogt(cMPVoiceCL, "**ERROR** BLiveVoice::addRemote error %i trying to cleanup with unreg for xuid[%I64u]", hr, player.getXuid());
         BDEBUG_ASSERTM((hr == S_OK), "BLiveVoice::addRemote - UnregisterRemoteTalker failed");
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::removeRemote(BVoicePlayerInternal& player)
{
   if (!mpXHVEngine)
      return;

   setHeadset(player.getXuid(), FALSE);

   // First make sure that we have this xuid already registered
   //    for the given clientID
   HRESULT hr = mpXHVEngine->UnregisterRemoteTalker(player.getXuid());

   player.setXHV(false);

   if (FAILED(hr))
   {
      nlogt(cMPVoiceCL, "**ERROR** BLiveVoice::removeRemote error %i trying to cleanup with unreg", hr);
      BDEBUG_ASSERTM((hr == S_OK), "BLiveVoice::removeRemote - UnregisterRemoteTalker failed");
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setTalking(uint session, XUID xuid, BOOL talking)
{
   if (session >= BVoice::cMaxSessions)
      return;

   BVoicePlayer* pPlayer = mSessions[session].getPlayer(xuid);
   if (pPlayer != NULL && pPlayer->getClientID() < cMaxClients)
   {
      if (mIsTalkingInternal[(session * cMaxClients) + pPlayer->getClientID()] != talking)
      {
         mIsTalkingInternal[(session * cMaxClients) + pPlayer->getClientID()] = talking;
         Sync::InterlockedExchangeExport(&mIsTalking[(session * cMaxClients) + pPlayer->getClientID()], talking);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setHeadset(uint controllerID, BOOL headset)
{
   if (controllerID >= XUSER_MAX_COUNT)
      return;

   if (mHeadsetPresentInternal[controllerID] != headset)
   {
      mHeadsetPresentInternal[controllerID] = headset;

      // we're indexed by clientID for the sim thread facing headset checks
      for (uint i=0; i < cMaxClients; ++i)
      {
         BVoicePlayerInternal& player = mPlayers[i];
         if (player.mControllerID == controllerID)
         {
            setHeadset(player, headset);
            break;
         }
      }

      if (headset)
      {
         // kick off an async read profile request
         if (!mReadProfileRequest[controllerID])
         {
            mProfileDirty[controllerID] = FALSE;
            mReadProfileRequest[controllerID] = HEAP_NEW(BReadProfileRequest, gNetworkHeap);
            mReadProfileRequest[controllerID]->init(controllerID);
         }
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setHeadset(XUID xuid, BOOL headset)
{
   for (uint i=0; i < cMaxClients; ++i)
   {
      BVoicePlayerInternal& player = mPlayers[i];
      if (player.mXuid == xuid)
      {
         setHeadset(player, headset);
         break;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::addClient(const BVoicePlayer& player)
{
   // the voice session is requesting us to add this player to the XHV
   // we need to determine whether the player is already registered
   // if so, increment the ref count and we're done
   // if not, add the player to our master talker list

   BASSERTM(player.mClientID < cMaxClients, "Invalid client ID");
   if (player.mClientID >= cMaxClients)
      return;

   BVoicePlayerInternal* pInternalPlayer = NULL;

   for (uint i=0; i < cMaxClients; ++i)
   {
      BVoicePlayerInternal& p = mPlayers[i];
      if (p.mXuid == player.mXuid)
      {
         p.mRefCount += 1;
         pInternalPlayer = &p;
         break;
      }
   }

   // add this player to the next available slot 
   if (pInternalPlayer == NULL)
   {
      for (uint i=0; i < cMaxClients; ++i)
      {
         if (mPlayers[i].getXuid() == 0)
         {
            mPlayers[i] = player;
            pInternalPlayer = &mPlayers[i];
            break;
         }
      }

      BASSERTM(pInternalPlayer, "Failed to find an available voice slot for player");
      if (pInternalPlayer == NULL)
         nlogt(cMPVoiceCL, "Failed to find an available voice slot for xuid[%I64u] session[%d] clientID[%d]", player.mXuid, player.mSessionID, player.mClientID);
   }

   if (pInternalPlayer == NULL)
      return;

   // set us up as a potential talker
   //setTalking(player.getSessionID(), player.mClientID, FALSE);
   setTalking(player.getSessionID(), player.getXuid(), FALSE);

   if (!pInternalPlayer->inXHV())
   {
      if (player.mIsLocal)
         addLocal(*pInternalPlayer);
      else
         addRemote(*pInternalPlayer);
   }

   // I should go through my players list and mute this new player for all local players
   //
   updateTalkerList();
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::removeClient(uint session, XUID xuid, bool shutdown)
{
   for (uint i=0; i < cMaxClients; ++i)
   {
      BVoicePlayerInternal& player = mPlayers[i];
      if (player.mXuid == xuid)
      {
         player.mRefCount -= 1;

         //setTalking(session, player.mClientID, FALSE);
         setTalking(session, player.getXuid(), FALSE);

         if (player.mRefCount <= 0)
         {
            // time to remove this player as a talker
            if (player.mIsLocal)
               removeLocal(player);
            else
               removeRemote(player);

            mPlayers[i].reset();

            if (!shutdown)
               updateTalkerList();
         }
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::getHeadset(XUID xuid, BOOL& headset)
{
   for (uint i=0; i < cMaxClients; ++i)
   {
      BVoicePlayerInternal& player = mPlayers[i];
      if (player.mXuid == xuid)
      {
         headset = player.mHeadset;
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setHeadset(BVoicePlayerInternal& player, BOOL headset)
{
   if (player.mHeadset != headset)
   {
      player.mHeadset = headset;
      //if (player.mClientID >= cMaxClients)
      //   return;

      // mark the headset value for all sessions
      for (uint i=0; i < BVoice::cMaxSessions; ++i)
      {
         BVoicePlayer* pPlayer = mSessions[i].getPlayer(player.getXuid());
         if (pPlayer != NULL)
         {
            pPlayer->setHeadset(headset);
            if (pPlayer->getClientID() < XNetwork::cMaxClients)
            {
               Sync::InterlockedExchangeExport(&mHeadsetPresent[(i * cMaxClients) + pPlayer->getClientID()], headset);

               if (mSessions[i].getEventHandle() != cInvalidEventReceiverHandle && pPlayer->isLocal())
                  gEventDispatcher.send(cInvalidEventReceiverHandle, mSessions[i].getEventHandle(), cNetEventVoiceHeadsetPresent, pPlayer->getClientID(), headset);
            }
         }
      }

      // tell everyone else about this client's headset
      //if (mSessionEventHandle != cInvalidEventReceiverHandle && player.mIsLocal)
      //   gEventDispatcher.send(cInvalidEventReceiverHandle, mSessionEventHandle, cNetEventVoiceHeadsetPresent, player.mClientID, headset);
   }
}

//==============================================================================
//
//==============================================================================
bool BLiveVoice::isFriend(uint localControllerID, XUID xuid) const
{
   BOOL result = FALSE;

   if (localControllerID >= XUSER_MAX_COUNT)
      return false;

   uint rc = XUserAreUsersFriends(localControllerID, &xuid, 1, &result, NULL);
   if (rc == ERROR_NO_SUCH_USER)
      return true;
   if (rc == ERROR_SUCCESS && result)
   {
      // they're our friend, let's verify the headset/speaker settings
      if (mHeadsetPresentInternal[localControllerID] && !mSpeakersEnabled[localControllerID])
         return true;
   }

   // ok, so either the XUserAreUsersFriends call failed
   // or they're not our friend
   // or they have voice routing out their speakers
   //
   // now we need to make sure that everyone else signed-in locally is a friend of this user
   // 

   //Go through all controllers return true ONLY if the target XUID is friends with EVERYONE who has an active profile
   for (uint i=0; i < XUSER_MAX_COUNT; i++)
   {
      BOOL result = FALSE;
      if ((localControllerID != i) && // if this controller is not me
         (XUserGetSigninState(i) != eXUserSigninState_NotSignedIn) && // and a person is signed-in
         ((XUserAreUsersFriends(i, &xuid, 1, &result, NULL) != ERROR_SUCCESS) || !result)) // and they're not our friend
      {
         return false;
      }
   }

   return true;
}

//==============================================================================
//
//==============================================================================
bool BLiveVoice::checkPrivilege(uint localControllerID, XPRIVILEGE_TYPE priv) const
{
   BOOL result = FALSE;

   //We need to get the most restrictive privileges for the following bits:
   // XPRIVILEGE_COMMUNICATIONS = 252,
   // XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY = 251,
   if ((priv == XPRIVILEGE_COMMUNICATIONS) || 
       (priv == XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY))
   {
      uint rc = XUserCheckPrivilege(XUSER_INDEX_ANY, priv, &result);
      if (rc == ERROR_NO_SUCH_USER || rc == ERROR_NOT_LOGGED_ON)
         return true;
      return (rc == ERROR_SUCCESS) && result;
   }

   if (localControllerID >= XUSER_MAX_COUNT)
      return false;

   return (XUserCheckPrivilege(localControllerID, priv, &result) == ERROR_SUCCESS) && result;
}

//==============================================================================
// 
//==============================================================================
bool BLiveVoice::isMuted(uint localControllerID, XUID xuid, BOOL& tcr90Mute) const
{
   BOOL result = TRUE;

   if (localControllerID >= XUSER_MAX_COUNT)
      return true;

   uint rc = XUserMuteListQuery(localControllerID, xuid, &result);
   if (rc == ERROR_NOT_LOGGED_ON)
   {
      tcr90Mute = FALSE;
      return false;
   }

   if (rc != ERROR_SUCCESS || result)
   {
      // the call failed or they're on our mute list, mute them
      tcr90Mute = FALSE;
      return true;
   }

   // otherwise they're not on our mute list

   // if they're only using a headset and not routing through the speakers, they're not muted
   if (mHeadsetPresentInternal[localControllerID] && !mSpeakersEnabled[localControllerID])
   {
      tcr90Mute = FALSE;
      return false;
   }

   // voice is being routed out through the speakers, now we need to verify that
   // nobody else has muted this user

   for (uint i=0; i < XUSER_MAX_COUNT; ++i)
   {
      BOOL onMuteList = TRUE;
      if ((localControllerID != i) &&
         (XUserGetSigninState(i) != eXUserSigninState_NotSignedIn) && // a user is signed-in
         ((XUserMuteListQuery(i, xuid, &onMuteList) != ERROR_SUCCESS) || onMuteList)) // and they're on a mute list, somewhere
      {
         return true;
      }
   }

   tcr90Mute = FALSE;

   return false;
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::updateTalkerList()
{
   if (mSessionIndex < BVoice::cMaxSessions)
   {
      BVoiceSession& session = mSessions[mSessionIndex];
      session.updateTalkerList();
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::updateTalkerList(BVoiceSession& session, BVoicePlayer& localPlayer)
{
   if (session.mEventHandle == cInvalidEventReceiverHandle)
      return;

   nlogt(cMPVoiceCL, "BLiveVoice::updateTalkerList -- current[%d:%I64u] given[%d:%I64u] localPlayer[%I64u]", mSessionIndex, mSessionEventHandle, session.mSessionID, session.mEventHandle, localPlayer.getXuid());

   // determine if this session is our primary session
   bool primarySession = (session.mEventHandle == mSessionEventHandle);

   // if this is the primary session, meaning the one with audio focus, i.e. party or game
   // then I need to mute all the current XHV participants that are not members of the session
   //
   // once we have all non-participants muted, I can determine the mute list for players in this session
   // and then mute/unmute those depending on the various settings on the console and whether that
   // user has us muted
   // 
   if (primarySession)
   {
      for (uint i=0; i < cMaxClients; ++i)
      {
//-- FIXING PREFIX BUG ID 4156
         const BVoicePlayerInternal& player = mPlayers[i];
//--
         if (player.mXuid != 0)
         {
            for (int j=session.mPlayers.getSize()-1; j >= 0; --j)
            {
//-- FIXING PREFIX BUG ID 4155
               const BVoicePlayer& sessionPlayer = session.mPlayers[j];
//--
               if (player.mXuid == sessionPlayer.mXuid)
                  break;
            }
            if (j < 0)
            {
               // player not found in the session, mute it
               nlogt(cMPVoiceCL, "BLiveVoice::updateTalkerList -- muting player not found in primary session xuid[%I64u]", player.mXuid);
               mutePlayer(localPlayer.mControllerID, player.mXuid, true);
            }
         }
      }
   }

   XUID* pCurrentTalkerList = &localPlayer.mTalkerList[0];

   // check the privileges of the user
   //
   // if they are not authorized for communication then we need to mute everybody
   bool comm = checkPrivilege(localPlayer.mControllerID, XPRIVILEGE_COMMUNICATIONS);

   // if they are only authorized to communicate with their friends, then we need to check if the client is a friend
   bool friendsOnly = (!comm && checkPrivilege(localPlayer.mControllerID, XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY));

   bool changed = false;
   uint talkerIndex = 0;

   XUID talkerList[cMaxClients];

   // first let's clear the talker list and start over
   IGNORE_RETURN(Utils::FastMemSet(talkerList, 0, sizeof(talkerList)));

   // run through all the players and match up their talker lists
   // against mine
   for (int i=session.mPlayers.getSize()-1; i >= 0; --i)
   {
      BVoicePlayer& player = session.mPlayers[i];

      // skip ourselves
      if (player.mXuid == localPlayer.mXuid)
         continue;

      if (player.mClientID >= cMaxClients)
         continue;

      // reset the mute status of this client
      bool mute = true;
      BOOL tcr90Mute = TRUE; // assume we're muted because of TCR #90
      BOOL sessionMute = FALSE;
      XUID clientXuid = player.mXuid;

      // verify the comm privilege
      //
      // XXX isFriend will be false if the clientXuid == mLocalXuid
      // this is ok for our situation since we don't want a loopback voice afaik,
      // if that changes, we need to redo our checks here to show that when
      // clientXuid==mLocalXuid, you're a friend
      if (comm || (friendsOnly && isFriend(localPlayer.mControllerID, clientXuid)))
      {
         // * if nobody has muted this xuid
         // * and we're in the same channel
         // then we can check against their talker list to insure that perhaps they have muted us
         if (localPlayer.mChannelCode == player.mChannelCode && !isMuted(localPlayer.mControllerID, clientXuid, tcr90Mute))
         {
            // we're definitely not muting them because of TCR #90
            // of course, they may still have us muted, but that's different
            // this is for local checks only
            tcr90Mute = FALSE;

            long i;
            for (i=0; !changed && i < cMaxClients; ++i)
            {
               // I consider this client a talker, so let's check our list to make sure they're on it
               if (pCurrentTalkerList[i] == clientXuid)
                  break;
            }
            // I do not yet know about this talker
            if (i == cMaxClients)
               changed = true;

            // else I already know about this talker and have sent an update in the past
            //
            // so add them to our temporary talkerList but there's no need to broadcast a new update

            talkerList[talkerIndex++] = clientXuid;

            // if we successfully queried the mute list for the given client and they are not muted
            // then we might be able to allow them as a talker
            //
            // retrieve the talkerlist for this client/player and if they're muting us, then don't allow them as a talker
//-- FIXING PREFIX BUG ID 4157
            const XUID* pClientTalkerList = player.mTalkerList;
//--
            for (i=0; i < cMaxClients; ++i)
            {
               // the client has us on their talker list, so we can un-mute them
               if (pClientTalkerList[i] == localPlayer.mXuid)
               {
                  mute = false;
                  break;
               }
            }
         }
      }

      // if this player got the session mute smackdown then we override everything
      if (player.mSessionMute[localPlayer.mControllerID])
      {
         sessionMute = TRUE;
         mute = true;
      }

      nlogt(cMPVoiceCL, "BLiveVoice::updateTalkerList -- clientXuid[%I64u] sessionMute[%d] mute[%d] tcr90Mute[%d]", clientXuid, sessionMute, mute, tcr90Mute);

      //// check if we should mute this player
      //if (player.mIsMuted != mute)
      //{
         // we're muting this player for the session
         player.mIsMuted = mute;

         // if our session is the current one, then we need to inform XHV
         if (primarySession)
            mutePlayer(localPlayer.mControllerID, player.mXuid, mute);

         // also update our mutelist
         Sync::InterlockedExchangeExport(&mMuteList[session.mSessionID][localPlayer.mControllerID][player.mClientID], (tcr90Mute << 16) | sessionMute);
      //}

      // send mute event to our session event handler and let them mute the actual BClient so we stop/start sending/receiving voice packets
      BVoiceMuteRequestPayload* pRequest = HEAP_NEW(BVoiceMuteRequestPayload, gNetworkHeap);
      pRequest->init(player.mAddr, player.mIsMuted);
      gEventDispatcher.send(cInvalidEventReceiverHandle, session.mEventHandle, cNetEventVoiceMuteClient, 0, 0, pRequest);
   }

   // update our current talker list
   IGNORE_RETURN(Utils::FastMemCpy(pCurrentTalkerList, talkerList, sizeof(XUID)*cMaxClients));

   // if I've changed my mute list, send an update to all clients in the session
   if (changed)
   {
      nlogt(cMPVoiceCL, "BLiveVoice::updateTalkerList -- sending updated talker list to everyone");
      BVoiceTalkerListPayload* pPayload = HEAP_NEW(BVoiceTalkerListPayload, gNetworkHeap);
      BDEBUG_ASSERT(pPayload);

      IGNORE_RETURN(Utils::FastMemCpy(pPayload->mXuids, pCurrentTalkerList, sizeof(XUID)*cMaxClients));
      pPayload->mOwnerXuid = localPlayer.mXuid;
      pPayload->mCount = talkerIndex;

      gEventDispatcher.send(cInvalidEventReceiverHandle, session.mEventHandle, cNetEventVoiceTalkerList, 0, 0, pPayload);
   }
   else
   {
      nlogt(cMPVoiceCL, "BLiveVoice::updateTalkerList -- Nothing sent out to peers");
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processDeinit(const BEvent& event)
{
   // shutdown all the sessions
   for (uint i=0; i < BVoice::cMaxSessions; ++i)
   {
      BVoiceSession& session = mSessions[i];
      session.deinit();
   }

   BHandle hItem;
   BVoiceBuffer* pData = mQueuedData.getHead(hItem);
   while (pData)
   {
      HEAP_DELETE(pData, gNetworkHeap);

      pData = mQueuedData.getNext(hItem);
   }
   mQueuedData.reset();

   //if (mXNotifyHandle != INVALID_HANDLE_VALUE)
   //{
   //   CloseHandle(mXNotifyHandle);
   //   mXNotifyHandle = INVALID_HANDLE_VALUE;
   //}
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processInitSession(const BEvent& event)
{
   BDEBUG_ASSERTM(event.mFromHandle != cInvalidEventReceiverHandle, "Missing a from handle, unable to complete voice session registration");
   if (event.mFromHandle == cInvalidEventReceiverHandle)
      return;

   BDEBUG_ASSERTM(event.mPrivateData < BVoice::cMaxSessions, "Missing a session ID, unable to init voice session");
   if (event.mPrivateData >= BVoice::cMaxSessions)
      return;

   nlogt(cMPVoiceCL, "BLiveVoice::processInitSession -- sessionID[%d] switch[%d]", event.mPrivateData, event.mPrivateData2);

   BVoiceSession& session = mSessions[event.mPrivateData];
   session.init(this, event.mFromHandle, event.mPrivateData);

   // also immediately switch to the new session
   if (event.mPrivateData2)
   {
      mSessionEventHandle = session.mEventHandle;
      mSessionIndex = event.mPrivateData;

      session.updateTalkerList();
   }
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::init(BLiveVoice* pVoice, BEventReceiverHandle eventHandle, uint sessionId)
{
   mpVoice = pVoice;
   mEventHandle = eventHandle;
   mSessionID = sessionId;
   mHeadsetCache = 0;
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::deinit()
{
   if (mpVoice == NULL)
      return;

   nlogt(cMPVoiceCL, "BVoiceSession::deinit -- sessionID[%d]", mSessionID);

   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      mpVoice->removeClient(mSessionID, mPlayers[i].mXuid, true);
   }

   mPlayers.clear();
   mpVoice = NULL;
   mEventHandle = cInvalidEventReceiverHandle;
   mSessionID = BVoice::cMaxSessions;

   for (uint i=0; i < mTalkerListQueue.getSize(); ++i)
   {
      BVoiceTalkerListPayload* pList = mTalkerListQueue[i];
      if (pList)
         HEAP_DELETE(pList, gNetworkHeap);
   }
   mTalkerListQueue.clear();
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::addClient(uint clientID, uint controllerID, XUID xuid, const SOCKADDR_IN& addr)
{
   if (mpVoice == NULL)
      return;

   BASSERTM(clientID < XNetwork::cMaxClients, "Invalid client ID");
   if (clientID >= XNetwork::cMaxClients)
      return;

   // first insure that we aren't already registered
   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mXuid == xuid)
         return;
   }

   nlogt(cMPVoiceCL, "BVoiceSession::addClient -- clientID[%d] xuid[%I64u]", clientID, xuid);

   // add us
   BVoicePlayer player;
   player.init(clientID, controllerID, xuid, addr, mSessionID);

   uint index = mPlayers.add(player);

   mpVoice->addClient(mPlayers[index]);

   for (uint i=0; i < mTalkerListQueue.getSize(); ++i)
   {
      BVoiceTalkerListPayload* pList = mTalkerListQueue[i];
      if (pList == NULL)
      {
         mTalkerListQueue.removeIndex(i);
         i--;
      }
      else if (pList->mOwnerXuid == xuid)
      {
         updateTalkers(*pList);

         mTalkerListQueue.removeIndex(i);
         i--;
      }
   }

   // first query for the existing headset status by XUID from mpVoice
   mpVoice->getHeadset(xuid, player.mHeadset);
   if (player.mHeadset)
      mHeadsetCache |= (TRUE << clientID);

   // broadcast the current state of our headsets
   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      BVoicePlayer& p = mPlayers[i];
      if (p.isLocal() && p.getClientID() < XNetwork::cMaxClients && mEventHandle != cInvalidEventReceiverHandle)
         gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cNetEventVoiceHeadsetPresent, p.getClientID(), p.mHeadset);
   }

   // check our headset cache and update accordingly
   if (mHeadsetCache >> clientID & 0x1)
      updateHeadset(clientID, TRUE);
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::removeClient(XUID xuid)
{
   if (mpVoice == NULL)
      return;

   nlogt(cMPVoiceCL, "BVoiceSession::removeClient -- sessionID[%d] xuid[%I64u]", mSessionID, xuid);

   bool found = false;

   // first insure that we aren't already registered
   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mXuid == xuid)
      {
         found = true;
         mPlayers.removeIndex(i);
         break;
      }
   }

   if (found)
      mpVoice->removeClient(mSessionID, xuid);

   for (int i=mTalkerListQueue.getSize()-1; i >= 0; --i)
   {
      BVoiceTalkerListPayload* pList = mTalkerListQueue[i];
      if (pList)
         HEAP_DELETE(pList, gNetworkHeap);

      mTalkerListQueue.removeIndex(i, false);
   }
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::updateTalkers(const BVoiceTalkerListPayload& request)
{
   if (mpVoice == NULL)
      return;

   nlogt(cMPVoiceCL, "BVoiceSession::updateTalkers -- sessionID[%d] requestOwner[%I64u]", mSessionID, request.mOwnerXuid);
   for (int i=mPlayers.getSize()-1; i >= 0; --i)
      nlogt(cMPVoiceCL, "                                xuid[%I64u]", mPlayers[i].mXuid);

   bool found = false;
   for (uint i=0; !found && i < mPlayers.getSize(); ++i)
   {
      if (mPlayers[i].mXuid == request.mOwnerXuid)
         found = true;
   }

   if (!found)
   {
      // queue up the talker packet
      BVoiceTalkerListPayload* pList = HEAP_NEW(BVoiceTalkerListPayload, gNetworkHeap);

      Utils::FastMemCpy(pList->mXuids, request.mXuids, sizeof(XUID)*BTalkersPacket::cMaxClients);
      pList->mOwnerXuid = request.mOwnerXuid;
      pList->mCount = request.mCount;

      mTalkerListQueue.add(pList);
      return;
   }

   bool updateTalkers = false;

   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mXuid == request.mOwnerXuid)
      {
         uint count = Math::Min<uint>(XNetwork::cMaxClients, request.mCount);

         uint currentCount = 0;
         for (uint j=0; j < XNetwork::cMaxClients; ++j)
         {
            if (mPlayers[i].mTalkerList[j] != 0)
               currentCount++;
         }

         if (currentCount != request.mCount)
         {
            IGNORE_RETURN(Utils::FastMemSet(mPlayers[i].mTalkerList, 0, sizeof(XUID)*XNetwork::cMaxClients));
            IGNORE_RETURN(Utils::FastMemCpy(mPlayers[i].mTalkerList, request.mXuids, sizeof(XUID)*count));

            updateTalkers = true;
         }
         else
         {
            // they may be identical, verify that each xuid in our current list exists in the given list
            for (uint j=0; j < XNetwork::cMaxClients && j < request.mCount; ++j)
            {
               bool found = false;
               if (request.mXuids[j] != 0)
               {
                  for (uint k=0; k < XNetwork::cMaxClients; ++k)
                  {
                     if (mPlayers[i].mTalkerList[k] == request.mXuids[j])
                     {
                        found = true;
                        break;
                     }
                  }

                  if (!found)
                  {
                     IGNORE_RETURN(Utils::FastMemSet(mPlayers[i].mTalkerList, 0, sizeof(XUID)*XNetwork::cMaxClients));
                     IGNORE_RETURN(Utils::FastMemCpy(mPlayers[i].mTalkerList, request.mXuids, sizeof(XUID)*count));

                     updateTalkers = true;
                     break;
                  }
               }
            }
         }

         break;
      }
   }

   if (updateTalkers)
      updateTalkerList();
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::updateHeadset(uint clientID, BOOL headset)
{
   if (mpVoice == NULL)
      return;

   mHeadsetCache |= (headset << clientID);

   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mClientID == clientID)
      {
         mPlayers[i].mHeadset = headset;
         mpVoice->setHeadset(mPlayers[i].mXuid, headset);
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::setChannel(XUID xuid, uint channel)
{
   if (mpVoice == NULL)
      return;

   bool updateTalkers = false;

   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mXuid == xuid)
      {
         mPlayers[i].mChannelCode = channel;
         updateTalkers = true;
         break;
      }
   }

   if (updateTalkers)
      updateTalkerList();
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::setChannel(uint channel)
{
   if (mpVoice == NULL)
      return;

   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      mPlayers[i].mChannelCode = channel;
   }

   updateTalkerList();
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::updateTalkerList()
{
   if (mpVoice == NULL)
      return;

   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      nlogt(cMPVoiceCL, "BVoiceSession::updateTalkerList -- session[%d] channel[%d] controllerID[%d] xuid[%I64u] local[%d]",
         mPlayers[i].getSessionID(), mPlayers[i].mChannelCode, mPlayers[i].mControllerID, mPlayers[i].getXuid(), mPlayers[i].isLocal());

      if (mPlayers[i].mIsLocal && mPlayers[i].mControllerID < XUSER_MAX_COUNT)
         mpVoice->updateTalkerList(*this, mPlayers[i]);
   }
}

//==============================================================================
// 
//==============================================================================
void BVoiceSession::mutePlayer(uint controllerID, XUID xuid, BOOL mute)
{
   if (mpVoice == NULL)
      return;

   if (controllerID >= XUSER_MAX_COUNT)
      return;

   bool updateTalkers = false;

   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mXuid == xuid)
      {
         mPlayers[i].mSessionMute[controllerID] = mute;
         updateTalkers = true;
         break;
      }
   }

   if (updateTalkers)
      updateTalkerList();
}

//==============================================================================
// 
//==============================================================================
bool BVoiceSession::isValidXUID(XUID xuid) const
{
   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mXuid == xuid)
         return true;
   }
   return false;
}

//==============================================================================
// 
//==============================================================================
bool BVoiceSession::isHeadsetPresent(XUID xuid) const
{
   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mXuid == xuid)
         return mPlayers[i].isHeadsetPresent();
   }
   return false;
}

//==============================================================================
// 
//==============================================================================
BVoicePlayer* BVoiceSession::getPlayer(XUID xuid)
{
   for (int i=mPlayers.getSize()-1; i >= 0; --i)
   {
      if (mPlayers[i].mXuid == xuid)
         return &mPlayers[i];
   }
   return NULL;
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setSessionInternal(uint session)
{
   if (session >= BVoice::cMaxSessions)
      return;

   BVoiceSession& s = mSessions[session];

   nlogt(cMPVoiceCL, "BLiveVoice::setSessionInternal -- session[%d] eventHandle[%I64u] oldSession[%d] oldEventHandle[%I64u]", session, s.mEventHandle, mSessionIndex, mSessionEventHandle);

   mSessionEventHandle = s.mEventHandle;
   mSessionIndex = session;

   // if this session is not initialized, then don't bother updating the talker list
   if (mSessionEventHandle == cInvalidEventReceiverHandle)
      return;

   // reset all the headset information so our headset check will prompt everyone in the session
   // forcing their internal caches to reset to the correct value
   for (uint i=0; i < cMaxClients; ++i)
      mPlayers[i].mHeadset = FALSE;

   IGNORE_RETURN(Utils::FastMemSet(mHeadsetPresentInternal, 0, sizeof(mHeadsetPresentInternal)));

   // now that I've swapped sessions, I need to verify the mute list
   // to make sure we're all good

   // for example, if I swap from the game session to the party session
   // my former enemies on the opposite team may now be in my party and therefore
   // we need to unmute them in the XHV (provided all the other checks still pass)
   //
   // call updateTalkerList on the session, that will call the appropriate BLiveVoice/XHV methods
   s.updateTalkerList();
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::setSessionNext(uint current)
{
   if (++current >= BVoice::cMaxSessions)
      current = 0;

   setSessionInternal(current);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processDeinitSession(const BEvent& event)
{
   BDEBUG_ASSERTM(event.mPrivateData < BVoice::cMaxSessions, "Missing a session ID, unable to deinit voice session");
   if (event.mPrivateData >= BVoice::cMaxSessions)
      return;

   BVoiceSession& session = mSessions[event.mPrivateData];
   session.deinit();

   setSessionNext(event.mPrivateData);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processDeinitSession(BEventReceiverHandle eventHandle)
{
   BDEBUG_ASSERTM(eventHandle != cInvalidEventReceiverHandle, "Missing a from handle, unable to deinit voice session");
   if (eventHandle == cInvalidEventReceiverHandle)
      return;

   for (uint i=0; i < BVoice::cMaxSessions; ++i)
   {
      BVoiceSession& session = mSessions[i];
      if (session.mEventHandle == eventHandle)
      {
         // first switch us to the next session
         setSessionNext(i);

         // then deinit us
         session.deinit();
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processInitClient(const BEvent& event, const BVoiceRequestPayload& request)
{
   // add a client to the given voice session and the XHV
   // all clients added to the XHV will need to be immediately muted
   BDEBUG_ASSERTM(event.mFromHandle != cInvalidEventReceiverHandle, "Missing a from handle, unable to complete voice client registration");
   if (event.mFromHandle == cInvalidEventReceiverHandle)
      return;

   for (uint i=0; i < cMaxVoiceSessions; ++i)
   {
      BVoiceSession& session = mSessions[i];
      if (session.mEventHandle == event.mFromHandle)
      {
         session.addClient(request.mClientID, request.mControllerID, request.mXuid, request.mAddr);
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processDeinitClient(const BEvent& event, const BVoiceRequestPayload& request)
{
   // remove client from the session and from the XHV
   BDEBUG_ASSERTM(event.mFromHandle != cInvalidEventReceiverHandle, "Missing a from handle, unable to complete voice client deregistration");
   if (event.mFromHandle == cInvalidEventReceiverHandle)
      return;

   nlogt(cMPVoiceCL, "BLiveVoice::processDeinitClient -- xuid[%I64u] clientID[%d] channel[%d]", request.mXuid, request.mClientID, request.mChannel);

   for (uint i=0; i < cMaxVoiceSessions; ++i)
   {
      BVoiceSession& session = mSessions[i];
      if (session.mEventHandle == event.mFromHandle)
      {
         session.removeClient(request.mXuid);
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processSetSession(const BEvent& event)
{
   BDEBUG_ASSERTM(event.mPrivateData < BVoice::cMaxSessions, "Missing a session ID, unable to set voice session");
   if (event.mPrivateData >= BVoice::cMaxSessions)
      return;

   setSessionInternal(event.mPrivateData);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processVoice(const BEvent& event, BVoiceBuffer& voice)
{
   BDEBUG_ASSERTM(event.mFromHandle != cInvalidEventReceiverHandle, "Missing a from handle, unable to process voice");
   if (event.mFromHandle == cInvalidEventReceiverHandle)
      return;

   if (mSessionIndex >= BVoice::cMaxSessions)
      return;

//-- FIXING PREFIX BUG ID 4158
   const BVoiceSession& session = mSessions[mSessionIndex];
//--

   if (session.mEventHandle == event.mFromHandle)
   {
      // verify that the session we received the event from is the current session
      processIncomingVoice(voice);
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processTalkersList(const BEvent& event, const BVoiceTalkerListPayload& request)
{
   BDEBUG_ASSERTM(event.mFromHandle != cInvalidEventReceiverHandle, "Missing a from handle, unable to update talkers list");
   if (event.mFromHandle == cInvalidEventReceiverHandle)
      return;

   nlogt(cMPVoiceCL, "BLiveVoice::processTalkersList -- eventHandle[%I64u] requestOwner[%I64u]", event.mFromHandle, request.mOwnerXuid);

   for (uint i=0; i < cMaxVoiceSessions; ++i)
   {
      BVoiceSession& session = mSessions[i];
      if (session.mEventHandle == event.mFromHandle)
      {
         session.updateTalkers(request);
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processHeadset(const BEvent& event)
{
   BDEBUG_ASSERTM(event.mFromHandle != cInvalidEventReceiverHandle, "Missing a from handle, unable to update headset status");
   if (event.mFromHandle == cInvalidEventReceiverHandle)
      return;

   for (uint i=0; i < cMaxVoiceSessions; ++i)
   {
      BVoiceSession& session = mSessions[i];
      if (session.mEventHandle == event.mFromHandle)
      {
         session.updateHeadset(event.mPrivateData, event.mPrivateData2);
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processSetChannel(const BEvent& event, const BVoiceRequestPayload& request)
{
   BDEBUG_ASSERTM(event.mPrivateData < BVoice::cMaxSessions, "Missing a session ID, unable to set voice session");
   if (event.mPrivateData >= BVoice::cMaxSessions)
      return;

   nlogt(cMPVoiceCL, "BLiveVoice::processSetChannel -- session[%d] channel[%d] xuid[%I64u]", event.mPrivateData, request.mChannel, request.mXuid);

   BVoiceSession& session = mSessions[event.mPrivateData];

   session.setChannel(request.mXuid, request.mChannel);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processSetChannel(const BEvent& event)
{
   BDEBUG_ASSERTM(event.mPrivateData < BVoice::cMaxSessions, "Missing a session ID, unable to set voice session");
   if (event.mPrivateData >= BVoice::cMaxSessions)
      return;

   nlogt(cMPVoiceCL, "BLiveVoice::processSetChannel -- session[%d] channel[%d]", event.mPrivateData, event.mPrivateData2);

   BVoiceSession& session = mSessions[event.mPrivateData];

   session.setChannel(event.mPrivateData2);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processMigrateSession(const BEvent& event)
{
   BVoice::Session fromSession = static_cast<BVoice::Session>(event.mPrivateData & 0x0000FFFF);
   BVoice::Session toSession = static_cast<BVoice::Session>((event.mPrivateData >> 16) & 0x0000FFFF);
   BVoice::Channel channel = static_cast<BVoice::Channel>(event.mPrivateData2);

   if (fromSession >= BVoice::cMaxSessions)
      return;
   if (toSession >= BVoice::cMaxSessions)
      return;
   if (channel >= BVoice::cMax)
      return;

   // attempt to switch active session to toSession IF all the members
   // of the fromSession are also in the toSession and also set the channel
   //
   // otherwise, simplly set the channel of the fromSession to the given channel

   BVoiceSession& from = mSessions[fromSession];
   BVoiceSession& to = mSessions[toSession];

   for (int i=from.mPlayers.getSize()-1; i >= 0; --i)
   {
      if (!to.isValidXUID(from.mPlayers[i].mXuid))
      {
         from.setChannel(channel);
         return;
      }
   }

   // set the channel first so we don't spam updates
   to.setChannel(channel);

   // now we can set the active session which will trigger updates
   setSessionInternal(toSession);
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processMute(const BEvent& event, const BVoiceSessionMuteRequestPayload& request)
{
   BDEBUG_ASSERTM(event.mPrivateData < BVoice::cMaxSessions, "Missing a session ID, unable to mute player for the session");
   if (event.mPrivateData >= BVoice::cMaxSessions)
      return;

   BVoiceSession& session = mSessions[event.mPrivateData];

   session.mutePlayer(event.mPrivateData2, request.mXuid, request.mMute);
}

//==============================================================================
// 
//==============================================================================
BReadProfileRequest::BReadProfileRequest() :
   mdwSettingSizeMax(0),
   mpSettingResults(NULL),
   mControllerID(XUSER_MAX_COUNT)
{
   mSettingID[0] = XPROFILE_OPTION_VOICE_MUTED;
   mSettingID[1] = XPROFILE_OPTION_VOICE_THRU_SPEAKERS;
   IGNORE_RETURN(Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped)));
}

//==============================================================================
// 
//==============================================================================
BReadProfileRequest::~BReadProfileRequest()
{
   if (mpSettingResults)
   {
      gNetworkHeap.Delete(mpSettingResults);
      mpSettingResults = NULL;
   }
}

//==============================================================================
// 
//==============================================================================
void BReadProfileRequest::init(uint controllerID)
{
   if (controllerID >= XUSER_MAX_COUNT)
      return;

   mControllerID = controllerID;

   // determine how much memory we need to allocate to read the profile settings
   DWORD dwStatus = XUserReadProfileSettings(0,                   // A title in your family or 0 for the current title
                                             0,                   // Player index (not used)
                                             2,                   // Number of settings to read
                                             &mSettingID[0],      // List of settings to read
                                             &mdwSettingSizeMax,  // Results size (0 to determine maximum)
                                             NULL,                // Results (not used)
                                             NULL);               // Overlapped (not used)

   // validate the return code and data
   BASSERT( dwStatus == ERROR_INSUFFICIENT_BUFFER );
   BASSERT( mdwSettingSizeMax > 0 );

   // NOTE: The game is responsible for freeing this memory when it is no longer needed
   BYTE* pData = reinterpret_cast<BYTE*>(gNetworkHeap.New(mdwSettingSizeMax));
   mpSettingResults = (XUSER_READ_PROFILE_SETTING_RESULT*)pData;

   // kick off the read for the profile
   dwStatus = XUserReadProfileSettings(0,                   // A title in your family or 0 for the current title
                                       mControllerID,       // Player index making the request
                                       2,                   // Number of settings to read
                                       &mSettingID[0],      // List of settings to read
                                       &mdwSettingSizeMax,  // Results size
                                       mpSettingResults,    // Results go here
                                       &mOverlapped);       // Overlapped struct

   BASSERT( dwStatus == ERROR_SUCCESS || dwStatus == ERROR_IO_PENDING );
}

//==============================================================================
// 
//==============================================================================
void BReadProfileRequest::cancel()
{
   if (XHasOverlappedIoCompleted(&mOverlapped) == FALSE)
   {
      XCancelOverlapped(&mOverlapped);

      DWORD dwResult;
      if (XGetOverlappedResult(&mOverlapped, &dwResult, FALSE) != ERROR_SUCCESS || dwResult != ERROR_SUCCESS)
      {
         BASSERTM(FALSE, "Why did XCancelOverlapped not complete?!");
      }

      IGNORE_RETURN(Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped)));
   }
}

//==============================================================================
// 
//==============================================================================
BOOL BReadProfileRequest::isComplete()
{
   return XHasOverlappedIoCompleted(&mOverlapped);
}

//==============================================================================
// Default return value is TRUE which triggers the most restrictive setting
//==============================================================================
BOOL BReadProfileRequest::getVoiceThruSpeakers()
{
   if (!isComplete())
   {
      BASSERTM(FALSE, "Read profile request has not completed");
      return TRUE;
   }

   if (mpSettingResults == NULL)
      return FALSE;

   DWORD dwResult;
   if (XGetOverlappedResult(&mOverlapped, &dwResult, FALSE) != ERROR_SUCCESS || dwResult != ERROR_SUCCESS)
      return TRUE;

   if (mpSettingResults->dwSettingsLen < 2)
      return TRUE;

//-- FIXING PREFIX BUG ID 4159
   const XUSER_PROFILE_SETTING& setting = mpSettingResults->pSettings[1];
//--

   BASSERTM(setting.user.dwUserIndex == mControllerID, "Setting retrieved for the wrong controller");
   if (setting.user.dwUserIndex != mControllerID)
      return TRUE;

   BASSERTM(setting.dwSettingId == XPROFILE_OPTION_VOICE_THRU_SPEAKERS, "Wrong setting retrieved from the profile");
   if (setting.dwSettingId != XPROFILE_OPTION_VOICE_THRU_SPEAKERS)
      return TRUE;

   return (setting.data.nData != XPROFILE_VOICE_THRU_SPEAKERS_OFF);
}

//==============================================================================
// 
//==============================================================================
BOOL BReadProfileRequest::getVoiceMuted()
{
   if (!isComplete())
   {
      BASSERTM(FALSE, "Read profile request has not completed");
      return TRUE;
   }

   if (mpSettingResults == NULL)
      return FALSE;

   DWORD dwResult;
   if (XGetOverlappedResult(&mOverlapped, &dwResult, FALSE) != ERROR_SUCCESS || dwResult != ERROR_SUCCESS)
      return TRUE;

   if (mpSettingResults->dwSettingsLen == 0)
      return TRUE;

   const XUSER_PROFILE_SETTING& setting = mpSettingResults->pSettings[0];

   BASSERTM(setting.user.dwUserIndex == mControllerID, "Setting retrieved for the wrong controller");
   if (setting.user.dwUserIndex != mControllerID)
      return TRUE;

   BASSERTM(setting.dwSettingId == XPROFILE_OPTION_VOICE_MUTED, "Wrong setting retrieved from the profile");
   if (setting.dwSettingId != XPROFILE_OPTION_VOICE_MUTED)
      return TRUE;

   return setting.data.nData;
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::queryProfiles()
{
   // need to check the current profile reads for their completion
   // and update ourselves accordingly
   bool updateTalkers = false;

   for (uint i=0; i < XUSER_MAX_COUNT; ++i)
   {
      if (mReadProfileRequest[i])
      {
         if (mReadProfileRequest[i]->isComplete())
         {
            mSpeakersEnabled[i] = mReadProfileRequest[i]->getVoiceThruSpeakers();

            Sync::InterlockedExchangeExport(&mProfileMute[i], mReadProfileRequest[i]->getVoiceMuted());

            HEAP_DELETE(mReadProfileRequest[i], gNetworkHeap);
            mReadProfileRequest[i] = NULL;

            updateTalkers = true;
         }
      }
   }

   // whenever we complete a profile read request, we need to update the talker list for the primary session
   if (updateTalkers)
      updateTalkerList();

   queryDirtyProfiles();
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::queryProfiles(DWORD profileMask)
{
   for (uint i=0; i < XUSER_MAX_COUNT; i++)
   {
      // are we enabled for voice and is our headset present?
      // if not, there's no sense attempting to read from the user's profile
      if (mHasVoice[i] && mHeadsetPresentInternal[i])
      {
         // was it one of the profiles updated
         if (profileMask & (1 << i))
         {
            // we need to check the user's profile for whether they've enabled
            // XPROFILE_OPTION_VOICE_THRU_SPEAKERS
            if (mReadProfileRequest[i])
            {
               // there's currently a read request in progress, so just mark
               // this user as dirty to kick off another read after the current one finishes
               mProfileDirty[i] = TRUE;
            }
            else
            {
               // be sure to reset any dirty flags and then kick off a new profile read
               mProfileDirty[i] = FALSE;
               mReadProfileRequest[i] = HEAP_NEW(BReadProfileRequest, gNetworkHeap);
               mReadProfileRequest[i]->init(i);
            }
         }
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::queryDirtyProfiles()
{
   for (uint i=0; i < XUSER_MAX_COUNT; i++)
   {
      // if we have a dirty profile
      if (mProfileDirty[i])
      {
         // and the person is still enabled for voice and has a headset present
         if (mHasVoice[i] && mHeadsetPresentInternal[i])
         {
            // and we're not already reading their profile
            // if we are reading their profile, then leave the dirty flag set
            if (!mReadProfileRequest[i])
            {
               // kick off a new profile read request
               mProfileDirty[i] = FALSE;
               mReadProfileRequest[i] = HEAP_NEW(BReadProfileRequest, gNetworkHeap);
               mReadProfileRequest[i]->init(i);
            }
         }
         else
         {
            // if we're no longer enabled for voice
            // or we're enabled but don't have a headset, then no point
            // in reading the profile as voice will come out of the speakers
            mProfileDirty[i] = FALSE;
         }
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveVoice::processXNotify()
{
   DWORD id = 0;
   ULONG_PTR param = 0;

   if (XNotifyGetNext(mXNotifyHandle, 0, &id, &param))
   {
      switch (id)
      {
         case XN_SYS_PROFILESETTINGCHANGED:
            {
               // if a user has changed their profile, I need to read their profile settings to
               // retrieve the value of XPROFILE_OPTION_VOICE_THRU_SPEAKERS
               // I'll want the value for every profile listed
               // this should be the same code that is executed on voice init
               // and should prevent people from hearing anything until I know about their mute options
               //
               // which means I should have a global mute check that I can tweak to prevent any processing of voice data
               // until the profile has been read and the talker list update and then I can start sending/receiving voice data
               //
               //
               queryProfiles(static_cast<DWORD>(param));
               break;
            }

         // * if my mute list changes
         // * someone signs-in on a different controller (or signs out)
         // * or someone adds/removes a friend,
         // I need update my talker list because of TCR #90
         case XN_SYS_MUTELISTCHANGED:
         case XN_SYS_SIGNINCHANGED:
         case XN_FRIENDS_FRIEND_ADDED:
         case XN_FRIENDS_FRIEND_REMOVED:
            {
               // should trigger a dirty bit that rechecks the mute list
               updateTalkerList();
               break;
            }
      }
   }
}

//==============================================================================
// 
//==============================================================================
bool BLiveVoice::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cVoiceEventTimer:
         {
            // poll the voice system for new data and to set talkers
            update();
            break;
         }

      case cNetEventVoiceRecv:
         {
            BVoiceBuffer* pBuf = reinterpret_cast<BVoiceBuffer*>(event.mPrivateData);
            BDEBUG_ASSERT(pBuf);
            if (pBuf == NULL)
               break;

            // XXX process the voice payload
            processVoice(event, *pBuf);

            mAllocator.free(pBuf);
            break;
         }

      case cNetEventVoiceTalkerList:
         {
            // this will be handled on the sim helper thread, so we have to be careful in sending packets
//-- FIXING PREFIX BUG ID 4160
            const BVoiceTalkerListPayload* pPayload = reinterpret_cast<BVoiceTalkerListPayload*>(event.mpPayload);
//--
            BDEBUG_ASSERT(pPayload);
            if (pPayload == NULL)
               break;

            // update our talker list for the given client
            processTalkersList(event, *pPayload);
            break;
         }

      case cNetEventVoiceHeadsetPresent:
         {
            processHeadset(event);
            break;
         }

      case cVoiceEventSetChannel:
         {
            // what do I need in order to properly set a player's channel?
            // I'll need to know the session and the xuid
//-- FIXING PREFIX BUG ID 4161
            const BVoiceRequestPayload* pPayload = reinterpret_cast<BVoiceRequestPayload*>(event.mpPayload);
//--
            if (pPayload == NULL)
               processSetChannel(event);
            else
               processSetChannel(event, *pPayload);
            break;
         }

      case cVoiceEventMigrateSession:
         {
            processMigrateSession(event);
            break;
         }

      case cVoiceEventMutePlayer:
         {
//-- FIXING PREFIX BUG ID 4162
            const BVoiceSessionMuteRequestPayload* pPayload = reinterpret_cast<BVoiceSessionMuteRequestPayload*>(event.mpPayload);
//--
            BDEBUG_ASSERT(pPayload);
            if (pPayload == NULL)
               break;
            processMute(event, *pPayload);
            break;
         }

      case cNetEventVoiceInitClient:
         {
//-- FIXING PREFIX BUG ID 4163
            const BVoiceRequestPayload* pPayload = reinterpret_cast<BVoiceRequestPayload*>(event.mpPayload);
//--
            BDEBUG_ASSERT(pPayload);
            if (pPayload != NULL)
               processInitClient(event, *pPayload);
            break;
         }

      case cNetEventVoiceDeinitClient:
         {
//-- FIXING PREFIX BUG ID 4164
            const BVoiceRequestPayload* pPayload = reinterpret_cast<BVoiceRequestPayload*>(event.mpPayload);
//--
            BDEBUG_ASSERT(pPayload);
            if (pPayload != NULL)
               processDeinitClient(event, *pPayload);
            break;
         }

      case cNetEventVoiceDeinitSession:
         {
            // BSession is shutting down, lookup the session based on the from event handle
            processDeinitSession(event.mFromHandle);
            break;
         }

      case cVoiceEventSetSession:
         {
            processSetSession(event);
            break;
         }

      case cVoiceEventInitSession:
         {
            processInitSession(event);
            break;
         }

      case cVoiceEventDeinit:
         {
            processDeinit(event);
            break;
         }

      case cVoiceEventXNotify:
         {
            processXNotify();
            break;
         }

      case cEventClassClientRemove:
         {
            if (mpXHVEngine)
            {
               mpXHVEngine->Release();
               mpXHVEngine = NULL;
            }
            break;
         }
   }
   return false;
}

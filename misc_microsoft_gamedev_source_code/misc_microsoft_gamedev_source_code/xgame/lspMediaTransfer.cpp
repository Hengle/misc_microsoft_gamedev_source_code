//==============================================================================
// lspMediaTransfer.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#include "common.h"
#include "configsgame.h"
#include "user.h"
#include "usermanager.h"
#include "gamedirectories.h"

#include "lspManager.h"
#include "lspMediaTransfer.h"

// xcore
#include "stream\dynamicStream.h"
#include "file\xboxFileUtils.h"
#include "file\win32FindFiles.h"
#include "file\win32FileStream.h"
#include "file\xcontentStream.h"

// xsystem
#include "notification.h"
#include "bfileStream.h"

// xnetwork
#include "NetPackets.h"
#include "MaxSendSize.h"

IMPLEMENT_FREELIST(BLSPMediaTask, 2, &gSimHeap);

uint BLSPMediaTask::mNextID = 0;

//==============================================================================
// 
//==============================================================================
BLSPMediaFriendsEnum::BLSPMediaFriendsEnum(uint userIndex) : 
   mEnumHandle(INVALID_HANDLE_VALUE),
   mUserIndex(userIndex),
   mpBuffer(NULL),
   mpHeaderStream(NULL),
   mBufferSize(0),
   mNumItems(0),
   mLastError(ERROR_SUCCESS)
{
   Utils::FastMemSet(&mOverlapped, 0, sizeof(XOVERLAPPED));
}

//==============================================================================
// 
//==============================================================================
BLSPMediaFriendsEnum::~BLSPMediaFriendsEnum()
{
   if (mEnumHandle != INVALID_HANDLE_VALUE)
      CloseHandle(mEnumHandle);
   mEnumHandle = INVALID_HANDLE_VALUE;

   delete[] mpBuffer;
   mpBuffer = NULL;

   delete mpHeaderStream;
   mpHeaderStream = NULL;
}

//==============================================================================
// 
//==============================================================================
bool BLSPMediaFriendsEnum::isComplete()
{
   // if we're not ERROR_SUCCESS, then we must have failed, bail early
   if (mLastError > ERROR_SUCCESS)
      return true;

   if (mEnumHandle == INVALID_HANDLE_VALUE)
   {
      if (mUserIndex >= XUSER_MAX_COUNT)
      {
         mLastError = ERROR_INVALID_PARAMETER;
         return true;
      }

      mBufferSize = 0;
      DWORD result = XFriendsCreateEnumerator(mUserIndex, 0, cMaxFriends, &mBufferSize, &mEnumHandle);
      if (FAILED(result))
      {
         mLastError = result;
         return true;
      }

      if (mpBuffer)
      {
         delete[] mpBuffer;
         mpBuffer = NULL;
      }
      mpBuffer = new uchar[mBufferSize];

      result = XEnumerate(mEnumHandle, mpBuffer, mBufferSize, NULL, &mOverlapped);
      if (result != ERROR_IO_PENDING && result != ERROR_SUCCESS)
      {
         mLastError = result;
         return true;
      }
   }

   if (XHasOverlappedIoCompleted(&mOverlapped))
   {
      DWORD result = XGetOverlappedResult(&mOverlapped, &mLastError, FALSE);
      if (result == ERROR_IO_INCOMPLETE || result == ERROR_IO_PENDING)
         return false;
      return true;
   }
   return false;
}

//==============================================================================
// 
//==============================================================================
uint32 BLSPMediaFriendsEnum::getNumFriends()
{
   if (!XHasOverlappedIoCompleted(&mOverlapped))
      return 0;

   mNumItems = mOverlapped.InternalHigh;
   return mNumItems;
}

//==============================================================================
// 
//==============================================================================
const PXONLINE_FRIEND BLSPMediaFriendsEnum::getFriend(uint i)
{
   if (i >= getNumFriends())
      return NULL;

   if (mpBuffer == NULL)
      return NULL;

   return reinterpret_cast<PXONLINE_FRIEND>(mpBuffer + (i*sizeof(XONLINE_FRIEND)));
}

//==============================================================================
// 
//==============================================================================
BDynamicStream* BLSPMediaFriendsEnum::getHeaderStream()
{
   if (mpHeaderStream)
      return mpHeaderStream;

   mpHeaderStream = new BDynamicStream();

   mpHeaderStream->printf("<fl>");
   uint count = getNumFriends();
   for (uint i=0; i < count; ++i)
   {
//-- FIXING PREFIX BUG ID 2418
      const PXONLINE_FRIEND pFriend = reinterpret_cast<PXONLINE_FRIEND>(mpBuffer + (i*sizeof(XONLINE_FRIEND)));
//--
      mpHeaderStream->printf("<f x='%I64u'/>", pFriend->xuid);
   }
   mpHeaderStream->printf("</fl>");

   return mpHeaderStream;
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTask::onAcquire()
{
   mXuid = INVALID_XUID;
   mMediaID = 0;
   mStreamID = 0;
   mpContent = NULL;
   mpCallback = NULL;
   mpFileStream = NULL;
   mpHeaderStream = NULL;
   mpFriendsEnum = NULL;
   mUserIndex = XUSER_MAX_COUNT;
   mID = 0;
   mRequestID = 0;
   mCRC = 0;
   mTimeout = 20000;
   mTransferred = 0;
   mCommand = cCommandNone;
   mState = cStateIdle;
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTask::onRelease()
{
   if (mpCallback != NULL)
      mpCallback->mediaTaskRelease(mID, ERROR_SUCCESS);

   delete mpContent;
   delete mpFileStream;
   delete mpHeaderStream;
   delete mpFriendsEnum;
   mpContent = NULL;
   mpCallback = NULL;
   mpFileStream = NULL;
   mpHeaderStream = NULL;
   mpFriendsEnum = NULL;
   mTimeout = 0;
}

//==============================================================================
// 
//==============================================================================
uint BLSPMediaTask::init(BMediaCommand command, uint timeout, uint userIndex, XUID xuid, ILSPMediaTask* pCallback, uint crc)
{
   mUserIndex = userIndex;
   mXuid = xuid;
   mCRC = crc;
   mTimeout = timeout;
   mpCallback = pCallback;
   mID = ++mNextID;
   mCommand = command;
   mState = cStatePending;

   return mID;
}

//==============================================================================
// 
//==============================================================================
uint BLSPMediaTask::init(BMediaCommand command, uint timeout, uint userIndex, XUID xuid, ILSPMediaTask* pCallback, uint64 mediaID)
{
   mUserIndex = userIndex;
   mXuid = xuid;
   mTimeout = timeout;
   mpCallback = pCallback;
   mMediaID = mediaID;
   mID = ++mNextID;
   mCommand = command;
   mState = cStatePending;

   return mID;
}

//==============================================================================
// 
//==============================================================================
uint BLSPMediaTask::init(BMediaCommand command, uint timeout, uint userIndex, XUID xuid, ILSPMediaTask* pCallback, uint64 mediaID, const XCONTENT_DATA& data, BDynamicStream* pStream)
{
   BASSERTM(pStream != NULL, "Missing header information");

   mpContent = new XCONTENT_DATA;

   Utils::FastMemCpy(mpContent, &data, sizeof(XCONTENT_DATA));

   mpHeaderStream = pStream;
   mUserIndex = userIndex;
   mXuid = xuid;
   mTimeout = timeout;
   mpCallback = pCallback;
   mMediaID = mediaID;
   mID = ++mNextID;
   mCommand = command;
   mState = cStatePending;

   return mID;
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTask::restart()
{
   // reset our task back to a state as if we haven't done anything yet
   if (mpFileStream)
      mpFileStream->seek(0);

   mStreamID = 0;
   mTransferred = 0;
   mState = cStateServerPending;
}

//==============================================================================
// 
//==============================================================================
BStream* BLSPMediaTask::getStream()
{
   if (mpFileStream)
      return mpFileStream;

   BASSERTM(mpContent != NULL, "Missing XCONTENT_DATA information");
   if (mpContent == NULL)
      return NULL;

   BXContentStream* pStream = new BXContentStream();

   if (!pStream->open(mUserIndex, *mpContent, cSFReadable | cSFOpenExisting | cSFSeekable | cSFOptimizeForSequentialAccess))
   {
      delete pStream;
      gConsoleOutput.error("BLSPMediaTask::getStream : Unable to open storage item %s\n", mpContent->szDisplayName);
      return NULL;
   }

   mpFileStream = pStream;

   return mpFileStream;
}

//==============================================================================
// 
//==============================================================================
BStream* BLSPMediaTask::getTempStream()
{
   if (mpFileStream)
      return mpFileStream;

   BDynamicStream* pStream = new BDynamicStream();

   mpFileStream = pStream;

   return mpFileStream;
}

//==============================================================================
// 
//==============================================================================
BLSPMediaFriendsEnum* BLSPMediaTask::getFriendsEnum()
{
   if (mpFriendsEnum != NULL)
      return mpFriendsEnum;

   mpFriendsEnum = new BLSPMediaFriendsEnum(mUserIndex);
   return mpFriendsEnum;
}

//==============================================================================
// 
//==============================================================================
BLSPMediaTransfer::BLSPMediaTransfer() :
   BLSPTitleServerConnection(cDefaultLSPMediaTransferPort, cConfigLSPMediaTransferPort, cDefaultLSPMediaServiceID, cConfigLSPMediaServiceID),
   mpCurrentTask(NULL),
   //mpConn(NULL),
   mLastTime(0),
   mRetryTime(0),
   mRetryInterval(2500),
   mNumRetries(0),
   mMaxRetries(1),
   mState(cStateIdle),
   //mPort(cDefaultLSPMediaTransferPort),
   mStopped(true),
   mWaitingOnResponse(false),
   //mUploading(false),
   mEnabled(false),
   mCompress(false)
{
   //Utils::FastMemSet(&mOverlapped, 0, sizeof(BLSPOVERLAPPED));
   //Utils::FastMemSet(&mServerAddr, 0, sizeof(IN_ADDR));

   //mOverlapped.hEvent = INVALID_HANDLE_VALUE;

   mCompress = gConfig.isDefined("LSPEnableMediaCompression");
}

//==============================================================================
// 
//==============================================================================
BLSPMediaTransfer::~BLSPMediaTransfer()
{
   //shutdown();
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPMediaTransfer::sendReady(BXStreamConnection& conn)
{
   if (!mpCurrentTask || mpCurrentTask->mState != BLSPMediaTask::cStateInProgress)
      return;

   //if (!mpConn)
   //   return;

   //if (!mpConn->isConnected())
   //{
   //   setState(cStateReconnect);
   //   return;
   //}

   // hold up on passing more data to the connection until it's ready
   //if (!mpConn->isSendReady())
   //   return;

   if (mpCurrentTask->getCommand() == BLSPMediaTask::cCommandUpload)
   {
      uint chunkSize = mpCurrentTask->getStream()->readBytes(mFileBuffer, 1024);

      uchar* pBuf = mFileBuffer;
      void* pData = reinterpret_cast<void*>(pBuf);

      BChunkPacket::eCommands command = BChunkPacket::cCommandChunk;
      if (chunkSize < 1024 || chunkSize == 0)
         command = BChunkPacket::cCommandComplete;
      BChunkPacket packet(getServiceID(), command, mpCurrentTask->getStreamID(), pData, chunkSize);

      send(packet, mCompress);

      mLastTime = timeGetTime();

      mpCurrentTask->updateTransferred(chunkSize);

      if (chunkSize < 1024 || chunkSize == 0)
      {
         mpCurrentTask->getStream()->close();
         mpCurrentTask->mState = BLSPMediaTask::cStateServerPending;
         mState = cStateServerPending;
      }

      //long sendSize = cSendBufferSize;
      //Utils::FastMemSet(mSendBuffer, 0, sendSize);

      //pData = mSendBuffer;
      //packet.serializeInto(&pData, &sendSize);

      //// if the send fails, the connection should clean up and eventually remove us
      //// so the negative state doesn't really matter
      //if (mpConn->send(pData, sendSize, mCompress))
      //{
      //   //mState = cStateSendPending;
      //   mLastTime = timeGetTime();

      //   mpCurrentTask->updateTransferred(chunkSize);

      //   if (chunkSize < 1024 || chunkSize == 0)
      //   {
      //      mpCurrentTask->getStream()->close();
      //      mpCurrentTask->mState = BLSPMediaTask::cStateServerPending;
      //      mState = cStateServerPending;
      //   }
      //}
      //else
      //{
      //   stop();
      //}
   }
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPMediaTransfer::connected(BXStreamConnection& connection)
{
   BLSPTitleServerConnection::connected(connection);

   if (mpCurrentTask == NULL || mpCurrentTask->mState == BLSPMediaTask::cStateDelete)
   {
      setState(cStateSelectCommand);
      return;
   }

   // need to retry the current task
   mpCurrentTask->restart();

   mWaitingOnResponse = false;

   if (mpCurrentTask->mCommand == BLSPMediaTask::cCommandListFriends)
      setState(cStatePreRequest);
   else
      setState(cStateRequest);

   //mState = cStateRequest;
   //setRetries(10);
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPMediaTransfer::dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size)
{
   //if (serviceId != cServiceIdMedia)
   //   return;

   mLastTime = timeGetTime();

   switch (type)
   {
      case BPacketType::cMediaCommandPacket:
         {
            BMediaPacket packet;
            packet.deserializeFrom(pData, size);

            BLSPMediaTask* pTask = NULL;
            for (uint i=0; i < mTasks.getSize(); ++i)
            {
               if (mTasks[i]->mRequestID == packet.getID())
               {
                  pTask = mTasks[i];
                  break;
               }
            }

            if (pTask == NULL)
               return;

            switch (packet.mCommand)
            {
               case BMediaPacket::cCommandListResponse:
                  {
                     // parse out the response from the server and add to our list of media
                     //
                     // the media list could be for either screen shots or record games, do I need/want to make a distinction?
                     //
                     // should I just call the mediaListResponse method with the raw data pointer and size
                     // and let that service parse out the response?
                     if (pTask->mpCallback)
                        (pTask->mpCallback)->mediaListResponse(pTask->getID(), ERROR_SUCCESS, packet.mCRC, packet.mTTL, packet.mpData, packet.mSize);

                     pTask->mState = BLSPMediaTask::cStateDelete;

                     setState(cStateSelectCommand);
                  }
                  break;

               case BMediaPacket::cCommandRetrieveResponse:
                  {
                     // pull out the stream id
                     mState = cStateServerPending;
                     if (pTask)
                     {
                        pTask->setStreamID(packet.mStreamID);
                        pTask->mState = BLSPMediaTask::cStateInProgress;
                     }

                     mLastTime = timeGetTime();
                     mWaitingOnResponse = false;
                  }
                  break;

               case BMediaPacket::cCommandRetrieveComplete:
                  {
                     // send the next file?
                     // remove the current file
                     //if (pTask)
                     //{
                     //   pTask->mState = BLSPMediaTask::cStateDelete;

                     //   if (pTask->mpCallback)
                     //      (pTask->mpCallback)->mediaDownloadResponse(pTask->getID());

                     //   //mTasks.remove(pTask);
                     //   //BLSPMediaTask::releaseInstance(pTask);

                     //   pTask = NULL;
                     //}
                     //setState(cStateSelectCommand);
                  }
                  break;

               case BMediaPacket::cCommandStoreResponse:
                  {
                     // pull out the stream id
                     mState = cStateServerPending;
                     if (pTask)
                     {
                        pTask->setStreamID(packet.mStreamID);
                        pTask->mState = BLSPMediaTask::cStateInProgress;
                     }
                     else
                        setState(cStateSelectCommand);

                     mLastTime = timeGetTime();
                     mWaitingOnResponse = false;
                  }
                  break;

               case BMediaPacket::cCommandStoreComplete:
                  {
                     // send the next file?
                     // remove the current file
                     if (pTask)
                     {
                        pTask->mState = BLSPMediaTask::cStateDelete;

                        if (pTask->mpCallback)
                           (pTask->mpCallback)->mediaUploadResponse(pTask->getID(), ERROR_SUCCESS);

                        //mTasks.remove(pTask);
                        //BLSPMediaTask::releaseInstance(pTask);

                        pTask = NULL;
                     }
                     setState(cStateSelectCommand);
                  }
                  break;

               case BMediaPacket::cCommandDeleteResponse:
                  {
                     if (pTask->mpCallback)
                        (pTask->mpCallback)->mediaDeleteResponse(pTask->getID(), ERROR_SUCCESS);

                     pTask->mState = BLSPMediaTask::cStateDelete;

                     setState(cStateSelectCommand);
                  }
                  break;

               default:
                  break;
            }
         }
         break;

      case BPacketType::cChunkPacket:
         {
            BChunkPacket packet;
            packet.deserializeFrom(pData, size);

            BLSPMediaTask* pTask = NULL;
            for (uint i=0; i < mTasks.getSize(); ++i)
            {
               if (mTasks[i]->getStreamID() == packet.getStreamID())
               {
                  pTask = mTasks[i];
                  break;
               }
            }

            if (pTask == NULL)
               return;

            switch (packet.mCommand)
            {
               case BChunkPacket::cCommandChunk:
                  {
                     // how do we want to handle the download?
                     // queue up all the data into a dynamic stream and then let the requestor handle
                     // the final save to wherever?
                     //
                     pTask->updateTransferred(packet.mSize);

                     pTask->getTempStream()->writeBytes(packet.mpData, packet.mSize);
                  }
                  break;
               case BChunkPacket::cCommandComplete:
                  {
                     pTask->mState = BLSPMediaTask::cStateDelete;

                     pTask->updateTransferred(packet.mSize);

                     pTask->getTempStream()->writeBytes(packet.mpData, packet.mSize);
                     if (pTask->mpCallback)
                        (pTask->mpCallback)->mediaDownloadResponse(pTask->getID(), ERROR_SUCCESS);

                     setState(cStateSelectCommand);
                  }
                  break;
               default:
                  break;
            }
         }
         break;

      default:
         return;
   }
}

//==============================================================================
// BXStreamConnection::BXStreamObserver
//==============================================================================
void BLSPMediaTransfer::disconnected(uint status)
{
   BLSPTitleServerConnection::disconnected(status);

   if (mpCurrentTask != NULL || mTasks.getSize() > 0)
      mState = cStateReconnect;
   else
      mState = cStateDisconnected;

   // if we get disconnected, need to kick back to a reconnect state
   // but only if we haven't entered an idle, complete or timed out state
   // this isn't a very good way of handling disconnects at this level, we need a more
   // intelligent recovery... if I was in the middle of attempting a file upload
   // and I didn't error out for some other reason, then this needs to go through the reconnect phase
   //if (mUploading)
   //   mState = cStateReconnect;
}

//==============================================================================
// BXStreamConnection::BXStreamObserver
//==============================================================================
void BLSPMediaTransfer::shutdown()
{
   BLSPTitleServerConnection::shutdown();

   // my connection is going away, cleanup what we can and go away
   setState(cStateDone);

   //if (mOverlapped.hEvent != INVALID_HANDLE_VALUE)
   //   CloseHandle(mOverlapped.hEvent);

   //if (mpConn)
   //   mpConn->removeObserver(this);

   mpCurrentTask = NULL;

   //mOverlapped.hEvent = INVALID_HANDLE_VALUE;
   //mpConn = NULL;

   mWaitingOnResponse = false;
   //mUploading = false;

   uint count = mTasks.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPMediaTask* pCommand = mTasks[i];
      if (!pCommand)
         continue;
      BLSPMediaTask::releaseInstance(pCommand);
   }

   mTasks.clear();

   // need to remove my observer from the lsp manager
   //gLSPManager.cancel(cTCP, mPort, &mOverlapped);
   BLSPTitleServerConnection::shutdown();
}

//==============================================================================
// 
//==============================================================================
BLSPMediaTask* BLSPMediaTransfer::getTaskByID(uint taskID)
{
   uint count = mTasks.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPMediaTask* pTemp = mTasks[i];
      if (pTemp && pTemp->getID() == taskID)
      {
         return pTemp;
      }
   }
   return NULL;
}

//==============================================================================
// 
//==============================================================================
BLSPMediaTask* BLSPMediaTransfer::createTask()
{
   // create a new task and add to the tasks list
   // the task will initially be created as an unknown/idle task
   // and should be ignored by the media transfer this class
   BLSPMediaTask* pTask = BLSPMediaTask::getInstance();
   mTasks.add(pTask);

   mStopped = false;

   BXStreamConnection * c = getConnection();
   if (! c || !c->isConnecting())
      connect();
   else if (isFinished())
      setState(cStateSelectCommand);

   return pTask;
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::terminateTask(BLSPMediaTask* pTask)
{
   if (pTask == NULL)
      return;

   // this is a "pull the rug out from under us" type of operation
   //
   // the task may be in-progress and waiting on a response from the server
   //
   // or we could be shutting down in which case the memory clean-up is not our concern
   // as the process gets ripped apart anyway, but we should at least make an attempt to remove it
   if (pTask == mpCurrentTask)
   {
      mpCurrentTask = NULL;
      // set the state to select another task from the list
   }
   mTasks.remove(pTask);
   BLSPMediaTask::releaseInstance(pTask);

   if (mTasks.getSize() == 0)
      mStopped = true;
}

//==============================================================================
// 
//==============================================================================
//void BLSPMediaTransfer::connect()
//{
//   long port;
//   if (gConfig.get(cConfigLSPMediaTransferPort, &port))
//      mPort = static_cast<ushort>(port);
//
//   if (mOverlapped.hEvent == INVALID_HANDLE_VALUE)
//      mOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//   else
//      ResetEvent(mOverlapped.hEvent);
//
//   DWORD dwResult = gLSPManager.connect(cTCP, mPort, &mOverlapped, BXStreamConnection::eFlagUseHeader, 0);
//
//   if (dwResult == ERROR_NOT_LOGGED_ON)
//   {
//      // I think the upper layers will need to handle the sign-in portion
//      // if anything we should attempt to sign in and if it still fails
//      // then we can fail
//      setState(cStateTimedout);
//      return;
//   }
//   else if (dwResult == ERROR_ACCESS_DENIED)
//   {
//      // I'm not allowed to upload
//      setState(cStateTimedout);
//      return;
//   }
//
//   mState = cStateConnect;
//
//   mUploading = true;
//
//   setRetryInterval(250);
//}

//==============================================================================
// 
//==============================================================================
//void BLSPMediaTransfer::disconnect()
//{
//   if (mOverlapped.hEvent != INVALID_HANDLE_VALUE)
//      CloseHandle(mOverlapped.hEvent);
//
//   mOverlapped.hEvent = INVALID_HANDLE_VALUE;
//
//   if (mpConn)
//      mpConn->removeObserver(this);
//
//   mpConn = NULL;
//
//   //mOverlapped.pXStreamConn = NULL;
//
//   mState = cStateDisconnected;
//}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::reconnect()
{
   if (!BLSPTitleServerConnection::reconnect())
   {
      disconnect();
      mState = cStateDisconnected;
   }
   else
   {
      // insure that if reconnect() returns true that means
      // we're in the stage of connecting/connected/reconnecting...
      mState = cStateConnecting;
   }
   //else
   //{
   //   // by the time we reach this point, a connection may have already been re-established
   //   // and we should immediately start our task back up
   //   // otherwise, we need to wait for the connected callback

   //   if (BLSPTitleServerConnection::i
   //   mState = cStateConnecting;

   //   // reset our tasks
   //   mWaitingOnResponse = false;
   //   //mUploading = false;

   //   if (mpCurrentTask != NULL)
   //   {
   //      mpCurrentTask->restart();
   //      mpCurrentTask->mState = BLSPMediaTask::cStatePending;
   //   }

   //   mpCurrentTask = NULL;
   //}
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::start()
{
   mStopped = false;

   setState(cStateSelectCommand);
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::stop()
{
   mStopped = true;

   // shut everything down and go dark until the game tells us otherwise
   shutdown();

   setState(cStateStop);
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::reset()
{
   stop();

   // want to start up again but wait 10 seconds
   start();
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::service()
{
   if (mStopped)
      return;

   switch (mState)
   {
      //case cStateSend:
      //   {
      //      // XXX need to throttle this
      //      if (!mpCurrentTask)
      //      {
      //         setState(cStateSelectCommand);
      //      }
      //      else
      //      {
      //         if (mpConn)
      //            sendReady(*mpConn);
      //         else
      //            setState(cStateReconnect);
      //      }
      //      break;
      //   }

      case cStatePreRequest:
         {
            if (mpCurrentTask == NULL)
            {
               mWaitingOnResponse = false;
               setState(cStateSelectCommand);
            }
            else if (mpCurrentTask->mCommand != BLSPMediaTask::cCommandListFriends)
            {
               // this should never happen because our current task reassignment should have handled the state change
               BASSERTM(false, "In cStatePreRequest with the wrong command type");
               mWaitingOnResponse = false;
               setState(cStateRequest);
            }
            else if (mpCurrentTask->getFriendsEnum())
            {
               // check the status of the friends list enumeration
               // if there's an error, then we need to propagate that error back up
               if ((mpCurrentTask->getFriendsEnum())->isComplete())
               {
                  if (SUCCEEDED((mpCurrentTask->getFriendsEnum())->getLastError()))
                  {
                     setState(cStateRequest);
                  }
                  else
                  {
                     if (mpCurrentTask->mpCallback)
                        (mpCurrentTask->mpCallback)->mediaListResponse(mpCurrentTask->getID(), (mpCurrentTask->getFriendsEnum())->getLastError(), 0, 0, NULL, 0);
                     setState(cStateSelectCommand);
                  }
               }
            }
            else
            {
               mWaitingOnResponse = false;
               setState(cStateRequest);
            }
            break;
         }

      case cStateRequest:
         {
            if (mpCurrentTask == NULL)
            {
               mWaitingOnResponse = false;
               setState(cStateSelectCommand);
            }
            else if (mWaitingOnResponse)
            {
               if ((timeGetTime() - mLastTime) > mpCurrentTask->getTimeout())
               {
                  setState(cStateTimedout);
                  mLastTime = 0;
               }
            }
            else if (timeGetTime() > mRetryTime)
            {
               if (! getConnection())
               {
                  setState(cStateReconnect);
                  break;
               }

               if (mpCurrentTask == NULL)
               {
                  setState(cStateSelectCommand);
                  break;
               }

               BMediaPacket packet(getServiceID());

               BMediaPacket::eCommands command;

               switch (mpCurrentTask->mCommand)
               {
                  case BLSPMediaTask::cCommandList:
                  case BLSPMediaTask::cCommandListFriends:
                     command = BMediaPacket::cCommandListRequest;
                     break;

                  case BLSPMediaTask::cCommandUpload:
                     command = BMediaPacket::cCommandStoreRequest;
                     break;

                  case BLSPMediaTask::cCommandDownload:
                     command = BMediaPacket::cCommandRetrieveRequest;
                     break;

                  case BLSPMediaTask::cCommandDelete:
                     command = BMediaPacket::cCommandDeleteRequest;
                     break;

                  default:
                     setState(cStateSelectCommand);
                     return;
               }

               packet.init(command, mpCurrentTask->getXuid(), mpCurrentTask->getMediaID(), mpCurrentTask->getCRC());

               if (mpCurrentTask->getHeaderStream())
               {
                  packet.mpData = (mpCurrentTask->getHeaderStream())->getBuf().getData();
                  packet.mSize = static_cast<uint16>((mpCurrentTask->getHeaderStream())->getBuf().getSizeInBytes());
               }

               //long size = cSendBufferSize;
               //Utils::FastMemSet(mSendBuffer, 0, cSendBufferSize);

               //void* pData = mSendBuffer;
               //packet.serializeInto(&pData, &size);

               send(packet, mCompress);

               mpCurrentTask->mRequestID = packet.getID();

               //HRESULT hr = mpConn->send(pData, size, mCompress);

               //// why would this fail?
               //if (FAILED(hr))
               //{
               //   mRetryTime = timeGetTime() + mRetryInterval;
               //   if (++mNumRetries > mMaxRetries)
               //      setState(cStateTimedout);
               //   return;
               //}

               mLastTime = timeGetTime();
               mWaitingOnResponse = true;
            }
            break;
         }

      //case cStateConnecting:
      //   {
      //      break;
      //   }

      //case cStateConnected:
      //   {
      //      mState = cStateSelectCommand;
      //      setRetries(10);
      //      break;
      //   }

      case cStateReconnect:
         {
            reconnect();
            break;
         }

      //case cStateConnect:
      //   {
      //      if (mpConn)
      //      {
      //         if (mpConn->isConnected())
      //            setState(cStateConnected);
      //         else
      //         {
      //            reconnect();
      //         }
      //      }
      //      else if (timeGetTime() > mRetryTime)
      //      {
      //         DWORD dwResult = WaitForSingleObject(mOverlapped.hEvent, 0);

      //         if (dwResult == WAIT_TIMEOUT)
      //         {
      //            // we only want to adjust our retry interval
      //            mRetryTime = timeGetTime() + mRetryInterval;
      //            // do not timeout here, allow the connection layer to timeout
      //            // and inform us of what happened
      //         }
      //         else if (dwResult == WAIT_OBJECT_0)
      //         {
      //            if (mOverlapped.dwExtendedError == ERROR_SUCCESS)
      //            {
      //               BLSPConnectionHandler* pHandler = gLSPManager.getConnection(cTCP, mPort);

      //               if (!pHandler)
      //               {
      //                  // need error/restart condition here instead of "done"
      //                  setState(cStateReconnect);
      //               }
      //               else
      //               {
      //                  mpConn = pHandler->mpXStreamConn;
      //               }

      //               if (mpConn)
      //               {
      //                  mpConn->addObserver(this);

      //                  // the data connection has completed, we're free to begin our upload
      //                  mState = cStateConnected;
      //               }
      //               else
      //               {
      //                  // really need error state/conditions
      //                  // XXX need to figure out how many times we want to attempt the reconnect
      //                  // and the interval between attemps
      //                  setState(cStateReconnect);
      //               }
      //            }
      //            else
      //            {
      //               // what to do if the underlying xnet/connection code failed?
      //               // should I attempt to have it re-establish or simply fail?
      //               // I think the underlying layers should be the ones to attempt a retry
      //               // and if they've failed, assume there's nothing I could do at this point
      //               // other than cleanup and fail/log/alert
      //               setState(cStateTimedout);
      //            }
      //         }
      //         else
      //         {
      //            // need a better state/error for this
      //            setState(cStateTimedout);
      //         }
      //      }
      //      break;
      //   }

      case cStateTimedout:
         {
            // right now, we've tried our best to upload the stats log, at this point we simply have to fail
            // when we have multiple upload stats boxes, we can roll-over and try another one
            if (timeGetTime() > mRetryTime)
               start();
            break;
         }

      case cStateSelectCommand:
         {
            selectCommand();
            break;
         }

      case cStateInit:
         {
            connect();
            break;
         }

      case cStateServerPending:
         {
            if ((timeGetTime() - mLastTime) > 120000)
            {
               reconnect();
            }
            break;
         }

      case cStateDisconnected:
         {
            cleanup();
            setState(cStateDone);
            break;
         }
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::selectCommand()
{
   if (mpCurrentTask != NULL)
   {
      // XXX issue an error to the component that originally requested this command
      mTasks.remove(mpCurrentTask);
      BLSPMediaTask::releaseInstance(mpCurrentTask);

      mpCurrentTask = NULL;

      mWaitingOnResponse = false;
   }

   if (mTasks.size() == 0)
   {
      setState(cStateDone);
   }
   else
   {
      // pick the first command from the list
      BLSPMediaTask* pCommand = NULL;
      uint count = mTasks.getSize();
      for (uint i=0; i < count; ++i)
      {
         BLSPMediaTask* pTemp = mTasks[i];
         if (pTemp && pTemp->mState == BLSPMediaTask::cStatePending)
         {
            pCommand = pTemp;
            break;
         }
      }

      if (!pCommand)
      {
         for (uint i=count-1; i >= 0; --i)
         {
            BLSPMediaTask* pTemp = mTasks[i];
            if (!pTemp)
            {
               mTasks.removeIndex(i);
            }
            else if (pTemp->mState == BLSPMediaTask::cStateDelete)
            {
               mTasks.removeIndex(i);
               BLSPMediaTask::releaseInstance(pTemp);
            }
         }
      }
      else
      {
         if (pCommand->getContent() != NULL && pCommand->getStream() == NULL)
         {
            mTasks.remove(pCommand);
            BLSPMediaTask::releaseInstance(pCommand);
            return;
         }

         mpCurrentTask = pCommand;
         mpCurrentTask->mState = BLSPMediaTask::cStateServerPending;

         mWaitingOnResponse = false;

         if (mpCurrentTask->mCommand == BLSPMediaTask::cCommandListFriends)
            setState(cStatePreRequest);
         else
            setState(cStateRequest);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::cleanup()
{
   mStopped = true;
   mWaitingOnResponse = false;
   //mUploading = false;

   uint count = mTasks.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPMediaTask* pCommand = mTasks[i];
      if (!pCommand)
         continue;
      BLSPMediaTask::releaseInstance(pCommand);
   }
   mTasks.clear();
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::setState(eState state)
{
   if (mState == state)
      return;
   mState = state;
   if (mState == cStateTimedout)
   {
      // if we timeout, then stop everything
      // will resume in a few
      stop();
      setRetryInterval(2500);
      mStopped = false; // we're not actually dropped
      mState = state; // redo the state since stop() sets us to cStateStop
   }
   //else if (mState == cStateIdle)
   //   mUploading = false;
   else if (mState == cStateDone)
   {
      //mUploading = false;
      mStopped = true;
   }
   else if (mState == cStateRequest)
   {
      setRetries(2, 5000);
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::setRetries(uint maxRetries, DWORD interval)
{
   mNumRetries = 0;
   mMaxRetries = maxRetries;
   mRetryInterval = (interval == 0 ? 2500 : interval);
   mRetryTime = 0;
}

//==============================================================================
// 
//==============================================================================
void BLSPMediaTransfer::setRetryInterval(DWORD interval)
{
   mRetryInterval = interval;
   mRetryTime = timeGetTime() + mRetryInterval;
}

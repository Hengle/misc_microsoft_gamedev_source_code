//==============================================================================
// lspServiceRecord.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#include "common.h"
#include "configsgame.h"
#include "user.h"
#include "usermanager.h"
#include "userprofilemanager.h"

#include "lspManager.h"
#include "lspServiceRecord.h"

// xcore
#include "stream\byteStream.h"

// xnetwork
#include "NetPackets.h"
#include "MaxSendSize.h"

uint BLSPServiceRecordTask::mNextID = 0;

//==============================================================================
// 
//==============================================================================
BLSPServiceRecordTask::BLSPServiceRecordTask() :
   mXuid(INVALID_XUID),
   mpServiceRecordCache(NULL),
   mID(0),
   mRequestID(0),
   mTimeout(0),
   mCommand(cCommandNone),
   mState(cStateIdle)
{
}

//==============================================================================
// 
//==============================================================================
uint BLSPServiceRecordTask::init(BLSPServiceRecordCache* pServiceRecordCache, BServiceRecordCommand command, uint timeout, XUID xuid)
{
   mXuid = xuid;
   mpServiceRecordCache = pServiceRecordCache;
   mTimeout = timeout;
   mID = ++mNextID;
   mCommand = command;
   mState = cStatePending;

   return mID;
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecordTask::queryResponse(const void* pData, int32 size, uint ttl)
{
   if (pData == NULL || size == 0)
      return;

   // parse the data portion looking for the level
   // <sr l='[0-50]'/>
   BByteStream* pStream = HEAP_NEW(BByteStream, gSimHeap);
   BXMLReader* pReader = HEAP_NEW(BXMLReader, gSimHeap);

   pStream->set(pData, size);

   if (pReader->load(pStream, cXFTXML))
      queryResponse(*pReader, ttl);

   HEAP_DELETE(pReader, gSimHeap);
   HEAP_DELETE(pStream, gSimHeap);
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecordTask::queryResponse(BXMLReader& reader, uint ttl)
{
   // <sr l='[0-50]'/>
   BXMLNode node = reader.getRootNode();

   if (node.getName().compare("sr") == 0)
   {
      uint32 level=0;
      if (node.getAttribValueAsUInt32("l", level))
         queryResponse(level, ttl);
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecordTask::queryResponse(uint level, uint ttl)
{
   // attempt to update the user profile with the new level information
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser == NULL)
      return;
   if (pUser->getXuid() != mXuid)
      return;

   BUserProfile* pProfile = pUser->getProfile();
   if (pProfile == NULL)
      return;

   if (mpServiceRecordCache)
      mpServiceRecordCache->addUser(mXuid, level, ttl);

   pProfile->updateMatchmakingLevel(level);
}

//==============================================================================
// 
//==============================================================================
BLSPServiceRecord::BLSPServiceRecord(BLSPServiceRecordCache* pServiceRecordCache) :
   BLSPTitleServerConnection(cDefaultLSPServiceRecordPort, cConfigLSPServiceRecordPort, cDefaultLSPServiceRecordServiceID, cConfigLSPServiceRecordServiceID),
   mpCurrentTask(NULL),
   mpServiceRecordCache(pServiceRecordCache),
   mDefaultTTL(cDefaultServiceRecordTTL),
   mLastTime(0),
   mRetryTime(0),
   mRetryInterval(2500),
   mNumRetries(0),
   mMaxRetries(1),
   mState(cStateIdle),
   mStopped(true),
   mWaitingOnResponse(false),
   mEnabled(false)
{
   long ttl;
   if (gConfig.get(cConfigLSPDefaultServiceRecordTTL, &ttl))
      mDefaultTTL = static_cast<uint>(ttl);
}

//==============================================================================
// 
//==============================================================================
BLSPServiceRecord::~BLSPServiceRecord()
{
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPServiceRecord::sendReady(BXStreamConnection& conn)
{
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPServiceRecord::connected(BXStreamConnection& connection)
{
   BLSPTitleServerConnection::connected(connection);

   if (mpCurrentTask == NULL || mpCurrentTask->getState() == BLSPServiceRecordTask::cStateDelete)
   {
      setState(cStateSelectCommand);
      return;
   }

   mWaitingOnResponse = false;

   setState(cStateRequest);
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPServiceRecord::dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size)
{
   mLastTime = timeGetTime();

   switch (type)
   {
      case BPacketType::cServiceRecordPacket:
         {
            BServiceRecordPacket packet;
            packet.deserializeFrom(pData, size);

            BLSPServiceRecordTask* pTask = NULL;
            for (uint i=0; i < mTasks.getSize(); ++i)
            {
               if (mTasks[i]->getRequestID() == packet.getID())
               {
                  pTask = mTasks[i];
                  break;
               }
            }

            if (pTask == NULL)
               return;

            switch (packet.mCommand)
            {
               case BServiceRecordPacket::cCommandSuccess:
                  {
                     // parse the data portion looking for the level
                     // <sr l='[0-50]'/>
                     pTask->queryResponse(packet.mpData, packet.mSize, packet.mTTL);
                  }
                  break;

               default:
                  break;
            }

            // received something other than success

            pTask->setState(BLSPServiceRecordTask::cStateDelete);

            setState(cStateSelectCommand);
         }
         break;

      default:
         return;
   }
}

//==============================================================================
// BXStreamConnection::BXStreamObserver
//==============================================================================
void BLSPServiceRecord::disconnected(uint status)
{
   BLSPTitleServerConnection::disconnected(status);

   mState = cStateDisconnected;
}

//==============================================================================
// BXStreamConnection::BXStreamObserver
//==============================================================================
void BLSPServiceRecord::shutdown()
{
   BLSPTitleServerConnection::shutdown();

   // my connection is going away, cleanup what we can and go away
   setState(cStateDone);

   mpCurrentTask = NULL;

   mWaitingOnResponse = false;

   uint count = mTasks.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPServiceRecordTask* pCommand = mTasks[i];
      if (!pCommand)
         continue;
      HEAP_DELETE(pCommand, gSimHeap);
   }

   mTasks.clear();

   BLSPTitleServerConnection::shutdown();
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecord::query(XUID xuid)
{
   PBLSPServiceRecordUser pUser = mpServiceRecordCache->getUser(xuid);
   if (pUser == NULL)
      mpServiceRecordCache->addUser(xuid, 0, mDefaultTTL);
   else
      pUser->mLastUpdate = timeGetTime();

   BLSPServiceRecordTask* pTask = HEAP_NEW(BLSPServiceRecordTask, gSimHeap);
   pTask->init(mpServiceRecordCache, BLSPServiceRecordTask::cCommandQuery, 20000, xuid);
   mTasks.add(pTask);

   mStopped = false;

   BXStreamConnection* pConn = getConnection();
   if (!pConn || !pConn->isConnecting())
      connect();
   else if (isFinished())
      setState(cStateSelectCommand);
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecord::start()
{
   mStopped = false;

   setState(cStateSelectCommand);
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecord::stop()
{
   mStopped = true;

   // shut everything down and go dark until the game tells us otherwise
   shutdown();

   setState(cStateStop);
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecord::reset()
{
   stop();

   // want to start up again but wait 10 seconds
   start();
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecord::service()
{
   if (mStopped)
      return;

   switch (mState)
   {
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
               if (!getConnection())
               {
                  setState(cStateDisconnected);
                  break;
               }

               if (mpCurrentTask == NULL)
               {
                  setState(cStateSelectCommand);
                  break;
               }

               BServiceRecordPacket packet(getServiceID());

               BServiceRecordPacket::eCommands command;

               switch (mpCurrentTask->getCommand())
               {
                  case BLSPServiceRecordTask::cCommandQuery:
                     command = BServiceRecordPacket::cCommandRequest;
                     break;

                  default:
                     setState(cStateSelectCommand);
                     return;
               }

               packet.init(command, mpCurrentTask->getXuid());

               send(packet);

               mpCurrentTask->setRequestID(packet.getID());

               mLastTime = timeGetTime();
               mWaitingOnResponse = true;
            }
            break;
         }

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
               setState(cStateDisconnected);
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
void BLSPServiceRecord::selectCommand()
{
   if (mpCurrentTask != NULL)
   {
      mTasks.remove(mpCurrentTask);
      HEAP_DELETE(mpCurrentTask, gSimHeap);

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
      BLSPServiceRecordTask* pCommand = NULL;
      uint count = mTasks.getSize();
      for (uint i=0; i < count; ++i)
      {
         BLSPServiceRecordTask* pTemp = mTasks[i];
         if (pTemp && pTemp->getState() == BLSPServiceRecordTask::cStatePending)
         {
            pCommand = pTemp;
            break;
         }
      }

      if (!pCommand)
      {
         for (uint i=count-1; i >= 0; --i)
         {
            BLSPServiceRecordTask* pTemp = mTasks[i];
            if (!pTemp)
            {
               mTasks.removeIndex(i);
            }
            else if (pTemp->getState() == BLSPServiceRecordTask::cStateDelete)
            {
               mTasks.removeIndex(i);
               HEAP_DELETE(pTemp, gSimHeap);
            }
         }
      }
      else
      {
         mpCurrentTask = pCommand;
         mpCurrentTask->setState(BLSPServiceRecordTask::cStateServerPending);

         mWaitingOnResponse = false;

         setState(cStateRequest);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecord::cleanup()
{
   mStopped = true;
   mWaitingOnResponse = false;

   mpCurrentTask = NULL;

   uint count = mTasks.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPServiceRecordTask* pCommand = mTasks[i];
      if (!pCommand)
         continue;
      HEAP_DELETE(pCommand, gSimHeap);
   }
   mTasks.clear();
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecord::setState(eState state)
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
   else if (mState == cStateDone)
   {
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
void BLSPServiceRecord::setRetries(uint maxRetries, DWORD interval)
{
   mNumRetries = 0;
   mMaxRetries = maxRetries;
   mRetryInterval = (interval == 0 ? 2500 : interval);
   mRetryTime = 0;
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecord::setRetryInterval(DWORD interval)
{
   mRetryInterval = interval;
   mRetryTime = timeGetTime() + mRetryInterval;
}

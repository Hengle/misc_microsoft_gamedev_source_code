//==============================================================================
// FileTransferMgr.cpp
//
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "FileTransferMgr.h"
#include "Client.h"
#include "Session.h"
#include "filestream.h"

uint gFileTransferManagerDummy;  // disable warning LNK4221: no public symbols found; archive member will be inaccessible

//==============================================================================
// Defines
/*
#define CREATESTREAMID(from, to) (((unsigned char)from&0x0F)<<4)|((unsigned char)to&0x0F)
#define GETCLIENTIDFROM(streamID) ((unsigned char)streamID)>>4)&0x0F
#define GETCLIENTIDTO(streamID) ((unsigned char)streamID))&0x0F

BFileTransferGameInterface::~BFileTransferGameInterface()
{
   if (mFileTransferMgr)
   {
      mFileTransferMgr->removeGameInterface(this);
      mFileTransferMgr = NULL;
   }
}

//==============================================================================
// BFileTransferMgr::BFileTransferMgr
//==============================================================================
BFileTransferMgr::BFileTransferMgr(BSession* session, BFileTransferPlayerInterface* playerInterface) :  
   mSession(session), 
   mPlayerInterface(playerInterface),
   mGameInterface(NULL)
{
   BFATAL_ASSERT(session);
   BFATAL_ASSERT(playerInterface);

   session->addObserver(this);
}

//==============================================================================
// BFileTransferMgr::~BFileTransferMgr
//==============================================================================
BFileTransferMgr::~BFileTransferMgr()
{ 
   if (mGameInterface)
   {
      mGameInterface->attachFileTransferMgr(NULL);
      mGameInterface = NULL;
   }

   for (long idx=0; idx<BSession::cMaxClients; idx++)
   {
      BDynamicSimArray<BFileStream*> &stream = mStreams[idx];
      for (long i=0; i < stream.getNumber(); i++)
         BDELETE(stream[i]);
   }
   mSession->removeObserver(this);
}

//==============================================================================
// BFileTransferMgr::getClientID
//==============================================================================
DWORD BFileTransferMgr::getClientID(long playerID)
{
   return mPlayerInterface->getClientIDFromPlayerID(playerID);
}

//==============================================================================
// BFileTransferMgr::getPlayerID
//==============================================================================
long BFileTransferMgr::getPlayerID(DWORD clientID)
{   
   return mPlayerInterface->getPlayerIDFromClientID(clientID);
}

//==============================================================================
// BFileTransferMgr::service
//==============================================================================
void BFileTransferMgr::service(void)
{
   for (DWORD clientID=0; clientID<BSession::cMaxClients; clientID++)
   {
      BDynamicSimArray<BFileStream*> &streams = mStreams[clientID];

      for (long idx=streams.getNumber()-1; idx>=0; --idx)
      {
         BFileStream *stream = streams[idx];
         if (stream)
         {
            long state = stream->service();

            if (state == BDataStream::cStateTimedout)
            {
               nlog(cFileTransferNL, "BFileTransferMgr::service -- stream i:[%d] ID:[%d] timed out for client [%d].", idx, stream->getStreamID(), clientID);
               removeTransfer(clientID, stream->getStreamID(), cResultTimeout);
               continue;
            }
            else if (state == BDataStream::cStateComplete)
            {
               nlog(cFileTransferNL, "BFileTransferMgr::service -- stream i:[%d] ID:[%d] is complete for client [%d].", idx, stream->getStreamID(), clientID);
               removeTransfer(clientID, stream->getStreamID(), cResultComplete);
               continue;
            }

            if (mGameInterface)
               mGameInterface->transferUpdate(clientID, stream->getStreamID(), stream->getFilePercent());
         }
      }
   }   
}

//==============================================================================
// BFileTransferMgr::startBatch
//==============================================================================
bool BFileTransferMgr::startBatch(const BCHAR_T *name, DWORD clientID)
{
   if (!mSession || !mGameInterface)
   {
      return(false);
   }

   BClient* pClient = mSession->getClient(clientID);
   if (!pClient)
   {
      BASSERT(0);
      return(false);
   }

   // make sure not the local client
   if (pClient->isLocal())
   {
      BASSERT(0);
      return(false);
   }

   BFileStreamPacket packet(BFileStreamPacket::cStartBatch, name, 0);   
   HRESULT hr = pClient->SendPacket(packet);
   if (FAILED(hr))
   {
      nlog(cFileTransferNL, "BFileTransferMgr::startBatch -- failed to send packet. HR 0x%x", hr);
      BASSERT(0);
   }

   return(true);
}

//==============================================================================
// BFileTransferMgr::stopBatch
//==============================================================================
bool BFileTransferMgr::stopBatch(const BCHAR_T *name, DWORD clientID)
{
   if (!mSession || !mGameInterface)
   {
      return(false);
   }

   BClient* pClient = mSession->getClient(clientID);
   if (!pClient)
   {
      BASSERT(0);
      return(false);
   }

   // make sure not the local client
   if (pClient->isLocal())
   {
      BASSERT(0);
      return(false);
   }

   BFileStreamPacket packet(BFileStreamPacket::cStopBatch, name, 0);   
   HRESULT hr = pClient->SendPacket(packet);
   if (FAILED(hr))
   {
      nlog(cFileTransferNL, "BFileTransferMgr::stopBatch -- failed to send packet. HR 0x%x", hr);
      BASSERT(0);
   }

   return(true);
}

//==============================================================================
// BFileTransferMgr::sendFile
//==============================================================================
bool BFileTransferMgr::sendFile(const BCHAR_T *filename, DWORD clientID)
{
   // local client sends the file to the specified client
   if (!mSession || !mGameInterface || !filename)
   {
      return(false);
   }

   // find file in save directories
   long type, size, dirID;
   DWORD crc;

   if (!mGameInterface->getFileInfo(filename, dirID, type, size, crc))
   {
      return(false);
   }

   BClient* pClient = mSession->getClient(clientID);
   if (!pClient)
   {
      BASSERT(0);
      return(false);
   }

   // make sure not the local client
   if (pClient->isLocal())
   {
      BASSERT(0);
      return(false);
   }

   BFileStream *pStream = NULL;

   // TODO: search our existing file streams to see if we're already
   // transmitting this file
   BDynamicSimArray<BFileStream*> &streams = mStreams[clientID];
   for (long i=streams.getNumber()-1; i >= 0; --i)
   {
      pStream = streams.get(i);
      if (pStream)
      {
         BCHAR_T *name = pStream->getFilename();
         if (name)
         {
            long srcLength = strLength(name);
            if (strCompare(name, srcLength, filename, strLength(filename), false) == 0)
            {
               nlog(cFileTransferNL, "BFileTransferMgr::sendFile -- transfer of %s to client [%d] already found. Aborting.", BStrConv::toA(filename), clientID);
               removeTransfer(clientID, pStream->getStreamID(), cResultAbort);
               break;
            }
         }
      }
   }

   // get the next available streamID
   unsigned char streamID = CREATESTREAMID(mSession->getLocalClientID(), clientID);

   // create a new stream
   pStream = new BFileStream(pClient, streamID);
   if (!pStream)
   {
      BASSERT(0);
      return(false);
   }

   BFileStreamPacket packet(BFileStreamPacket::cStartSend, filename, size, crc, dirID, type, streamID);

   // initialize the file send
   if (!pStream->startFileSend(dirID, filename, size))
   {
      delete(pStream);
      return(false);
   }

   mStreams[clientID].add(pStream);

   // start a file transfer on the specified client
   HRESULT hr = pClient->SendPacket(packet);
   if (FAILED(hr))
   {
      nlog(cFileTransferNL, "BFileTransferMgr::sendFile -- failed to send packet. HR 0x%x", hr);
      BASSERT(0);
   }

   nlog(cFileTransferNL, "BFileTransferMgr::sendFile -- transfer of %s to client [%d] started.", BStrConv::toA(filename), clientID);
   if (mGameInterface)
      mGameInterface->transferStarted(clientID, pStream->getStreamID());

   return(true);
}

//==============================================================================
// BFileTransferMgr::requestFile
//==============================================================================
bool BFileTransferMgr::requestFile(BClient *client, const BCHAR_T* filename, DWORD localCRC)
{
   if (!mSession || !mGameInterface || !client)
   {
      return(false);
   }

   long type, size=0, dirID;
   DWORD crc=0;

   mGameInterface->getFileInfo(filename, dirID, type, size, crc);

   BFileStreamPacket packet(BFileStreamPacket::cRequestFile, filename, size, localCRC);

   // ASSERT that we aren't sending a file request to ourself
   BASSERT(!client->isLocal());

   HRESULT hr = client->SendPacket(packet);
   if (FAILED(hr))
   {
      nlog(cFileTransferNL, "BFileTransferMgr::requestFile -- failed to send packet. HR 0x%x", hr);
      BASSERT(0);
   }

   return(true);
}

//==============================================================================
// BFileTransferMgr::requestFile
//==============================================================================
bool BFileTransferMgr::requestFile(DWORD clientID, const BCHAR_T* filename, DWORD localCRC, long& streamID)
{
   if (requestFile(mSession->getClient(clientID), filename, localCRC))
   {
      streamID = CREATESTREAMID(clientID, mSession->getLocalClientID());
      return true;
   }

   streamID = -1;
   return false;
}

//==============================================================================
// BFileTransferMgr::requestFile
//==============================================================================
bool BFileTransferMgr::requestFile(const BCHAR_T* filename, DWORD localCRC)
{
   return requestFile(mSession->getHostClient(), filename, localCRC);
}

//==============================================================================
// BFileTransferMgr::requestFile
//==============================================================================
bool BFileTransferMgr::requestFileCRC(const BCHAR_T* filename, DWORD clientID)
{
   if (!mSession || !mGameInterface)
   {
      return(false);
   }

   long type, size=0, dirID;
   DWORD crc=0;

   mGameInterface->getFileInfo(filename, dirID, type, size, crc);

   BFileStreamPacket packet(BFileStreamPacket::cRequestCRC, filename, size, crc, dirID, type);

   // lookup the specified client
   if (mSession->getClient(clientID))
   {
      // make sure not the local client
      BASSERT(!mSession->getClient(clientID)->isLocal());

      // start a file transfer on the specified client
      HRESULT hr = mSession->getClient(clientID)->SendPacket(packet);
      if (FAILED(hr))
         nlog(cFileTransferNL, "BFileTransferMgr::requestFileCRC -- failed to send packet. HR 0x%x", hr);

      return(true);
   }

   return(false);
}

//==============================================================================
// BFileTransferMgr::processSessionEvent
//==============================================================================
void BFileTransferMgr::processSessionEvent(const BSessionEvent* pEvent)
{
   switch (pEvent->mEventID)
   {
      case BSession::cEventClientData:
      {
         DWORD clientID = (DWORD)pEvent->mData1;
         long streamID = -1;
         const void *data = SESSIONEVENT_DATA(pEvent);
         DWORD size = SESSIONEVENT_DATASIZE(pEvent);

         if (BTypedPacket::getType(data) == BPacketType::cDataStreamPacket)
         {
            BDataBlockPacket packet(BPacketType::cDataStreamPacket);
            packet.deserializeFrom(data, size);

            streamID = packet.getStreamID();
         }
         else if (BTypedPacket::getType(data) == BPacketType::cDataBlockFlowControlPacket)
         {
            BDataFlowControlPacket packet;
            packet.deserializeFrom(data, size);

            streamID = packet.getStreamID();
         }

         if (!handleClientData(clientID, data, size) && (streamID != -1))
         {
            BFileStream *stream = getStream(clientID, streamID);
            if (stream)
               stream->dataAvailable(data, size);
         }
      }
      break;

      case BSession::cEventClientDisconnect:
      {
         nlog(cFileTransferNL, "BFileTransferMgr::processSessionEvent -- client [%d] disconnected.", pEvent->mData1);
         clientDisconnected((DWORD)pEvent->mData1);
      }
      break;
   }
}

//==============================================================================
// BFileTransferMgr::removeTransfer
//==============================================================================
void BFileTransferMgr::removeTransfer(DWORD clientID, long streamID, long result)
{
   nlog(cFileTransferNL, "BFileTransferMgr::removeTransfer -- client [%d] stream [%d] result [%d].", clientID, streamID, result);
   BFileStream *stream = getStream(clientID, streamID);
   if (stream)
   {
      stream->stopFileTransfer();
      mStreams[clientID].remove(stream);
      delete stream;
   }

   if (mGameInterface)
      mGameInterface->transferComplete(clientID, streamID, result);
}

//==============================================================================
// BFileTransferMgr::handleClientData
//==============================================================================
bool BFileTransferMgr::handleClientData(DWORD clientIndex, const void *data, const DWORD size)
{
   if (BTypedPacket::getType(data) == BPacketType::cFileStreamPacket)
   {
      BFileStreamPacket packet;
      packet.deserializeFrom(data, size);

      switch (packet.mFileCommand)
      {
         case BFileStreamPacket::cRequestFile:
         {
            if (mGameInterface)
               mGameInterface->transferRequest(packet.mFileName, clientIndex, packet.mFileCRC);
         }
         break;

         case BFileStreamPacket::cRequestCRC:
         {
            if (!mGameInterface)
               break;

            DWORD crc=0;
            mGameInterface->getFileCRCInfo(packet.mFileName, packet.mFileDirID, crc);
            BFileStreamPacket response(BFileStreamPacket::cCRCResult, packet.mFileName, packet.mFileSize, crc);

            BClient *pClient = mSession->getClient(clientIndex);
            if (!pClient)
            {
               BASSERT(0);
               break;
            }

            HRESULT hr = pClient->SendPacket(response);
            if (FAILED(hr))
            {
               nlog(cFileTransferNL, "BFileTransferMgr::handleClientData -- failed to send packet. HR 0x%x", hr);
            }
         }
         break;

         case BFileStreamPacket::cCRCResult:
         {
            if (mGameInterface)
               mGameInterface->transferCRCResult(packet.mFileName, clientIndex, packet.mFileCRC);
         }
         break;         

         case BFileStreamPacket::cStartSend:
         {
            if (!mGameInterface)
               break;

            BClient *pClient = mSession->getClient(clientIndex);
            if (!pClient)
            {
               BASSERT(0);
               break;
            }

            // create a new stream
            BFileStream *pStream = new BFileStream(pClient, packet.mStreamID);
            if (!pStream)
            {
               BASSERT(0);
               break;
            }

            long dirID = packet.mFileDirID;
            BSimString filename = packet.mFileName;
            if (mGameInterface)
            {
               // give the receive side an opportunity to override the destination.
               mGameInterface->transferStartFileRecv(clientIndex, packet.mStreamID, dirID, filename);
            }

            // initialize the file receive
            // if (!pStream->startFileRecv(packet.mFileDirID, packet.mFileName, packet.mFileSize, packet.mFileCRC))
            if (!pStream->startFileRecv(dirID, filename, packet.mFileSize, packet.mFileCRC))
            {
               delete(pStream);
               break;
            }

            // add to be serviced
            mStreams[clientIndex].add(pStream);

            //mFileStreams[clientIndex] = pStream;

            if (mGameInterface)
               mGameInterface->transferStarted(clientIndex, pStream->getStreamID());
         }
         break;

         case BFileStreamPacket::cStartBatch:
            if (mGameInterface)
               mGameInterface->transferBatchStart(packet.mFileName, clientIndex);
         break;

         case BFileStreamPacket::cStopBatch:
            if (mGameInterface)
               mGameInterface->transferBatchStop(packet.mFileName, clientIndex);
         break;

         default:
         break;
      }
      return(true);
   }
   return(false);
}


//==============================================================================
// BFileTransferMgr::clientDisconnected
//==============================================================================
void BFileTransferMgr::clientDisconnected(DWORD clientID)
{
   // remove the streams for the client
   BDynamicSimArray<BFileStream*> &streams = mStreams[clientID];
   for (long i=streams.getNumber()-1; i >= 0; --i)
   {
      BFileStream *stream = streams[i];
      if (stream)
         removeTransfer(clientID, stream->getStreamID(), cResultAbort);
   }

   if (mGameInterface)
      mGameInterface->clientDisconnected(clientID);
}

//==============================================================================
// BFileTransferMgr::clientDisconnected
//==============================================================================
BFileStream* BFileTransferMgr::getStream(DWORD clientID, long streamID)
{
   if (clientID < 0)
      return NULL;

   BFileStream *stream = NULL;
   for (long i=0; i < mStreams[clientID].getNumber(); ++i)
   {
      stream = mStreams[clientID].get(i);
      if (stream && stream->getStreamID() == streamID)
         return stream;
   }
   return NULL;
}
*/

//==============================================================================
// mpLANIPConnector.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "multiplayercommon.h"
#include "mplanipconnector.h"

//==============================================================================
// Const declarations

//==============================================================================
// BMPLANIPConnector::BMPLANIPConnector
//==============================================================================
BMPLANIPConnector::BMPLANIPConnector(const SOCKADDR_IN &local, const SOCKADDR_IN &xlated, DWORD localCRC) :
   mpConnector(NULL), mDirectIPJoin(false), mLocalCRC(localCRC), mDirectIPSearchTime(0)
{
   mpConnector = new BSessionConnector(this);
   
   memcpy(&mLocalAddr, &local, sizeof(mLocalAddr));
   memcpy(&mXlatedLocalAddr, &xlated, sizeof(mXlatedLocalAddr));
}

//==============================================================================
// BMPLANIPConnector::~BMPLANIPConnector
//==============================================================================
BMPLANIPConnector::~BMPLANIPConnector()
{
   if (mpConnector)
      delete mpConnector;
   mpConnector = NULL;
   mDirectIPJoin = false;
}

//==============================================================================
// BMPLANIPConnector::service
//==============================================================================
void BMPLANIPConnector::service(void)
{
   if (mpConnector)
      mpConnector->service();

   if (mDirectIPJoin)
   {
      DWORD time = timeGetTime();
      if ((time - mDirectIPSearchTime) > cDirectIPTimeout)
      {
         mObservers.joinReply(NULL, BMPConnectorObserver::cResultGameNotFound);
         mDirectIPJoin = false;
      }
   }
}

//==============================================================================
// BMPLANIPConnector::joinGame
//==============================================================================
bool BMPLANIPConnector::joinGame(long index)
{
   if (index<0 || index>=mResults.getNumber())
      return(false);

   mDirectIPJoin = false;

   HRESULT hr = mpConnector->join(index, mLocalAddr, mXlatedLocalAddr, mLocalCRC, mNickname);
   if (FAILED(hr))
      blog("BMPLANIPConnector::JoinGame -- mpConnector->join failed HR:0x%x", hr);


   return(true);
}

//==============================================================================
// BMPLANIPConnector::joinGame
//==============================================================================
DWORD BMPLANIPConnector::joinGame(const BSimString& ipaddress)
{
   if (ipaddress.length() == 0)
      return(0);

   long inaddr = inet_addr( ipaddress.getPtr() );
   if ( inaddr == INADDR_NONE )
      return(0);

   memset( &mDirectIPAddr, 0, sizeof( mDirectIPAddr ) );
   mDirectIPAddr.sin_family = AF_INET;
   mDirectIPAddr.sin_addr.S_un.S_addr = inaddr;

   HRESULT hr = mpConnector->findIP(mDirectIPAddr);
   if (FAILED(hr))
      blog("BMPLANIPConnector::joinGame -- mpConnector->findIP failed HR:0x%x", hr);

   mDirectIPJoin = true;
   mDirectIPSearchTime = timeGetTime();
   return(cDirectIPTimeout);
}

//==============================================================================
// BMPLANIPConnector::refreshGameList
//==============================================================================
void BMPLANIPConnector::refreshGameList(void)
{
   if (!mDirectIPJoin)
   {
      // why do we have to do this? Because the session connector needs work.
      delete mpConnector;
      mpConnector = new BSessionConnector(this);

      SOCKADDR_IN findAddr;
      memset(&findAddr, 0, sizeof(findAddr));
      findAddr.sin_addr.S_un.S_addr = INADDR_BROADCAST;
      findAddr.sin_family = AF_INET;
      HRESULT hr = mpConnector->find( findAddr );      
      if (FAILED(hr))
         blog("BMPLANIPConnector::refreshGameList -- mpConnector->find failed HR:0x%x", hr);
   }
}

//==============================================================================
// BMPLANIPConnector::findListUpdated
//==============================================================================
void BMPLANIPConnector::findListUpdated(BSessionConnector *connector)
{
   long count = connector->getSessionDescriptorAmount();
   if (mDirectIPJoin)
   {
      // look through the session descriptors for the ip we are trying to connect to
      for (long idx=0; idx<count; idx++)
      {
         BSessionDescriptor *desc = connector->getSessionDescriptor(idx);
         if (!desc)
            continue;

         if ( (memcmp(&desc->mRemoteAddress.sin_addr, &mDirectIPAddr.sin_addr, sizeof(mDirectIPAddr.sin_addr)) == 0) || 
              (memcmp(&desc->mTranslatedRemoteAddress.sin_addr, &mDirectIPAddr.sin_addr, sizeof(mDirectIPAddr.sin_addr)) == 0)) 
         {
            HRESULT hr = mpConnector->joinIP(idx, mLocalAddr, mXlatedLocalAddr, mLocalCRC, mNickname);
            if (FAILED(hr))
               blog("BMPLANIPConnector::findListUpdated -- mpConnector->joinIP failed HR:0x%x", hr);
            return;
         }
      }
   }
   else
   {
      mResults.setNumber(count);
      for (long idx=0; idx<count; idx++)
      {
         mResults[idx] = (BMPGameDescriptor*)connector->getSessionDescriptor(idx)->mAdvertiseInfo;
      }

      mObservers.findListUpdated(mResults);
   }
}

//==============================================================================
// BMPLANIPConnector::joinRequest
//==============================================================================
void BMPLANIPConnector::joinRequest(BSessionConnector *connector, const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, eJoinResult &result)
{
   connector; name; crc; remoteAddress; translatedRemoteAddress; result;
}

//==============================================================================
// BMPLANIPConnector::joinReply
//==============================================================================
void BMPLANIPConnector::joinReply(BSessionConnector *connector, long index, eJoinResult result)
{   
   long ourResult;
   switch (result)
   {
      case cJoinOK:
         ourResult = BMPConnectorObserver::cResultJoined;
         break;

      case cJoinFull:
         ourResult = BMPConnectorObserver::cResultRejectFull;
         break;

      case cJoinCRCMismatch:
         ourResult = BMPConnectorObserver::cResultRejectCRC;
         break;
      
      case cJoinUserExists:
         ourResult = BMPConnectorObserver::cResultUserExists;
         break;

      default:
      case cJoinRejected:
         ourResult = BMPConnectorObserver::cResultRejectUnknown;
         break;
   }

   if (result != cJoinPending)
   {
      BMPGameDescriptor *desc = NULL;
      if (index>=0 && index<connector->getSessionDescriptorAmount())
         desc = (BMPGameDescriptor*)connector->getSessionDescriptor(index)->mAdvertiseInfo;

      mObservers.joinReply(desc, ourResult);

      mDirectIPJoin = false;
      mDirectIPSearchTime = 0;
   }
}

//==============================================================================
// eof: mpLANIPConnector.h
//==============================================================================

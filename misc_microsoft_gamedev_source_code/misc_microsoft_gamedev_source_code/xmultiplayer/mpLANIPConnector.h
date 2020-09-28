//==============================================================================
// mpLANIPConnector.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _MPLANIPCONNECTOR_H_
#define _MPLANIPCONNECTOR_H_

//==============================================================================
// Includes
#include "mptypes.h"
#include "ObserverList.h"
#include "SessionConnector.h"
#include "mpgamedescriptor.h"
#include "mpConnectorObserver.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
class BMPLANIPConnector : public BSessionConnector::BSCObserver,	
						  public BMPConnectorObserver
{
public:
	/*
   class BMPConnectorObserver
   {
      public:
         enum
         {
            cResultJoined,
            cResultRejectFull,
            cResultRejectCRC,
            cResultUserExists,
            cResultRejectUnknown,
            cResultGameNotFound,
            cResultMax
         };

         virtual void findListUpdated(const BMPGameDescriptorPtrArray &results) { results; }
         virtual void joinReply(const BMPGameDescriptor* desc, long result) { desc; result; }
   };
   */
   void           addObserver(BMPConnectorObserver* o)  { mObservers.Add(o); }
   void           removeObserver(BMPConnectorObserver* o) { mObservers.Remove(o); }

   BMPLANIPConnector(const SOCKADDR_IN &local, const SOCKADDR_IN &xlated, DWORD localCRC);
   virtual ~BMPLANIPConnector();

   virtual void   service(void);
   virtual bool   joinGame(long index);
   virtual DWORD  joinGame(const BSimString& ipaddress);
   virtual void   refreshGameList(void);
   virtual void   setNickname(const BSimString& name) { mNickname=name; }

   // BSessionConnector::BSCObserver
   virtual void   findListUpdated(BSessionConnector *connector);
   virtual void   joinRequest(BSessionConnector *connector, const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, eJoinResult &result);
   virtual void   joinReply(BSessionConnector *connector, long index, eJoinResult result);

protected:
	/*
   class BMPCOList : public BObserverList<BMPConnectorObserver>
   {
	   DECLARE_OBSERVER_METHOD (findListUpdated, (const BMPGameDescriptorPtrArray &results), (results))
      DECLARE_OBSERVER_METHOD (joinReply, (const BMPGameDescriptor* desc, long result), (desc, result))
   };
	*/
   enum
   {
      cDirectIPTimeout = 15000
   };

   SOCKADDR_IN                mLocalAddr;
   SOCKADDR_IN                mXlatedLocalAddr;
   SOCKADDR_IN                mDirectIPAddr;
   BSessionConnector          *mpConnector;
   BMPCOList                  mObservers;
   BMPGameDescriptorPtrArray  mResults;
   DWORD                      mLocalCRC;
   DWORD                      mDirectIPSearchTime;
   bool                       mDirectIPJoin;
   BSimString                    mNickname;
};

//==============================================================================
#endif // _MPLANIPCONNECTOR_H_

//==============================================================================
// eof: mpLANIPConnector.h
//==============================================================================

//==============================================================================
// mpLiveConnector.h
//
// Copyright (c) 2006, Ensemble Studios
//==============================================================================

#ifndef _MPLiveConnector_H_
#define _MPLiveConnector_H_

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
class BMPLiveConnector :  public BSessionConnector::BSCObserver,	
						  public BMPConnectorObserver
{
public:

	void           addObserver(BMPConnectorObserver* o)  { mObservers.Add(o); }
	void           removeObserver(BMPConnectorObserver* o) { mObservers.Remove(o); }

	BMPLiveConnector(const SOCKADDR_IN &local, const SOCKADDR_IN &xlated, DWORD localCRC);
	virtual ~BMPLiveConnector();

	virtual void   service(void);
	virtual bool   joinGame(long index);
	//virtual DWORD  joinGame(const BSimString& ipaddress);   //TODO: Change this to be a support function for game invites
	virtual void   refreshGameList(void);
	virtual void   setNickname(const BSimString& name) { mNickname=name; }


    // BSessionConnector::BSCObserver
    virtual void   findListUpdated(BSessionConnector *connector);
    virtual void   joinRequest(BSessionConnector *connector, const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, eJoinResult &result);
    virtual void   joinReply(BSessionConnector *connector, long index, eJoinResult result);

protected:


	enum
	{
		cDirectIPTimeout = 15000
	};

	SOCKADDR_IN                mLocalAddr;
	SOCKADDR_IN                mXlatedLocalAddr;
	BSessionConnector          *mpConnector;
	BMPCOList                  mObservers;
	BMPGameDescriptorPtrArray  mResults;
	DWORD                      mLocalCRC;
	DWORD                      mDirectIPSearchTime;
	bool                       mDirectIPJoin;
	BSimString                    mNickname;
};

//==============================================================================
#endif // _MPLiveConnector_H_

//==============================================================================
// eof: mpLiveConnector.h
//==============================================================================

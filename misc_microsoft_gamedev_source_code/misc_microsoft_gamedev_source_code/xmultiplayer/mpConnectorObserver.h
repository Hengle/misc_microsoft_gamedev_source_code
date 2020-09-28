//==============================================================================
// mpConnectorObserver.h
//
// Copyright (c) 2006, Ensemble Studios
//==============================================================================

#ifndef _MPCONNECTOROBSERVER_H_
#define _MPCONNECTOROBSERVER_H_

//==============================================================================
// Includes
#include "mptypes.h"
#include "ObserverList.h"
#include "SessionConnector.h"
#include "mpgamedescriptor.h"


//==============================================================================
//class BMPLANIPConnector : public BSessionConnector::BSCObserver
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
protected:
	class BMPCOList : public BObserverList<BMPConnectorObserver>
	{
	DECLARE_OBSERVER_METHOD (findListUpdated, (const BMPGameDescriptorPtrArray &results), (results))
	DECLARE_OBSERVER_METHOD (joinReply, (const BMPGameDescriptor* desc, long result), (desc, result))
	};
};

//==============================================================================
#endif // _MPCONNECTOROBSERVER_H_

//==============================================================================
// eof: mpConnectorObserver.h
//==============================================================================

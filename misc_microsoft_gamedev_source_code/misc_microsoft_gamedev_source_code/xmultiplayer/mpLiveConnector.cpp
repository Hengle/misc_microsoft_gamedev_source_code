//==============================================================================
// BMPLiveConnector.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "multiplayercommon.h"
#include "mpLiveConnector.h"
#include "liveSystem.h"
#include "liveSessionSearch.h"

//==============================================================================
// Const declarations

//==============================================================================
// BMPLiveConnector::BMPLiveConnector
//==============================================================================
BMPLiveConnector::BMPLiveConnector(const SOCKADDR_IN &local, const SOCKADDR_IN &xlated, DWORD localCRC) :
mDirectIPJoin(false), 
mLocalCRC(localCRC), 
mDirectIPSearchTime(0)
{

    mpConnector = new BSessionConnector(this);

	memcpy(&mLocalAddr, &local, sizeof(mLocalAddr));
	memcpy(&mXlatedLocalAddr, &xlated, sizeof(mXlatedLocalAddr));
    gLiveSystem->setLiveRequired( true );
}

//==============================================================================
// BMPLiveConnector::~BMPLiveConnector
//==============================================================================
BMPLiveConnector::~BMPLiveConnector()
{
    if (mpConnector)
        delete mpConnector;
    mpConnector = NULL;

    gLiveSystem->setLiveRequired( false );
}

//==============================================================================
// BMPLiveConnector::service
//==============================================================================
void BMPLiveConnector::service(void)
{
    //Throw an update to the SessionConnector
    if (mpConnector)
        mpConnector->service();


    //See if there are results ready from a search
    BLiveSessionSearch* currentSearch = gLiveSystem->getSessionSearch();
    if (currentSearch)
    {
        if (currentSearch->getResultsState() == BLiveSessionSearch::cSessionSearchStateResultsReady)
        {
            //We have results to process
                
            if (currentSearch->getNumberOfSearchResults()==0)
            {
                //However - the result is there are no sessions out there
                //TODO - Flag back to the UI to display that there are no sessions
                currentSearch->setResultsAsProcessed();
                return;
            }

            //Find the indexes of the properties I am looking for
            //TODO - free the data in the mResults array - MEMORY LEAK HERE!!
            long count = currentSearch->getNumberOfSearchResults();
            mResults.setNumber(count);
            for (long idx=0; idx<count; idx++)
            {
                //Map the Live session search results data into the MP game descriptor
                XSESSION_SEARCHRESULT* gameInfo = currentSearch->getSearchResultRecord( idx );
                BASSERT(gameInfo);
                mResults[idx] = new BMPGameDescriptor;

                //TODO - Find out if these dynamically change per request or not, if not - then static them up at create
                DWORD nameIndex=99999;
                for (DWORD pIndex=0;pIndex<gameInfo->cProperties;pIndex++)
                {
                    if (gameInfo->pProperties[pIndex].dwPropertyId == X_PROPERTY_GAMER_HOSTNAME)
                    {
                        nameIndex = pIndex;
                        break;
                    }
                }
                if (nameIndex != 99999)
                {
                    wchar_t name[50];
                    wcscpy_s( name, 50, (wchar_t*)gameInfo->pProperties[nameIndex].value.string.pwszData );
                    mResults[idx]->setName( name );
                }
                else
                {
                    mResults[idx]->setName( L"(not found)" );
                }

            }

            mObservers.findListUpdated(mResults);            

            //Mark them as processed
            currentSearch->setResultsAsProcessed();
        }
    }
}

//==============================================================================
// BMPLiveConnector::joinGame
//==============================================================================
bool BMPLiveConnector::joinGame(long index)
{
	//To join a session via the search index, we need a valid search
    int rowCount = gLiveSystem->getSessionSearch()->getNumberOfSearchResults();
	if (rowCount < 0)
    {
        //There are no results there
        return false;
    }
	if (index<0 || index>=rowCount)
    {
        //Out of bounds request
		return false;
    }

    XSESSION_SEARCHRESULT* sessionInfo = gLiveSystem->getSessionSearch()->getSearchResultRecord(index);
    if (sessionInfo==NULL)
    {
        //Could not find that index
        return false;
    }

    //Ok - we have the session info, we need to connect to that host
    //First hack into the session connector, clear its list, and stuff in this ONE entry which points to our desired connection
    IN_ADDR hostTargetIP;
    
    //No need for this - we are in-secure mode so the IP is not translated into secure form 0.x.y.0
    /*int result = XNetXnAddrToInAddr( &sessionInfo->info.hostAddress, &sessionInfo->info.sessionID, &hostTargetIP);
    if (result!=0)
    {
        //Error code here?  Odd - abort
        //todo log it
        return false;
    }
    */
    hostTargetIP = sessionInfo->info.hostAddress.ina;
   
    SOCKADDR_IN hostTarget;
    hostTarget.sin_addr = hostTargetIP;
    hostTarget.sin_family = AF_INET;
    hostTarget.sin_port = 2300;
    /*
    SOCKADDR_IN hostTargetAd;
    hostTargetAd = hostTarget;
    hostTargetAd.sin_port = 2289;
    
    BSessionDescriptor* newDescriptor = mpConnector->sessionListHack( hostTargetAd, hostTarget, hostTarget, NULL, 0  );

 //   const SOCKADDR_IN &advertiseAddress, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress,
 //       void *advertiseInfo, DWORD advertiseInfoSize)

*/

    HRESULT hr = mpConnector->findIP( hostTarget );
    if (FAILED(hr))
    {
        blog("BMPLiveConnector::JoinGame -- mpConnector->find failed HR:0x%x", hr);
    }

//	hr = mpConnector->joinIP(0, mLocalAddr, mXlatedLocalAddr, mLocalCRC, mNickname);
	if (FAILED(hr))
    {
		blog("BMPLiveConnector::JoinGame -- mpConnector->join failed HR:0x%x", hr);
    }




	//TODO - move the code we need from SessionConnector so that anything needed for Live is done here in this class


	return(true);
}



//==============================================================================
// BMPLiveConnector::refreshGameList
//==============================================================================
void BMPLiveConnector::refreshGameList(void)
{
	gLiveSystem->launchSessionSearch();
}





//==============================================================================
// BMPLiveConnector::findListUpdated
//==============================================================================
void BMPLiveConnector::findListUpdated(BSessionConnector *connector)
{
    //We don't get our find list updated from scanning - so this should never be called
/*	long count = connector->getSessionDescriptorAmount();
	mResults.setNumber(count);
	for (long idx=0; idx<count; idx++)
	{
		mResults[idx] = (BMPGameDescriptor*)connector->getSessionDescriptor(idx)->mAdvertiseInfo;
	}

	mObservers.findListUpdated(mResults);
    */
    connector;
}

//==============================================================================
// BMPLiveConnector::joinRequest
//==============================================================================
void BMPLiveConnector::joinRequest(BSessionConnector *connector, const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, eJoinResult &result)
{
	//WTF is this for?
	connector; name; crc; remoteAddress; translatedRemoteAddress; result;
}

//==============================================================================
// BMPLANIPConnector::joinReply
//==============================================================================
void BMPLiveConnector::joinReply(BSessionConnector *connector, long index, eJoinResult result)
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

		mDirectIPSearchTime = 0;
	}
}

//==============================================================================
// eof: BMPLiveConnector.h
//==============================================================================

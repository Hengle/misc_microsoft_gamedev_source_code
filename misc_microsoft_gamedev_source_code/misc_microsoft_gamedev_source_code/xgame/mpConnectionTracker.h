//==============================================================================
// mpConnectionTracker.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#ifndef _MPCONNECTIONTRACKER_H_
#define _MPCONNECTIONTRACKER_H_

const DWORD cMPSessionMaxConnections = 8;
const DWORD cMPDefaultConnectionInitTimeout = 8000;

//This class represents a single entry in the list
class BmpConnectionEntry
{
public:
   XNADDR      mXnAddr;
   DWORD       mLastUpdated;
   XUID        mXuid;
   bool        mInUse;

   BmpConnectionEntry();
   bool isXNAddrEqual( XNADDR* p );
   void clear();
   void set( XNADDR* pXn, XUID xuid );
};

//This class the list of approved and pending connections
//  They are removed from the list once they are fully connected or if they time out
class BmpConnectionTracker
{
public:
   BmpConnectionTracker();
   bool  addEntry( XNADDR* xnAddr, XUID xuid );
   void  dropEntry( XNADDR* xnAddr );
   void  update();

protected:
   DWORD mTimeOutDelay;
   BmpConnectionEntry mConnectionList[cMPSessionMaxConnections];

   bool  findEntry( XNADDR* xnAddr, DWORD* index );
};

#endif  //_MPCONNECTIONTRACKER_H_

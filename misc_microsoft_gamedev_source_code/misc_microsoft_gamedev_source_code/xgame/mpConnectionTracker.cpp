//--------------------------------------------------------------------------------------
// mpConnectionTracker.cpp
//
// Copyright (c) 2006 Ensemble Studios
//--------------------------------------------------------------------------------------
#include "Common.h"
#include "mpConnectionTracker.h"


BmpConnectionEntry::BmpConnectionEntry()
: mLastUpdated(0)
, mInUse(false)
, mXuid(0)
{
  memset( &mXnAddr, NULL, sizeof( mXnAddr) );
}

bool BmpConnectionEntry::isXNAddrEqual( XNADDR* p)
{
   if (!mInUse)
   {
      return false;
   }

   return (memcmp(&mXnAddr, p, sizeof(mXnAddr)) == 0);
}

void BmpConnectionEntry::clear()
{
   mLastUpdated =0;
   mInUse = false;
   mXuid = 0;
   memset( &mXnAddr, NULL, sizeof( mXnAddr) );
}

void BmpConnectionEntry::set( XNADDR* pXn, XUID xuid )
{
   memcpy( &mXnAddr, pXn, sizeof(mXnAddr) );
   mXuid = xuid;
   mInUse = true;
   mLastUpdated = timeGetTime();
}



BmpConnectionTracker::BmpConnectionTracker()
{
   mTimeOutDelay = cMPDefaultConnectionInitTimeout;
}

bool BmpConnectionTracker::addEntry( XNADDR* xnAddr, XUID xuid )
{
   DWORD index;
   if (findEntry( xnAddr, &index))
   {
      return false;
   }
   for (index=0;index<cMPSessionMaxConnections;index++)
   {
      if (!mConnectionList[index].mInUse)
      {
         mConnectionList[index].set( xnAddr, xuid );

         return true;
      }
   }
   //TODO - log that we are out of space in the list - could not add
   return false;
}

void BmpConnectionTracker::dropEntry( XNADDR* xnAddr )
{
   DWORD index;
   if (findEntry( xnAddr, &index))
   {
      mConnectionList[index].clear();
      return;
   }
}

bool BmpConnectionTracker::findEntry( XNADDR* xnAddr, DWORD* index )
{
   BmpConnectionEntry* p;
   for (DWORD i = 0; i<cMPSessionMaxConnections; i++)
   {
      p = &mConnectionList[i];
      if ((p->mInUse) &&
         (p->isXNAddrEqual( xnAddr )))
      {
         *index = i;
         return true;
      }
   }
   return false;
}

void BmpConnectionTracker::update()
{
   DWORD currentTime = timeGetTime();
   BmpConnectionEntry* p;
   for (DWORD i = 0; i<cMPSessionMaxConnections; i++)
   {
      p = &mConnectionList[i];
      if ((p->mInUse) &&
          ( (p->mLastUpdated + mTimeOutDelay) < currentTime) )
      {
         //its old - need to clear it out of the session
         //TODO!
         p->clear();
      }
   }
}

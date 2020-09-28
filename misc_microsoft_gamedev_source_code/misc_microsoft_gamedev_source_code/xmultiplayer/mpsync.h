//==============================================================================
// MPSync.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _MPSYNC_H_
#define _MPSYNC_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
class BMPSyncNotify
{
public:
   virtual bool      incomingSyncData(long fromId, long checksumID, void *checksum, long checksumSize) = 0;
   virtual void      removeSyncedPlayer(long fromId) = 0;
};

//==============================================================================
class BMPSyncObject
{
public:
   BMPSyncObject() : mNotify(NULL) {}

   virtual void      attachSyncNotify(BMPSyncNotify *notify) { mNotify=notify; }
   virtual bool      sendSyncData(long uid, void *checksum, long checksumSize) = 0;
   virtual long      getSyncedCount(void) const = 0;
   virtual void      outOfSync(void) const = 0;

protected:
   void              notifyPlayerDrop(long fromId)
   {
      if (mNotify)
         mNotify->removeSyncedPlayer(fromId);
   }

   bool              notifySyncData(long fromId, long checksumID, void *checksum, long checksumSize)
   {
      if (!mNotify)
         return(false);

      return(mNotify->incomingSyncData(fromId, checksumID, checksum, checksumSize));
   }

private:
   BMPSyncNotify    *mNotify;
};

//==============================================================================
#endif // _MPSYNC_H_

//==============================================================================
// eof: mpsync.h
//==============================================================================
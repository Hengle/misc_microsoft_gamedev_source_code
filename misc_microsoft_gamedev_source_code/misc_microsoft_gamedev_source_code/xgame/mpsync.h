//==============================================================================
// MPSync.h
//
// Copyright (c) 2003-2008, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
class BMPSyncNotify
{
public:
   virtual bool      incomingSyncData(long fromId, long checksumID, uint checksum) = 0;
   virtual void      removeSyncedPlayer(long fromId) = 0;
};

//==============================================================================
class BMPSyncObject
{
   public:
      BMPSyncObject() : mNotify(NULL) {}

      virtual void      attachSyncNotify(BMPSyncNotify *notify) { mNotify=notify; }
      virtual bool      sendSyncData(long uid, uint checksum) = 0;
      virtual long      getSyncedCount(void) const = 0;
      virtual void      outOfSync(void) const = 0;

   protected:
      void notifyPlayerDrop(long fromId)
      {
         if (mNotify)
            mNotify->removeSyncedPlayer(fromId);
      }

      bool notifySyncData(long fromId, long checksumID, uint checksum)
      {
         if (!mNotify)
            return(false);

         return (mNotify->incomingSyncData(fromId, checksumID, checksum));
      }

   private:
      BMPSyncNotify    *mNotify;
};

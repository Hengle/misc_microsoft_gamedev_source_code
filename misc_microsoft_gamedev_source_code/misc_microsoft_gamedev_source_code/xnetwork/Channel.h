//==============================================================================
// Channel.h
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes

#include "ObserverList.h"
#include "Session.h"

//==============================================================================
// Forward declarations

class BChannelPacket;
class BClient;

//==============================================================================
// Const declarations

namespace BChannelType
{
   enum eBuiltinTypes { cNoChannel = 0, cTimeSyncChannel, cNumberOfBuiltinTypes };
};

//==============================================================================
class BChannel
{
   public:
      class BChannelObserver
      {
         public:
            virtual void channelDataReceived(const long fromClientIndex, const void* data, const DWORD size) = 0;
      };

      class BChannelObserverList : public BObserverList<BChannelObserver>
      {
         DECLARE_OBSERVER_METHOD (channelDataReceived,
            (const long fromClientIndex, const void *data, const DWORD size),
            (fromClientIndex, data, size))
      };

      void           addObserver(BChannelObserver* pObserver) { mObserverList.Add(pObserver); }
      void           removeObserver(BChannelObserver* pObserver) { mObserverList.Remove(pObserver); }

      // Constructors
      BChannel();
      BChannel(long channelID, BSession* pSession, bool syncOnDisconnect=false);

      // Destructors
      virtual ~BChannel();

      virtual HRESULT                     dispose() { return S_OK; }

      void                                init(long channelID, BSession* pSession, bool syncOnDisconnect=false);

      long                                getChannelID() const { return mChannelID; }

      uint                                getNextPacketID();

      virtual void                        service() {}
      virtual long                        _SendPacket(BChannelPacket& packet, long* sizeOut=0, DWORD flags=0, const char* file=0, long line=0);
      virtual long                        _SendPacketTo(BClient* pClient, BChannelPacket& packet, long* sizeOut=0, DWORD flags=0, const char* file=0, long line=0);
      virtual long                        _SendPacketTo(BMachine* pMachine, BChannelPacket& packet, long* sizeOut=0, DWORD flags=0, const char* file=0, long line=0);

      virtual void                        channelDataReceived(const long clientIndex, const void* data, const DWORD size);

   protected:
      BChannelObserverList*               getObserverList() { return &mObserverList; }
      BSession*                           getSession() const { return mpSession; }

   private:
      BChannelObserverList                mObserverList;
      BSession*                           mpSession;
      long                                mChannelID;
      uint                                mPacketID;
      bool                                mSyncOnDisconnect : 1;

}; // BChannel

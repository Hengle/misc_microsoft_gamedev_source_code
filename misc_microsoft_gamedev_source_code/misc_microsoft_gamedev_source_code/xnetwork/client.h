//==============================================================================
// client.h
//
// Copyright (c) Ensemble Studios, 2001-2008
//==============================================================================

#pragma once

   //==============================================================================
// Includes

#include "Packet.h"
#include "UDP360Connection.h"
//#include "Ping.h"
#include "MaxSendSize.h"
#include "datastream.h"
#include "ObserverList.h"

//==============================================================================
// Forward declarations

class BSession;
//class BPing;
class BTimingHistory;

//==============================================================================
// Const declarations

#ifndef BUILD_FINAL
   #define SendData(data, size) _SendData(data, size, 0, __FILE__, __LINE__)
   #define SendPacketUnreliable(packet) _SendPacket(packet, 0, cSendUnreliable, __FILE__, __LINE__)
   #define SendPacketImmediate(packet) _SendPacket(packet, 0, cSendImmediate, __FILE__, __LINE__)
   #define SendPacket(packet) _SendPacket(packet, 0, 0, __FILE__, __LINE__)
   #define SendPacketWithFlags(packet, flags) _SendPacket(packet, 0, flags, __FILE__, __LINE__)
   #define SendPacketTo(client, packet) _SendPacketTo(client, packet, 0, 0, __FILE__, __LINE__)
   #define SendPacketImmediateTo(client, packet) _SendPacketTo(client, packet, 0, cSendImmediate, __FILE__, __LINE__)
   #define SendPacketRS(packet, sizeOut) _SendPacket(packet, sizeOut, 0, __FILE__, __LINE__)   
   #define SendPacketRSTo(client, packet, sizeOut) _SendPacketTo(client, packet, sizeOut, 0, __FILE__, __LINE__)
#else
   #define SendData(data, size) _SendData(data, size, 0)
   #define SendPacketUnreliable(packet) _SendPacket(packet, 0, cSendUnreliable)
   #define SendPacketImmediate(packet) _SendPacket(packet, 0, cSendImmediate, __FILE__, __LINE__)
   #define SendPacket(packet) _SendPacket(packet, 0, 0)
   #define SendPacketWithFlags(packet, flags) _SendPacket(packet, 0, flags)
   #define SendPacketTo(client, packet) _SendPacketTo(client, packet, 0, 0)
   #define SendPacketImmediateTo(client, packet) _SendPacketTo(client, packet, 0, cSendImmediate, __FILE__, __LINE__)
   #define SendPacketRS(packet, sizeOut) _SendPacket(packet, sizeOut, 0)
   #define SendPacketRSTo(client, packet, sizeOut) _SendPacketTo(client, packet, sizeOut, 0)
#endif


//==============================================================================
//class BClient :   public BPing::BPingTransport, 
//                  public BPing::BPingObserver,
//                  public BDataStreamTransport
class BClient
{
   public:
      //class BClientObserver
      //{
      //   public:
      //      //virtual void         clientNotResponding(BClient *client) {client;}
      //      //virtual void         clientPingUpdated(BClient *client, long ping) {client; ping;}
      //      virtual void         clientConnected(BClient *client) {client;}
      //      virtual void         clientDisconnected(BClient *client) {client;}
      //};

  //    class BClientObserverList :
		//	public BObserverList <BClientObserver>
		//{
  //  //     DECLARE_OBSERVER_METHOD (clientNotResponding,
		//		//(BClient *client),
		//		//(client))
  //  //     DECLARE_OBSERVER_METHOD (clientPingUpdated,
		//		//(BClient *client, long ping),
		//		//(client, ping))
  //       DECLARE_OBSERVER_METHOD (clientConnected,
		//		(BClient *client),
		//		(client))
  //       DECLARE_OBSERVER_METHOD (clientDisconnected,
		//		(BClient *client),
		//		(client))
		//};  

      //void           addObserver(BClientObserver *o) { mObserverList.Add(o); }
      //void           removeObserver(BClientObserver *o) { mObserverList.Remove(o); }

      // Constructors
      BClient();
      //BClient(XUID xuid, const BSimString& gamertag, const SOCKADDR_IN& addr);
      //BClient(BUDP360Connection* conn, const DWORD clientID, const DWORD controllerID, const bool host);
      //BClient(BUDP360Connection* conn, const DWORD clientID, const XUID xuid, const BSimString& gamertag, const bool host);

      // Destructors
      ~BClient();

      void           init(int32 id, XUID xuid, const BSimString& gamertag, const XNADDR& xnAddr, const SOCKADDR_IN& addr);

      // Functions
      //HRESULT        _SendData(const void* data, const long size, const DWORD flags=0, const char *file=0, long line=0);
      //HRESULT        _SendPacket(BPacket& packet, long* sizeOut, DWORD flags=0, const char* file=0, long line=0);

      int32          getID() const { return mID; }

      //DWORD          getControllerID() const { return mControllerID; }
      XUID           getXuid() const { return mXuid; }

      //bool           isMuted() const { return mMuted; }
      //void           setMute(bool mute) { mMuted = mute; }

      //void           setName(const BSimString& name) { mGamertag=name; }
      const BSimString& getName() const { return mGamertag; }

      void             setGamertag(const BSimString &gamertag) { mGamertag = gamertag; }
      const BSimString& getGamertag() const { return mGamertag; }

      //bool           isHost() const { return mHost; }
      //void           setHost(bool host) { mHost = host; }
      BOOL           isLocal() const { return mLocal; }
      void           setConnected(bool c) { mConnected = c; }
      bool           isConnected() const { return mConnected; }
      //void           disconnect();

      //HRESULT        getAddresses(SOCKADDR_IN* localAddress, SOCKADDR_IN* translatedLocalAddress);

      //void           service();

      //BTimingHistory* getTimingHistory() { return mTimingHistory; }

      //void           setWaitingForTimeStop(bool v) { mWaitingForTimeStop = v; }
      //bool           isWaitingForTimeStop() const { return mWaitingForTimeStop; }

      //void           clientDisconnectReported(DWORD fromClientID, bool add);
      //bool           shouldDisconnect(DWORD connectedClients);

      //void           setDisconnectClients(long c) { mDisconnectClients = c; }
      //long           getDisconnectClients() const { return mDisconnectClients; }
      //void           incDisconnectCount() { mDisconnectCount++; }
      //long           getDisconnectCount() const { return mDisconnectCount; }

      //BPing*         getPing() { return mPing; }
      //virtual void   sendPingPacket(BPacket& packet);
      //virtual void   pingNotResponding();
      //virtual void   pingUpdated();  

      //BUDP360Connection*    getConnection() const { return(mConnection); }

      // BDataStream::BDataStreamTransport
      //virtual void   sendPacket(BPacket& packet);

      const XNADDR&        getXnAddr() const { return mXnAddr; }
      const SOCKADDR_IN&   getAddr() const { return mAddr; }

      uint32         getPingAverage() const { return mPingAverage; }
      uint32         getPingStdDev() const { return mPingStdDev; }

      void           updatePing(uint32 avg, uint32 stdDev);

   private:

      // Functions

      // Variables

      //BYTE                 mSendBuffer[cMaxSendSize];
      //BClientObserverList  mObserverList;
      BSimString           mGamertag;

      XNADDR               mXnAddr;
      SOCKADDR_IN          mAddr;

      XUID                 mXuid;

      //long                 mDisconnectCount;
      //long                 mDisconnectClients;
      //DWORD                mDisconnectReports;
      //DWORD                mControllerID;

      //BPing*               mPing;
      //BTimingHistory*      mTimingHistory;
      //BUDP360Connection*   mConnection;

      uint32               mPingAverage;
      uint32               mPingStdDev;

      int32                mID;

      BOOL                 mLocal;

      bool                 mConnected : 1;
      //bool                 mHost : 1;
      //bool                 mWaitingForTimeStop : 1;

      //bool                 mMuted : 1;

}; // BClient

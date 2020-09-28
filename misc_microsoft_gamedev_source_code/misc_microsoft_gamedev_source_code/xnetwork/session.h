//==============================================================================
// session.h
//
// Copyright (c) Ensemble Studios, 2001-2008
//==============================================================================

#pragma once

//==============================================================================
// Includes

#include "Client.h"

#include "NetEvents.h"
#include "xconnection.h"
#include "xudpsocket.h"
#include "xnetwork.h"

#include "containers\queue.h"

//==============================================================================
// Forward declarations

class BPacket;
class BSessionEvent;
class BTimingInfo;
class BTimeSync;

typedef uint32 ClientID;
typedef int32 BMachineID;
typedef int32 BClientID;

const uint32 cMPInvalidClientID = UINT32_MAX;

//==============================================================================
class BChannelData : public IPoolable
{
   public:
      BChannelData();
      ~BChannelData();

      void init(uint32 machineID, long channelID, uint packetID, uint size, BSessionEvent* pEvent);

      // IPoolable
      void onAcquire();
      void onRelease();
      DECLARE_FREELIST(BChannelData, 4);

      BMachineID getMachineID() const { return mMachineID; }
      uint getChannelID() const { return mChannelID; }
      uint getPacketID() const { return mPacketID; }
      uint getSize() const { return mSize; }
      const void* getData() const;

      bool isProcessed() const;

      BMachineID     mMachineID;
      long           mChannelID;
      uint           mPacketID;
      uint           mSize;
      BSessionEvent* mpEvent;
};

//==============================================================================
class BDisconnectEntry
{
   public:
      enum BDisconnectState
      {
         cStateNone = 0,
         cStateSent,
         cStateDone
      };

      BDisconnectEntry();

      void init(BMachineID machineID);
      void set(BMachineID machineID, BDisconnectState state);

      BDisconnectState getState(BMachineID) const;

      BDisconnectState getState() const { return mState; }
      void setState(BDisconnectState state) { mState = state; }

   private:
      BDisconnectState mStates[XNetwork::cMaxClients];
      BMachineID       mMachineID;
      BDisconnectState mState;
};

//==============================================================================
class BDisconnectTracker
{
   public:
      BDisconnectTracker();

      bool add(BMachineID machineID);
      BDisconnectEntry* get(BMachineID machineID);

      void set(BMachineID machineID, BDisconnectEntry::BDisconnectState state);
      void reset(BMachineID machineID);

   private:
      BDisconnectEntry mMachines[XNetwork::cMaxClients];
};

//==============================================================================
class BChannelInfo
{
   public:
      BChannelInfo();

      inline void init(uint channelID);
      void reset();
      inline void set(uint packetID) { mPacketID = packetID; }

      void serialize(BSerialBuffer& sb);
      void deserialize(BSerialBuffer& sb);

      uint getChannelID() const { return mChannelID; }
      uint getPacketID() const { return mPacketID; }
      inline bool isValid() const { return mIsValid; }

   private:
      uint mChannelID;
      uint mPacketID;
      bool mIsValid : 1;
};

//==============================================================================
class BChannelTracker
{
   public:
      BChannelTracker();

      enum
      {
         cMaxChannels = 10,
      };

      void init(BMachineID machineID);

      inline void set(uint channelID, uint packetID);

      const BChannelInfo* getChannelInfo(uint channelID) const;

      uint getPacketID(uint channelID) const;
      bool isValid(uint channelID) const;

      void serialize(BSerialBuffer& sb);
      void deserialize(BSerialBuffer& sb);

      BChannelTracker& operator=(const BChannelTracker& source);

      BMachineID getMachineID() const { return mMachineID; }

   private:
      BChannelInfo mChannels[cMaxChannels];
      BMachineID   mMachineID;
};

//==============================================================================
class BClientHolder
{
   public:

      enum BClientHolderState
      {
         // don't change the order of these
         cStateConnecting,
         cStateConnected,
         cStateKicked,
         cStateDisconnecting,
         cStateDisconnected,
         cStateDeleteMe,
         // to here

         cStateMax,
         cStateOpen,
         cStateClosed,
         cStateInvalid
      };

      BClientHolder(BClient* pClient, BClientHolderState state) :
         mpClient(pClient),
         mState(state),
         mMachineID(-1),
         mDisconnectReason(0)
         {}

      BClientHolder() :
         mpClient(NULL),
         mState(cStateOpen),
         mMachineID(-1),
         mDisconnectReason(0)
         { }

      void reset() { mpClient=NULL; mState=cStateOpen; mMachineID=-1; mDisconnectReason=0; }

      BClient*             mpClient;            // 4
      BClientHolderState   mState;              // 4
      BMachineID           mMachineID;          // 4
      int32                mDisconnectReason;   // 4 - track the reason for the disconnect
};

//==============================================================================
class BProxy
{
   public:

      BProxy();

      void init(const XNADDR& xnaddr, const XNADDR& proxyXnAddr);

      BProxy& operator=(const BProxy& proxy);

      const XNADDR& getXnAddr() const { return mXnAddr; }
      const XNADDR& getProxyXnAddr() const { return mProxyXnAddr; }

      void serialize(BSerialBuffer& sb);
      void deserialize(BSerialBuffer& sb);

   private:
      XNADDR     mXnAddr;
      XNADDR     mProxyXnAddr;
};

//==============================================================================
class BProxyInfo
{
   public:
      BProxyInfo();

      void add(const BProxy& proxy);
      void add(const XNADDR& xnaddr, const XNADDR& proxyXnAddr);

      uint getCount() const { return mCount; }
      const BProxy* getProxy(uint index) const;

      const BProxy* find(const XNADDR& xnaddr) const;

      BProxyInfo& operator=(const BProxyInfo& user);

      void serialize(BSerialBuffer& sb);
      void deserialize(BSerialBuffer& sb);

   private:
      BProxy mProxy[XNetwork::cMaxClients];
      uint8  mCount;
};

//==============================================================================
// BSessionUser is used to store a primary/secondary user on a single console
// Before hosting/joining a game, you need to register the primary/secondary users
struct BSessionUser
{
   XUID        mXuid;         // 8
   BSimString  mGamertag;     // 8
   BClientID   mClientID;     // 4
   uint        mControllerID; // 4

   BSessionUser() :
      mControllerID(XUSER_MAX_COUNT),
      mClientID(-1),
      mXuid(0)
   {}

   BSessionUser(uint controllerID, XUID xuid, const BSimString& gamertag) :
      mControllerID(controllerID),
      mXuid(xuid),
      mGamertag(gamertag)
   {}

   void reset()
   {
      mXuid = 0;
      mGamertag.empty();
      mClientID = -1;
      mControllerID = XUSER_MAX_COUNT;
   }


   BSessionUser& operator=(const BSessionUser& user);

   void serialize(BSerialBuffer& sb);
   void deserialize(BSerialBuffer& sb);
};

//==============================================================================
class BProxyRequest
{
   public:
      enum BRequestState
      {
         cStateInit,
         cStatePingSent,
         cStateComplete,
      };

      BProxyRequest();

      void                 init(const SOCKADDR_IN& fromAddr, const SOCKADDR_IN& toAddr, const XNADDR& toXnAddr, uint timeout);

      const IN_ADDR&       getFromAddr() const { return mFromAddress.sin_addr; }
      const SOCKADDR_IN&   getFromSockAddr() const { return mFromAddress; }

      const XNADDR&        getToXnAddr() const { return mToXnAddr; }
      const IN_ADDR&       getToAddr() const { return mToAddress.sin_addr; }
      const SOCKADDR_IN&   getToSockAddr() const { return mToAddress; }

      BRequestState        getState() const { return mState; }
      void                 setState(BRequestState state, uint32 time=0);

      uint                 getRequestTimeout() const { return mRequestTimeout; }

      uint                 getNonce() const { return mNonce; }

   private:
      XNADDR         mToXnAddr;
      SOCKADDR_IN    mFromAddress;
      SOCKADDR_IN    mToAddress;

      uint           mRequestTime;
      uint           mResponseTime;
      uint           mRequestTimeout;

      uint           mNonce;

      BRequestState  mState;
};

//==============================================================================
class BMachine
{
   public:
      enum
      {
         cMaxUsers = 2,
         cInvalidMachineID = -1,
      };

      enum BMachineState
      {
         // don't change the order of these
         cStateConnecting,
         cStateConnected,
         cStateKicked,
         cStateDisconnecting,
         cStateDisconnected,
         cStateDeleteMe,
         // to here

         cStateMax,
         cStateOpen,
         cStateClosed,
         cStateInvalid
      };

      BMachine();

      BMachineID getID() const { return mID; }

      const XNADDR&        getXnAddr() const { return mXnAddr; }
      const IN_ADDR&       getAddr() const { return mAddress.sin_addr; }
      const SOCKADDR_IN&   getSockAddr() const { return mAddress; }
      const SOCKADDR_IN&   getProxySockAddr() const { return mProxyAddress; }

      uint32               getPingAverage() const { return mPingAverage; }
      uint32               getPingStdDev() const { return mPingStdDev; }

      BMachineState        getState() const { return mState; }

      void init(BMachineID id, const XNADDR& xnaddr, const IN_ADDR& addr, uint16 port, BMachineState state);
      void reset();

      bool isLocal() const { return mLocal; }
      bool isConnected() const { return (mState == cStateConnected); }
      uint getClientCount();

      void updatePing(uint32 avg, uint32 stdDev);

      void setState(BMachineState state);

      bool isEnabled() const { return mEnabled; }
      void enable();
      void disable();

      bool isProxied() const { return mProxied; }

      void setProxy(const SOCKADDR_IN& proxyAddr, uint ping, const XNADDR& xnaddr);

      BSessionUser   mUsers[cMaxUsers]; // 24 * 2 == 48
      XNADDR         mXnAddr; // 36
      XNADDR         mProxyXnAddr; // 36
      SOCKADDR_IN    mAddress; // 4 // This is now the singular IP (0.x.y.0) for that target connection
      SOCKADDR_IN    mProxyAddress; // 4 // This is now the singular IP (0.x.y.0) for that target connection
      BMachineID     mID; // 4
      BMachineState  mState; // 4

      uint32         mPingAverage;
      uint32         mPingStdDev;

      uint32         mProxyPing;

      bool           mLocal : 1;   // 1 (1/8)
      bool           mEnabled : 1; //   (2/8)
      bool           mProxied : 1; //   (3/8)
};

//==============================================================================
//
//==============================================================================
class BXConnector
{
   public:

      BXConnector();

      BXConnector(const BXConnector& source);

      enum BXConnectorState
      {
         cStateIdle = 0,
         cStateConnecting,
         cStateConnected,
         cStateFailed
      };

      void init(const XNADDR& xnaddr, uint16 port, const XNKID& xnkid);

      void init(BMachineID machineID, const XNADDR& xnaddr, uint16 port, const XNKID& xnkid, BSessionUser users[]);

      void reset();

      void connect();

      void service();

      DWORD getStatus(bool recheck=false);

      void terminate();

      void setProxy(const SOCKADDR_IN& proxyAddr, uint ping, const XNADDR& proxyXnAddr);
      void clearProxy();

      const XNADDR&     getXnAddr() const { return mXnAddr; }
      const XNKID&      getXnKid() const { return mXnkid; }
      const IN_ADDR&    getAddr() const { return mAddr; }

      const SOCKADDR_IN& getProxyAddr() const { return mProxyAddr; }
      const XNADDR*      getProxyXnAddr() const { return (mProxied ? &mProxyXnAddr : NULL); }

      uint16            getPort() const { return mPort; }

      BXConnectorState  getState() const { return mState; }

      BMachineID        getMachineID() const { return mMachineID; }

      bool              isProxied() const { return mProxied; }

      uint              getProxyPing() const { return mProxyPing; }

      BSessionUser      mUsers[BMachine::cMaxUsers];

   private:

      XNADDR            mXnAddr;

      XNADDR            mProxyXnAddr;
      SOCKADDR_IN       mProxyAddr;

      XNKID             mXnkid;

      IN_ADDR           mAddr;

      // XNET_CONNECT_STATUS_IDLE      The connection has not started. 
      // XNET_CONNECT_STATUS_PENDING   The connection is being established. 
      // XNET_CONNECT_STATUS_CONNECTED The connection has been established. 
      // XNET_CONNECT_STATUS_LOST      The connection has been lost.
      DWORD            mStatus;

      BXConnectorState mState;

      BMachineID       mMachineID;

      uint             mProxyPing;

      uint16           mPort;

      bool             mProxied : 1;
};

//==============================================================================
class BIncomingData
{
   public:
      BIncomingData() {}

      void init(uint32 eventID, const SOCKADDR_IN& addr, const uchar* pData, uint size)
      {
         mEventID = eventID;
         mSize = size;
         mAddr = addr;
         BDEBUG_ASSERT(size <= cBufSize);
         Utils::FastMemCpy(mBuf, pData, size);
      }

      enum { cBufSize = cMaxSendSize };
      uchar                mBuf[cBufSize];
      SOCKADDR_IN          mAddr;
      uint32               mEventID;
      uint                 mSize;
};

//==============================================================================
struct BSessionBuffer
{
   enum { cBufSize = cMaxSendSize };
   uchar       mBuf[cBufSize];
   SOCKADDR_IN mAddr;
   uint64      mReserved;
   uint        mSize;
   uint        mFlags;
   // 1152 + 16 + 8 + 4 + 4 = 1184
};

//==============================================================================
typedef BSynchronizedBlockAllocator<BSessionBuffer, 64, false> BSessionBufferAllocator;

//==============================================================================
class BSession :  public BXConnectionInterface,
                  public BEventReceiver
{
   public:

      enum
      {
         cSessionEventInit = cNetEventFirstUser,
         cSessionEventInitPending, // initialize a client as pending, awaiting the completion of the connect
         cSessionEventDeinit,
         cSessionEventBroadcast,
         cSessionEventBroadcast2, // broadcasts to pending connections as well
         cSessionEventSend,
         cSessionEventTimer,
         cSessionEventMute,
         cSessionEventInitHost,
         cSessionEventInitPeers,
         cSessionEventDeinitPeer,

         cSessionEventConnDeinit,

         cSessionEventInitProxy,
         cSessionEventProxyRequestTimer,
         cSessionEventProxyResponseTimer,

#ifndef BUILD_FINAL
         cSessionEventProxyInfo,
#endif

         cSessionEventTotal
      };

      enum BDisconnectReason
      { 
         cNormal=0,
         cTransportLost,
         cSessionTerminated,
         cHostDecision,
         cHostCancelledGame,
         cFailedClientConnect,
         cConnectionRejected,
         cSessionClosed,
         cBuildMismatch,
         cSessionFull,
         cGameDeleted,
         cNumReasons
      };

      //These are the reasons for a join failure that the session can report back up the stack
      enum BJoinReasonCode
      {
         cResultJoinOk,
         cResultRejectFull,
         cResultRejectCRC,
         cResultRejectUserExists,
         cResultRejectUnknown,
         cResultHostNotFound,
         cResultHostConnectFailure,
         cResultRejectInternalError,
         cResultRejectConnectFailure,
         cLiveSessionFailure,
         cResultMax
      };

      enum
      {
         cEventJoinFailed,                // data1 failure code BJoinFailureReason, data2 unused      - Not able to join the session
         cEventConnected,                 // data1, data2 unused                                      - Fully connected to the session
         cEventDisconnected,              // data1, data2 unused
         cEventClientConnect,             // data1 client ID, data2 unused
         cEventClientDisconnect,          // data1 client ID, data2 unused
         cEventClientData,                // data1 client ID, data2 size, data is at end of event
         cEventClientPing,                // data1 client ID, data2 unused
         cEventChannelData,               // data1 client ID, data2 size, data is at end of event
         cEventTimingData,                // data1 client ID, data2 size, data is at end of event

         cEventTotal,
         cEventInvalid
      };

      class BSessionEventObserver
      {
         public:
            virtual void processSessionEvent(const BSessionEvent* pEvent) { pEvent; }
      };
      
      class BClientConnector
      {
         public:
            //Callback going up when we have a client requesting to connect to the host of the session.
            //  If approved, then they are assigned a client ID, and allowed to try and fully connect to the session
            virtual bool sessionConnectionRequest(const BSessionUser users[], BJoinReasonCode* reasonCode) = 0;
      };

      class BSessionEventObserverList : public BObserverList<BSessionEventObserver>
      {
         DECLARE_OBSERVER_METHOD (processSessionEvent,
            (const BSessionEvent* pEvent),
            (pEvent))
      };

      // Constructors
      BSession();

      // Destructors
      virtual ~BSession();

      virtual HRESULT dispose();
      void disposeConnectors();

      void init(DWORD checksum, BClientConnector* pClientConnector, BTimingInfo* pTimingInfo, BVoiceInterface* pVoiceInterface, bool doNotProxyToHost=false);

      bool addUser(uint controllerID, XUID xuid, const BSimString& gamertag);
      bool joinUser(uint controllerID, XUID xuid, const BSimString& gamertag);

      void sendPeersPacket(const SOCKADDR_IN& addr);

      void handleInitPeer(const uchar* pData, const uint size, const SOCKADDR_IN& addr);
      void handleInitClient(const uchar* pData, const uint size, const SOCKADDR_IN& addr);

      void handleJoinRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr);
      void handleClientJoinRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr);

      void handleJoinResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr);
      void handleClientJoinResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr);

      // You're all wrong, it's a BSession::dataReceived method now
      // all you observers can take a flying leap
      void dataReceived(const uchar* pData, const uint size, const SOCKADDR_IN& addr);

      void                 addObserver(BSessionEventObserver* o) { mObserverList.Add(o); }
      void                 removeObserver(BSessionEventObserver* o) { mObserverList.Remove(o); }

      HRESULT              host(const XNKID& sessionkey, WORD gamePortNumber);

      void                 disconnect(long reason);

      bool                 join(const XNADDR& xnaddr, uint16 port, const XNKID& xnkid);
      void                 handleJoin();

      // set a client slot open or closed (all start as open by default)
      void                 setMaxClientCount(long count);
      inline uint32        getMaxClientCount() const { return mMaxClientAmount; }
      inline uint32        getMaxMachineCount() const { return mMaxClientAmount; }
      HRESULT              kickClient(const long clientIndex);
      void                 disconnectClient(const long clientIndex);

      HRESULT              _SendPacket(BPacket& packet, long* pSizeOut=0, uint flags=0, const char* pFile=0, long line=0);
      HRESULT              SendBroadcast2(BPacket& packet, uint flags=0); // do not use this for normal game traffic
      HRESULT              SendPacketToAddr(const SOCKADDR_IN& addr, BPacket& packet);
      HRESULT              _SendPacketTo(const SOCKADDR_IN& addr, BPacket& packet, long* pSizeOut=0, uint flags=0, const char* pFile=0, long line=0);
      HRESULT              _SendPacketTo(BClient* pTargetClient, BPacket& packet, long* pSizeOut=0, uint flags=0, const char* pFile=0, long line=0);
      HRESULT              _SendPacketTo(BMachine* pTargetMachine, BPacket& packet, long* pSizeOut=0, uint flags=0, const char* pFile=0, long line=0);
      HRESULT              _SendTimingPacket(BPacket& packet);

      void                 service();

      BClient*             getClient(BClientID clientID) const;
      BClient*             getClientByXuid(XUID xuid) const;

      bool                 isClient(long clientIndex);

      BMachineID           getLocalMachineID() const { return mLocalMachineID; }
      BMachineID           getHostMachineID() const { return mHostMachineID; }

      bool                 getLocalClients(BClient* pLocalClient1, BClient* pLocalClient2) const;
      bool                 getHostClients(BClient* pLocalClient1, BClient* pLocalClient2) const;

      BMachine*            getLocalMachine();
      BMachine*            getHostMachine();

      BMachine*            getMachine(BMachineID machineID);
      BMachineID           findMachineIDFromClientID(BClientID clientID);

      bool                 isHosted() const;

      BOOL                 isLocalClientID(BClientID clientID);
      BOOL                 isHostClientID(BClientID clientID);

      int32                getConnectedClientAmount() const;
      int32                getActiveClientAmount() const;
      int32                getClientCount() const { return getActiveClientAmount(); }
      int32                getMachineCount() const { return XNetwork::cMaxClients; }
      uint                 getActiveMachineAmount() const;

      BOOL                 isDisconnecting() const { return ((mFlags&cFlagDisconnecting) == cFlagDisconnecting); }

      void                 setLocalXnAddr(const XNADDR& xnaddr);

      bool                 isConnected() { return ((mFlags&cFlagConnected) == cFlagConnected); }

      void                 beginClientDisconnect(const long clientIndex, BDisconnectReason reason);

      void                 socketDisconnect(const BMachineID machineID, const uint error);

      void                 checkSessionDisconnect();
      void                 finishClientDisconnect(BMachineID machineID);

      uint32               getMaxPing() const;
      uint32               getAveragePing() const;
      uint32               getMaxPingDeviation() const;

      BTimeSync*           getTimeSync() const { return mpTimeSync; }

      BEventReceiverHandle getConnectionEventHandle() const { return mConnectionEventHandle; }

      // BXConnectionInterface
      virtual BXConnection* getConnection(uint64 id);

      static uint64        getUniqueID(const XNADDR& xnaddr);

#ifndef BUILD_FINAL
      BMachineID mMachineProxyInfo[XNetwork::cMaxClients];
      BSimString mUserProxyInfo[XNetwork::cMaxClients*2];
#endif

      int32                getUpdateInterval() const { return mUpdateInterval; }

   private:

      BXConnection*        getConnectionByAddr(const IN_ADDR& addr);
      BXConnection*        getConnectionByAddr(const uint addr);

      void                 beginMachineDisconnectInternal(const BMachineID machineID, bool init, BDisconnectReason reason);
      void                 beginClientDisconnectInternal(const long clientIndex, bool init, BDisconnectReason reason);

      // Functions
      void                 processEvents();

      enum 
      { 
         cFlagConnected             = 0x01,
         cFlagDisconnecting         = 0x02,
      };

      void                          setConnected(bool v);
      void                          setDisconnecting(bool v);

      void                          releaseIncomingData();
      void                          releaseIncomingData(const BMachine& machine);

      BMachineID                    mapAddrToMachineID(const SOCKADDR_IN& addr);
      BMachineID                    mapAddrToMachineID(const IN_ADDR& addr);

      long                          mapAddrToClientIndex(const SOCKADDR_IN& addr);
      long                          mapAddrToClientIndex(const IN_ADDR& addr);
      BMachine*                     getMachineByAddr(const IN_ADDR& addr);
      BMachine*                     getMachineByXnAddr(const XNADDR& addr);

      const BProxyRequest*          getProxyRequestByAddr(const IN_ADDR& addr) const;
      const BProxyRequest*          getProxyRequestByAddrAndXnAddr(const SOCKADDR_IN& addr, const XNADDR& xnaddr) const;

      BMachineID                    findNextAvailableMachineID();
      int32                         findNextAvailableClientID(int32 startingIndex=0);
      bool                          findNextAvailableClientID(int32& index1, int32& index2);

      bool                          createUser(uint controllerID, XUID xuid, const BSimString& gamertag);

      bool                          createHost();
      bool                          createLocal(BMachineID machineID, BSessionUser users[]);

      bool                          createClient(const SOCKADDR_IN& addr, BSessionUser users[], BMachineID& machineID);
      bool                          createClient(const XNADDR& xnaddr, BSessionUser users[], BMachineID& machineID, const BProxyInfo& proxyInfo);
      bool                          createClient(const BXConnector& connector);

      void                          kickClient(long fromIndex, const void* data, const DWORD size);

      void                          handlePeersRequest(const uchar* pData, uint size, const SOCKADDR_IN& addr);
      void                          handlePeersResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr);

      void                          handleProxyRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr);
      void                          handleProxyResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr);
      void                          handleProxyPing(const uchar* pData, const uint size, const SOCKADDR_IN& addr);
      void                          handleProxyPong(const uchar* pData, const uint size, const SOCKADDR_IN& addr);

      void                          handleDisconnectRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr);
      void                          handleDisconnectSync(const uchar* pData, const uint size, const SOCKADDR_IN& addr);
      void                          handleDisconnectResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr);

      void                          checkDisconnect(BMachineID fromMachineID, BMachineID forMachineID);

      void                          connectionRejected(const void* data, const DWORD size);

      void                          addEvent(uint32 eventID, uint32 data1=0, uint32 data2=0, const void* pData=NULL, uint64 data3=0, int32 data4=0);
      void                          releaseEvent(BSessionEvent* pEvent);

      BSessionBuffer*               allocBuffer();

      void                          stopTimer();

      void                          deinitPeer(uint addr);
      void                          disablePeer(uint addr);

      // BEventReceiver
      virtual bool                  receiveEvent(const BEvent& event, BThreadIndex threadIndex);

      // Variables

      BXNetBufferAllocator          mSendBufferAllocator; // 332808
      BXNetBufferAllocator          mRecvBufferAllocator; // 332808

      BSessionBufferAllocator       mSessionBufferAllocator; // 76040

      BXUDPSocket                   mUDPThread; // 4280

      BXNetBuffer                   mTempNetBuffer; // 1296
      BSessionBuffer                mTempSessionBuffer; // 1184

      // we no longer match up clients to connections, instead we have machines paired with a connection
      BMachine                      mMachines[XNetwork::cMaxClients]; // 176 * 6 == 1056

      BChannelTracker               mChannelTracker[XNetwork::cMaxClients]; // 744

      BVoiceBuffer                  mTempVoiceBuffer; // 656

      BDisconnectTracker            mDisconnectTracker; // 192

      BClientHolder                 mClients[XNetwork::cMaxClients]; // 12 * 6 == 72

      BFreeList<BSessionEvent>      mEventFreeList; // 56

      BSessionUser                  mLocalUsers[BMachine::cMaxUsers]; // 24 * 2 == 48

      BPointerList<BSessionEvent>   mEventList; // 36
      BPointerList<BChannelData>    mChannelDataList; // 36

      XNADDR                        mHostXnAddr; // 36
      XNADDR                        mLocalXnAddr;

      BSessionEventObserverList     mObserverList; // 20

      BDynamicNetArray<BXConnector>    mXConnectors; // 16
      BDynamicNetArray<BXConnection*>  mXConnections; // 12
      BDynamicNetArray<BXConnection*>  mXConnectionsPending;

      BDynamicNetArray<BIncomingData*> mIncomingData; // 12

      BDynamicNetArray<BProxyRequest*> mProxyRequests; // 12
      BDynamicNetArray<XNADDR>         mProxyPending; // 12

      XUID                          mLocalXUID;       // 8, local player XUID
      XNKID                         mSessionXNKID;    // 8, XNKID for the session I'm hosting or joining

      BSimString                    mLocalGamerTag; // 8

      BEventReceiverHandle          mConnectionEventHandle; // 8
      BEventReceiverHandle          mVoiceEventHandle;

      BMachineID                    mHostMachineID; // 4
      BMachineID                    mLocalMachineID; // 4

      uint32                        mMaxClientAmount;

      DWORD                         mFlags;
      DWORD                         mChecksum;

      BTimeSync*                    mpTimeSync;

      BClientConnector*             mpClientConnector;

      BVoiceBufferAllocator*        mpVoiceAllocator;
      BVoiceInterface*              mpVoiceInterface;

      IN_ADDR                       mHostSecureINADDR; // 4
      DWORD                         mJoinRequestTimer;

      BWin32WaitableTimer           mPeerTimer; // 4
      BWin32WaitableTimer           mConnectionTimer;
      BWin32WaitableTimer           mProxyRequestTimer;
      BWin32WaitableTimer           mProxyResponseTimer;

      BXConnector*                  mpHostConnector;

      uint32                        mConnectionTimeout;

      uint                          mProxyRequestTimeout;
      uint                          mProxyResponseTimeout;
      uint                          mProxyPingTimeout;

      uint                          mJoinRequestTimeout;

      int32                         mUpdateInterval;

      WORD                          mHostPort;

      bool                          mRequirePeersResponse : 1;

      bool                          mEventProcessing : 1;
      bool                          mProxySupport : 1;

      bool                          mPeerTimerSet : 1;
      bool                          mConnectionTimerSet : 1;
      bool                          mProxyRequestTimerSet : 1;
      bool                          mProxyResponseTimerSet : 1;
      bool                          mDoNotProxyToHost : 1;
}; // BSession

//==============================================================================
class BSessionEvent
{
   public:
      BSessionEvent() :
         mEventID(BSession::cEventInvalid),
         mData1(0),
         mData2(0),
         mData3(0),
         mData4(0),
         mAutoRelease(true),
         mProcessed(false)
         {}

      void init(uint32 eventID, uint32 data1, uint32 data2, uint64 data3, int32 data4=0, const void* pData=NULL, bool autoRelease=true)
      {
         mEventID = eventID;
         mData1 = data1;
         mData2 = data2;
         mData3 = data3;
         mData4 = data4;
         if (pData != NULL)
         {
            void* pDst = ((char*)this)+sizeof(BSessionEvent);
            Utils::FastMemCpy(pDst, pData, data2);
         }
         mAutoRelease = autoRelease;
         mProcessed = false;
      }

      void setAutoRelease(bool autoRelease) { mAutoRelease = autoRelease; }
      bool getAutoRelease() const { return mAutoRelease; }

      void setProcessed() { mProcessed = true; }
      bool isProcessed() const { return mProcessed; }

      uint64   mData3; // used to pass around the XUID
      uint32   mEventID;
      uint32   mData1;
      uint32   mData2;
      int32    mData4;
      bool     mAutoRelease : 1; // if this is set to true, then the session event will be released after processing
                                 // otherwise it will be up to someone else to free the memory
      bool     mProcessed : 1;
}; // BSessionEvent

#define SESSIONEVENT_DATA(e) (((char*)e)+sizeof(BSessionEvent));
#define SESSIONEVENT_DATASIZE(e) ((e)->mData2);

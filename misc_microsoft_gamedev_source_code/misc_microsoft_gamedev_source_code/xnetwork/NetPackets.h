//==============================================================================
// NetPackets.h
//
// Copyright (c) Ensemble Studios, 2001-2008
//==============================================================================

#pragma once

//==============================================================================
// Includes

#include "PacketTypes.h"
#include "Packet.h"
#include "xnetwork.h"
#include "session.h"

//==============================================================================
// Forward declarations
class BDynamicStream;
class BSessionDescriptor;

//==============================================================================
class BChannelPacket : public BTypedPacket
{
public:
   BChannelPacket();
   BChannelPacket(long type);

   virtual ~BChannelPacket();

   void                    setType(uint32 type);
   void                    setChannel(long channel);
   void                    setPacketID(uint packetID);

   void                    setTimeOffset(uint32 offset);

   uint                    getPacketID() const { return mPacketID; }

   static long             getType(const void* pData);
   static long             getChannel(const void* pData);
   static uint             getPacketID(const void* pData);

   static uint32           getTimeOffset(const void* pData);

   virtual void            serializeSignature(BSerialBuffer& sb);
   virtual void            deserializeSignature(BSerialBuffer& sb);
   static long             getSignatureSize();

private:
   uint16                  mTimeOffset;
   uint16                  mPacketID;
   uint8                   mType;
   uint8                   mChannel;
};

//==============================================================================
class BDataBlockPacket : public BTypedPacket
{
   // NOTE: This packet requires immediate serialization, because it doesn't copy data for itself, but rather
   // copies it only on serialization - so the data you pass in must last until that point
   // it also requires immediate grabbing of data after deserialization, because after it goes out of scope,
   // mData is no longer valid
public:     
   BDataBlockPacket(long packetType = BPacketType::cDataBlockPacket) : BTypedPacket(packetType), mStreamID(0), mData(0), mSize(0) {}      
   BDataBlockPacket(unsigned char streamID, void *data, long size, long packetType = BPacketType::cDataBlockPacket) 
      : BTypedPacket(packetType), mStreamID(0), mData(0), mSize(0)
   {
      mStreamID = streamID;
      mData = data;
      mSize = size;
   }      

   virtual ~BDataBlockPacket(void) {}  

   virtual void            serialize(BSerialBuffer& sb) { BTypedPacket::serialize(sb); sb.add(mStreamID), sb.add(mSize); sb.add(mData, mSize); }      
   virtual void            deserialize(BSerialBuffer& sb) { BTypedPacket::deserialize(sb); sb.get(&mStreamID), sb.get(&mSize); sb.getPointer(&mData, &mSize); }

   uint8                   getStreamID() { return mStreamID; }

public:
   uint8                   mStreamID;
   const void*             mData;
   int32                   mSize;
};

//==============================================================================
class BDataFlowControlPacket : public BTypedPacket
{
public:
   enum eCommands { cCommandInvalid, cCommandAckRequest, cCommandAck, cCommandComplete };
   BDataFlowControlPacket(long packetType = BPacketType::cDataBlockFlowControlPacket) : BTypedPacket(packetType), mStreamID(0), mCommand(0), mData(0) {}
   BDataFlowControlPacket(unsigned char streamID, unsigned char command, long data, long packetType = BPacketType::cDataBlockPacket) 
      : BTypedPacket(packetType), mStreamID(0), mCommand(cCommandInvalid), mData(0)
   {
      mStreamID = streamID;
      mCommand = command;
      mData = data;
   }

   virtual ~BDataFlowControlPacket(void) {}  

   virtual void            serialize(BSerialBuffer& sb) { BTypedPacket::serialize(sb); sb.add(mStreamID), sb.add(mCommand); sb.add(mData); }      
   virtual void            deserialize(BSerialBuffer& sb) { BTypedPacket::deserialize(sb); sb.get(&mStreamID), sb.get(&mCommand); sb.get(&mData); }

   unsigned char           getStreamID() { return mStreamID; }

public:
   uint8                   mStreamID;
   uint8                   mCommand;
   int32                   mData;      
};

//==============================================================================
class BWrapperPacket : public BTypedPacket
{
   // NOTE: Be careful when using this packet! It must be sent across the network right away otherwise the packet
   // passed into the ctor will go out of scope and it will crashz0r
public:
   BWrapperPacket(void) : BTypedPacket(BPacketType::cWrapperPacket), mPacket(0), mData(0), mSize(0) {}
   BWrapperPacket(BPacket *packet) : BTypedPacket(BPacketType::cWrapperPacket), mPacket(packet) {}

   virtual void            serialize(BSerialBuffer& sb) 
   { 
      BTypedPacket::serialize(sb); 
      if (mPacket)
         mPacket->serializeInto(sb);
   }      
   virtual void            deserialize(BSerialBuffer& sb) 
   { 
      BTypedPacket::deserialize(sb);          
      mSize = 0;
      sb.getPointer(&mData, &mSize);
   }

   const void*              mData;              
   int32                    mSize;

private:
   BPacket*                 mPacket;
};

//==============================================================================
class BMessagePacket : public BChannelPacket
{
   public:
      BMessagePacket(long packetType);
      BMessagePacket(long message, long packetType, int32 playerID=-1);

      virtual ~BMessagePacket() {}

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

   public:
      int32 mMessage;
      int32 mPlayerID;
};

//==============================================================================
class BMessageDataPacket : public BChannelPacket
{
public:
   BMessageDataPacket(long packetType) : BChannelPacket(packetType) {}
   BMessageDataPacket(long message, long data, long packetType) : BChannelPacket(packetType), mMessage(message), mData(data) {}

   virtual ~BMessageDataPacket(void) {}  

   virtual void            serialize(BSerialBuffer& sb)   
   { 
      BChannelPacket::serialize(sb); 
      sb.add(mMessage); 
      sb.add(mData);
   }
   virtual void            deserialize(BSerialBuffer& sb) 
   { 
      BChannelPacket::deserialize(sb); 
      sb.get(&mMessage); 
      sb.get(&mData);
   }

public:
   int32                   mMessage;      
   int32                   mData;
};

//==============================================================================
class BTypedMessageDataPacket : public BTypedPacket
{
public:
   enum eMessages { cEnumSessions = 1, cSessionAvailable, cClientConnected, cClientDisconnected };

   BTypedMessageDataPacket(long packetType = BPacketType::cTypedMessageDataPacket) : BTypedPacket(packetType) {}      
   BTypedMessageDataPacket(long message, long data1, long data2=0, long packetType = BPacketType::cTypedMessageDataPacket) : BTypedPacket(packetType), mMessage(message), mData1(data1), mData2(data2) {}

   virtual ~BTypedMessageDataPacket(void) {}

   virtual void            serialize(BSerialBuffer& sb)   { BTypedPacket::serialize(sb); sb.add(mMessage); sb.add(mData1); sb.add(mData2); }
   virtual void            deserialize(BSerialBuffer& sb) { BTypedPacket::deserialize(sb); sb.get(&mMessage); sb.get(&mData1); sb.get(&mData2); }

public:               
   int32                   mMessage;      
   int32                   mData1;
   int32                   mData2;
};

//==============================================================================
class BTypedTextPacket : public BTypedPacket
{
public:
   BTypedTextPacket(long packetType = BPacketType::cTypedTextPacket) : BTypedPacket(packetType), mText(0), mTextLength(0) {}      
   BTypedTextPacket(const BCHAR_T *text, long packetType = BPacketType::cTypedTextPacket) : 
   BTypedPacket(packetType), mText(0), mTextLength(0)
   {
      if (text)
      {
         mTextLength = bcslen(text)+1;
         mText = new BCHAR_T [mTextLength]; 
         StringCchCopy(mText, mTextLength, text);            
      }         
   }
   virtual ~BTypedTextPacket(void) 
   {
      if (mText) delete mText;
   }

   virtual void            serialize(BSerialBuffer& sb) 
   { 
      BTypedPacket::serialize(sb);
      sb.add(mTextLength); 
      if (mTextLength)
         sb.add(mText, mTextLength); 
   }
   virtual void            deserialize(BSerialBuffer& sb) 
   {
      BTypedPacket::deserialize(sb);
      if (mText) delete mText;
      mText = 0;
      sb.get(&mTextLength);
      if (mTextLength)
      {
         mText = new BCHAR_T [mTextLength];
         sb.get(&mText, mTextLength);
      }
   }

public:
   int32                   mTextLength;
   BCHAR_T*                mText;
};

//==============================================================================
class BTwoLongsTypedPacket : public BTypedPacket
{
public:
   BTwoLongsTypedPacket(long packetType) : BTypedPacket(packetType), mData1(0), mData2(0) {}
   BTwoLongsTypedPacket(long data1, long data2, long packetType) : BTypedPacket(packetType), mData1(data1), mData2(data2) {}

   virtual void            serialize(BSerialBuffer& sb) { BTypedPacket::serialize(sb); sb.add(mData1); sb.add(mData2); }
   virtual void            deserialize(BSerialBuffer& sb) { BTypedPacket::deserialize(sb); sb.get(&mData1); sb.get(&mData2); }

public:
   int32                   mData1;      
   int32                   mData2;
};

//==============================================================================
class BLongPacket : public BTypedPacket
{
public:
   BLongPacket(long packetType) : BTypedPacket(packetType), mData1(0) {}
   BLongPacket(long data1, long packetType) : BTypedPacket(packetType), mData1(data1) {}

   virtual void            serialize(BSerialBuffer& sb) { BTypedPacket::serialize(sb); sb.add(mData1);  }
   virtual void            deserialize(BSerialBuffer& sb) { BTypedPacket::deserialize(sb); sb.get(&mData1);  }

public:
   int32                   mData1;      

};

//==============================================================================
class BFileStreamPacket : public BTypedPacket
{
public:
   enum eCommands { cInvalidCmd, cStartSend, cStopSend, cAbortSend, cRequestFile, cRequestCRC, cCRCResult, cStartBatch, cStopBatch };

   BFileStreamPacket(long packetType = BPacketType::cFileStreamPacket) : BTypedPacket(packetType),
      mFileCommand(cInvalidCmd), mFileSize(0), mFileCRC(0), mFileType(-1), mFileDirID(-1), mStreamID(0), mFileNameLength(0), mFileName(NULL) {}

   BFileStreamPacket(long command, const BCHAR_T *filename, long filesize, DWORD crc=0, long dirID=-1, long filetype = -1, unsigned char streamID=0,
      long packetType = BPacketType::cFileStreamPacket) : BTypedPacket(packetType)
   {
      BASSERT(filename);

      mFileCommand      = command;
      mFileCRC          = crc;
      mFileSize         = filesize;
      mFileType         = filetype;
      mFileDirID        = dirID;
      mStreamID         = streamID;
      mFileNameLength   = strLength(filename)+1;
      mFileName         = new BCHAR_T [mFileNameLength];
      strCopy(mFileName, mFileNameLength, filename);
   }
   virtual ~BFileStreamPacket() { if (mFileName) delete mFileName; }

   virtual void            serialize(BSerialBuffer& sb)
   {
      BTypedPacket::serialize(sb);
      sb.add(mFileCommand);
      sb.add(mFileCRC);
      sb.add(mFileSize);
      sb.add(mFileType);
      sb.add(mFileDirID);
      sb.add(mStreamID);
      sb.add(mFileNameLength);
      sb.add(mFileName, mFileNameLength);
   }
   virtual void            deserialize(BSerialBuffer& sb)
   {
      BTypedPacket::deserialize(sb);
      sb.get(&mFileCommand);
      sb.get(&mFileCRC);
      sb.get(&mFileSize);
      sb.get(&mFileType);
      sb.get(&mFileDirID);
      sb.get(&mStreamID);
      if (mFileName) delete mFileName;
      sb.get(&mFileNameLength);
      mFileName = new BCHAR_T [mFileNameLength];
      sb.get(&mFileName, mFileNameLength);
   }

public:
   int32                   mFileCommand;
   uint32                  mFileCRC;
   int32                   mFileSize;
   int32                   mFileType;
   int32                   mFileDirID;
   uint8                   mStreamID;
   int32                   mFileNameLength;
   BCHAR_T*                mFileName;
};

//==============================================================================
class BDWORDVectorPacket : public BTypedPacket
{
public:
   BDWORDVectorPacket(long packetType) : BTypedPacket(packetType), mDWORDS(0) {}      
   BDWORDVectorPacket(DWORD *dwords, long count, long packetType) : 
      BTypedPacket(packetType),
      mDWORDS(0),
      mCount(count)
   {
      BASSERT(mCount);
      mDWORDS = new uint32[mCount];
      memcpy(mDWORDS, dwords, count*sizeof(uint32));
   }

   virtual ~BDWORDVectorPacket(void) 
   {
      if (mDWORDS)
         delete[] mDWORDS;
   }  

   virtual void            serialize(BSerialBuffer& sb)   
   { 
      BTypedPacket::serialize(sb); 
      sb.add(mCount);
      for (uint32 i=0; i < mCount; i++)
         sb.add(mDWORDS[i]);
   }
   virtual void            deserialize(BSerialBuffer& sb) 
   { 
      BTypedPacket::deserialize(sb);          
      sb.get(&mCount);
      mDWORDS = new uint32[mCount];
      for (uint32 i=0; i < mCount; i++)
         sb.get(&mDWORDS[i]);
   }

public:               
   uint32*                 mDWORDS;
   uint32                  mCount;
};

//==============================================================================
// 
//==============================================================================
class BInitPeersResponsePacket : public BTypedPacket
{
   public:
      BInitPeersResponsePacket(long packetType=BPacketType::cInitPeersResponsePacket);

      void setLocalInfo(const XNADDR& xnaddr, const BSessionUser users[]);
      void addPeer(const XNADDR& xnaddr, bool status, const XNADDR* pProxyXnAddr=NULL);

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

      XNADDR       mPeers[XNetwork::cMaxClients];
      BProxyInfo   mProxy;
      XNADDR       mXnAddr;
      BSessionUser mUsers[BMachine::cMaxUsers];
      uint16       mStatus;
      uint8        mCount;
};

//==============================================================================
// 
//==============================================================================
class BProxyPacket : public BTypedPacket
{
   public:
      BProxyPacket(long packetType=BPacketType::cProxyRequestPacket);

      void addPeer(const XNADDR& xnaddr, bool status, uint32 avgPing=0, uint32 stdDev=0);

      void setNonce(uint32 nonce);
      uint32 getNonce() const { return mNonce; }

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

      XNADDR       mPeers[XNetwork::cMaxClients];
      uint32       mPings[XNetwork::cMaxClients];
      uint32       mNonce;
      uint16       mStatus;
      uint8        mCount;
};

//==============================================================================
class BAddressesPacket : public BTypedPacket
{
public:
   //enum { cMaxAddressesAmount = 16 };

   BAddressesPacket(long packetType);

   void addAddress(BMachineID machineID, const XNADDR& xnaddr, const BSessionUser users[]);

   virtual void serialize(BSerialBuffer& sb);
   virtual void deserialize(BSerialBuffer& sb);

   uint32         mAmount;
   XNADDR         mXnAddrs[XNetwork::cMaxClients];
   BSessionUser   mUsers[XNetwork::cMaxClients][BMachine::cMaxUsers];
   BMachineID     mMachineIDs[XNetwork::cMaxClients];
};

//==============================================================================
class BSessionConnectorPacket : public BTypedPacket
{
   public:

      BSessionConnectorPacket(long packetType, long result=0);
      BSessionConnectorPacket(long packetType, long result, BMachineID machineID, const BSessionUser users[]);
      BSessionConnectorPacket(long packetType, const XNADDR& xnaddr, BMachineID machineID, const BSessionUser users[]);
      BSessionConnectorPacket(long packetType, const XNADDR& xnaddr, BMachineID machineID, const BSessionUser users[], const BProxyInfo& proxyInfo);
      BSessionConnectorPacket(long packetType, DWORD crc, const XNADDR& xnaddr, const BSessionUser users[]);

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

      BProxyInfo   mProxy;
      BSessionUser mUsers[BMachine::cMaxUsers];
      XNADDR       mXnAddr;
      int32        mResult;
      uint32       mCRC;
      int8         mMachineID;
};

//==============================================================================
class BDisconnectPacket : public BTypedPacket
{
   public:
      BDisconnectPacket();
      BDisconnectPacket(long packetType);

      void init(const BChannelTracker& tracker);

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

      const BChannelTracker& getTracker() const { return mChannelTracker; }

   private:
      BChannelTracker mChannelTracker;
};

//==============================================================================
class BDisconnectSyncPacket : public BTypedPacket
{
   public:
      BDisconnectSyncPacket();

      void init(BMachineID machineID, uint size, const void* pData);

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

      BMachineID  getMachineID() const { return mMachineID; }
      const void* getData() const { return mpData; }
      uint        getSize() const { return static_cast<uint>(mSize); }

   private:
      BMachineID  mMachineID;
      const void* mpData;
      uint16      mSize;
};

//==============================================================================
class BPlayerIDPacket : public BChannelPacket
{
public:
   BPlayerIDPacket()
      : BChannelPacket(BPacketType::cPlayerIndexPacket)
      , mPlayerID(0)
      , mXuid(0) {}

   BPlayerIDPacket(long playerID, const XUID xuid)
      : BChannelPacket(BPacketType::cPlayerIndexPacket)
      , mPlayerID(playerID)
      , mXuid(xuid) {}

   virtual void serialize(BSerialBuffer& sb)   
   { 
      BChannelPacket::serialize(sb); 

      sb.add(mPlayerID);
      sb.add(mXuid);
   }

   virtual void deserialize(BSerialBuffer& sb)   
   { 
      BChannelPacket::deserialize(sb); 

      sb.get(&mPlayerID);
      sb.get(&mXuid);
   }

   int32 mPlayerID;
   XUID  mXuid;
};

//==============================================================================
class BDWORDSAndStringsPacket : public BTypedPacket
{
public:
   BDWORDSAndStringsPacket(long packetType) : BTypedPacket(packetType), mDWORDS(0), mStrings(0), mCount(0) {}      
   BDWORDSAndStringsPacket(uint32 *dwords, const char **addresses, uint32 count, long packetType) :
      BTypedPacket(packetType),
      mDWORDS(0),
      mCount(count),
      mStrings(0)
   {
      BASSERT(mCount > 0);
      mDWORDS = new uint32[mCount];
      memcpy(mDWORDS, dwords, count*sizeof(uint32));
      mStrings = new char*[mCount];
      for (uint32 i=0; i < count; i++)
      {
         if (addresses[i])
         {
            const long length = strlen(addresses[i])+1;
            mStrings[i] = new char[length];
            StringCchCopyA(mStrings[i], length, addresses[i]);
         }
         else
            mStrings[i] = 0;
      }
   }

   virtual ~BDWORDSAndStringsPacket(void)
   {
      if (mDWORDS) delete[] mDWORDS;
      for (uint32 i=0; i < mCount; i++)
      {
         if (mStrings && mStrings[i])
            delete mStrings[i];
      }
      if (mStrings)
         delete[] mStrings;
   }

   virtual void            serialize(BSerialBuffer& sb)
   {
      BTypedPacket::serialize(sb);
      sb.add(mCount);
      for (uint32 i=0; i < mCount; i++)
         sb.add(mDWORDS[i]);
      for (uint32 i=0; i < mCount; i++)
      {
         int32 len = 0;
         if (mStrings[i])
            len = strlen(mStrings[i])+1;
         sb.add(len);
         if (len)
            sb.add(mStrings[i], len);
      }
   }
   virtual void            deserialize(BSerialBuffer& sb)
   {
      BTypedPacket::deserialize(sb);
      sb.get(&mCount);
      mDWORDS = new uint32[mCount];
      for (uint32 i=0; i < mCount; i++)
         sb.get(&mDWORDS[i]);
      mStrings = new char*[mCount];
      for (uint32 i=0; i < mCount; i++)
      {
         int32 len = 0;
         sb.get(&len);
         if (len)
         {
            mStrings[i] = new char[len];
            sb.get(&mStrings[i], len);
         }
         else
            mStrings[i] = 0;
      }
   }

public:
   uint32*                  mDWORDS;
   int8**                   mStrings;
   uint32                   mCount;
};

//==============================================================================
class BTimeIntervalPacket : public BChannelPacket
{
   public:
      BTimeIntervalPacket();
      BTimeIntervalPacket(long type);
      BTimeIntervalPacket(long type, uint32 interval, uint32 timing);

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

   public:
      uint8 mInterval;
      uint8 mTiming;
};

//==============================================================================
class BTimeSyncPacket : public BChannelPacket
{
   public:
      BTimeSyncPacket();
      BTimeSyncPacket(long type);
      BTimeSyncPacket(long type, uint32 time, uint32 timing);

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

   public:
      uint32 mTime;
      uint8  mTiming;
};

//==============================================================================
class BVotePacket : public BChannelPacket
{
public:
   BVotePacket(long type) : BChannelPacket(type), mVoteType(-1), mVote(-1), mArg1(-1) {}
   BVotePacket(long type, long votetype, long vote = -1, long arg1 = -1) : BChannelPacket(type), mVoteType(votetype), mVote(vote), mArg1(arg1) {}

   virtual void               serialize(BSerialBuffer& sb) { BChannelPacket::serialize(sb); sb.add(mVoteType); sb.add(mVote); sb.add(mArg1); }
   virtual void               deserialize(BSerialBuffer& sb) { BChannelPacket::deserialize(sb); sb.get(&mVoteType); sb.get(&mVote); sb.get(&mArg1); }

   int32                      mVoteType;
   int32                      mVote;
   int32                      mArg1;
};

//==============================================================================
class BLSPPacket : public BPacket
{
   public:
      static const uint8 cPacketVersion = 1;

      BLSPPacket();
      BLSPPacket(uint8 serviceId, uint8 packetType);

      virtual ~BLSPPacket() {};

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

      uint32 getID() const;
      uint8 getType() const;
      uint8 getServiceId() const;
      uint8 getVersion() const;
      int32 getResult() const;

      void setServiceId(uint8 serviceId);

   private:
      static uint32 mNextId;

      uint32 mId;
      int32  mResult;   // this is an HRESULT value that individual services may interpret however they want
      uint8  mServiceId;
      uint8  mType;
      uint8  mBaseVersion;
};

//==============================================================================
class BLSPCommandPacket : public BLSPPacket
{
   public:
      enum eCommands { cCommandInvalid, cCommandHello, cCommandHelloResponse, cCommandReset };

      BLSPCommandPacket() :
         BLSPPacket(0, BPacketType::cLSPCommandPacket),
         mCommand(cCommandInvalid)
      {}

      BLSPCommandPacket(uint8 serviceId, eCommands command) :
         BLSPPacket(serviceId, BPacketType::cLSPCommandPacket)
      {
         mCommand = static_cast<int8>(command);
      }

      virtual ~BLSPCommandPacket() {}

      virtual void serialize(BSerialBuffer& sb)
      {
         BLSPPacket::serialize(sb);
         sb.add(mCommand);
      }

      virtual void deserialize(BSerialBuffer& sb)
      {
         BLSPPacket::deserialize(sb);
         sb.get(&mCommand);
      }

   public:
      int8 mCommand;
};

//==============================================================================
class BStreamPacket : public BLSPPacket
{
   public:

      static const uint8 cStreamPacketVersion = 1;

      enum eCommands { cCommandInvalid, cCommandStoreRequest, cCommandStoreResponse, cCommandStoreComplete, cCommandRetrieveRequest, cCommandRetrieveResponse, cCommandRetrieveComplete };

      BStreamPacket() :
         BLSPPacket(0, BPacketType::cStreamPacket),
         mStreamID(0),
         mCommand(cCommandInvalid),
         mXuid(0)
      {}

      BStreamPacket(uint8 serviceId, eCommands command) :
         BLSPPacket(serviceId, BPacketType::cStreamPacket),
         mStreamID(0),
         mXuid(0)
      {
         mCommand = static_cast<int8>(command);
      }

      BStreamPacket(uint8 serviceId, eCommands command, const XUID xuid, const char* pGamertag, const char* pFilename) :
         BLSPPacket(serviceId, BPacketType::cStreamPacket),
         mStreamID(0),
         mGamertag(pGamertag),
         mFilename(pFilename),
         mXuid(xuid)
      {
         mCommand = static_cast<int8>(command);
      }

      virtual ~BStreamPacket() {}

      virtual void serialize(BSerialBuffer& sb)
      {
         BLSPPacket::serialize(sb);
         sb.add(mCommand);
         sb.add(mStreamID);
         sb.add(mXuid);
         sb.add(mGamertag);
         sb.add(mFilename);
      }

      virtual void deserialize(BSerialBuffer& sb)
      {
         BLSPPacket::deserialize(sb);
         sb.get(&mCommand);
         sb.get(&mStreamID);
         sb.get(&mXuid);
         sb.get(&mGamertag);
         sb.get(&mFilename);
      }

   public:
      BString     mGamertag;
      BString     mFilename;
      XUID        mXuid;
      uint64      mStreamID;
      int8        mCommand;
};

//==============================================================================
class BMediaPacket : public BLSPPacket
{
   public:

      static const uint8 cMediaPacketVersion = 1;

      enum eCommands
      {
         cCommandInvalid,
         cCommandStoreRequest,
         cCommandStoreResponse,
         cCommandStoreComplete,
         cCommandRetrieveRequest,
         cCommandRetrieveResponse,
         cCommandRetrieveComplete,
         cCommandListRequest,
         cCommandListResponse,
         cCommandDeleteRequest,
         cCommandDeleteResponse,
      };

      BMediaPacket();
      BMediaPacket(uint8 serviceId);
      BMediaPacket(uint8 serviceId, eCommands command);
      BMediaPacket(uint8 serviceId, eCommands command, const XUID xuid, const uint64 mediaID);

      void init(eCommands command, const XUID xuid, const uint64 mediaID, const uint32 crc=0);

      virtual ~BMediaPacket() {}

      uint32 getTTL() const;

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

   public:
      uint64      mMediaID;
      uint64      mStreamID; // only used for store/retrieve requests
      XUID        mXuid;
      uint32      mCRC;
      uint32      mTTL;
      uint16      mSize;
      const void* mpData;
      int8        mCommand;
      uint8       mVersion;
};

//==============================================================================
class BServiceRecordPacket : public BLSPPacket
{
   public:

      enum eCommands
      {
         cCommandInvalid,
         cCommandRequest,
         cCommandSuccess,
         cCommandDoesNotExist,
         cCommandError,
      };

      BServiceRecordPacket();
      BServiceRecordPacket(uint8 serviceId);
      BServiceRecordPacket(uint8 serviceId, eCommands command);
      BServiceRecordPacket(uint8 serviceId, eCommands command, const XUID xuid);

      void init(eCommands command, const XUID xuid);

      virtual ~BServiceRecordPacket() {}

      uint32 getTTL() const;

      virtual void serialize(BSerialBuffer& sb);
      virtual void deserialize(BSerialBuffer& sb);

   public:
      XUID        mXuid;
      uint32      mTTL;
      uint16      mSize;
      const void* mpData;
      int8        mCommand;
};

//==============================================================================
class BChunkPacket : public BLSPPacket
{
   // NOTE: This packet requires immediate serialization, because it doesn't copy data for itself, but rather
   // copies it only on serialization - so the data you pass in must last until that point
   // it also requires immediate grabbing of data after deserialization, because after it goes out of scope,
   // mData is no longer valid
   public:
      enum eCommands { cCommandInvalid, cCommandChunk, cCommandAck, cCommandComplete };

      BChunkPacket() :
         BLSPPacket(0, BPacketType::cChunkPacket),
         mStreamID(0),
         mpData(0),
         mSize(0),
         mCommand(cCommandInvalid)
      {}

      BChunkPacket(uint8 serviceId, eCommands command, uint64 streamID, void* pData, long size) :
         BLSPPacket(serviceId, BPacketType::cChunkPacket),
         mStreamID(0),
         mpData(0),
         mSize(0),
         mCommand(cCommandInvalid)
      {
         mStreamID = streamID;
         mpData = pData;
         mSize = static_cast<int16>(size);
         mCommand = static_cast<int8>(command);
      }

      virtual ~BChunkPacket() {}

      virtual void serialize(BSerialBuffer& sb)
      {
         BLSPPacket::serialize(sb);
         sb.add(mCommand);
         sb.add(mStreamID);
         sb.add(mSize);
         sb.add(mpData, mSize);
      }

      virtual void deserialize(BSerialBuffer& sb)
      {
         BLSPPacket::deserialize(sb);
         sb.get(&mCommand);
         sb.get(&mStreamID);
         // only chunks and complete commands will have data
         if (mCommand == cCommandChunk || mCommand == cCommandComplete)
         {
            sb.get(&mSize);
            int32 size = static_cast<int32>(mSize);
            sb.getPointer(&mpData, &size);
         }
      }

      uint64 getStreamID() const { return mStreamID; }

   public:
      uint64      mStreamID;
      const void* mpData;
      int16       mSize;
      int8        mCommand;
};

//==============================================================================
//
//==============================================================================
class BAuthUser
{
   public:
      BAuthUser();
      BAuthUser(XUID xuid, BSimString& gamerTag);

      void              serialize(BSerialBuffer& sb);
      void              deserialize(BSerialBuffer& sb);

      void              setXuid(XUID xuid);

      XUID              getXuid() const;
      const BSimString& getGamerTag() const;
      bool              isBanMedia() const;
      bool              isBanMatchMaking() const;
      bool              isBanEverything() const;

   private:
      XUID        mXuid;
      BSimString  mGamerTag;
      bool        mBanMedia;
      bool        mBanMatchMaking;
      bool        mBanEverything;
};

//==============================================================================
//
//==============================================================================
class BAuthPacket : public BLSPPacket
{
   public:
      enum eCommands { cCommandInvalid, cCommandAuthRequest, cCommandAuthResponse };

      BAuthPacket();

      BAuthPacket(const uint8 serviceId, const eCommands command);
      virtual ~BAuthPacket();

      virtual void serialize(BSerialBuffer& sb);

      virtual void deserialize(BSerialBuffer& sb);

      eCommands getCommand() const;

      uint getTTL() const;

      const BAuthUser& getMachine() const;

      const BSmallDynamicSimArray<BAuthUser>& getUsers() const;

      void setMachine(XUID xuid);
      void addUser(XUID xuid, BSimString& gamerTag);

   private:
      eCommands   mCommand;
      BAuthUser   mMachine;
      BSmallDynamicSimArray<BAuthUser> mUsers;
      uint        mTTL;
};

//==============================================================================
//
//==============================================================================
class BConfigDataRequestPacket : public BLSPPacket
{
   public:

      BConfigDataRequestPacket(uint8 serviceId, WORD staticVersion, WORD dynamicVersion);
      virtual ~BConfigDataRequestPacket();

      virtual void serialize(BSerialBuffer& sb);

      virtual void deserialize(BSerialBuffer& sb);

   private:
      WORD mStaticVersion;
      WORD mDynamicVersion;
};

//==============================================================================
//
//==============================================================================
class BRequestFileUploadPacket : public BLSPPacket
{
   public:
      BRequestFileUploadPacket(const uint8 serviceID, const BSimString& gamerTag, XUID id, const BSimString& fileName, uint fileSize);
      virtual ~BRequestFileUploadPacket();
      virtual void serialize(BSerialBuffer& sb);

   private: // disable implicit constructors and operator= (don't want these getting into containers incorrectly)
      BRequestFileUploadPacket();
      BRequestFileUploadPacket(const BRequestFileUploadPacket& );
      BRequestFileUploadPacket& operator= (const BRequestFileUploadPacket& );

   private:
      const XUID mXuid;
      const BSimString mGamerTag;
      const BSimString mFileName;
      uint mFileSize;
};

//==============================================================================
//
//==============================================================================
class BFileUploadBlockPacket : public BLSPPacket
{
public:
   explicit BFileUploadBlockPacket(const uint8 serviceID, BStream& source);
   virtual ~BFileUploadBlockPacket();
   virtual void serialize(BSerialBuffer& sb);
private:
   BStream& mStream; // the source MUST remain in scope for the duration of the lifetime of this packet!
};

//==============================================================================
//
//==============================================================================
class BFileUploadCompletePacket : public BLSPPacket
{
public:
   BFileUploadCompletePacket(const uint8 serviceID);
   virtual ~BFileUploadCompletePacket();
};

//==============================================================================
//
//==============================================================================
class BConfigDataResponsePacket : public BLSPPacket
{
   public:

      enum
      {
         cNoData = 0,
         cStatic = 1,
         cDynamic = 2,
      };

      const static byte cConfigDataResponseMaxEntries = 10;

      BConfigDataResponsePacket();
      virtual ~BConfigDataResponsePacket();

      virtual void deserialize(BSerialBuffer& sb);

      struct BHopperEntry
      {
         uint32 mHostDelayTimeBase;
         uint32 mHostDelayTimeRandom;
         uint32 mHostTime;
         uint32 mAverageWait; // dynamic only
         uint16 mUserCount; // dynamic only
         uint16 mHopperIndex;
         byte mFastScanSearchCount;
         byte mNormalSearchCount;
         bool mEnabled;

         BHopperEntry() :
            mHostDelayTimeBase(0),
            mHostDelayTimeRandom(0),
            mHostTime(0),
            mAverageWait(0),
            mUserCount(0),
            mHopperIndex(0),
            mFastScanSearchCount(0),
            mNormalSearchCount(0),
            mEnabled(false)
            {}
      };
      //Global entries:
      uint32      mTTL;
      uint32      mStaticTTL;
      uint32      mDynamicTTL;
      uint32      mMaxMatchmakingTimeTotal;
      uint32      mGlobalUserCount; // dynamic only
      uint16      mPreferredPing;
      uint16      mMaxPing;
      int16       mStaticVersion;
      int16       mDynamicVersion;
      bool        mTwoStageQOSEnabled;
      //Per Hopper entries:
      BSmallDynamicSimArray<BHopperEntry> mEntries;
};

//==============================================================================
//
//==============================================================================
class BConfigDataResponsePacketV2 : public BLSPPacket
{
public:

   enum
   {
      cNoData = 0,
      cStatic = 1,
      cDynamic = 2,
   };

   BConfigDataResponsePacketV2();
   virtual ~BConfigDataResponsePacketV2();

   virtual void deserialize(BSerialBuffer& sb);

   struct BHopperEntry
   {      
      uint16 mUserCount;   // dynamic only
      byte   mHopperIndex;
   };

   //Global entries:
   const char* mStaticData;
   int32       mStaticDataSize;
   uint32      mTTL;
   uint32      mStaticTTL;
   uint32      mDynamicTTL;
   int16       mStaticVersion;
   int16       mDynamicVersion;
   uint32      mGlobalUserCount; // dynamic only
   //Per Hopper dynamic data entries:
   BSmallDynamicSimArray<BHopperEntry> mEntries;
};

//==============================================================================
//
//==============================================================================
class BPerfLogPacket : public BLSPPacket
{
   public:

      BPerfLogPacket();
      BPerfLogPacket(uint8 serviceId);

      virtual ~BPerfLogPacket();

      virtual void serialize(BSerialBuffer& sb);

      //virtual void deserialize(BSerialBuffer& sb);        //Only deserialized on the host

      enum
      {
         cPerfLogMaxXuids = 3,
      };

      //Just making these public since the accessors would just be direct manipulation
      struct BPerfScanData
      {
         uint32   mSearchTime; // 4
         uint32   mScanTime; // 4
         byte     mResultCount; // 1
         byte     mPostQOSResultCount; // 1
         byte     mConnectionAttempts; // 1

         BPerfScanData()
         {
            mSearchTime = 0;
            mScanTime = 0;
            mResultCount = 0;
            mPostQOSResultCount = 0;
            mConnectionAttempts = 0;
         }
      };

      uint64         mXuids[cPerfLogMaxXuids]; // 8 * 3 == 24
      BPerfScanData  mFastScanData; // 11
      BPerfScanData  mFullScanData;
      uint64         mAverageRating; // 8
      uint64         mMatchMakingNonce;
      uint32         mTotalRunTime;
      uint32         mHostDelayTime;
      uint32         mHostTime;
      float          mAverageSigma;
      float          mAverageMu;
      uint16         mAverageGameCount;
      byte           mEventCode;
      byte           mHopperIndex;
      byte           mPartyCount;
      byte           mHostAttemptedJoins;
      byte           mHostSuccessfulJoins;
};
//cPerfLogCyclePacket

//==============================================================================
//
//==============================================================================
class BPerfLogPacketV2 : public BLSPPacket
{
public:

   BPerfLogPacketV2();
   BPerfLogPacketV2(uint8 serviceId);

   virtual ~BPerfLogPacketV2();

   virtual void serialize(BSerialBuffer& sb);
   //Helper function to convert time in milliseconds to time in 1/10th of a second (capped to uint16 range)
   static void setTimeFromMSTime( uint16* targetValue, uint timeValue);
   static void addTimeFromMSTime( uint16* targetValue, uint timeValue);

   //virtual void deserialize(BSerialBuffer& sb);        //Only deserialized on the host

   enum
   {
      cPerfLogMaxXuids = 3,
   };

   //Just making these public since the accessors would just be direct manipulation
   struct BPerfScanData
   {
      byte     mResultCount;        // 1
      byte     mPostQOSResultCount; // 1
      byte     mConnectionAttempts; // 1

      BPerfScanData()
      {
         mResultCount = 0;
         mPostQOSResultCount = 0;
         mConnectionAttempts = 0;
      }
   };

   uint64         mMatchMakingNonce;         // 8
   uint64         mLaunchedMatchHostNonce;   // 8
   uint64         mXuids[cPerfLogMaxXuids];  // 8 * 3 == 24
   BPerfScanData  mFastScanData;             // 3
   BPerfScanData  mSearchData;               // 3
   uint16         mTotalRunTime;             // 2   These times are measured in whole tenth's of a second. IE: 107 = 10.7 seconds
   uint16         mHostDelayTime;            // 2   The max tracked time for anything is 109 minutes, plenty long
   uint16         mHostTime;                 // 2
   uint16         mFastScanTime;             // 2
   uint16         mSearchTime;               // 2
   uint16         mAverageGameCount;         // 2
   uint16         mLaunchedMatchPingAverage; // 2
   byte           mAverageRating;            // 1 //Whole number from 0 to 50
   byte           mEventCode;                // 1
   byte           mHopperIndex;              // 1
   byte           mCycleCount;               // 1
   byte           mLaunchedMatchSkillAverage;   // 1
   byte           mLaunchedMatchRatingQuality;  // 1   A number from 0 to 100 of how well matched the game was

};
//BPerfLogPacketV2

//==============================================================================
class BTalkersPacket : public BTypedPacket
{
   public:
      enum { cMaxClients = XNetwork::cMaxClients };

      BTalkersPacket();

      BTalkersPacket(const XUID ownerXuid, const XUID* pXuids);

      virtual void serialize(BSerialBuffer& sb);

      virtual void deserialize(BSerialBuffer& sb);

      XUID        mXuids[cMaxClients];
      XUID        mOwnerXuid;
      uint32      mCount;
};

//==============================================================================
class BVoiceHeadsetPacket : public BTypedPacket
{
   public:
      BVoiceHeadsetPacket();
      BVoiceHeadsetPacket(uint clientID, BOOL headset);

      virtual void serialize(BSerialBuffer& sb);

      virtual void deserialize(BSerialBuffer& sb);

      uint mClientID;
      uint mHeadset;
};

//==============================================================================
class BTickerRequestPacket : public BLSPPacket
{
   public:
      BTickerRequestPacket(const uint8 serviceId);
      virtual ~BTickerRequestPacket();

      virtual void serialize(BSerialBuffer& sb);

      void addUser(XUID xuid);

   private:
      BSmallDynamicSimArray<XUID> mUsers;
      uint8 mLanguage;
};

//==============================================================================
class BTickerResponsePacket : public BLSPPacket
{
   public:
      BTickerResponsePacket();
      virtual ~BTickerResponsePacket();

      virtual void deserialize(BSerialBuffer& sb);

      const BString & getTickerText() const;
      const uint8 getPriority() const;
      const int getLifetime() const;

   private:
      BString mTickerText;
      uint8 mPriority;
      int mLifetime;
};

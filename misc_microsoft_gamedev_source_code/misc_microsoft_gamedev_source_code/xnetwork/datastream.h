//==============================================================================
// datastream.h
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

#ifndef _DATASTREAM_H_
#define _DATASTREAM_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BDataStream;
class BPacket;

//==============================================================================
// Const declarations

//==============================================================================
// The BDataStream class has two modes of operation:
// #1 Pull Send - call startDataSend(), and override your child class' getDataFromSource() function
//   which will automatically be called to pull data from a source
// #2 Receive - incomming data will call putDataToDestination, which you need to override

class BDataStreamTransport
{
   public:
      virtual void      sendPacket(BPacket &packet)=0;
};

//==============================================================================
class BDataStream
{
   public:

      enum eState             {  cStateIdle=0, cStateSend, cStateRecv, cStateComplete, cStateTimedout, cTotalStates };

      BDataStream(BDataStreamTransport *transport, unsigned char streamID);
      virtual ~BDataStream();

      virtual bool            startDataSend(void);
      virtual bool            startDataRecv(void);
      virtual void            stopDataTransfer(void);  

      virtual long            service(void);      
      virtual bool            dataAvailable(const void* data, long size);

      virtual DWORD           getDataFromSource(unsigned char &streamID, void* buffer, DWORD size) { streamID;buffer;size;return 0; }
      virtual bool            putDataToDestination(const void* buffer, DWORD size) { buffer;size;return true; }

   protected:      
      virtual void            streamComplete(void) { mCurrentState=cStateComplete; }
      enum                    {  cDefaultBufferSize = 250, cBytesPerAck = 4000, cDefaultTimeout = 10000 };
      long                    getState(void) const { return mCurrentState; }      

      unsigned char           mStreamID;

   private:
      BDataStream();

      bool                    mWaitingOnAck;    // waiting for send ack
      long                    mBytesTransfered; // per acknowledgement
      DWORD                   mLastDataTime;    // last time data was sent/recv
      eState                  mCurrentState;
      BYTE                    mBuffer[cDefaultBufferSize];
      BDataStreamTransport    *mTransport;
};

//==============================================================================
#endif // _DATASTREAM_H_

//==============================================================================
// eof: datastream.h
//==============================================================================


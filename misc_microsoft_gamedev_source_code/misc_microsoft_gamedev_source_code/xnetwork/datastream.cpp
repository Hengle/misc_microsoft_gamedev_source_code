//==============================================================================
// DataStream.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "datastream.h"
#include "netpackets.h"

//==============================================================================
// BDataStream::BDataStream
//==============================================================================
BDataStream::BDataStream(BDataStreamTransport *transport, unsigned char streamID): mCurrentState(BDataStream::cStateIdle), 
                             mWaitingOnAck(false), mLastDataTime(0), mBytesTransfered(0), 
                             mTransport(transport), mStreamID(streamID)
{
}

//==============================================================================
// BDataStream::~BDataStream
//==============================================================================
BDataStream::~BDataStream()
{
}

//==============================================================================
// BDataStream::startDataSend
//==============================================================================
bool BDataStream::startDataSend()
{
   mCurrentState = cStateSend;   
   mWaitingOnAck = false;

   return (true);
}

//==============================================================================
// BDataStream::startDataRecv
//==============================================================================
bool BDataStream::startDataRecv(void)
{
   mCurrentState = cStateRecv;
   mLastDataTime = timeGetTime();
   
   return (true);
}

//==============================================================================
// BDataStream::stopDataTransfer
//==============================================================================
void BDataStream::stopDataTransfer(void)
{
   mCurrentState = cStateIdle;    
}

//==============================================================================
// BDataStream::service
//==============================================================================
long BDataStream::service(void)
{
   if (!mTransport)
      return(cStateIdle);

   // sending data
   if (mCurrentState == cStateSend)
   {
      // if we are waiting on a response, check for a time out
      if (mWaitingOnAck)
      {
         if ( (timeGetTime() - mLastDataTime) > cDefaultTimeout)
         {
            mCurrentState = cStateTimedout;
            mLastDataTime = 0;
         }
      }
      else
      {
         bool done = false;
         unsigned char streamID = 0;
         while (mBytesTransfered < cBytesPerAck)
         {
            DWORD bytes = getDataFromSource(streamID, mBuffer, cDefaultBufferSize);
            if (bytes!=0)
            {
               BDataBlockPacket packet(streamID, mBuffer, bytes, BPacketType::cDataStreamPacket);

               // send the chunk
               mTransport->sendPacket(packet);
               mBytesTransfered+=bytes;            
            }
            else
            {
               done = true;
               break;
            }
         }

         if (!done && mBytesTransfered > 0)
         {
            BDataFlowControlPacket packet;
            packet.mCommand = BDataFlowControlPacket::cCommandAckRequest;
            packet.mStreamID = streamID;
            packet.mData = min(cBytesPerAck, mBytesTransfered);
         
            mWaitingOnAck = true;

            mTransport->sendPacket(packet);
            mLastDataTime = timeGetTime();
         }

         if (done)
         {
            BDataFlowControlPacket packet;
            packet.mCommand = BDataFlowControlPacket::cCommandComplete;
            packet.mStreamID = streamID;
            mTransport->sendPacket(packet);
            mLastDataTime = timeGetTime();
            mCurrentState = cStateComplete;
            streamComplete();
         }
      }
   }
   // receiving data
   else if (mCurrentState == cStateRecv)
   {
      // check for a timeout
      if ( (timeGetTime() - mLastDataTime) > cDefaultTimeout)
      {
         mCurrentState = cStateTimedout;
         mLastDataTime = 0;
      }
   }

   return(mCurrentState);
}

//==============================================================================
// BDataStream::dataAvailable
//==============================================================================
bool BDataStream::dataAvailable(const void* data, long size)
{  
   bool bHandled = false;

   if (BTypedPacket::getType(data) == BPacketType::cDataBlockFlowControlPacket)
   {
      BDataFlowControlPacket packet;
      packet.deserializeFrom(data, size);

      switch (packet.mCommand)
      {
         case BDataFlowControlPacket::cCommandAckRequest:
         {
            // if we are the receiver and an ack is requested from sender
            if ( (mCurrentState == cStateRecv) )
            {
               // if we got the amount of bytes sent, send the ack
               if (mBytesTransfered == packet.mData)
               {
                  BDataFlowControlPacket rpacket;
                  rpacket.mStreamID = packet.mStreamID;
                  rpacket.mCommand = BDataFlowControlPacket::cCommandAck;
                  rpacket.mData = packet.mData;                  
                  mTransport->sendPacket(rpacket);

                  mBytesTransfered = 0;
                  mLastDataTime = timeGetTime();                  
               }
            }
            break;
         }

         case BDataFlowControlPacket::cCommandAck:
         {
            // if we are waiting on recv to ack, reset to CTS
            if ( mWaitingOnAck )
            {
               mBytesTransfered = 0;
               mLastDataTime = timeGetTime();
               mWaitingOnAck = false;
            }
            break;
         }
         
         case BDataFlowControlPacket::cCommandComplete:
         {
            streamComplete();
         }
         break;
      }
      return(true);
   }

   // if the stream is receiving data and this is a packet we are interested in
   if (mCurrentState == cStateRecv)
   {
      const void *rdata;
      long rsize;
      if (BTypedPacket::getType(data) == BPacketType::cDataStreamPacket)
      {
         BDataBlockPacket packet(BPacketType::cDataStreamPacket);
         packet.deserializeFrom(data, size);
         rdata = packet.mData;
         rsize = packet.mSize;
      }
      else if (BTypedPacket::getType(data) == BPacketType::cWrapperPacket)
      {
         BWrapperPacket packet;
         packet.deserializeFrom(data, size);
         rdata = packet.mData;
         rsize = packet.mSize;
      }
      else
         return false;

      // if there was an error putting data
      if (!putDataToDestination(rdata, rsize))
      {
         // stop the transfer
         stopDataTransfer();
      }
      else
      {
         mBytesTransfered += rsize;
         mLastDataTime = timeGetTime();
         bHandled=true;
      }
   }

   return(bHandled);
}

//==============================================================================
// OrderedChannel.cpp
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "OrderedChannel.h"
#include "Session.h"
#include "TimeSync.h"
#ifndef _STANDALONE
   // @TITAN -- fix this to have proper library dependencies 
   //#include "syncmacros.h"
   #define syncCommData(str, data) ((void)0)
#else
   #define syncCommData(str, data) ((void)0)
#endif

//==============================================================================
// Defines

//==============================================================================
// 
//==============================================================================
void BSequencedData::init(void* pData, long size, BMachineID machineID, DWORD time)
{
   mpData = pData;
   mSize = size;
   mMachineID = machineID;
   mTime = time;
}

//==============================================================================
// 
//==============================================================================
BOrderedChannel::BOrderedChannel() :
   BChannel(),
   mInService(false),
   mDataList(&gNetworkHeap)
{
}

//==============================================================================
// 
//==============================================================================
BOrderedChannel::BOrderedChannel(long channelID, BSession* pSession) :
   BChannel(channelID, pSession),
   mInService(false),
   mDataList(&gNetworkHeap)
{
}

//==============================================================================
// BOrderedChannel::service
//==============================================================================
void BOrderedChannel::service()
{
   // if you re-enter this function, it will 'splode, so let's not do that
   if (mInService)
      return;

   mInService = true;

   // go through recv queue, pull of things that are available
   // in order of oldest to newest, client # ordered

   if (getSession()->getTimeSync() && getSession()->getTimeSync()->isTimeRolling())
   {
      if (getSession()->getTimeSync()->getRecvTime() != mLastRecvTime)
      {
         // time has rolled forward, we need to look for newly available data

         // iterate over data, notifying

         BHandle h;
         BSequencedData* p = mDataList.getHead(h);
         while (p)
         {
            // if data is older to the current recv time, notify   (NOES on "or equal" - says DOUG!)
            if (p->mTime < getSession()->getTimeSync()->getRecvTime())
            {
               syncCommData("ordered channel - data available[0]", (long)((char *)p->mpData)[0]);
               syncCommData("ordered channel - data available[1]", (long)((char *)p->mpData)[1]);
               syncCommData("  p->mSize", p->mSize);
               syncCommData("  p->time", p->mTime);
               syncCommData("  getRecvTime()", getSession()->getTimeSync()->getRecvTime());

               nlog(cTimeSyncNL, "ordered channel - data available[0] %ld", (long)((char *)p->mpData)[0]);
               nlog(cTimeSyncNL, "ordered channel - data available[1] %ld", (long)((char *)p->mpData)[1]);
               nlog(cTimeSyncNL, "  p->mSize %ld", p->mSize);
               nlog(cTimeSyncNL, "  p->time %ld", p->mTime);
               nlog(cTimeSyncNL, "  getRecvTime() %ld", getSession()->getTimeSync()->getRecvTime());

               syncCommData("  notifyDataAvailable - fromMachine", p->mMachineID);
               nlog(cTimeSyncNL, "  notifyDataAvailable - fromMachine[%ld]", p->mMachineID);
               getObserverList()->channelDataReceived(p->mMachineID, p->mpData, p->mSize);

               // check to make sure dispose wasn't called because of the above function
               if (!mDataList.isEmpty())
               {
                  if (p->mpData)
                     HEAP_DELETE(p->mpData, gNetworkHeap);
                  HEAP_DELETE(p, gNetworkHeap);
                  p = mDataList.removeAndGetNext(h);
               }
               else
                  p = 0;
            }
            else
               p = mDataList.getNext(h);
         }

         mLastRecvTime = getSession()->getTimeSync()->getRecvTime();
      }
   }

   mInService = false;
}

//==============================================================================
// BOrderedChannel::~BOrderedChannel
//==============================================================================
BOrderedChannel::~BOrderedChannel()
{
   dispose();
} // BOrderedChannel::~BOrderedChannel

//==============================================================================
// 
HRESULT BOrderedChannel::dispose()
{
   BHandle h;
   BSequencedData* p = mDataList.getHead(h);
   while (p)
   {
      if (p->mpData)
         HEAP_DELETE(p->mpData, gNetworkHeap);
      HEAP_DELETE(p, gNetworkHeap);
      p = mDataList.getNext(h);
   }
   mDataList.reset();

   return BChannel::dispose();
} // BOrderedChannel::~BOrderedChannel

//==============================================================================
// BPeerOrderedChannel::channelDataReceived
//==============================================================================
void BPeerOrderedChannel::channelDataReceived(const long clientIndex, const void* pData, const DWORD size)
{
   // because of the channel syncing that occurs upon clients disconnecting, we don't want to lookup the
   // BMachine here since it will return one that's been reset to the defaults, meaning it's getID() method
   // will return -1
   //
//-- FIXING PREFIX BUG ID 7542
   //const BMachine* pMachine = getSession()->getMachine(clientIndex);
//--
   // don't bother asserting here as we may still receive channel data for recently disconnected clients
   //BDEBUG_ASSERTM(pClient, "Failed to find client for channel data");
   //if (pMachine == NULL)
   //   return;

   // have to do seperate data copy
   void* pNewData = gNetworkHeap.New(size);
   Utils::FastMemCpy(pNewData, pData, size);

   // the clientIndex is now the BMachineID since we retro-fitted code to still support the notion of a client mapping to an actual player
   // and the machine can support two clients for splitscreen modes
   //
   //BMachineID machineID = pMachine->getID();
   BMachineID machineID = clientIndex;
   uint32 sendTime = getSession()->getTimeSync()->getClientSendTime(machineID);

   uint32 offsetTime = BChannelPacket::getTimeOffset(pData);
   uint32 seqTime = sendTime + offsetTime;

   // insert data in order, time first, then player number
   // go backwards through list
   nlog(cGameTimingNL, "stuff ordered data - size[%d], fromMachine[%ld], sendTime[%d], offsetTime[%d], seqTime[%d]", size, machineID, sendTime, offsetTime, seqTime);

   if (mDataList.getSize() == 0)
   {
      nlog(cGameTimingNL, "push_front");
      BSequencedData* pSeqData = HEAP_NEW(BSequencedData, gNetworkHeap);
      pSeqData->init(pNewData, size, machineID, seqTime);
      mDataList.addToHead(pSeqData);
   }
   else
   {
      BHandle h;
      BSequencedData* p = mDataList.getHead(h);
      while (p)
      {
         // if time is less than current position insert
         if (p->mTime > seqTime)
         {
            nlog(cGameTimingNL, "  inserting");
            BSequencedData* pSeqData = HEAP_NEW(BSequencedData, gNetworkHeap);
            pSeqData->init(pNewData, size, machineID, seqTime);
            mDataList.addBefore(pSeqData, h);
            break;
         }
         // if time is same, but clientID is less than current position insert
         else if  (
                     (p->mTime == seqTime) &&
                     (p->mMachineID < machineID)
                  )
         {
            //nlog(cGameTimingNL, "  inserting");
            BSequencedData* pSeqData = HEAP_NEW(BSequencedData, gNetworkHeap);
            pSeqData->init(pNewData, size, machineID, seqTime);
            mDataList.addBefore(pSeqData, h);
            break;
         }
         // else go to next element
         else
            p = mDataList.getNext(h);
      }
      if (!p)
      {
         //nlog(cGameTimingNL, "  push_back");
         BSequencedData* pSeqData = HEAP_NEW(BSequencedData, gNetworkHeap);
         pSeqData->init(pNewData, size, machineID, seqTime);
         mDataList.addToTail(pSeqData);
      }
   }
}

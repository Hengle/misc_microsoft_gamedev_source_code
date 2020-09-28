//==============================================================================
// OrderedChannel.h
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes

#include "Channel.h"
#include "Session.h"
#include "ObserverList.h"
#include "containers\PointerList.h"

//==============================================================================
// Forward declarations

class BTimeSync;

//==============================================================================
// Const declarations

//==============================================================================
class BSequencedData
{
   public:
      BSequencedData() :
         mpData(NULL),
         mSize(0),
         mMachineID(-1),
         mTime(0)
      {}

      BSequencedData(void* pData, long size, BMachineID machineID, DWORD time) :
         mpData(pData),
         mSize(size),
         mMachineID(machineID),
         mTime(time)
      {}

      void init(void* pData, long size, BMachineID machineID, DWORD time);

      void* mpData;
      long  mMachineID;
      long  mSize;
      DWORD mTime;
};

//==============================================================================
class BOrderedChannel : public BChannel
{
   public:
      virtual void                              service();

      virtual void                              sessionConnected() {}
      virtual void                              sessionDisconnected(const long reason) {reason;}

      virtual HRESULT dispose();

   protected:
      // Constructors
      BOrderedChannel();
      BOrderedChannel(long channelID, BSession* pSession);

      BPointerList<BSequencedData>              mDataList;

      // Destructors - garbage collected object
      virtual ~BOrderedChannel();      

   private:
      DWORD                                     mLastRecvTime;
      bool                                      mInService : 1;
}; // BOrderedChannel


//==============================================================================
class BPeerOrderedChannel : public BOrderedChannel
{
   public:
      // Constructors
      BPeerOrderedChannel() :
         BOrderedChannel() {}

      BPeerOrderedChannel(long channelID, BSession* pSession) : 
         BOrderedChannel(channelID, pSession) {}

      virtual void channelDataReceived(const long clientIndex, const void* pData, const DWORD size);

   protected:
      virtual ~BPeerOrderedChannel() {}
}; // BPeerOrderedChannel

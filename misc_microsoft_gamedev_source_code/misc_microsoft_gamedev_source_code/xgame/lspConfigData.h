//==============================================================================
// lspConfigData.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

#include "lspTitleServerConnection.h"
#include "lspCache.h"

//==============================================================================
// 
//==============================================================================
class BLSPConfigData : public BLSPTitleServerConnection
{
   public:
      BLSPConfigData(BLSPConfigCache* pConfigCache);
      virtual ~BLSPConfigData();

      // BXStreamConnection::BXStreamObserver
      virtual void   connected(BXStreamConnection&);
      virtual void   dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size);

      //This starts the process to request the data from the host for a particular hopper set
      void           requestConfigData(BMatchMakingHopperList* pHopperList);
      //Call this to see if it has the data and is ready to apply it to a hopper set

   private:
      BLSPConfigCache*        mpConfigCache;
      BMatchMakingHopperList* mpHopperList;
      bool                    mWaitingOnResponse : 1;
};

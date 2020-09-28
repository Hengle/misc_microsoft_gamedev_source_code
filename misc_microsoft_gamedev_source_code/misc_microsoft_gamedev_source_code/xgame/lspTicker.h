//==============================================================================
// lspTicker.h
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

#pragma once

#include "lspTitleServerConnection.h"
#include "lspCache.h"

class BUITicker;

//==============================================================================
// 
//==============================================================================
class BLSPTicker : public BLSPTitleServerConnection
{
   public:
      BLSPTicker(BUITicker *);
      virtual ~BLSPTicker();

      // BXStreamConnection::BXStreamObserver
      virtual void  connected(BXStreamConnection&);
      virtual void  dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size);

      void  removeTicker ();

   private:

      bool           mWaitingOnResponse : 1;
      BUITicker * mTicker;
};

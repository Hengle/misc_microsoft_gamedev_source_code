//==============================================================================
// lspPerfLog.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

#include "lspTitleServerConnection.h"

//==============================================================================
// 
//==============================================================================
// This LSP system lets the matchmaking system (potentially other systems as well)
//   post performance data up to the LSP.  It packets the data up, sends it, and the
//   LSP takes it, parses it, and writes it out to disk as an XML file

class BLSPPerfLog : public BLSPTitleServerConnection //: public BXStreamConnection::BXStreamObserver
{
   public:
      BLSPPerfLog();
      ~BLSPPerfLog();

      // BXStreamConnection::BXStreamObserver
      virtual void   connected(BXStreamConnection&);

      void           postPerfData(BPerfLogPacketV2* pDataPacket);

   private:
      BPerfLogPacketV2     mOutboundPacket;
      bool                 mWaitingOnResponse : 1;
      bool                 mUnsentRecord : 1;
};

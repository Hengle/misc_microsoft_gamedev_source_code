//==============================================================================
// lspAuth.h
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

#pragma once

#include "lspTitleServerConnection.h"
#include "lspCache.h"

//==============================================================================
// 
//==============================================================================
class BLSPAuth : public BLSPTitleServerConnection
{
   public:
      BLSPAuth(BLSPAuthCache* pAuthCache);
      virtual ~BLSPAuth();

      // BXStreamConnection::BXStreamObserver
      virtual void  connected(BXStreamConnection&);
      virtual void  dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size);

      bool requestSucceeded() const;

   private:

      void initCache();

      BLSPAuthCache* mpAuthCache;
      uint           mDefaultTTL;
      bool           mWaitingOnResponse : 1;
      bool           mSucceeded : 1;
};

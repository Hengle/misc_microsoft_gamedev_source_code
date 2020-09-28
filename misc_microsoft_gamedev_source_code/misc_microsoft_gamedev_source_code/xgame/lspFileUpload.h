//==============================================================================
// lspFileUpload.h
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

#pragma once

#include "lspTitleServerConnection.h"

typedef BStringTemplate<BCHAR_T, BSimFixedHeapAllocator> BSimString;

class BStream;

class BLSPFileUpload : public BLSPTitleServerConnection
{
   public:
      BLSPFileUpload(XUID xuid, const BSimString& gamerTag, const BSimString& fileName, BStream* pStream, uint16 defaultPort, long configPortOverride);
      virtual ~BLSPFileUpload();

      void init();

      // BXStreamConnection::BXStreamObserver
      virtual void   connected(BXStreamConnection&);
      virtual void   sendReady(BXStreamConnection&);

   private:
      const BSimString  mGamertag;
      const BSimString  mFileName;
      XUID              mXuid;
      BStream*          mpStream;
};

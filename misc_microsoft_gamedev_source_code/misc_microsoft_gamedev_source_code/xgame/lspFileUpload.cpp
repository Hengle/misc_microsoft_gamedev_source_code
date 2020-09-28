//==============================================================================
// lspFileUpload.cpp
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

#include "common.h"
#include "configsgame.h"
#include "user.h"
#include "usermanager.h"
#include "lspFileUpload.h"
#include "stream\stream.h"
#include "NetPackets.h"
#include "file\win32FileStream.h"

//Logging
#include "mpcommheaders.h"
#include "commlog.h"

//==============================================================================
// 
//==============================================================================
BLSPFileUpload::BLSPFileUpload(XUID xuid, const BSimString& gamerTag, const BSimString& fileName, BStream* pStream, uint16 defaultPort, long configPortOverride) :
   BLSPTitleServerConnection(defaultPort, configPortOverride, cDefaultLSPStatsServiceID, cConfigLSPStatsServiceID, 0),
   mGamertag(gamerTag),
   mFileName(fileName),
   mXuid(xuid),
   mpStream(pStream)
{
}

//==============================================================================
// 
//==============================================================================
BLSPFileUpload::~BLSPFileUpload()
{
   SYSTEMTIME t;
   GetSystemTime(&t);
   nlog(cLSPCL, "BLSPFileUpload::~BLSPFileUpload -- state[%u] dt[%04d%02d%02dT%02d%02d%02d]", getState(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

   delete mpStream;
   mpStream = 0;
}

//==============================================================================
// 
//==============================================================================
void BLSPFileUpload::init()
{
   if (mpStream)
   {
      mpStream->seek(0);
      resetTimeout();
      connect();
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPFileUpload::connected(BXStreamConnection& connection)
{
   BLSPTitleServerConnection::connected(connection);

   if (mpStream)
   {
      uint fileSize = static_cast<uint>(mpStream->bytesLeft());

      send(BRequestFileUploadPacket(getServiceID(), mGamertag, mXuid, mFileName, fileSize));
   }
   else
      setState(cStatePendingDone);
}

//==============================================================================
// 
//==============================================================================
void BLSPFileUpload::sendReady(BXStreamConnection&)
{
   if (mpStream)
   {
      if (mpStream->bytesLeft() > 0)
      {
         send(BFileUploadBlockPacket(getServiceID(), *mpStream));
      }
      else
      {
         send(BFileUploadCompletePacket(getServiceID()));
         delete mpStream;
         mpStream = 0;
         setState(cStatePendingDone);
      }
   }
}

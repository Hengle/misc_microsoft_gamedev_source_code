//==============================================================================
// lspTicker.cpp
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

#include "common.h"
#include "configsgame.h"
#include "user.h"
#include "usermanager.h"
#include "lspManager.h"
#include "lspTicker.h"
#include "LiveSystem.h"
#include "uiticker.h"
// xnetwork
#include "NetPackets.h"

//==============================================================================
// 
//==============================================================================
BLSPTicker::BLSPTicker(BUITicker * ticker) :
   BLSPTitleServerConnection(cDefaultLSPTickerPort, cConfigLSPTickerPort, cDefaultLSPTickerServiceID, cConfigLSPTickerServiceID)
   , mTicker(ticker)
{
}

//==============================================================================
// 
//==============================================================================
BLSPTicker::~BLSPTicker()
{
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPTicker::dataReceived(uint8, const int32 type, int32, const void* pData, int32 size)
{
   BSerialBuffer sb;
   sb.resetSource(pData, size);

   switch (type)
   {
      case BPacketType::cTickerResponsePacket:
         {
            BTickerResponsePacket p;
            p.deserialize(sb);
            if(mTicker)
               mTicker->addString(p.getTickerText(), p.getPriority(), p.getLifetime());
         }
         break;

      default:
         break;
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPTicker::connected(BXStreamConnection& connection)
{
   BLSPTitleServerConnection::connected(connection);

   BTickerRequestPacket packet(getServiceID());

   // loop on all signed-in gold-enabled accounts
   for (uint i=0; i < XUSER_MAX_COUNT; ++i)
   {
      XUSER_SIGNIN_STATE signInState = XUserGetSigninState(i);

      if (signInState != eXUserSigninState_NotSignedIn)
      {
         XUID xuid;
         if (XUserGetXUID(i, &xuid) == ERROR_SUCCESS)
         {
            BOOL result;

            if (XUserCheckPrivilege(i, XPRIVILEGE_MULTIPLAYER_SESSIONS, &result) == ERROR_SUCCESS && result)
               packet.addUser(xuid);
         }
      }
   }

   send(packet);
}

void BLSPTicker::removeTicker()
{
   mTicker = 0;
}

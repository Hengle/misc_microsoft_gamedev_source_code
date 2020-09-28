//==============================================================================
// lspAuth.cpp
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

#include "common.h"
#include "configsgame.h"
#include "user.h"
#include "usermanager.h"
#include "lspManager.h"
#include "lspAuth.h"
#include "LiveSystem.h"

// xnetwork
#include "NetPackets.h"

//==============================================================================
// 
//==============================================================================
BLSPAuth::BLSPAuth(BLSPAuthCache* pAuthCache) :
   BLSPTitleServerConnection(cDefaultLSPAuthPort, cConfigLSPAuthPort, cDefaultLSPAuthServiceID, cConfigLSPAuthServiceID),
   mpAuthCache(pAuthCache),
   mDefaultTTL(cDefaultAuthTTL),
   mWaitingOnResponse(false),
   mSucceeded(false)
{
   long ttl;
   if (gConfig.get(cConfigLSPDefaultAuthTTL, &ttl))
      mDefaultTTL = static_cast<uint>(ttl);

   initCache();
}

//==============================================================================
// 
//==============================================================================
BLSPAuth::~BLSPAuth()
{
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPAuth::dataReceived(uint8, const int32 type, int32, const void* pData, int32 size)
{
   BSerialBuffer sb;
   sb.resetSource(pData, size);

   switch (type)
   {
      case BPacketType::cAuthPacket:
         {
            BAuthPacket packet;
            packet.deserialize(sb);
            switch (packet.getCommand())
            {
               case BAuthPacket::cCommandAuthResponse:
                  {
                     if (mpAuthCache)
                     {
                        const BAuthUser& machine = packet.getMachine();
                        mpAuthCache->updateMachine(machine.getXuid(), machine.isBanMedia(), machine.isBanMatchMaking(), machine.isBanEverything(), packet.getTTL());

                        for (uint i=0; i < packet.getUsers().getSize(); ++i)
                        {
                           const BAuthUser& user = packet.getUsers()[i];

                           mpAuthCache->addUser(user.getXuid(), user.isBanMedia(), user.isBanMatchMaking(), user.isBanEverything(), packet.getTTL());
                        }
                     }

                     mWaitingOnResponse = false;
                     mSucceeded = true;
                     setState(cStateDone);
                  }
                  break;
               default:
                  break;
            }
         }
         break;

      default:
         break;
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPAuth::connected(BXStreamConnection& connection)
{
   BLSPTitleServerConnection::connected(connection);

   BAuthPacket packet(getServiceID(), BAuthPacket::cCommandAuthRequest);

   packet.setMachine(BLiveSystem::getMachineId());

   CHAR gamerTag[XUSER_NAME_SIZE];

   // loop on all signed-in gold-enabled accounts
   for (uint i=0; i < XUSER_MAX_COUNT; ++i)
   {
      XUSER_SIGNIN_STATE signInState = XUserGetSigninState(i);

      if (signInState != eXUserSigninState_NotSignedIn)
      {
         if (XUserGetName(i, gamerTag, XUSER_NAME_SIZE) == ERROR_SUCCESS)
         {
            gamerTag[XUSER_NAME_SIZE - 1] = '\0';

            BSimString name = gamerTag;

            XUID xuid;
            if (XUserGetXUID(i, &xuid) == ERROR_SUCCESS)
            {
               BOOL result;

               if (XUserCheckPrivilege(i, XPRIVILEGE_MULTIPLAYER_SESSIONS, &result) == ERROR_SUCCESS && result)
                  packet.addUser(xuid, name);
            }
         }
      }
   }

   send(packet);
   mWaitingOnResponse = true;
}

//==============================================================================
// 
//==============================================================================
void BLSPAuth::initCache()
{
   if (mpAuthCache == NULL)
      return;

   if (mpAuthCache->getMachine().mXuid == INVALID_XUID)
      mpAuthCache->updateMachine(BLiveSystem::getMachineId(), true, true, true, mDefaultTTL);
   else
      mpAuthCache->updateMachine();

   // loop on all signed-in gold-enabled accounts
   for (uint i=0; i < XUSER_MAX_COUNT; ++i)
   {
      XUSER_SIGNIN_STATE signInState = XUserGetSigninState(i);

      if (signInState != eXUserSigninState_NotSignedIn)
      {
         XUID xuid;
         if (XUserGetXUID(i, &xuid) == ERROR_SUCCESS)
         {
            PBLSPAuthUser pUser = mpAuthCache->getUser(xuid);
            if (pUser == NULL)
               mpAuthCache->addUser(xuid, true, true, true, mDefaultTTL);
            else
               pUser->mLastUpdate = timeGetTime();
         }
      }
   }
}

bool BLSPAuth::requestSucceeded() const
{
   return mSucceeded;
}

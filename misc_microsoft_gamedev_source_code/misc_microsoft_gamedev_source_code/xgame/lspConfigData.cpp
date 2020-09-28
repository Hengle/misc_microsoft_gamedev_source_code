//==============================================================================
// lspConfigData.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#include "common.h"
#include "configsgame.h"
#include "econfigenum.h"
#include "user.h"
#include "usermanager.h"
#include "lspManager.h"
#include "lspConfigData.h"
#include "matchMakingHopperList.h"
#include "LiveSystem.h"

// xnetwork
#include "NetPackets.h"

//Logging
#include "mpcommheaders.h"
#include "commlog.h"

//==============================================================================
// 
//==============================================================================
BLSPConfigData::BLSPConfigData(BLSPConfigCache* pConfigCache) :
   BLSPTitleServerConnection(cDefaultLSPConfigPort, cConfigLSPConfigDataPort, cDefaultLSPConfigServiceID, cConfigLSPConfigServiceID),
   mpConfigCache(pConfigCache),
   mpHopperList(NULL),
   mWaitingOnResponse(false)
{
   nlog(cLSPCL, "BLSPConfigData::BLSPConfigData - Object created");

   if (mpConfigCache)
      mpConfigCache->updateCache();
}

//==============================================================================
// 
//==============================================================================
BLSPConfigData::~BLSPConfigData()
{
   mpHopperList = 0;
   nlog(cLSPCL, "BLSPConfigData::~BLSPConfigData - Object destroyed");
}

//==============================================================================
// BXStreamConnection::BXStreamObserver interface
//==============================================================================
void BLSPConfigData::dataReceived(uint8, const int32 type, int32, const void* pData, const int32 size)
{
   if (!mpHopperList)
   {
      mWaitingOnResponse = false;
      setState(cStateDone);
      return;
   }

   BSerialBuffer sb;
   sb.resetSource(pData, size);

   // only supporting the response packet
   if (type != BPacketType::cConfigDataResponsePacket)
      return;

   if (!gConfig.isDefined(cConfigLSPConfigDataPacketVersion2))
   {

      BConfigDataResponsePacket packet;
      packet.deserialize(sb);

      if (packet.mStaticVersion != -1)
      {
         if (mpConfigCache)
            mpConfigCache->updateStatic(packet.mStaticVersion, packet.mStaticTTL);

         //mpHopperList->setPreferredPing(packet.mPreferredPing);
         //mpHopperList->setMaxPing(packet.mMaxPing);
         //mpHopperList->setTwoStageQOSEnabled(packet.mTwoStageQOSEnabled);
         //mpHopperList->setMaxMatchmakingTimeAllowed(packet.mMaxMatchmakingTimeTotal);

         uint count = packet.mEntries.getSize();
         nlog(cLSPCL, "BLSPConfigData::dataReceived - processing static data section with %i entries", count);
         for (uint i=0; i < count; ++i)
         {
//-- FIXING PREFIX BUG ID 1779
            const BConfigDataResponsePacket::BHopperEntry& entry = packet.mEntries[i];
//--

            //Find the hopper for that index
            BMatchMakingHopper* const pHopper = mpHopperList->findHopperByHopperIndex(entry.mHopperIndex);
            if (!pHopper)
            {
               //No hopper local from remote config???
               nlog(cLSPCL, "BLSPConfigData::dataReceived - ERROR: Config has data for hopper index %d but I don't have that locally", entry.mHopperIndex);
            }
            else
            {
               //Process that hopper
               //pHopper->mEnabled = entry.mEnabled;
               pHopper->mFastScanSearchCount = entry.mFastScanSearchCount;
               pHopper->mNormalSearchCount = entry.mNormalSearchCount;
               pHopper->mHostDelayTimeBase = entry.mHostDelayTimeBase;
               pHopper->mHostDelayTimeRandom = entry.mHostDelayTimeRandom;
               pHopper->mHostTimeout = entry.mHostTime;
            }
         }
      }

      if (packet.mDynamicVersion != -1)
      {
         if (mpConfigCache)
            mpConfigCache->updateDynamic(packet.mDynamicVersion, packet.mDynamicTTL);   

         mpHopperList->getDynamicData()->mGlobalUserCount = packet.mGlobalUserCount;

         uint count = packet.mEntries.getSize();
         nlog(cLSPCL, "BLSPConfigData::dataReceived - processing dynamic data section with %i entries", count);
         for (uint i=0; i < count; i++)
         {
//-- FIXING PREFIX BUG ID 1780
            const BConfigDataResponsePacket::BHopperEntry& entry = packet.mEntries[i];
//--

            //Find the hopper for that index
            BMatchMakingHopper* const pHopper = mpHopperList->findHopperByHopperIndex(entry.mHopperIndex);
            if (!pHopper)
            {
               //No hopper local from remote config???
               nlog(cLSPCL, "BLSPConfigData::dataReceived - ERROR: Config has data for hopper index %d but I don't have that locally", entry.mHopperIndex);
            }
            else
            {
               //Process that hopper
               pHopper->mUserCount = entry.mUserCount;
               pHopper->mAverageWait = entry.mAverageWait;
            }
         }
      }
   }
   else
   {
      //New version here
      BConfigDataResponsePacketV2 packet;
      packet.deserialize(sb);

      if (packet.mStaticVersion != -1)
      {
         //If the deserialize failed - then the data size will be zero - check for this
         if (packet.mStaticDataSize!=0)
         {
            if (mpConfigCache)
               mpConfigCache->updateStatic(packet.mStaticVersion, packet.mStaticTTL*1000);   //Data from LSP is in seconds

            BByteStream dataStream(packet.mStaticData, packet.mStaticDataSize);
            //dataStream.setWritable(true);
            //uint count = dataStream.writeBytes(packet.mStaticData, packet.mStaticDataSize);
            //BASSERT(count==(uint)packet.mStaticDataSize);
            BXMLReader reader;
            if(!reader.load((BStream*)&dataStream,cXFTXML))
            {
               BASSERTM(false, "BLiveSystem::loadMatchMakingHoppers - could not load mpGameSetsLive.xml!!!");
               nlog(cMPGameCL, "BLiveSystem::loadMatchMakingHoppers - no game set file (mpGameSetsLive.xml) found");
               mpHopperList->clear();
            }
            else
            {
               BXMLNode rootNode(reader.getRootNode());
               nlog(cMPGameCL, "BLiveSystem::loadMatchMakingHoppers - loaded new mpGameSetsLive.xml from LSP");
               mpHopperList->clear();
               mpHopperList->loadFromXML(rootNode);        
            }
         }
         else
         {
            nlog(cMPGameCL, "BLiveSystem::loadMatchMakingHoppers - We got a static data packet, but the payload was empty");
         }
      }

      if (packet.mDynamicVersion != -1)
      {
         if (mpConfigCache)
            mpConfigCache->updateDynamic(packet.mDynamicVersion, packet.mDynamicTTL*1000);   //Data from LSP is in seconds

         //OMG HAX!!!!
         //The issue here is that since we have this buffered version that *could* be out there of the hopper list
         //  We want to update that one if it is there 
         // However, if that file did not change as we didn't just read it in - then we want to update the old one that is out there
         //WOuld be better if we just removed hopper list point in the interface and let this access it directly (as in the hack)
         //Or if we passed in the second one
         //For now - lets just update both
         BMatchMakingHopperList* hopperListToUpdate = mpHopperList;
         if (!mpHopperList->isLoaded())
         {
            //use base list
            hopperListToUpdate = gLiveSystem->getHopperList();
            BASSERT(hopperListToUpdate->isLoaded());
         }
         hopperListToUpdate->getDynamicData()->mGlobalUserCount = packet.mGlobalUserCount;

         uint count = packet.mEntries.getSize();
         nlog(cLSPCL, "BLSPConfigData::dataReceived - processing dynamic data section with %i entries", count);
         for (uint i=0; i < count; i++)
         {
            BConfigDataResponsePacketV2::BHopperEntry& entry = packet.mEntries[i];

            //Find the hopper for that index
            BMatchMakingHopper* const pHopper = hopperListToUpdate->findHopperByHopperIndex(entry.mHopperIndex);
            if (!pHopper)
            {
               //No hopper local from remote config???
               nlog(cLSPCL, "BLSPConfigData::dataReceived - ERROR: Config has data for hopper index %d but I don't have that locally", entry.mHopperIndex);
            }
            else
            {
               //Process that hopper
               //Adjust it because the data sent is a uint16 with the value /10
               pHopper->mUserCount = (WORD(entry.mUserCount)*10);
               //If it sends me a big ass number, don't roll it over with this goofy random
               if (pHopper->mUserCount<0xFFFFFFF0)
               {
                  pHopper->mUserCount += DWORD(rand()%10);
               }
            }
         }
      }
   }

   mWaitingOnResponse = false;
   setState(cStateDone);
}

//==============================================================================
// 
//==============================================================================
void BLSPConfigData::connected(BXStreamConnection& connection)
{
   if (mpConfigCache == NULL)
   {
      setState(cStateDone);
      return;
   }

   BLSPTitleServerConnection::connected(connection);
   setState(cStateRequest);

   nlog(cLSPCL, "BLSPConfigData::connected - connected, sending request");
   send(BConfigDataRequestPacket(getServiceID(), mpConfigCache->getStatic().mVersion, mpConfigCache->getDynamic().mVersion));
   mWaitingOnResponse = true;
}

//==============================================================================
// 
//==============================================================================
void BLSPConfigData::requestConfigData(BMatchMakingHopperList* pHopperList)
{
   mpHopperList = pHopperList;
   connect();
   if (mpConfigCache)
   {
      //Go ahead and refresh the last 'updated' times so that IF this request fails, 
      //  then we obey the current TTL before requesting again
      mpConfigCache->updateCache();
   }
}

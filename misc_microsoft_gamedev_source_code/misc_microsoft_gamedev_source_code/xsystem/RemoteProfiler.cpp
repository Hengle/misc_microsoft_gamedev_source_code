//==============================================================================
// RemoteProfiler.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
//
#include "xsystem.h"
//#include "common.h"

#if defined(ENABLE_TIMELINE_PROFILER)

#include "RemoteToolsHost.h"
#include "RemoteProfiler.h"
#include "SerialBuffer.h"
#include "SocksReliableSocket.h"
#include "SocksHelper.h"
//#include "kb.h"
//#include "game.h"
//#include "database.h"
//#include "adefines.h"
//#include "world.h"
//#include "team.h"
//#include "player.h"
//#include "ai.h"
//#include "xsruntime.h"
//#include "xssource.h"



extern BRemoteProfiler gRemoteProfiler;

BRemoteProfiler::BRemoteProfiler()
{
   //mSections.reserve(128);
   mID=gRemoteToolsHost.addObserver(this);
   mNumSentSections = 0;
}

BRemoteProfiler::~BRemoteProfiler()
{
   gRemoteToolsHost.removeObserver(mID);
}

//==============================================================================
//==============================================================================
bool BRemoteProfiler::read(IN BSerialBuffer& sb, IN long packetType)
{
   mSendFrameCrit.lock();
	switch(packetType) 
	{
      case BRemoteProfiler::cTimelineControl:
      {
         BTimelineControlPacket packet;
         packet.deserialize(sb);
         if(packet.mControlValue == BTimelineControlPacket::cStart)
         {    
            Start();
         }
         else if(packet.mControlValue == BTimelineControlPacket::cStop)
         {
            Stop();
         }
         else if(packet.mControlValue == BTimelineControlPacket::cFrameAck)
         {
            mFrameNumberAck = packet.mFrameNumberAck;
         }
         else if(packet.mControlValue == BTimelineControlPacket::cConfigureSections)
         {
            for(int i = 0; i < packet.mNumSections; i++)
            {
               gBProfileSystem.enableSection(static_cast<BSectionID>(packet.mSectionIDs[i]), (packet.mSectionValues[i] == 0)?false:true);
            }
         }
         break;
      }
		default:
         {
            mSendFrameCrit.unlock();
			   return(false);
         }
	}
   mSendFrameCrit.unlock();
   return(true);
}

//==============================================================================
//==============================================================================
//void BRemoteProfiler::registerSection(BSectionID id, const BProfileSection& profSection)
//{
//   if (mSections.size() < (id + 1U))
//      mSections.resize(id + 1);
//   mSections[id] = &profSection;
//}

//==============================================================================
//==============================================================================
//void BRemoteProfiler::insertSample(bool startOfFrame, double frameBaseTime, const BRetiredProfileSample& sample)
//{
//   frameBaseTime;
//   BDEBUG_ASSERT(sample.mSectionID < mSections.size());
//   
//   if((startOfFrame) && (mNextFrame.size() > 0))
//   {
//      mFrameNumber++;
//
//      bool dontSend = false;
//      long framesBehind = mFrameNumber - mFrameNumberAck;
//      //if(framesBehind > 30)
//      //{
//      //   if((rand() % 100) < framesBehind)
//      //   {
//      //      dontSend = true;
//      //   }
//      //}
//      if(framesBehind > 1000)
//      {
//         dontSend = true;
//      }
//      else if(framesBehind > 500)
//      {
//         if((rand() % 100) < 95)
//         {
//            dontSend = true;
//         }
//      }
//      else if(framesBehind > 100)
//      {
//         if((rand() % 100) < 80)
//         {
//            dontSend = true;
//         }
//      }
//      else if(framesBehind > 30)
//      {
//         if((rand() % 100) < 50)
//         {
//            dontSend = true;
//         }
//      }      
//
//
//      if(dontSend == false)
//      {
//         mCurFrame.swap(mNextFrame);
//         SendFrame(&mCurFrame, &mSections);
//         mNextFrame.resize(0);
//      }
//      else
//      {
//         mNextFrame.resize(0);
//      }
//   }
//
//   mFrameBaseTime = frameBaseTime;
//
//   mNextFrame.pushBack(sample);      
//}

BDynamicArray<BDynamicArray<BRetiredProfileSample>*> gTestQueue;
BDynamicArray<double> gOffsets;
BDynamicArray<int> gThreadIDs;
BDynamicArray<int> gFrameNumbers;
//==============================================================================
//==============================================================================
void BRemoteProfiler::insertFrame(unsigned long frameNumber, double frameBaseTime, BDynamicArray<BRetiredProfileSample> *pFrame, const BDynamicArray<BProfileSection*> *pIdentifiers,int threadID)
{
   mSendFrameCrit.lock();

   if((*pFrame)[0].mSectionID != gBProfileSystem.mMainFrameID)
   {
      gFrameNumbers.pushBack(frameNumber);
      gTestQueue.pushBack(pFrame);
      gOffsets.pushBack(frameBaseTime);
      gThreadIDs.pushBack(threadID);
      mSendFrameCrit.unlock();
      return;
   }

   mDataChunkNumber++;
   
   mFrameNumber = frameNumber;
   if(mbFirstTimeInit == true)
   {
      mbFirstTimeInit = false;
      mFrameNumberAck = mFrameNumber;
   }

   bool dontSend = false;
   long framesBehind = mFrameNumber - mFrameNumberAck;
   if(framesBehind > 1000)
   {
      dontSend = true;
   }
   else if(framesBehind > 500)
   {
      if((rand() % 100) < 95)
      {
         dontSend = true;
      }
   }
   else if(framesBehind > 100)
   {
      if((rand() % 100) < 80)
      {
         dontSend = true;
      }
   }
   else if(framesBehind > 30)
   {
      if((rand() % 100) < 50)
      {
         dontSend = true;
      }
   }      
   if(dontSend == false)
   {

      for (uint i = 0; i < gTestQueue.size(); i++)
      {
         BDynamicArray<BRetiredProfileSample>* pThreadFrame = gTestQueue[i];
         mFrameBaseTime = gOffsets[i];
         SendFrame(pThreadFrame, pIdentifiers, gThreadIDs[i], gFrameNumbers[i]);      
      }
      gFrameNumbers.clear();
      gTestQueue.clear();
      gOffsets.clear();
      gThreadIDs.clear();
  
      mFrameBaseTime = frameBaseTime;
      SendFrame(pFrame, pIdentifiers, threadID,frameNumber);      
   }
   mSendFrameCrit.unlock();
}
//==============================================================================
//==============================================================================
void BRemoteProfiler::SendFrame(BDynamicArray<BRetiredProfileSample> *pFrame, const BDynamicArray<BProfileSection*> *pIdentifiers, int threadID, int frameNumber)
{
   BFPUPrecision fpuPrecision(true);

   //Send any new Identifiers
   ushort identifiersPerPacket = 30;  //this number may need to be adjusted to something smarter
                                      //since identifiers are variable length.  
   BSectionID identifiersToSend = (BSectionID)pIdentifiers->size() - mNumSentSections;
   BSampleIdentifierPacket idPacket;
   while(identifiersToSend > 0)
   {
      idPacket.mpIdentifiers = pIdentifiers;
      idPacket.mStartIdentifier = mNumSentSections ; //pFrame->size() - identifiersToSend;
      idPacket.mNumIdentifiers = (identifiersToSend < identifiersPerPacket)?identifiersToSend:identifiersPerPacket;
      gRemoteToolsHost.sendPacket(idPacket);
      identifiersToSend = identifiersToSend - idPacket.mNumIdentifiers; 
      mNumSentSections = static_cast<BSectionID>(mNumSentSections + idPacket.mNumIdentifiers);
   }
   mNumSentSections = (BSectionID)pIdentifiers->size();


   bool useCompression = true;
   if(useCompression == false)
   {
      //Send 30 measurements per packet.
      uint measurementsPerPacket = 30;
      uint measurementsToSend = pFrame->size();
      BRetiredSamplesPacket packet;
      while(measurementsToSend > 0)
      {
         packet.mpFrame = pFrame;
         packet.mStartSample = pFrame->size() - measurementsToSend;
         packet.mNumSamples = (measurementsToSend < measurementsPerPacket)?measurementsToSend:measurementsPerPacket;
         gRemoteToolsHost.sendPacket(packet);
         measurementsToSend -= packet.mNumSamples;
      }
   }
   else if(useCompression == true)
   {
      uint size;
      const uchar* buffer = compressFrame(size,pFrame, pIdentifiers);
      uint bytesToSend = size;
      uint bytesPerPacket = 900;

      BCompressedFramePacket packet;

      //packet.mStartOfFrame = true;
      while(bytesToSend > 0)
      {
         packet.mThreadID = threadID;
         packet.mFrameNumber = frameNumber;//mFrameNumber;
         //packet.mDataChunkNumber = mDataChunkNumber;
         packet.mFrameBaseTime = mFrameBaseTime;
         packet.mTotalSamples = pFrame->size();
         packet.mTotalLength = size;
         packet.mBuffer = buffer;
         packet.mLength = (bytesPerPacket < bytesToSend)?bytesPerPacket:bytesToSend;
         packet.mOffset = size - bytesToSend;
         bytesToSend -= packet.mLength;
         gRemoteToolsHost.sendPacket(packet);

         ////only send packets if first frame.
         //if((*pFrame)[0].mSectionID == gBProfileSystem.mMainFrameID)
         //{
         //   gRemoteToolsHost.sendPacket(packet);
         //}
      }
   }
}

const uchar* BRemoteProfiler::compressFrame(uint &size, BDynamicArray<BRetiredProfileSample> *pFrame, const BDynamicArray<BProfileSection*> *pIdentifiers)
{

   static BDynamicCoderBuf coderBuf(65536);
   BBitPacker<BDynamicCoderBuf> bitPacker(&coderBuf);
   bool success;
   
   // Reset the compressor's model to empty and compress all the samples to a buffer.
   mComp.resetModel();
      
   coderBuf.setPos(0);
   bitPacker.encodeStart();
   
   for (uint i = 0; i < pFrame->size(); i++)
   {
      if(i>= pFrame->size())
         break;

      const BRetiredProfileSample& sample = (*pFrame)[i];
      BASSERT(sample.mSectionID < pIdentifiers->size());
            
      success = mComp.codeSample(bitPacker, sample, (*pIdentifiers)[sample.mSectionID]);
      BASSERT(success);
   }
  
   //const uint totalInBytes = mCurFrame.size() * sizeof(BRetiredProfileSample);
   const uint totalOutBytes = coderBuf.getPos();
   
   success = bitPacker.encodeEnd();
   BASSERT(success);

   size = totalOutBytes;

   
   return coderBuf.getBuf();
}


//==============================================================================
//==============================================================================
void BRemoteProfiler::Start()
{
   if(gBProfileSystem.isEnabled() == false)
      return;
   mFrameNumber = 0;
   mDataChunkNumber = 0;
   mNumSentSections = 0;
   mFrameNumberAck = 0;
   mbFirstTimeInit = true;
   gBProfileSystem.setVisualizer(this);
   gBProfileSystem.setActive(true);

   //gBProfileSystem.resendAllSections();
}

//==============================================================================
//==============================================================================
void BRemoteProfiler::Stop()
{ 
   if(gBProfileSystem.isEnabled() == false)
      return;
   gBProfileSystem.setVisualizer(NULL);
   gBProfileSystem.setActive(false);

}



#else
uint gRemoteProfilerDummy;
#endif

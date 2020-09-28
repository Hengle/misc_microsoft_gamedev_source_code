//==============================================================================
// RemoteProfiler.h
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================

#ifndef remoteprofiler_h
#define remoteprofiler_h

#if defined(ENABLE_TIMELINE_PROFILER)

//==============================================================================
// Includes
//
#include "Socket.h"
#include "Packet.h"
#include "socksreliablesocket.h"
#include "simpletypes.h"
#include "timelineprofiler.h"
//#include "alignedarray.h"
//#include "RemoteToolsHost.h"
//#include "RemoteXSDebugger.h"
#include "bitpacker.h"
#include "universalcodes.h"
#include "timelinesamplecodec.h"
//==============================================================================
//
class BRemoteToolsHost;
//class BRemoteXSDebugger;
class BRemoteProfiler;
class BTimelineControlPacket;
class BRetiredSamplesPacket;
class BSampleIdentifier;

extern BRemoteToolsHost gRemoteToolsHost;
extern BRemoteProfiler gRemoteProfiler;

//==============================================================================
//

//==============================================================================
class BRemoteProfiler : public BProfileVisualizerInterface, public BRemoteToolsHostObserver
{
public:
	BRemoteProfiler();
	~BRemoteProfiler();

	enum 
	{ 
      cTimelineControl = 50,//BRemoteXSDebugger::cNumberOfPackets,
      cRetiredSamples,
      cSampleIdentifier,
      cCompressedFramePacket,
		cNumberOfTimelineTypedPackets 
	};

   virtual bool read (IN BSerialBuffer& sb, IN long packetType);
   virtual void insertFrame(unsigned long frameNumber, double frameBaseTime, BDynamicArray<BRetiredProfileSample> *pFrame, const BDynamicArray<BProfileSection*> *pIdentifiers, int threadID);

   void SendFrame(BDynamicArray<BRetiredProfileSample> *pFrame, const BDynamicArray<BProfileSection*> *pIdentifiers, int threadID, int frameNumber);

   void Start();
   void Stop();

protected:
	long mID;

private:
   BCriticalSection mSendFrameCrit;

   BSectionID mNumSentSections;

   BTimelineSampleCodec mComp;
   const uchar* compressFrame(uint &size, BDynamicArray<BRetiredProfileSample> *pFrame, const BDynamicArray<BProfileSection*> *pIdentifiers);

   unsigned long mFrameNumber;
   unsigned long mFrameNumberAck;
   unsigned long mDataChunkNumber;
   double mFrameBaseTime;
   bool mbFirstTimeInit;

};

//==============================================================================
class BTimelineControlPacket: public BTypedPacket
{
public:
   
   enum //TimelineControlValues
   {
      cStart = 1,
      cStop,
      cFrameAck,
      cConfigureSections,
   };
   BTimelineControlPacket(long packetType=BRemoteProfiler::cTimelineControl) : BTypedPacket(packetType) {}

	virtual void            deserialize(BSerialBuffer &sb) 
	{ 
		BTypedPacket::deserialize(sb); 
		sb.get(&mControlValue); 
      
      if(mControlValue == cFrameAck)
      {
         sb.get(&mFrameNumberAck); 
      }
      else if(mControlValue == cConfigureSections)
      {
         int id;
         BYTE value;
         sb.get(&mNumSections);
         for(int i = 0; i < mNumSections; i++)
         {
            sb.get(&id);
            mSectionIDs.pushBack(id);
            sb.get(&value);
            mSectionValues.pushBack(value);
         }
      }
	}

public:
	unsigned char mControlValue;

   //cFrameAck
   uint32 mFrameNumberAck;

   //cConfigureSections
   int mNumSections;
   BDynamicArray<int> mSectionIDs;
   BDynamicArray<BYTE> mSectionValues;
   

};

//==============================================================================
class BRetiredSamplesPacket : public BTypedPacket
{
   public:
		BRetiredSamplesPacket(long packetType=BRemoteProfiler::cRetiredSamples) : BTypedPacket(packetType) {}

		virtual void            serialize(BSerialBuffer &sb)
										{
											BTypedPacket::serialize(sb); 
                                 sb.add((int)mNumSamples);
											for (uint i = mStartSample; i < (mNumSamples + mStartSample); i++)
                                 {
                                    BRetiredProfileSample& sample = (*mpFrame)[i];
                                    sb.add((BSectionID)sample.mSectionID);
                                    sb.add(reinterpret_cast<void*>(&sample.mCPUStartScaledTime), 4);
                                    sb.add(reinterpret_cast<void*>(&sample.mCPUEndScaledTime), 4);
                                    sb.add(reinterpret_cast<void*>(&sample.mGPUStartScaledTime), 4);
                                    sb.add(reinterpret_cast<void*>(&sample.mGPUEndScaledTime), 4);
                                    sb.add(static_cast<short>(sample.mLevel));
                                    sb.add(static_cast<int>(sample.mUserID));
                                 }
										}

   public:
   BDynamicArray<BRetiredProfileSample> *mpFrame;
   uint mStartSample;
   uint mNumSamples; 
};


//==============================================================================
class BCompressedFramePacket : public BTypedPacket
{
   public:
      BCompressedFramePacket(long packetType=BRemoteProfiler::cCompressedFramePacket) : BTypedPacket(packetType) {}

		virtual void            serialize(BSerialBuffer &sb)
										{
											BTypedPacket::serialize(sb); 
                                 sb.add(mThreadID);
                                 sb.add(mFrameNumber);
                                 sb.add((void*)(&mFrameBaseTime), 8);
                                 sb.add(static_cast<int>(mTotalSamples));
                                 sb.add(static_cast<int>(mTotalLength));
                                 sb.add(static_cast<int>(mLength));
                                 sb.add((void*)(mBuffer+mOffset), mLength);      
										}

   public:
      const uchar* mBuffer;
      int mThreadID;
      uint32 mFrameNumber;     
      double mFrameBaseTime;
      uint mTotalSamples;
      uint mTotalLength;
      uint mLength;
      uint mOffset;
  
};
//==============================================================================
class BSampleIdentifierPacket : public BTypedPacket
{
   public:
      BSampleIdentifierPacket(long packetType=BRemoteProfiler::cSampleIdentifier) : BTypedPacket(packetType) {}

		virtual void            serialize(BSerialBuffer &sb)
										{
											BTypedPacket::serialize(sb); 
                                 sb.add((BSectionID)mNumIdentifiers);
                                 sb.add((BSectionID)mStartIdentifier);
											for (BSectionID i = mStartIdentifier; i < (mNumIdentifiers + mStartIdentifier); i++)
                                 {
                                    const BProfileSection& identifier = *(*mpIdentifiers)[i];
                                    short length = static_cast<short>(strlen(identifier.name()));
                                    sb.add(static_cast<short>(length));
                                    sb.add((void*)(identifier.name()), length);
                                    if(identifier.cpuOnly() == false)
                                    {
                                       sb.add((BYTE)0);
                                    }
                                    else
                                    {
                                       sb.add((BYTE)1);
                                    }
                                 }
										}

   public:
   const BDynamicArray<BProfileSection*> *mpIdentifiers;
   BSectionID mStartIdentifier;
   BSectionID mNumIdentifiers; 
};

#endif // BUILD_FINAL
#endif // remoteprofiler_h

//==============================================================================
// EOF: RemoteProfiler.h
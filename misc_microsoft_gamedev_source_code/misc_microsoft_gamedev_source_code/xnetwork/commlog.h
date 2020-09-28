//==============================================================================
// commlog.h
//
// Copyright (c) Ensemble Studios, 2003-2008
//==============================================================================

#pragma once

#define COMM_LOG_NAME "phxCommLog"

//==============================================================================
class BCommLogEntry
{
   public:
      BCommLogEntry() :
         mHeaderID(-1)
      {
         mData[0] = 0;
      }

      enum { cCommLogMaxString = 256 };

      char     mData[cCommLogMaxString];
      int32    mHeaderID;
};

typedef BDynamicSimArray<BCommLogEntry> BCommLogEntryArray;

//==============================================================================
class BCommLogInfo
{
   public:
      BCommLogInfo() :
         mID(-1),
         mActive(false)
      {
      }

      BCommLogInfo& operator=(const BCommLogInfo& info)
      {
         mID = info.mID;
         mName = info.mName;
         mConfig = info.mConfig;
         mActive = info.mActive;
         return(*this);
      }

      BSimString  mName;
      BSimString  mConfig;
      int32       mID;
      bool        mActive : 1;
};

typedef BDynamicSimArray<BCommLogInfo> BCommLogInfoArray;

//==============================================================================
class BCommLogPayload : public BEventPayload
{
   public:
      BCommLogPayload(uint32 headerID, const char* pText):
         mText(pText),
         mHeaderID(headerID)
      {
      }

      virtual void deleteThis(bool delivered)
      {
         delete this;
      }

      BString mText;
      uint32  mHeaderID;
};

//==============================================================================
class BCommLogReceiver : public BEventReceiver
{
   public:

      bool init();
      void deinit();   

      enum
      {
         cEventLog = cEventClassFirstUser,
      };

   private:
      virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

//==============================================================================
class BCommLog
{
   public:

      static BOOL          initialize(const BCHAR_T* filepath);   
      static uint32        addHeader(const char* header, const char* config);
      static void          enableHeader(uint32 headerID, bool enable=true);
      static void          commLog(uint32 headerID, const char* pText, ...);
      static void          commLogErr(uint32 headerID, DWORD errCode);
      static void          commLogThreaded(uint32 headerID, const char* text, ...);
      static void          commLogConfigChange(long configEnum, bool beingDefined);
      static void          commLogAllHeaders(long configEnum, bool beingDefined);
      static void          setUseHistory(bool val, long maxEntries);
      static void          dumpHistory();
      static void          cycleLogFile();
      static void          closeLogFile();

   protected:
      BCommLog() {}
      static void          addToHistory(uint32 headerID, const char* pText);
     
      enum
      {
         cBufferSize = 1024
      };

      static char                mLogBuffer[cBufferSize];
      static BCommLogInfoArray   mHeaderInfo;

      static BCommLogEntryArray  mHistory;

      static BCommLogReceiver*   mpReceiver;

      static BEventReceiverHandle mSimEventHandle;

      static uint32              mHistoryCurrentEntry;
      static long                mLogFileID;
      static bool                mInitialized;
      static bool                mUseHistory;
};


#define DEFINE_COMMHEADER(x) \
   long x##temp = -1; \
   const long &x = x##temp;

#define DECLARE_COMMHEADER(x, text, config) \
   x##temp = BCommLog::addHeader(text, config);


#ifndef LTCG
#define NLOG_ENABLED
#endif

#ifdef NLOG_ENABLED
   #define nlog BCommLog::commLog
   #define nlogError BCommLog::commLogErr
   #define nlogt BCommLog::commLogThreaded
#else
   #define nlog ((void)0)   
   #define nlogError ((void)0)
   #define nlogt ((void)0)
#endif

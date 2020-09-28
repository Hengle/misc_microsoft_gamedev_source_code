//==============================================================================
// commlog.cpp
//
// Copyright (c) Ensemble Studios, 2003-2008
//==============================================================================

//==============================================================================
// Includes
#include "precompiled.h"
#include "commlog.h"
#include "config.h"
//#include "configenum.h"

// xsystem
#include "xfs.h"
#include "econfigenum.h"

//==============================================================================
// Const declarations
bool BCommLog::mInitialized = false;
long BCommLog::mLogFileID = -1;
BCommLogInfoArray BCommLog::mHeaderInfo;
char BCommLog::mLogBuffer[cBufferSize] = { 0 };
bool BCommLog::mUseHistory = false;
uint32 BCommLog::mHistoryCurrentEntry = 0;
BEventReceiverHandle BCommLog::mSimEventHandle = cInvalidEventReceiverHandle;
BCommLogEntryArray BCommLog::mHistory;
BCommLogReceiver* BCommLog::mpReceiver = NULL;

//============================================================================
bool BCommLogReceiver::init()
{
   eventReceiverInit(cThreadIndexSim);

   return true;
}

//============================================================================
void BCommLogReceiver::deinit()
{
   eventReceiverDeinit(true);
}

//============================================================================
bool BCommLogReceiver::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (threadIndex == cThreadIndexSim)
   {
      switch (event.mEventClass)
      {
         case cEventLog:
            {
//-- FIXING PREFIX BUG ID 7543
               const BCommLogPayload* pPayload = reinterpret_cast<BCommLogPayload*>(event.mpPayload);
//--
               BDEBUG_ASSERT(pPayload);
               if (!pPayload)
                  break;

               BCommLog::commLog(pPayload->mHeaderID, pPayload->mText.asNative());

               break;
            }
      }
   }

   return false;
}

//==============================================================================
// 
//==============================================================================
BOOL BCommLog::initialize(const BCHAR_T* filepath)
{
   if (mInitialized)
      return TRUE;

   char fileName[512];

 #ifndef BUILD_FINAL
    if (!gXFS.isActive() || gConfig.isDefined(cConfigLogToCacheDrive))
    {
       SYSTEMTIME st;
       GetLocalTime(&st);

       //If no XFS then write them to disk
       //char newName[512];
       //strcpy_s(newName, 512, filepath);
       //char* p = &newName[1];
       //bsnprintf(fileName, 512, "game:%s", p);
       bsnprintf(fileName, 512, "%s%02d%02dT%02d%02d%02d.txt", filepath, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    }
    else
    {
       gXFS.setFailReportFileName(filepath, fileName, sizeof(fileName));
    }
#else
   //If no XFS then write them to disk
   //char newName[512];
   //strcpy_s(newName, 512, filepath);
   //char* p = &newName[1];
   bsnprintf(fileName, 512, "%s", filepath);
#endif

   mLogFileID = gLogManager.openLogFile(fileName);
   if (mLogFileID < 0)
   {
      //BASSERTM(FALSE, "BCommLog::initialize -- failed to open log file.");
      return FALSE;
   }

   registerCommHeaders();

   long count = mHeaderInfo.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      mHeaderInfo[idx].mID = gLogManager.createHeader(mHeaderInfo[idx].mName.getPtr(), mLogFileID, BLogManager::cBaseHeader, false, false, true, false, false);
      if (mHeaderInfo[idx].mID < 0)
         BFAIL("BCommLog::initialize -- failed to add header.");

      mHeaderInfo[idx].mActive = gConfig.isDefined(mHeaderInfo[idx].mConfig);
   }

   // rg [1/17/05] - FIXME
#if 0
   if (gConfig.isDefined(cConfigFlushCommLogs))
      gLogManager.setPostWriteAction(mLogFileID, BLogManager::cPostWriteFlush);
   if (gConfig.isDefined(cConfigCloseCommLogs))
      gLogManager.setPostWriteAction(mLogFileID, BLogManager::cPostWriteClose);
#endif

   mInitialized = true;

   if (mpReceiver == NULL)
   {
      mpReceiver = new BCommLogReceiver();

      mpReceiver->init();

      __lwsync();
      InterlockedExchange64((LONG64*)&mSimEventHandle, mpReceiver->getEventHandle());
   }

   return TRUE;
}

//==============================================================================
// 
//==============================================================================
uint32 BCommLog::addHeader(const char* header, const char* config)
{
   // search for head first before adding it
   uint32 count = mHeaderInfo.getSize();
   for (uint32 i=0; i < count; i++)
   {
      if (mHeaderInfo[i].mName.compare(header) == 0)
         return i;
   }

   uint32 index = count;
   mHeaderInfo.setNumber(index+1);
   if (mInitialized)
   {
      mHeaderInfo[index].mID = gLogManager.createHeader(header, mLogFileID, BLogManager::cBaseHeader, false, false, true, false, false);
      if (mHeaderInfo[index].mID < 0)
         BFAIL("BCommLog::addHeader -- failed to add header.");
   }

   mHeaderInfo[index].mActive = FALSE;
   mHeaderInfo[index].mName.set(header);
   mHeaderInfo[index].mConfig.set(config);
   mHeaderInfo[index].mActive = gConfig.isDefined(config);

   return index;
}

//==============================================================================
// 
//==============================================================================
void BCommLog::enableHeader(uint32 headerID, bool enable)
{
   if (headerID >= mHeaderInfo.getSize())
   {
      BFAIL("BCommLog::enableHeader -- invalid header ID.");
      return;
   }

   mHeaderInfo[headerID].mActive = enable;
}

//==============================================================================
// 
//==============================================================================
void BCommLog::commLog(uint32 headerID, const char* pText, ...)
{
   if (!mInitialized)
      return;

   if (headerID >= mHeaderInfo.getSize())
   {
      //BFAIL("BCommLog::doLog -- invalid header ID.");
      return;
   }

   // header logging is off
   if (!mHeaderInfo[headerID].mActive)
      return;

   va_list ap;
   va_start(ap, pText);
   long length = bvsnprintf(mLogBuffer, cBufferSize-1, pText, ap);
   length;
   va_end(ap);
   mLogBuffer[cBufferSize-1] = 0;

   BASSERT(length >= 0);
   BASSERT(length < cBufferSize);

   if (mUseHistory)
   {
      addToHistory(headerID, mLogBuffer);
   }
   else
   {
      // send it to all relevant output streams
      finalBlogh(mHeaderInfo[headerID].mID, mLogBuffer);
      #ifdef BUILD_DEBUG
      //trace(mLogBuffer);
      #endif
   }
}

//==============================================================================
// 
//==============================================================================
void BCommLog::commLogErr(uint32 headerID, DWORD errCode)
{
   const DWORD BufferLength = 0x100;
   CHAR Buffer [BufferLength + 1];

#ifdef XBOX
   strcpy_s(Buffer, sizeof(Buffer), "Not yet implemented on Xbox");
#else
   DWORD Length = FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM, NULL, errCode, LANG_NEUTRAL, Buffer, BufferLength, NULL);
   Buffer [Length] = 0;
#endif   

   BCommLog::commLog(headerID, "\tError: 0x%08x %u: %s\r\n", errCode, errCode, Buffer);
}

//==============================================================================
// 
//==============================================================================
void BCommLog::commLogThreaded(uint32 headerID, const char* text, ...)
{
   if (mSimEventHandle == cInvalidEventReceiverHandle)
      return;

   char buffer[cBufferSize];

   va_list ap;
   va_start(ap, text);
   long length = bvsnprintf(buffer, cBufferSize-1, text, ap);
   length;
   va_end(ap);
   buffer[cBufferSize-1] = 0;

   BASSERT(length >= 0);
   BASSERT(length < cBufferSize);

   BCommLogPayload* pPayload = new BCommLogPayload(headerID, buffer);

   gEventDispatcher.send(cInvalidEventReceiverHandle, mSimEventHandle, BCommLogReceiver::cEventLog, 0, 0, pPayload);
}

//==============================================================================
// 
//==============================================================================
void BCommLog::commLogConfigChange(long configEnum, bool beingDefined)
{
   BSimString text;
   if (!gConfig.getFormalName(configEnum, &text))
   {
      BFAIL("BCommLog::commLogConfigChange -- failed to find config name.");
      return;
   }

   uint32 count = mHeaderInfo.getSize();
   for (uint32 idx=0; idx < count; idx++)
   {
      if (mHeaderInfo[idx].mConfig.compare(text) == 0)
      {
         mHeaderInfo[idx].mActive = beingDefined;
         break;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BCommLog::commLogAllHeaders(long configEnum, bool beingDefined)
{
   configEnum;
   uint32 count = mHeaderInfo.getSize();
   for (uint32 idx=0; idx < count; idx++)
   {
      mHeaderInfo[idx].mActive = beingDefined;
   }
}

//==============================================================================
// 
//==============================================================================
void BCommLog::setUseHistory(bool val, long maxEntries) 
{ 
   if (maxEntries < 0)
   {
      BASSERT(0);
      return;
   }

   if (mUseHistory == val)
      return;

   // turning history on, and we have less entries than we need
   if (val && (mHistory.getNumber() < maxEntries))
      mHistory.setNumber(maxEntries);

   mUseHistory = val; 
}

//==============================================================================
// 
//==============================================================================
void BCommLog::dumpHistory()
{
   if (!mUseHistory)
      return;

   uint32 count = mHistory.getSize();
   for (uint32 idx=0; idx < count; idx++)
   {
      // starting from the current entry, iterate over the history
      uint32 offset = mHistoryCurrentEntry + idx;

      // wrap when needed
      offset %= count;

      // if an empty update, skip it
      if ((mHistory[offset].mHeaderID < 0) || (mHistory[offset].mData[0] == 0))
         continue;

      // hello Mr. Logfile
      finalBlogh(mHistory[offset].mHeaderID, mHistory[offset].mData);
   }

   gLogManager.flushLogFile(mLogFileID);
}

//==============================================================================
// 
//==============================================================================
void BCommLog::addToHistory(uint32 headerID, const char* pText)
{
   if (!mUseHistory)
      return;

   // set the current entry in the history
   mHistory[mHistoryCurrentEntry].mHeaderID = headerID;
   long size = sizeof(mHistory[mHistoryCurrentEntry].mData);
   memcpy(mHistory[mHistoryCurrentEntry].mData, pText, size);
   mHistory[mHistoryCurrentEntry].mData[size-1] = 0;

   // increment, and wrap
   mHistoryCurrentEntry++;
   mHistoryCurrentEntry%=mHistory.getNumber();
}

//==============================================================================
// 
//==============================================================================
void BCommLog::cycleLogFile()
{
   closeLogFile();

   initialize(COMM_LOG_NAME);
}

//==============================================================================
// 
//==============================================================================
void BCommLog::closeLogFile()
{
   if (mLogFileID != -1)
   {
      gLogManager.closeLogFile(mLogFileID);
      mLogFileID = -1;
   }

   mInitialized = false;

   __lwsync();
   InterlockedExchange64((LONG64*)&mSimEventHandle, cInvalidEventReceiverHandle);

   if (mpReceiver)
   {
      mpReceiver->deinit();
      delete mpReceiver;
      mpReceiver = NULL;
   }
}

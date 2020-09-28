//==============================================================================
// gamefilemanifest.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "gamefilemanifest.h"

#include "lspManager.h"
#include "lspMediaTransfer.h"

uint8 BGameFileManifest::mNextUpdateNumber = 0;

//==============================================================================
// 
//==============================================================================
BGameFileManifest::BGameFileManifest() :
   //mID(0),
   //mOwnerXuid(0),
   mFileSize(0),
   mStorageType(eStorageUnknown),
   //mSize(0),
   mpContent(NULL),
   mpStream(NULL),
   mUserIndex(0),
   mMediaTaskID(0),
   mPercentTransferred(0),
   mMapStringID(0),
   mMapType(0),
   mVersion(0),
   mLastError(ERROR_SUCCESS),
   mType(0),
   mUpdateNumber(0),
   mSaved(false),
   mForceRefresh(false),
   mFileError(false)
{
   Utils::FastMemSet(&mTime, 0, sizeof(SYSTEMTIME));
   Utils::FastMemSet(&mLocalTime, 0, sizeof(SYSTEMTIME));
}

//==============================================================================
// 
//==============================================================================
BGameFileManifest::~BGameFileManifest()
{
   delete mpContent;
   mpContent = NULL;

   delete mpStream;
   mpStream = NULL;
}

//==============================================================================
// 
//==============================================================================
void BGameFileManifest::update()
{
   BString name;
   const char* pName = getName().asANSI(name);
   BString desc;
   const char* pDesc = getDesc().asANSI(desc);

   mLocalTime = getDate();

   FILETIME utcFileTime;
   if (SystemTimeToFileTime(&mLocalTime, &utcFileTime))
   {
      FILETIME localFileTime;
      if (FileTimeToLocalFileTime(&utcFileTime, &localFileTime))
      {
         FileTimeToSystemTime(&localFileTime, &mLocalTime);
      }
   }

   float flen = getLength();
   if (flen > 0.0f)
   {
      uint32 len = static_cast<uint32>(flen * 1000.0f);

      // convert the length from milliseconds to hour:min:sec
      int hours = len / 3600000;
      len -= (hours * 3600000);
      int minutes = len / 60000;
      len -= (minutes * 60000);
      int seconds = len / 1000;

      mName.format("%04d/%02d/%02d %02d:%02d:%02d %s - Length %02d:%02d:%02d - %s", mLocalTime.wYear, mLocalTime.wMonth, mLocalTime.wDay, mLocalTime.wHour, mLocalTime.wMinute, mLocalTime.wSecond, pName, hours, minutes, seconds, pDesc);
   }
   else
   {
      mName.format("%04d/%02d/%02d %02d:%02d:%02d %s - %s", mLocalTime.wYear, mLocalTime.wMonth, mLocalTime.wDay, mLocalTime.wHour, mLocalTime.wMinute, mLocalTime.wSecond, pName, pDesc);
   }

   mRecordingName = mFilename;
   mRecordingName.removeExtension();
}

//==============================================================================
// 
//==============================================================================
void BGameFileManifest::setContent(const XCONTENT_DATA& content)
{
   if (mpContent == NULL)
      mpContent = new XCONTENT_DATA;

   Utils::FastMemCpy(mpContent, &content, sizeof(XCONTENT_DATA));
}

//==============================================================================
// 
//==============================================================================
uint BGameFileManifest::getPercentTransferred()
{
   if (mMediaTaskID == 0)
      return mPercentTransferred;

   // need to query the media task for the total file size and then how much we've transferred
   if (mFileSize == 0)
      return mPercentTransferred;

//-- FIXING PREFIX BUG ID 2330
   const BLSPMediaTask* pTask = gLSPManager.getMediaTaskByID(mMediaTaskID);
//--
   if (pTask == NULL)
   {
      mMediaTaskID = 0;
      return mPercentTransferred;
   }

   mPercentTransferred = static_cast<uint>(static_cast<double>(pTask->getTransferred()) / static_cast<double>(mFileSize) * 100.0);

   return mPercentTransferred;
}

//==============================================================================
// 
//==============================================================================
int __cdecl BGameFileManifest::compareFunc(const void* pElem1, const void* pElem2)
{
//-- FIXING PREFIX BUG ID 2331
   const BGameFileManifest* pFile1 = *(BGameFileManifest**)pElem1;
//--
//-- FIXING PREFIX BUG ID 2332
   const BGameFileManifest* pFile2 = *(BGameFileManifest**)pElem2;
//--

   FILETIME ft1;
   FILETIME ft2;

   if (SystemTimeToFileTime(&pFile1->mTime, &ft1) == 0)
      return 1;

   if (SystemTimeToFileTime(&pFile2->mTime, &ft2) == 0)
      return -1;

   LONG cmp = CompareFileTime(&ft2, &ft1);
   if (cmp == 0)
   {
      if (pFile1->getStorageType() == eStorageUser && pFile2->getStorageType() != eStorageUser)
         return -1;
      return 1;
   }

   return cmp;
}

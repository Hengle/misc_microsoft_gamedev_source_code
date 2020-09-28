//==============================================================================
// gamefilemanifest.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "gamefileheader.h"
#include "gamefileversion.h"
#include "storagetype.h"

//==============================================================================
// BGameFileManifest
//==============================================================================
class BGameFileManifest : public BEventPayload
{
   public:

               BGameFileManifest();
               ~BGameFileManifest();

      void     update();

      virtual void deleteThis(bool delivered)
      {
         delete this;
      }

      const BString& getRecordingName() const { return mRecordingName; }

      bool canEdit() const { return (mStorageType == eStorageUser); }
      bool isSaved() const { return mSaved; }

      const BString& getFilename() const { return mFilename; }
      void setFilename(const BString& filename) { mFilename = filename; }

      const BUString& getName() const { return mHeader.mName; }
      const BUString& getDesc() const { return mHeader.mDesc; }
      const BString& getAuthor() const { return mHeader.mAuthor; }
      const XUID getAuthorXuid() const { return mHeader.mXuid; }
      const SYSTEMTIME& getDate() const { return mHeader.mDate; }
      float getLength() const { return mHeader.mLength; }

      const XCONTENT_DATA* getContent() const { return mpContent; }
      void setContent(const XCONTENT_DATA& content);

      DWORD getUserIndex() const { return mUserIndex; }
      void setUserIndex(DWORD userIndex) { mUserIndex = userIndex; }

      DWORD getVersion() const { return mVersion; }
      void setVersion(DWORD version) { mVersion = version; mDetails.mVersion = version; }

      BStorageType getStorageType() const { return mStorageType; }
      void setStorageType(BStorageType storageType) { mStorageType = storageType; }

      DWORD getLastError() const { return mLastError; }
      void setLastError(DWORD error) { mLastError = error; }

      bool getFileError() const { return mFileError; }
      void setFileError() { mFileError = true; }

      bool getRefresh() const { return mForceRefresh; }
      void setRefresh(bool refresh=true) { mForceRefresh = refresh; }

      uint64 getID() const { return mHeader.mID; }

      uint getTempID() const { return mHeader.mTempID; }
      void setTempID(uint id) { mHeader.mTempID = id; }

      // for display purposes
      const SYSTEMTIME& getLocalTime() const { return mLocalTime; }
      const BUString& getScenarioName() const { return mScenarioName; }
      const BString& getMapName() const { return mMapName; }
      long getMapStringID() const { return mMapStringID; }
      long getMapType() const { return mMapType; }
      uint64 getFileSize() const { return mFileSize; }
      int32 getGameType() const { return mHeader.mGameType; }
      const BString& getPlayers() const { return mPlayers; }

      // interface to show if we're downloading/uploading this file
      void setMediaTaskID(uint taskID) { mMediaTaskID = taskID; }
      bool isTransferring() const { return (mMediaTaskID != 0); }
      uint getPercentTransferred();

      static int __cdecl compareFunc(const void* pElem1, const void* pElem2);

      static uint8 mNextUpdateNumber;

      BGameFileHeader mHeader;
      SYSTEMTIME        mTime; // creation time of file, for sorting purposes, use the header for recording date
      SYSTEMTIME        mLocalTime; // local time version, used for display purposes
      BString           mFilename;
      BString           mRecordingName; // the filename sans extension and path
      // XXX name temporarily used for our modemenu code so we can display a list
      BString           mName;
      BString           mMapName; // cached from the game settings
      BString           mPlayers; // list of players in the recording, for display purposes
      BUString          mScenarioName; // localized map name for display purposes
      uint64            mFileSize; // record game file size for display purposes
      BStorageType      mStorageType;
      //uint32            mSize;
      PXCONTENT_DATA    mpContent;
      BStream*          mpStream;
      DWORD             mUserIndex;

      uint              mPercentTransferred;
      uint              mMediaTaskID;

      long              mMapStringID; // for display purposes
      long              mMapType; // for display purposes

      DWORD             mVersion;
      BGameFileVersion mDetails;

      DWORD             mLastError;
      uint8             mType;
      uint8             mUpdateNumber;

      bool              mSaved : 1;
      bool              mForceRefresh : 1;
      bool              mFileError : 1;
};

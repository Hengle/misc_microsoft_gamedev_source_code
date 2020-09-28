//==============================================================================
// gamefile.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "gamefileheader.h"
#include "gamefileversion.h"
#include "storagetype.h"
#include "gamefilemacros.h"
#include "lspMediaTransfer.h"

// xcore
#include "stream\dynamicStream.h"
#include "file\xboxFileUtils.h"
#include "file\win32FindFiles.h"
#include "file\win32FileStream.h"
#include "file\xcontentStream.h"

// xsystem
#include "AsyncTaskManager.h"
#include "bitvector.h"

// Forward declarations
class BGameFileManifest;
class BGameSettings;
class BUser;
class BDecryptedStream;
class BEncryptedStream;
class BInflateStream;
class BDeflateStream;

//==============================================================================
// BGameXContentStream
//==============================================================================
class BGameXContentStream : public BXContentStream
{
   public:
      BGameXContentStream();

      virtual bool open(DWORD userIndex, const XCONTENT_DATA& content, uint flags);
      virtual bool close();

      bool isCorrupt() const { return mCorrupt; }

   private:
      XOVERLAPPED mOverlapped;
      bool        mCorrupt : 1;
};

//==============================================================================
// BGameFilePayload
//==============================================================================
class BGameFilePayload : public BEventPayload
{
   public:
      BGameFilePayload(DWORD userIndex, BStorageType storageType, const BFileDesc& fileDesc) :
         mTempID(0),
         mpContent(NULL),
         mUserIndex(userIndex),
         mStorageType(storageType),
         mForceRefresh(false)
      {
         init(fileDesc);
      }

      BGameFilePayload(DWORD userIndex, BStorageType storageType, uint tempID, const BFileDesc& fileDesc) :
         mTempID(tempID),
         mpContent(NULL),
         mUserIndex(userIndex),
         mStorageType(storageType),
         mForceRefresh(false)
      {
         init(fileDesc);
      }

      BGameFilePayload(DWORD userIndex, const XCONTENT_DATA& content) :
         mTempID(0),
         mpFileDesc(NULL),
         mUserIndex(userIndex),
         mStorageType(eStorageUser),
         mForceRefresh(false)
      {
         init(content);
      }

      BGameFilePayload(DWORD userIndex, uint tempID, const XCONTENT_DATA& content) :
         mTempID(tempID),
         mpFileDesc(NULL),
         mpStream(NULL),
         mUserIndex(userIndex),
         mStorageType(eStorageUser),
         mForceRefresh(false)
      {
         init(content);
      }

      BGameFilePayload(DWORD userIndex, BStorageType storageType, const XCONTENT_DATA& content, const BString& fileName) :
         mTempID(0),
         mpFileDesc(NULL),
         mpStream(NULL),
         mUserIndex(userIndex),
         mStorageType(storageType),
         mForceRefresh(false)
      {
         init(content);
         mFilename = fileName;
      }

      BGameFilePayload(DWORD userIndex, uint tempID, const XCONTENT_DATA& content, BStream* pStream) :
         mTempID(tempID),
         mpFileDesc(NULL),
         mpStream(pStream),
         mUserIndex(userIndex),
         mStorageType(eStorageUser),
         mForceRefresh(false)
      {
         init(content);
      }

      ~BGameFilePayload()
      {
         delete mpContent;
         mpContent = NULL;

         delete mpFileDesc;
         mpFileDesc = NULL;
      }

      virtual void deleteThis(bool delivered)
      {
         delete this;
      }

      void init(const XCONTENT_DATA& content)
      {
         mpContent = new XCONTENT_DATA;

         Utils::FastMemCpy(mpContent, &content, sizeof(XCONTENT_DATA));

         mFilename.set(content.szFileName, XCONTENT_MAX_FILENAME_LENGTH);
      }

      void init(const BFileDesc& fileDesc)
      {
         mpFileDesc = new BFileDesc(fileDesc);

         mFilename = fileDesc.filename();
      }

      const PXCONTENT_DATA getContent() const { return mpContent; }
      const BFileDesc* getFileDesc() const { return mpFileDesc; }

      BStream* getStream() const { return mpStream; }

      const BString& getFilename() const { return mFilename; }

      DWORD getUserIndex() const { return mUserIndex; }

      BStorageType getStorageType() const { return mStorageType; }
      void setStorageType(BStorageType storageType) { mStorageType = storageType; }

      bool getRefresh() const { return mForceRefresh; }
      void setRefresh() { mForceRefresh = true; }

      // for editing name/desc of game files in user storage
      BUString mName;
      BUString mDesc;

      // cache the filename from the content/filedesc
      BString mFilename;

      uint mTempID;

      PXCONTENT_DATA mpContent;
      BFileDesc* mpFileDesc;

      BStream* mpStream;

      DWORD mUserIndex;

      BStorageType mStorageType;

      bool mForceRefresh : 1;
};

//==============================================================================
// BGameFileStream
//==============================================================================
class BGameFileStream : public BDynamicStream
{
   public:
      // this allows me to dump record & save game data out to a file immediately
      //
      // this will only be used for recording & saving
      //
      // playback & loading will use the normal streams
      //
      // writeBytes and seek are the only two methods I need to override apparently

      BGameFileStream(uint flags = cSFReadable | cSFWritable | cSFSeekable, int reserveSize = -1, const BString& name = B("")) :
         BDynamicStream(flags, reserveSize, name),
         mpStream(NULL),
         mOutOfMemory(false)
      {
      }

      BGameFileStream(BStream* pStream, uint flags = cSFReadable | cSFWritable | cSFSeekable, int reserveSize = -1, const BString& name = B("")) :
         BDynamicStream(flags, reserveSize, name),
         mpStream(pStream),
         mOutOfMemory(false)
      {
      }

      ~BGameFileStream()
      {
         if (mpStream != NULL)
            mpStream->close();
         delete mpStream;
      }

      uint writeBytes(const void* p, uint n)
      {
         // stop writing when we're about to exceed our buffer size
         if (mOutOfMemory || mBuf.getCapacity() - mBuf.size() < n)
         {
            if (mpStream)
               return mpStream->writeBytes(p, n);
            mOutOfMemory = true;
            return 0;
         }

         uint r = BDynamicStream::writeBytes(p, n);

         // ignore the success/failure of the mpStream write
         if (mpStream != NULL && r)
            mpStream->writeBytes(p, n);

         return r;
      }

      int64 seek(int64 ofs64, bool absolute = true)
      {
         int64 r = BDynamicStream::seek(ofs64, absolute);

         if (mpStream != NULL && r != -1)
            mpStream->seek(ofs64, absolute);

         return r;
      }

      bool close()
      {
         if (mpStream != NULL)
            mpStream->close();

         return true;
      }

      bool isOutOfMemory() const { return mOutOfMemory; }

   private:
      BStream* mpStream;
      bool mOutOfMemory : 1;
};

//==============================================================================
// 
//==============================================================================
class BGameFileRequest
{
   public:
      enum BGameFileRequestType
      {
         eInvalidRequest,
         eSaveRequest,
         eDeleteRequest,
      };

      BGameFileRequest()
      {
         Utils::FastMemSet(&mContent, 0, sizeof(XCONTENT_DATA));
         mID = 0;
         mTempID = 0;
         mpSourceStream = NULL;
         mUserIndex = 0;
         mEventID = 0;
         mEvent = INVALID_HANDLE_VALUE;
         mSourceStorageType = eStorageUnknown;
         mRequestType = eInvalidRequest;
         mInProgress = false;
         mRefreshCache = false;
         mSaveGame = false;
      }

      static DWORD mNextEventID;

      XCONTENT_DATA        mContent;
      uint64               mID;
      BString              mSourceFilename;
      uint                 mTempID;
      BStream*             mpSourceStream;
      DWORD                mUserIndex;
      DWORD                mEventID; // used for device selector callback in the usermanager, insure that we're the one to request the async task
      HANDLE               mEvent;
      BStorageType         mSourceStorageType;
      BGameFileRequestType mRequestType;
      //bool                 mIsSaving : 1;
      bool                 mInProgress : 1;
      bool                 mRefreshCache : 1;
      bool                 mSaveGame : 1;
};

//==============================================================================
// BGameFile
//==============================================================================
class BGameFile : public BEventReceiverInterface, public BAsyncNotify, public ILSPMediaTask
{
   public:
               BGameFile();
               virtual ~BGameFile();

      virtual bool   setup();
      void           shutdown();
      virtual void   reset();

      // clean the game file cache, use when leaving the game file menu
      void     clean();

      // refresh the list of available game files
      void     refresh(uint port);

      bool     saveBase();
      bool     loadBase(bool userStorage, XCONTENT_DATA* pContentData);
      void     close();

      int      getGameFileType() const { return mGameFileType; }

      const BString& getCachePrefix() const { return mCachePrefix; }
      const BString& getGameFileExtension() const { return mGameFileExt; }
      const BString& getGameFileName() const { return mGameFileName; }

      enum BEditType
      {
         cEditName = 0,
         cEditDesc
      };

      bool     deleteFile(uint port, uint id);

      void     editFile(uint port, uint id, BEditType edit);

      bool     saveFile(uint port);
      void     saveFile(uint port, uint id);
      void     cancelSaveFile();
      bool     canSaveFile(uint port) const;

      bool     uploadFile(uint userIndex, uint manifestTempID);
      bool     downloadFile(uint userIndex, uint manifestTempID);

      bool     isMultiplayer() const { return mMultiplayer; }

      long     getVersion() const { return mBaseVersion; }

      //long     getLocalPlayerID() const { return mLocalPlayerID; }

      void     setFogOfWar(bool val) { mFogOfWar=val; }

      const BSmallDynamicSimArray<BGameFileManifest*>& getGameFiles() const { return mGameFiles; }

      uint8    getLastUpdate() const { return mLastUpdate; }

      bool     getHeaderAndSettings(BSimString& fileName, bool userStorage, XCONTENT_DATA* pContentData, BGameSettings* pSettings);

      const BGameFileHeader& getHeader() const { return mHeader; }

      bool     getSaveRequestInProgress() { return mSaveRequest.mInProgress; }

      enum
      {
         cEventClassSave = cEventClassFirstUser,
         cEventClassRefreshManifest,
         cEventClassContentEnumerate,
         cEventClassRefresh,
         cEventClassRefreshComplete,
         cEventClassEdit,
         cEventClassSaveHeader,
         cEventClassDelete,
      };

      enum
      {
         cGameFileRecord,
         cGameFileSave,
      };

      static const DWORD cBaseVersion;

      static const char* const cKeyPhrase;

   protected:

      void     resetWrappers();

      BStream* getStream(const BGameFilePayload& payload, bool readOnly);
      BStream* getStream(const BGameFileManifest& manifest);
      BStream* getStream(const char* pFileName, bool readOnly);
      BStream* getStream(BStorageType type, const char* pFileName, bool readOnly);
      BStream* getStream(DWORD userIndex, BStorageType type, const char* pFileName, bool readOnly);
      BStream* getWin32Stream(const char* pFileName, bool readOnly);
      BStream* getSystemStream(const char* pFileName, bool readOnly);
      BStream* getXContentStream(DWORD userIndex, const XCONTENT_DATA& content, uint flags);
      BStream* getXContentStream(const BGameFilePayload& payload, bool readOnly);
      BStream* getXContentStream(const BGameFileManifest& manifest, bool readOnly);

      bool     loadSettings(uint version, BGameSettings* pSettings, BStream* pStream, bool skipLoadFileSettings, DWORD& checksum, bool& multiplayer, long& localPlayerID);

      bool     refreshManifest(BStorageType type, BGameFileManifest& manifest, BGameSettings* pSettings, BStream* pStream);
      bool     refreshManifest(BStorageType type, BGameFileManifest& manifest, BStream* pStream);
      bool     refreshManifest(BStorageType type, BGameFileManifest& manifest, const BFileDesc& fileDesc);

      bool     refreshManifest(BGameFileManifest& manifest);

      BGameFileManifest* refreshManifest(const BGameFilePayload& payload);

      void     refresh(DWORD userIndex, uint searchID, BStorageType type);
      void     refreshComplete();
      void     refreshDisk(DWORD userIndex, uint searchID);
      void     refreshCache(DWORD userIndex, uint searchID);
      void     refreshStorage(DWORD userIndex, uint searchID);
      bool     refreshServer(DWORD userIndex, uint searchID);
      bool     refreshServerFriends(DWORD userIndex, uint searchID);

      void     enumerateResult(BGameFilePayload* pPayload);
      void     refreshResult(BGameFileManifest* pManifest);

      void     calcName(BSimString& name, BGameSettings* pSettings);
      void     calcFileName(BSimString& name, BGameSettings* pSettings);
      void     calcCacheName(BSimString& name, BGameSettings* pSettings);

      // BAsyncNotify - for the device selector UI
      // if we perform any more async task notifications, then
      // we'll need to see about setting the eventID somehow
      virtual void notify(DWORD eventID, void* pTask);

      void     beginSaveFile();
      void     endSaveFile(DWORD dwRet, BGameFilePayload* pPayload=NULL);

      void     endDeleteFile(DWORD dwRet, BGameFilePayload* pPayload=NULL);

      void     endEditFile(DWORD dwRet, BGameFilePayload* pPayload);

      DWORD    saveFile(const BGameFilePayload& pPayload);
      DWORD    saveHeader(const BGameFilePayload& payload);

      DWORD    deleteFile(const BGameFilePayload& payload);

      void     saveConfigs();
      void     saveConfigIsDefined(int config);
      void     saveConfigFloat(int config);

      void     loadConfigs();
      void     loadConfigIsDefined(int config, bool& saveTo);
      void     loadConfigFloat(int config, float& saveTo);

      void     restoreConfigs();
      void     restoreConfigIsDefined(int config, bool restoreFrom);
      void     restoreConfigFloat(int config, float restoreFrom);

      // ILSPMediaTask
      virtual void mediaTaskRelease(uint taskID, uint32 result);
      virtual void mediaListResponse(uint taskID, uint32 result, uint crc, uint ttl, const void* pData, int32 size);
      virtual void mediaDownloadResponse(uint taskID, uint32 result);
      virtual void mediaUploadResponse(uint taskID, uint32 result);
      virtual void mediaDeleteResponse(uint taskID, uint32 result);

      // BEventReceiverInterface
      bool     receiveEvent(const BEvent& event, BThreadIndex threadIndex);

      BGameFileHeader      mHeader;

      BSmallDynamicSimArray<BGameFileManifest*> mGameFiles;
      BSmallDynamicSimArray<BLSPMediaTask*> mMediaTasks;

      BSHA1Gen             mSha1Gen;

      // used to verify refresh requests to check if the user changed
      XUID                 mXuid;

      BString              mCachePrefix;
      BString              mGameFileExt;
      BString              mGameFileName;

      BGameFileRequest     mDeleteRequest;
      BGameFileRequest     mSaveRequest;

      uint64               mDataOfs;

      BEventReceiverHandle mEventHandleIO;
      BEventReceiverHandle mEventHandleSim;
      BEventReceiverHandle mEventHandleMisc;

      int                  mGameFileType; 
      int                  mGameDirID;

      BGameSettings*       mpGameSettings;

      BStream*             mpStream;

      BStream*             mpRawStream;

      BEncryptedStream*    mpEncryptRecStream;
      BDecryptedStream*    mpDecryptRecStream;

      BInflateStream*      mpInflateRecStream;
      BDeflateStream*      mpDeflateRecStream;

      BLSPMediaTask*       mpListMediaTask;
      BLSPMediaTask*       mpListFriendsMediaTask;
      BLSPMediaTask*       mpDownloadMediaTask;
      BLSPMediaTask*       mpUploadMediaTask;
      BLSPMediaTask*       mpDeleteMediaTask;

      uint                 mLastListTime;

      uint                 mListCRC; // last known CRC for list request
      uint                 mListTTL; // TTL for sending more list requests

      uint                 mListFriendsCRC; // last known CRC for friends list request
      uint                 mListFriendsTTL; // TTL for sending more list requests

      DWORD                mSelectDeviceEventID;

      DWORD    mBaseVersion;
      BGameFileVersion mDetails;

      long     mLocalPlayerID;

      volatile LONG mNextTempID;

      uint     mMaxMemorySize; // if streaming record game data into memory, stop recording when this size is reached

      volatile LONG mSearchID;

      volatile LONG mRefreshInProgress;

      float    mSaveConfigPlatoonRadius;
      float    mSaveConfigProjectionTime;
      float    mSaveConfigOverrideGroundIKRange;
      float    mSaveConfigOverrideGroundIKTiltFactor;
      float    mSaveConfigGameSpeed;
      bool     mSaveConfigAIDisable;
      bool     mSaveConfigAIShadow;
      bool     mSaveConfigNoVismap;
      bool     mSaveConfigNoRandomPlayerPlacement;
      bool     mSaveConfigDisableOneBuilding;
      bool     mSaveConfigBuildingQueue;
      bool     mSaveConfigUseTestLeaders;
      bool     mSaveConfigEnableFlight;
      bool     mSaveConfigNoBirthAnims;
      bool     mSaveConfigVeterancy;
      bool     mSaveConfigTrueLOS;
      bool     mSaveConfigNoDestruction;
      bool     mSaveConfigCoopSharedResources;
      bool     mSaveConfigMaxProjectileHeightForDecal;
      bool     mSaveConfigEnableSubbreakage;
      bool     mSaveConfigEnableThrowPart;
      bool     mSaveConfigAllowAnimIsDirty;
      bool     mSaveConfigNoVictoryCondition;
      bool     mSaveConfigAIAutoDifficulty;
      bool     mSaveConfigDemo;
      bool     mSaveConfigAsyncWorldUpdate;
      bool     mSaveConfigEnableHintSystem;
      bool     mSaveConfigPercentFadeTimeCorpseSink;
      bool     mSaveConfigCorpseSinkSpeed;
      bool     mSaveConfigCorpseMinScale;
      bool     mSaveConfigBlockOutsideBounds;
      bool     mSaveConfigAINoAttack;
      bool     mSaveConfigPassThroughOwnVehicles;
      bool     mSaveConfigEnableCapturePointResourceSharing;      
      bool     mSaveConfigNoUpdatePathingQuad;
      bool     mSaveConfigSlaveUnitPosition;
      bool     mSaveConfigTurning;
      bool     mSaveConfigHumanAttackMove;
      bool     mSaveConfigMoreNewMovement3;
      bool     mSaveConfigOverrideGroundIK;
      bool     mSaveConfigDriveWarthog;
      bool     mSaveConfigEnableCorpses;
      bool     mSaveConfigDisablePathingLimits;
      bool     mSaveConfigDisableVelocityMatchingBySquadType;
      bool     mSaveConfigActiveAbilities;
      bool     mSaveConfigAlpha;
      bool     mSaveConfigNoDamage;
      bool     mSaveConfigIgnoreAllPlatoonmates;
      bool     mSaveConfigClassicPlatoonGrouping;
      bool     mSaveConfigNoShieldDamage;
      bool     mSaveConfigEnableSubUpdating;
      bool     mSaveConfigMPSubUpdating;
      bool     mSaveConfigAlternateSubUpdating;
      bool     mSaveConfigDynamicSubUpdateTime;
      bool     mSaveConfigDecoupledUpdate;


      uint8    mLastUpdate;

      bool     mMultiplayer;

      bool     mFileSaving:1;
      bool     mFileLoading:1;

      bool     mFogOfWar : 1;
      bool     mSaveConfigNoFogMask : 1;

      // recording to the various devices is treated as a preference
      // if the device is unavailable, then BGameFile attempts to
      // choose the next best alternative
      bool     mSaveToCacheDrive : 1;
      bool     mRecordToMemory : 1;

      // cache drive enabled
      bool     mCacheEnabled : 1;

      // after the checks have been made to determine the appropriate
      // storage medium, these flags will be set
      bool     mUseCacheDrive : 1;
      // for retail builds, we must disable BFile usage
      bool     mUseBFile : 1;

      bool     mUsingGameFileStream : 1;

      // allow us to skip certain validity checks in case we're trying to debug a recording
      bool     mDebugging : 1;
};

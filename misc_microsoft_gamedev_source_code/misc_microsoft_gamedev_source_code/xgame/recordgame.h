//==============================================================================
// recordgame.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "gamefile.h"
#include "commands.h"

// Forward declarations
class BRecordUIUnitSound;

//==============================================================================
// BRecordGame
//==============================================================================
class BRecordGame : public BGameFile
{
   public:
               BRecordGame();
               ~BRecordGame();

      virtual bool   setup();

      bool     record();
      bool     play();
      void     stop();

      bool     isRecording() const { return mIsRecording; }
      bool     isPlaying() const { return mIsPlaying; }
      bool     isViewLocked() const { return mIsViewLocked; }
      bool     isSyncing() const { return mIsSyncing; }

      int      getCurrentPlayerID() const { return mCurrentPlayer; }
      void     setCurrentPlayer(long playerID) { mCurrentPlayer=playerID; }
      void     toggleViewLock();

      void     recordPlayers();
      void     recordUserPlayer();
      void     recordUpdateTimes(int64 currentUpdateTime, float currentUpdateLength, DWORD updateNumber);
      void     recordSubUpdateTime();
      void     recordUIUserSound(BCueIndex cueIndex);
      void     recordUIUnitSound(int protoObjectID, int soundType, bool suppressBankLoad, BVector position);
      void     recordUser();
      void     recordCommands(BCommandPointerArray& commands);
      void     recordSync();

      void     playPlayers();
      void     playUserPlayer();
      bool     playUpdateTimes(int64& currentUpdateTime, float& currentUpdateLength, DWORD updateNumber, int64 lastUpdateTime, double timerFrequency);
      void     playSubUpdateTime();
      void     playUser();
      void     playCommands(BCommandPointerArray& commands);
      void     playSync();

      virtual void   reset();

      static const DWORD cRecordGameVersion;

   private:

      BVector  mCameraPosition;
      BVector  mCameraForward;
      BVector  mCameraRight;

      BSmallDynamicSimArray<long> mSelectionList;
      BSmallDynamicSimArray<BCueIndex> mUIUserSounds;
      BSmallDynamicSimArray<BRecordUIUnitSound> mUIUnitSounds;

      unsigned __int64 mCallTableOffset;

      BString  mSyncFileName;

      BStream* mpSyncStream;

      DWORD    mRecordGameVersion;

      long     mCurrentPlayer;

      int      mPendingDataType;

      bool     mIsRecording : 1;
      bool     mIsPlaying : 1;
      bool     mIsViewLocked : 1;
      bool     mIsSyncing : 1;
};

extern BRecordGame gRecordGame;

//==============================================================================
// BRecordUIUnitSound
//==============================================================================
class BRecordUIUnitSound
{
   public:
      BVector  mPosition;
      uint16   mProtoObjectID;
      uint8    mSoundType:7;
      bool     mSuppressBankLoad:1;

      bool read(BStream& stream, DWORD version);
};

//==============================================================================
// recordgame.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "recordgame.h"
#include "camera.h"
#include "chunkwriter.h"
#include "commandmanager.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "entity.h"
#include "gamedirectories.h"
#include "gamefilemanifest.h"
#include "humanPlayerAITrackingData.h"
#include "modegame.h"
#include "modemanager.h"
#include "protoobject.h"
#include "selectionmanager.h"
#include "soundmanager.h"
#include "string\strPathHelper.h"
#include "syncmanager.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"

#define DEBUG_MARKERS

// Version
const DWORD BRecordGame::cRecordGameVersion = 7;
namespace BRecordGameVersion
{
   enum
   {
      cRecordGameVersion6 = 6,
   };
}
// 7 - increase the number of commands to support to a uint16

// Global
BRecordGame gRecordGame;

enum
{
   cRecordMarkerBase=156,
   cRecordMarkerPlayers,
   cRecordMarkerUserPlayer,
   cRecordMarkerUpdateTimes,
   cRecordMarkerUser,
   cRecordMarkerCommands,
   cRecordMarkerSubUpdateTime,
};

//==============================================================================
// BRecordGame::BRecordGame
//==============================================================================
BRecordGame::BRecordGame() :
   BGameFile(),
   mCameraPosition(cOriginVector),
   mCameraForward(cZAxisVector),
   mCameraRight(cXAxisVector),
   mSelectionList(),
   mUIUserSounds(),
   mUIUnitSounds(),
   mCallTableOffset(0),
   mSyncFileName(),
   mpSyncStream(NULL),
   mRecordGameVersion(0),
   mCurrentPlayer(1),
   mPendingDataType(-1),
   mIsRecording(false),
   mIsPlaying(false),
   mIsViewLocked(true),
   mIsSyncing(false)
{
}

//==============================================================================
// BRecordGame::~BRecordGame
//==============================================================================
BRecordGame::~BRecordGame()
{
   if (mIsRecording || mIsPlaying)
      stop();
}

//==============================================================================
// BRecordGame::setup
//==============================================================================
bool BRecordGame::setup()
{
   mGameFileType = cGameFileRecord;
   mGameDirID = cDirRecordGame;
   mCachePrefix = "cache:\\recordgame";
   mGameFileExt = ".rec";
   return BGameFile::setup();
}

//==============================================================================
// BRecordGame::record
//==============================================================================
bool BRecordGame::record()
{
   if (mIsRecording || mIsPlaying)
   {
      BASSERT(0);
      return false;
   }

   if (!saveBase())
      return false;

   mRecordGameVersion = cRecordGameVersion;
   mpStream->writeBytes(&mRecordGameVersion, sizeof(DWORD));

#ifdef DEBUG_MARKERS
   BYTE marker = cRecordMarkerBase;
   mpStream->writeBytes(&marker, sizeof(BYTE));
#endif

   mIsSyncing = false;
   if (gConfig.isDefined(cConfigRecordGameSync))
   {
      mSyncFileName = mGameFileName;
      strPathRemoveExtension(mSyncFileName);
      strPathAddExtension(mSyncFileName, "sync");
      mpSyncStream = getStream(mSyncFileName, false);
      if (mpSyncStream)
      {
         mIsSyncing = true;
         unsigned __int64 callTableOffset=0;
         mpSyncStream->writeBytes(&callTableOffset, sizeof(unsigned __int64));
      }
   }

   mCameraPosition = cOriginVector;
   mCameraForward = cZAxisVector;
   mCameraRight = cXAxisVector;

   mUIUserSounds.clear();
   mUIUnitSounds.clear();

   mIsRecording = true;

   return true;
}

//==============================================================================
// BRecordGame::play
//==============================================================================
bool BRecordGame::play()
{
   if (mIsRecording || mIsPlaying)
   {
      BASSERT(0);
      return false;
   }

   if (!loadBase(false, NULL))
      return false;

   mRecordGameVersion = 0;
   if (mBaseVersion >= 16)
   {
      if (mpStream->readBytes(&mRecordGameVersion, sizeof(DWORD)) != sizeof(DWORD))
         return false;
      if (mRecordGameVersion > cRecordGameVersion)
         return false;
   }

#ifdef DEBUG_MARKERS
   if (mRecordGameVersion >= 5)
   {
      BYTE marker;
      if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
      {
         BASSERT(0);
         return false;
      }
      if (marker != cRecordMarkerBase)
      {
         BASSERT(0);
         return false;
      }
   }
#endif

   mIsSyncing=false;
   if (gConfig.isDefined(cConfigRecordGameSync))
   {
      mSyncFileName = mGameFileName;
      strPathRemoveExtension(mSyncFileName);
      strPathAddExtension(mSyncFileName, "sync");
      mpSyncStream = getStream(mSyncFileName, true);
      if (mpSyncStream)
      {
         if (mpSyncStream->readBytes(&mCallTableOffset, sizeof(unsigned __int64)) != sizeof(unsigned __int64))
            mpSyncStream->close();
         else
         {
            if (mpSyncStream->seek(static_cast<int64>(mCallTableOffset)) != static_cast<int64>(mCallTableOffset))
               mpSyncStream->close();
            else
            {
               BSyncManager::getInstance()->activatePlaybackHistory();
               BSyncHistory* pHistory = BSyncManager::getInstance()->getPlaybackHistory();
               BChunkReader reader(mpSyncStream, false);
               pHistory->loadCallTable(&reader);
               mpSyncStream->seek(sizeof(unsigned __int64));
               mIsSyncing = true;
            }
         }
      }
   }

   mCameraPosition=cOriginVector;
   mCameraForward=cZAxisVector;
   mCameraRight=cXAxisVector;

   mCurrentPlayer=1;
   mPendingDataType=-1;
   mIsViewLocked=true;
   mIsPlaying=true;

   playSync();

   return true;
}

//==============================================================================
// BRecordGame::stop
//==============================================================================
void BRecordGame::stop()
{
   if (mIsPlaying)
   {
      BSyncManager::getInstance()->deactivatePlaybackHistory();

      gUserManager.notify(BEntity::cEventPlaybackDone, cInvalidObjectID, 0, 0);

      gModeManager.getModeGame()->setPaused(true);

      restoreConfigs();
   }
   else if (mIsRecording)
   {
      if (mpSyncStream)
      {
         unsigned __int64 callTableOffset = mpSyncStream->curOfs();
         BSyncHistory* pHistory = BSyncManager::getInstance()->getCurrentHistory();
         BChunkWriter writer(mpSyncStream, false);
         pHistory->saveCallTable(&writer);
         if (mpSyncStream->seek(0) == 0)
            mpSyncStream->writeBytes(&callTableOffset, sizeof(unsigned __int64));
      }
   }

   close();

   if (mpSyncStream)
   {
      mpSyncStream->close();
      mpSyncStream = NULL;
   }

   mIsRecording = false;
   mIsPlaying = false;
   mIsSyncing = false;
}

//==============================================================================
// BRecordGame::recordPlayers
//==============================================================================
void BRecordGame::recordPlayers()
{
   if(!mIsRecording)
      return;

   int numPlayers = gWorld->getNumberPlayers();
   for (int i=0; i<numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(i);

      // Save of tracking data code modeled after BUserProfile::saveCompressedData so that it can
      // be saved and loaded through the BHumanPlayerAITrackingData class.
      BHumanPlayerAITrackingData* pData = pPlayer->getTrackingData();
      if (pData)
      {
         // Write the AI data to a new stream
         BDynamicStream stream1;
         pData->saveValuesToMemoryBlock(&stream1);

         // Compress the data to another stream
         BDynamicStream stream2;
         BDeflateStream deflStream(stream2);
         deflStream.writeBytes(stream1.getBuf().getPtr(), stream1.getBuf().getSizeInBytes());
         deflStream.close();

         // Save the data 
         uint size = stream2.getBuf().getSizeInBytes();
         if (size > 0)
         {
            mpStream->writeBytes(&i, sizeof(int));
            uint totalSize = size + 4;
            mpStream->writeBytes(&totalSize, sizeof(uint));

            if (totalSize > 10240)
            {
               BASSERT(0);
               stop();
               return;
            }

            mpStream->writeBytes(&size, sizeof(uint));
            mpStream->writeBytes(stream2.getBuf().getPtr(), size);
         }
      }
   }

   int playerID = -1;
   mpStream->writeBytes(&playerID, sizeof(int));

#ifdef DEBUG_MARKERS
   BYTE marker = cRecordMarkerPlayers;
   mpStream->writeBytes(&marker, sizeof(BYTE));
#endif
}

//==============================================================================
// BRecordGame::recordUserPlayer
//==============================================================================
void BRecordGame::recordUserPlayer()
{
   if(!mIsRecording)
      return;
   long playerID=gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID();
   mpStream->writeBytes(&playerID, sizeof(long));

#ifdef DEBUG_MARKERS
   BYTE marker = cRecordMarkerUserPlayer;
   mpStream->writeBytes(&marker, sizeof(BYTE));
#endif
}

// User Flags
static const BYTE cUserFlagCameraPositionX=0x01;
static const BYTE cUserFlagCameraPositionY=0x02;
static const BYTE cUserFlagCameraPositionZ=0x04;
static const BYTE cUserFlagCameraForward=0x08;
static const BYTE cUserFlagCameraRight=0x10;
static const BYTE cUserFlagSelection=0x20;
static const BYTE cUserFlagUIUserSounds=0x40;
static const BYTE cUserFlagUIUnitSounds=0x80;

//==============================================================================
// BRecordGame::recordUser
//==============================================================================
void BRecordGame::recordUser()
{
   if(!mIsRecording)
      return;

   BYTE marker = cRecordMarkerUser;
   mpStream->writeBytes(&marker, sizeof(BYTE));

   BYTE userFlags=0;

   // Camera changes
//-- FIXING PREFIX BUG ID 2633
   const BCamera* pCamera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
//--
   BVector position=pCamera->getCameraLoc();
   BVector forward=pCamera->getCameraDir();
   BVector right=pCamera->getCameraRight();
   if(position.x!=mCameraPosition.x)
      userFlags|=cUserFlagCameraPositionX;
   if(position.y!=mCameraPosition.y)
      userFlags|=cUserFlagCameraPositionY;
   if(position.z!=mCameraPosition.z)
      userFlags|=cUserFlagCameraPositionZ;
   if(forward!=mCameraForward)
      userFlags|=cUserFlagCameraForward;
   if(right!=mCameraRight)
      userFlags|=cUserFlagCameraRight;
   if(mUIUserSounds.getSize()>0)
      userFlags|=cUserFlagUIUserSounds;
   if(mUIUnitSounds.getSize()>0)
      userFlags|=cUserFlagUIUnitSounds;

   mCameraPosition=position;
   mCameraForward=forward;
   mCameraRight=right;

   // Selection changes
   BSelectionManager* pSelMgr=gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();
   long count=pSelMgr->getNumberSelectedUnits();
   if(count>0)
   {
      BSmallDynamicSimArray<long> ids;
      for(long i=0; i<count; i++)
      {
         BEntityID id=pSelMgr->getSelected(i);
         if(id.getType()==BEntity::cClassTypeSquad)
         {
            ids.uniqueAdd(id.asLong());
         }
         else if(id.getType()==BEntity::cClassTypeUnit)
         {
//-- FIXING PREFIX BUG ID 2631
            const BUnit* pUnit=gWorld->getUnit(id);
//--
            if(pUnit)
            {
               BEntityID parentID=pUnit->getParentID();
               if(parentID!=cInvalidObjectID)
                  ids.uniqueAdd(parentID.asLong());
               else
                  ids.uniqueAdd(id.asLong());
            }
         }
         else
            ids.uniqueAdd(id.asLong());
      }
      ids.sort();
      if(ids.getNumber()!=mSelectionList.getNumber())
         userFlags|=cUserFlagSelection;
      else
      {
         for(long i=0; i<ids.getNumber(); i++)
         {
            if(ids[i]!=mSelectionList[i])
            {
               userFlags|=cUserFlagSelection;
               break;
            }
         }
      }
      if(userFlags&cUserFlagSelection)
      {
         mSelectionList.setNumber(0);
         for(long i=0; i<ids.getNumber(); i++)
            mSelectionList.add(ids[i]);
      }
   }
   else if(mSelectionList.getNumber()>0)
   {
      mSelectionList.setNumber(0);
      userFlags|=cUserFlagSelection;
   }

   // Write the changes
   mpStream->writeBytes(&userFlags, sizeof(BYTE));

   if(userFlags&cUserFlagCameraPositionX)
      mpStream->writeBytes(&mCameraPosition.x, sizeof(float));
   if(userFlags&cUserFlagCameraPositionY)
      mpStream->writeBytes(&mCameraPosition.y, sizeof(float));
   if(userFlags&cUserFlagCameraPositionZ)
      mpStream->writeBytes(&mCameraPosition.z, sizeof(float));
   if(userFlags&cUserFlagCameraForward)
      mpStream->writeBytes(&mCameraForward, sizeof(BVector));
   if(userFlags&cUserFlagCameraRight)
      mpStream->writeBytes(&mCameraRight, sizeof(BVector));

   if(userFlags&cUserFlagSelection)
   {
      long count=mSelectionList.getNumber();
      mpStream->writeBytes(&count, sizeof(long));
      for(long i=0; i<count; i++)
      {
         long id=mSelectionList[i];
         mpStream->writeBytes(&id, sizeof(long));
      }
   }

   if (userFlags&cUserFlagUIUserSounds)
   {
      uint8 count=(uint8)mUIUserSounds.getSize();
      mpStream->writeBytes(&count, sizeof(uint8));
      for(uint8 i=0; i<count; i++)
      {
         BCueIndex cueIndex=mUIUserSounds[i];
         mpStream->writeBytes(&cueIndex, sizeof(BCueIndex));
      }
   }
   mUIUserSounds.clear();

   if (userFlags&cUserFlagUIUnitSounds)
   {
      uint8 count=(uint8)mUIUnitSounds.getSize();
      mpStream->writeBytes(&count, sizeof(uint8));
      for(uint8 i=0; i<count; i++)
      {
//-- FIXING PREFIX BUG ID 2632
         const BRecordUIUnitSound& sound=mUIUnitSounds[i];
//--
         mpStream->writeBytes(&sound, sizeof(BRecordUIUnitSound));
      }
   }
   mUIUnitSounds.clear();

#ifdef DEBUG_MARKERS
   marker = cRecordMarkerUser;
   mpStream->writeBytes(&marker, sizeof(BYTE));
#endif
}

//==============================================================================
// BRecordGame::recordUpdateTimes
//==============================================================================
void BRecordGame::recordUpdateTimes(int64 currentUpdateTime, float currentUpdateLength, DWORD updateNumber)
{
   if(!mIsRecording)
      return;
   if(updateNumber==0)
      mpStream->writeBytes(&currentUpdateTime, sizeof(int64));
   //FIXME AJL 11/13/06 - Would be good to find a way to lessen the amount of space recording the update time takes up
   mpStream->writeBytes(&currentUpdateLength, sizeof(float));

   mHeader.updateLength(currentUpdateLength);

#ifdef DEBUG_MARKERS
   BYTE marker = cRecordMarkerUpdateTimes;
   mpStream->writeBytes(&marker, sizeof(BYTE));
#endif
}

//==============================================================================
// BRecordGame::recordSubUpdateTime
//==============================================================================
void BRecordGame::recordSubUpdateTime()
{
   if(!mIsRecording)
      return;

   BYTE marker = cRecordMarkerSubUpdateTime;
   mpStream->writeBytes(&marker, sizeof(BYTE));

   long time = gWorld->getSubUpdateTimeInMsecs();
   BASSERT(time <= 255);
   BYTE byteTime = (BYTE)time;
   mpStream->writeBytes(&byteTime, sizeof(BYTE));

#ifdef DEBUG_MARKERS
   marker = cRecordMarkerSubUpdateTime;
   mpStream->writeBytes(&marker, sizeof(BYTE));
#endif
}

//==============================================================================
// BRecordGame::recordUIUserSound
//==============================================================================
void BRecordGame::recordUIUserSound(BCueIndex cueIndex)
{
   if (!mIsRecording)
      return;
   if (cueIndex==cInvalidCueIndex)
      return;
   if (mUIUnitSounds.getSize()>=255)
      return;
   mUIUserSounds.add(cueIndex);
}

//==============================================================================
// BRecordGame::recordUIUnitSound
//==============================================================================
void BRecordGame::recordUIUnitSound(int protoObjectID, int soundType, bool suppressBankLoad, BVector position)
{
   if (!mIsRecording)
      return;
   if (protoObjectID<0 || protoObjectID>UINT16_MAX)
      return;
   if (soundType<0 || soundType>UINT8_MAX)
      return;
   if (mUIUnitSounds.getSize()>=255)
      return;
   BRecordUIUnitSound sound;
   sound.mProtoObjectID=(uint16)protoObjectID;
   sound.mSoundType=(uint8)soundType;
   sound.mSuppressBankLoad=suppressBankLoad;
   sound.mPosition = position;
   mUIUnitSounds.add(sound);
}

//==============================================================================
// BRecordGame::recordCommands
//==============================================================================
void BRecordGame::recordCommands(BCommandPointerArray& commands)
{
   if(!mIsRecording)
      return;

   // assert at 255 but still allow it
   BASSERTM(commands.getNumber() <= 255, "We should not be queueing more than 255 commands");

   // don't allow more than 65535 commands
   if (commands.getNumber() > 65535)
      return;

   BYTE marker = cRecordMarkerCommands;
   mpStream->writeBytes(&marker, sizeof(BYTE));

   // Record the command list
   long commandCount=commands.getNumber();

   uint16 count = static_cast<uint16>(commandCount);
   mpStream->writeBytes(&count, sizeof(uint16));

   for (long i=0; i<commandCount; i++)
   {
      BCommand* pCommand=commands[i];
      if(!pCommand)
      {
         WORD bufferLen=0;
         mpStream->writeBytes(&bufferLen, sizeof(WORD));
      }
      else
      {
         BSerialBuffer sb;
         pCommand->serialize(sb);
         long bufferLen=sb.getBufferSize();
         if(bufferLen>65535)
         {
            BASSERT(0);
            stop();
            return;
         }
         WORD wordLen=(WORD)bufferLen;
         mpStream->writeBytes(&wordLen, sizeof(WORD));
         mpStream->writeBytes(sb.getBuffer(), bufferLen);
      }
   }

#ifdef DEBUG_MARKERS
   marker = cRecordMarkerCommands;
   mpStream->writeBytes(&marker, sizeof(BYTE));
#endif
}

//==============================================================================
// BRecordGame::recordSync
//==============================================================================
void BRecordGame::recordSync()
{
   if (!mIsRecording || !mIsSyncing)
      return;

   BSyncHistory* pHistory = BSyncManager::getInstance()->getCurrentHistory();
   BSyncUpdate* pUpdate = pHistory->getCurrentUpdate();

   BChunkWriter writer(mpSyncStream, false);
   pUpdate->save(&writer);
}

//==============================================================================
// BRecordGame::playPlayers
//==============================================================================
void BRecordGame::playPlayers()
{
   if(!mIsPlaying)
      return;

   if (mRecordGameVersion < 2)
      return;

   int numPlayers = gWorld->getNumberPlayers();
   for (int i=0; i<numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(i);
      pPlayer->setTrackingData(NULL);
   }

   for (;;)
   {
      int playerID = -1;
      if (mpStream->readBytes(&playerID, sizeof(int)) != sizeof(int))
      {
         BASSERT(0);
         stop();
         return;
      }
      if (playerID == -1)
         break;

      uint size = 0;
      if (mpStream->readBytes(&size, sizeof(uint)) != sizeof(uint))
      {
         BASSERT(0);
         stop();
         return;
      }
      if (size == 0)
         continue;

      if (size > 10240)
      {
         BASSERT(0);
         stop();
         return;
      }

      byte* pBuf = new byte[size];
      if (!pBuf)
      {
         mpStream->skipBytes(size);
         continue;
      }
      if (mpStream->readBytes(pBuf, size) != size)
      {
         BASSERT(0);
         stop();
         return;
      }

      BHumanPlayerAITrackingData* pData = new BHumanPlayerAITrackingData();
      pData->loadValuesFromMemoryBlock(pBuf);

      BPlayer* pPlayer = gWorld->getPlayer(playerID);
      if (pPlayer)
         pPlayer->setTrackingData(pData);
      else
         delete pData;

      delete []pBuf;
   }

#ifdef DEBUG_MARKERS
   if (mRecordGameVersion >= 5)
   {
      BYTE marker;
      if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
      {
         BASSERT(0);
         stop();
         return;
      }
      if (marker != cRecordMarkerPlayers)
      {
         BASSERT(0);
         stop();
         return;
      }
   }
#endif
}

//==============================================================================
// BRecordGame::playUserPlayer
//==============================================================================
void BRecordGame::playUserPlayer()
{
   if(!mIsPlaying)
      return;
   long playerID=1;
   if (mpStream->readBytes(&playerID, sizeof(long)) != sizeof(long))
   {
      BASSERT(0);
      stop();
      return;
   }
   mCurrentPlayer=playerID;

#ifdef DEBUG_MARKERS
   if (mRecordGameVersion >= 5)
   {
      BYTE marker;
      if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
      {
         BASSERT(0);
         stop();
         return;
      }
      if (marker != cRecordMarkerUserPlayer)
      {
         BASSERT(0);
         stop();
         return;
      }
   }
#endif
}

//==============================================================================
// BRecordGame::playUser
//==============================================================================
void BRecordGame::playUser()
{
   if(!mIsPlaying)
      return;

   // Code to handle sub updating since the # of user recordings can be different the # of requested user playbacks
   if (mRecordGameVersion >= 3)
   {
      if (mPendingDataType == cRecordMarkerUser)
         mPendingDataType = -1;
      else if (mPendingDataType == cRecordMarkerCommands || mPendingDataType == cRecordMarkerSubUpdateTime)
         return;
      else if (mPendingDataType == -1)
      {
         BYTE marker=0;
         if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
         {
            stop();
            return;
         }
         if (marker == cRecordMarkerCommands || marker == cRecordMarkerSubUpdateTime)
         {
            mPendingDataType = marker;
            return;
         }
         else if (marker != cRecordMarkerUser)
         {
            BASSERT(0);
            stop();
            return;
         }
      }
      else
      {
         BASSERT(0);
         stop();
         return;
      }
   }

   if (gWorld->getUpdateNumber()==0)
      gUserManager.getUser(BUserManager::cPrimaryUser)->switchPlayer(mCurrentPlayer);

   bool doUserUpdates=(mIsViewLocked && gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID()==mCurrentPlayer);

   BYTE userFlags=0;
   if (mpStream->readBytes(&userFlags, sizeof(BYTE)) != sizeof(BYTE))
   {
      stop();
      return;
   }

   // Camera changes
   bool cameraChange=false;
   if(userFlags&cUserFlagCameraPositionX)
   {
      if (mpStream->readBytes(&mCameraPosition.x, sizeof(float)) != sizeof(float))
      {
         stop();
         return;
      }
      cameraChange=true;
   }
   if(userFlags&cUserFlagCameraPositionY)
   {
      if (mpStream->readBytes(&mCameraPosition.y, sizeof(float)) != sizeof(float))
      {
         stop();
         return;
      }
      cameraChange=true;
   }
   if(userFlags&cUserFlagCameraPositionZ)
   {
      if (mpStream->readBytes(&mCameraPosition.z, sizeof(float)) != sizeof(float))
      {
         stop();
         return;
      }
      cameraChange=true;
   }
   if(userFlags&cUserFlagCameraForward)
   {
      if (mpStream->readBytes(&mCameraForward, sizeof(BVector)) != sizeof(BVector))
      {
         stop();
         return;
      }
      cameraChange=true;
   }
   if(userFlags&cUserFlagCameraRight)
   {
      if (mpStream->readBytes(&mCameraRight, sizeof(BVector)) != sizeof(BVector))
      {
         stop();
         return;
      }
      cameraChange=true;
   }

   if(cameraChange && doUserUpdates)
   {
      BCamera* pCamera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
      pCamera->setCameraLoc(mCameraPosition);
      pCamera->setCameraDir(mCameraForward);
      pCamera->setCameraRight(mCameraRight);

      BVector up;
      up.assignCrossProduct(mCameraForward, mCameraRight);
      up.normalize();
      pCamera->setCameraUp(up);

      gUserManager.getUser(BUserManager::cPrimaryUser)->setFlagUpdateHoverPoint(true);
   }

   // Selection changes
   if(userFlags&cUserFlagSelection)
   {
      BSelectionManager* pSelMgr=gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();
      if(doUserUpdates)
         pSelMgr->clearSelections();
      long count=0;
      if (mpStream->readBytes(&count, sizeof(long)) != sizeof(long))
      {
         stop();
         return;
      }
      for(long i=0; i<count; i++)
      {
         long id=-1;
         if (mpStream->readBytes(&id, sizeof(long)) != sizeof(long))
         {
            stop();
            return;
         }
         if(id!=-1 && doUserUpdates)
         {
            BEntityID fullID(id);
            if(fullID.getType()==BEntity::cClassTypeSquad)
            {
//-- FIXING PREFIX BUG ID 2637
               const BSquad* pSquad=gWorld->getSquad(fullID);
//--
               if(pSquad)
               {
                  for(uint j=0; j<pSquad->getNumberChildren(); j++)
                  {
//-- FIXING PREFIX BUG ID 2636
                     const BUnit* pUnit=gWorld->getUnit(pSquad->getChild(j));
//--
                     if(pUnit && pUnit->isAlive())
                        pSelMgr->selectUnit(pUnit->getID());
                  }
               }
            }
            else if(fullID.getType()==BEntity::cClassTypeUnit)
            {
//-- FIXING PREFIX BUG ID 2638
               const BUnit* pUnit=gWorld->getUnit(fullID);
//--
               if(pUnit && pUnit->isAlive())
                  pSelMgr->selectUnit(fullID);
            }
            else
               pSelMgr->selectUnit(fullID);
         }
      }
   }

   if (userFlags&cUserFlagUIUserSounds)
   {
      uint8 count=(uint8)0;
      if (mpStream->readBytes(&count, sizeof(uint8)) != sizeof(uint8))
      {
         stop();
         return;
      }
      for (uint8 i=0; i<count; i++)
      {
         BCueIndex cueIndex=cInvalidCueIndex;
         if (mpStream->readBytes(&cueIndex, sizeof(BCueIndex)) != sizeof(BCueIndex))
         {
            stop();
            return;
         }
         if (doUserUpdates && cueIndex!=cInvalidCueIndex)
            gSoundManager.playCue(cueIndex);
      }
   }

   if (userFlags&cUserFlagUIUnitSounds)
   {
      uint8 count=(uint8)0;
      if (mpStream->readBytes(&count, sizeof(uint8)) != sizeof(uint8))
      {
         stop();
         return;
      }
      for (uint8 i=0; i<count; i++)
      {
         BRecordUIUnitSound sound;
         if (sound.read(*mpStream, mRecordGameVersion))
         {
            if (doUserUpdates)
            {
//-- FIXING PREFIX BUG ID 2639
               const BProtoObject* pProtoObject=gDatabase.getGenericProtoObject(sound.mProtoObjectID);
//--
               if (pProtoObject)
                  pProtoObject->playUISound(static_cast<BSoundType>(sound.mSoundType), static_cast<bool>(sound.mSuppressBankLoad), static_cast<BVector>(sound.mPosition), cInvalidProtoID);
            }
         }
      }
   }

#ifdef DEBUG_MARKERS
   if (mRecordGameVersion >= 5)
   {
      BYTE marker;
      if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
      {
         BASSERT(0);
         stop();
         return;
      }
      if (marker != cRecordMarkerUser)
      {
         BASSERT(0);
         stop();
         return;
      }
   }
#endif
}

//==============================================================================
// BRecordGame::playUpdateTimes
//==============================================================================
bool BRecordGame::playUpdateTimes(int64& currentUpdateTime, float& currentUpdateLength, DWORD updateNumber, int64 lastUpdateTime, double timerFrequency)
{
   if (!mIsPlaying)
      return false;
   bool firstUpdate=(updateNumber==0);
   if (firstUpdate)
   {
      if (mpStream->readBytes(&currentUpdateTime, sizeof(int64)) != sizeof(int64))
      {
         stop();
         return false;
      }
   }
   if (mpStream->readBytes(&currentUpdateLength, sizeof(float)) != sizeof(float))
   {
      stop();
      return false;
   }
   if (!firstUpdate)
   {
      DWORD len = (DWORD)(currentUpdateLength * timerFrequency);
      currentUpdateTime=lastUpdateTime+len;
   }

#ifdef DEBUG_MARKERS
   if (mRecordGameVersion >= 5)
   {
      BYTE marker;
      if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
      {
         BASSERT(0);
         stop();
         return false;
      }
      if (marker != cRecordMarkerUpdateTimes)
      {
         BASSERT(0);
         stop();
         return false;
      }
   }
#endif

   return true;
}

//==============================================================================
// BRecordGame::playSubUpdateTime
//==============================================================================
void BRecordGame::playSubUpdateTime()
{
   if (mRecordGameVersion < 4)
      return;

   if (!mIsPlaying)
      return;

   // Code to handle sub updating since the # of user recordings can be different the # of requested user playbacks
   if (mRecordGameVersion >= 5)
   {
      if (mPendingDataType == cRecordMarkerSubUpdateTime)
         mPendingDataType = -1;
      else if (mPendingDataType == cRecordMarkerUser)
      {
         for (;;)
         {
            playUser();
            if (!mIsPlaying)
               return;
            BYTE marker=0;
            if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
            {
               stop();
               return;
            }
            if (marker == cRecordMarkerSubUpdateTime)
               break;
            else if (marker == cRecordMarkerUser)
            {
               mPendingDataType = cRecordMarkerUser;
               continue;
            }
            else
            {
               BASSERT(0);
               stop();
               return;
            }
         }
      }
      else if (mPendingDataType == -1)
      {
         for (;;)
         {
            BYTE marker=0;
            if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
            {
               stop();
               return;
            }
            if (marker == cRecordMarkerSubUpdateTime)
               break;
            else if (marker == cRecordMarkerUser)
            {
               mPendingDataType = cRecordMarkerUser;
               playUser();
               if (!mIsPlaying)
                  return;
               continue;
            }
            else
            {
               BASSERT(0);
               stop();
               return;
            }
         }
      }
      else
      {
         BASSERT(0);
         stop();
         return;
      }
   }

   BYTE byteTime = 0;
   if (mpStream->readBytes(&byteTime, sizeof(BYTE)) != sizeof(BYTE))
   {
      stop();
      return;
   }

   gWorld->setAdjustedSubUpdateTime((long)byteTime);
   
#ifdef DEBUG_MARKERS
   if (mRecordGameVersion >= 5)
   {
      BYTE marker;
      if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
      {
         BASSERT(0);
         stop();
         return;
      }
      if (marker != cRecordMarkerSubUpdateTime)
      {
         BASSERT(0);
         stop();
         return;
      }
   }
#endif

   return;
}

//==============================================================================
// BRecordGame::playCommands
//==============================================================================
void BRecordGame::playCommands(BCommandPointerArray& commands)
{
   if(!mIsPlaying)
      return;

   // Code to handle sub updating since the # of user recordings can be different the # of requested user playbacks
   if (mRecordGameVersion >= 3)
   {
      if (mPendingDataType == cRecordMarkerCommands)
         mPendingDataType = -1;
      else if (mPendingDataType == cRecordMarkerUser)
      {
         for (;;)
         {
            playUser();
            if (!mIsPlaying)
               return;
            BYTE marker=0;
            if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
            {
               stop();
               return;
            }
            if (marker == cRecordMarkerCommands)
               break;
            else if (marker == cRecordMarkerUser)
            {
               mPendingDataType = cRecordMarkerUser;
               continue;
            }
            else
            {
               BASSERT(0);
               stop();
               return;
            }
         }
      }
      else if (mPendingDataType == -1)
      {
         for (;;)
         {
            BYTE marker=0;
            if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
            {
               stop();
               return;
            }
            if (marker == cRecordMarkerCommands)
               break;
            else if (marker == cRecordMarkerUser)
            {
               mPendingDataType = cRecordMarkerUser;
               playUser();
               if (!mIsPlaying)
                  return;
               continue;
            }
            else
            {
               BASSERT(0);
               stop();
               return;
            }
         }
      }
      else
      {
         BASSERT(0);
         stop();
         return;
      }
   }

   // There shouldn't be any user commands
   BASSERT(commands.getNumber()==0);

   uint commandCount = 0;

   // Read in the command list
   if (mRecordGameVersion <= BRecordGameVersion::cRecordGameVersion6)
   {
      BYTE count=0;
      if (mpStream->readBytes(&count, sizeof(BYTE)) != sizeof(BYTE))
      {
         stop();
         return;
      }
      commandCount = static_cast<uint>(count);
   }
   else
   {
      uint16 count = 0;
      if (mpStream->readBytes(&count, sizeof(uint16)) != sizeof(uint16))
      {
         stop();
         return;
      }
      commandCount = static_cast<uint>(count);
   }

   if(commandCount>0)
   {
      BYTE buffer[1024];
      BSerialBuffer sb;
      for (uint i=0; i<commandCount; i++)
      {
         long bufferLen=0;
         WORD wordLen=0;
         if (mpStream->readBytes(&wordLen, sizeof(WORD)) != sizeof(WORD))
         {
            stop();
            return;
         }
         bufferLen=(long)wordLen;
         if(bufferLen==0)
            return;
         if(bufferLen>sizeof(buffer))
         {
            BASSERT(0);
            stop();
            return;
         }
         if(bufferLen>0)
         {
            if (mpStream->readBytes(buffer, bufferLen) != (uint)bufferLen)
            {
               stop();
               return;
            }
            BCommand* pCommand = BCommandManager::createCommand(-1, BCommand::getCommandType(buffer), true);
            if(pCommand)
            {
               pCommand->deserializeFrom(buffer, bufferLen);
               if(commands.add(pCommand)==-1)
                  delete pCommand;
            }
         }
      }
   }

#ifdef DEBUG_MARKERS
   if (mRecordGameVersion >= 5)
   {
      BYTE marker;
      if (mpStream->readBytes(&marker, sizeof(BYTE)) != sizeof(BYTE))
      {
         BASSERT(0);
         stop();
         return;
      }
      if (marker != cRecordMarkerCommands)
      {
         BASSERT(0);
         stop();
         return;
      }
   }
#endif
}

//==============================================================================
// BRecordGame::playSync
//==============================================================================
void BRecordGame::playSync()
{
   if (!mIsPlaying || !mIsSyncing)
      return;

   unsigned __int64 offset = mpSyncStream->curOfs();
   if (offset >= mCallTableOffset)
      return;

   BSyncHistory* pHistory = BSyncManager::getInstance()->getPlaybackHistory();
   BSyncUpdate* pUpdate = pHistory->getCurrentUpdate();

   BChunkReader reader(mpSyncStream, false);
   pUpdate->load(&reader);
}

//==============================================================================
// BRecordGame::toggleViewLock
//==============================================================================
void BRecordGame::toggleViewLock()
{
   mIsViewLocked=!mIsViewLocked;

   if(mIsViewLocked)
   {
      BUser *user = gUserManager.getUser(BUserManager::cPrimaryUser);
      if(user->getPlayerID()!=mCurrentPlayer)
         user->switchPlayer(mCurrentPlayer);

      BCamera* pCamera=user->getCamera();
      pCamera->setCameraLoc(mCameraPosition);
      pCamera->setCameraDir(mCameraForward);
      pCamera->setCameraRight(mCameraRight);

      BVector up;
      up.assignCrossProduct(mCameraForward, mCameraRight);
      up.normalize();
      pCamera->setCameraUp(up);

      user->setFlagUpdateHoverPoint(true);

      if(mFogOfWar)
         gConfig.remove(cConfigNoFogMask);
      else
         gConfig.define(cConfigNoFogMask);
   }
}

//==============================================================================
// BRecordGame::reset
//==============================================================================
void BRecordGame::reset()
{
   BGameFile::reset();
   if (mpSyncStream != NULL)
   {
      mpSyncStream->close();
      delete mpSyncStream;
   }
   mpSyncStream = NULL;

   mIsPlaying = false;
   mIsRecording = false;
   mIsSyncing = false;
}

//==============================================================================
// BRecordUIUnitSound
//==============================================================================
bool BRecordUIUnitSound::read(BStream& stream, DWORD version)
{
   bool success = (stream.readBytes(this, sizeof(BRecordUIUnitSound)) == sizeof(BRecordUIUnitSound));
   return success;
}

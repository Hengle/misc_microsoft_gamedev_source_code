//==============================================================================
// chatmanager.h
//
// chatmanager manages all user chats - it's a pretty thin class
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "chatmanager.h"
#include "soundmanager.h"
#include "player.h"
#include "world.h"
#include "game.h"
#include "database.h"
#include "user.h"
#include "usermanager.h"
#include "configsgame.h"
#include "generaleventmanager.h"
#include "gamefile.h"

GFIMPLEMENTVERSION(BChatManager, 2);

//==============================================================================
// BChatMessage::BChatMessage
//==============================================================================
BChatMessage::BChatMessage(long stringID, const BSimString& sound, bool bQueueSound, bool allPlayers, int speakerID, const BPlayerIDArray& recipients, float duration) : 
mStringID(stringID),
mbQueueSound(bQueueSound),
mbAllPlayers(allPlayers),
mSpeakerID(speakerID),
mRecipientIDs(recipients),
mTimeToDisplay(duration),
mbIsNew(true),
mCueIndex(cInvalidCueIndex),
mVideoHandle(cInvalidVideoHandle),
mbNeverExpire(false),
mbAutoExpire(false),
mbForceSubtitles(false)
{
   mSound = sound;

   if (sound.length() > 0)
      setCueIndex(gSoundManager.getCueIndex(sound.getPtr()));

   // look up the string index
   mStringIDIndex = gDatabase.getLocStringIndex(mStringID);

   if(duration == 0)
   {
      mbNeverExpire = true;
   }
}

//==============================================================================
// BChatMessage::BChatMessage
//==============================================================================
BChatMessage::BChatMessage() :
mStringID(-1),
mStringIDIndex(-1),
mbQueueSound(false),
mbAllPlayers(false),
mSpeakerID(-1),
mTimeToDisplay(0.0f),
mbIsNew(false),
mCueIndex(cInvalidCueIndex),
mVideoHandle(cInvalidVideoHandle),
mbNeverExpire(false),
mbAutoExpire(false),
mbForceSubtitles(false)
{
}

//==============================================================================
// BChatMessage::updateTime
//==============================================================================
void BChatMessage::updateTime(float elapsedTime)
{
   mTimeToDisplay-=elapsedTime;
}


//==============================================================================
// BChatMessage::getChatString
//==============================================================================
const BUString& BChatMessage::getChatString()
{
   return gDatabase.getLocStringFromIndex(mStringIDIndex);
}

//==============================================================================
//==============================================================================
bool BChatMessage::save(BStream* pStream, int saveType)
{
   GFWRITESTRING(pStream, BSimString, mSound, 100);
   GFWRITEARRAY(pStream, BPlayerID, mRecipientIDs, uint16, 1000);
   GFWRITEVAR(pStream, int, mSpeakerID);
   GFWRITEVAR(pStream, long, mStringID);
   GFWRITEVAR(pStream, float, mTimeToDisplay);
   //GFWRITEVAR(pStream, BCueIndex, mCueIndex);
   //GFWRITEVAR(pStream, long, mStringIDIndex);
   //GFWRITEVAR(pStream, BBinkVideoHandle, mVideoHandle);
   GFWRITEBITBOOL(pStream, mbIsNew);
   GFWRITEBITBOOL(pStream, mbQueueSound);
   GFWRITEBITBOOL(pStream, mbAllPlayers);
   GFWRITEBITBOOL(pStream, mbNeverExpire);
   GFWRITEBITBOOL(pStream, mbAutoExpire);
   GFWRITEBITBOOL(pStream, mbForceSubtitles);
   return true;
}

//==============================================================================
//==============================================================================
bool BChatMessage::load(BStream* pStream, int saveType)
{
   GFREADSTRING(pStream, BSimString, mSound, 100);
   GFREADARRAY(pStream, BPlayerID, mRecipientIDs, uint16, 1000);
   GFREADVAR(pStream, int, mSpeakerID);
   GFREADVAR(pStream, long, mStringID);
   GFREADVAR(pStream, float, mTimeToDisplay);
   //GFREADVAR(pStream, BCueIndex, mCueIndex);
   //GFREADVAR(pStream, long, mStringIDIndex);
   GFREADBITBOOL(pStream, mbIsNew);
   GFREADBITBOOL(pStream, mbQueueSound);
   GFREADBITBOOL(pStream, mbAllPlayers);
   GFREADBITBOOL(pStream, mbNeverExpire);
   GFREADBITBOOL(pStream, mbAutoExpire);
   GFREADBITBOOL(pStream, mbForceSubtitles);
   mStringIDIndex = gDatabase.getLocStringIndex(mStringID);
   return true;
}

//==============================================================================
// BChatManager::BChatManager()
//==============================================================================
BChatManager::BChatManager() :
   mChatsEnabled(true),
   mForceSubtitlesOn(false),
   mpChatCompleted(NULL)
{
}

//==============================================================================
// BChatManager::~BChatManager()
//==============================================================================
BChatManager::~BChatManager( void )
{
   // The event manager handles clean up
   mpChatCompleted = NULL;
}

//==============================================================================
// BChatManager::init
//==============================================================================
bool BChatManager::init( void )
{
   gBinkInterface.registerValidCallback(this);
   mChatMessageList.setNumber(0);
   mpChatCompleted = NULL;
   return( true );
}

//==============================================================================
// BChatManager::reset
//==============================================================================
void BChatManager::reset( void )
{
   gBinkInterface.unregisterValidCallback(this);

   for (int i=0; i<mChatMessageList.getNumber(); i++)
   {
      BChatMessage * pMessage = mChatMessageList[i];
      if (!pMessage)
         continue;

      delete pMessage;
      mChatMessageList[i] = NULL;
   }
   mChatMessageList.clear();

   mChatsEnabled = true;
   mForceSubtitlesOn = false;

   // The event manager handles clean up
   mpChatCompleted = NULL;
}

//==============================================================================
// BChatManager::processSoundStoppedEvent
//==============================================================================
void BChatManager::processSoundStoppedEvent(BSoundEventParams& event)
{
   BChatMessage* pChat = getChat();
   if (!pChat)
      return;

   if (event.in_eventID == pChat->getCueIndex())
   {
      // let the chat know that the sound has expired.
      pChat->setCueIndex(cInvalidCueIndex);

//       if (pChat->getVideoHandle() == cInvalidVideoHandle)
//       {
//          // if we have more than this in the queue, then expire this message too
//          //    so the next chat can start
//          if (pChat->getAutoExpire() || mChatMessageList.getNumber() > 1)
//          {
//             pChat->setNeverExpire(false);
//             pChat->expire();
//          }
// 
//          //gGeneralEventManager.eventTrigger(BEventDefinitions::cChatCompleted, cInvalidPlayerID);
//       }
   }
}

//==============================================================================
// BChatManager::addChat
//==============================================================================
BChatMessage* BChatManager::addChat(long stringID, const BSimString& sound, bool bQueueSound, bool allPlayers, int speakerID, const BPlayerIDArray& recipientIDs, float duration)
{
   // grab the chat that is currently at the head of the queue and expire it.
   BChatMessage * pCurrentChat = getChat();

   // If we are not currently playing a sound with this text, then let's expire it 
   //    so the next sound can go through.
   if (pCurrentChat && (pCurrentChat->getCueIndex() == cInvalidCueIndex) && (pCurrentChat->getVideoHandle() == cInvalidVideoHandle))
   {
      pCurrentChat->expire();
      removeChat(pCurrentChat);
   }

   // add a new message, IF the player is an intended recipient.
   if (allPlayers)
   {
      BChatMessage* pMessage = new BChatMessage(stringID, sound, bQueueSound, allPlayers, speakerID, recipientIDs, duration);
      pMessage->setForceSubtitles(mForceSubtitlesOn);
      mChatMessageList.add(pMessage);
      return pMessage;
   }
   else
   {
//-- FIXING PREFIX BUG ID 4045
      const BUser* pPrimaryUser = gUserManager.getPrimaryUser();
//--
      if (pPrimaryUser)
      {
         BPlayerID playerID = pPrimaryUser->getPlayerID();
         BPlayerID coopPlayerID = pPrimaryUser->getCoopPlayerID();
         if (recipientIDs.contains(playerID) || recipientIDs.contains(coopPlayerID))
         {
            BChatMessage* pMessage = new BChatMessage(stringID, sound, bQueueSound, allPlayers, speakerID, recipientIDs, duration);
            pMessage->setForceSubtitles(mForceSubtitlesOn);
            mChatMessageList.add(pMessage);
            return pMessage;
         }
      }
   }
   return NULL;
}

//==============================================================================
// BChatManager::removeChat
//==============================================================================
void BChatManager::removeChat( BChatMessage* chatMessage)
{
   // for right now, just remove from the front of the queue.
   int index = 0;

   // is the index
   if (index >= mChatMessageList.getNumber())
      return;

   BChatMessage* pMessage = mChatMessageList[index];
   delete pMessage;
   mChatMessageList[index]=NULL;
   mChatMessageList.removeIndex(index);
}

//==============================================================================
// BChatManager::getChat
//==============================================================================
BChatMessage* BChatManager::getChat()
{
   if (mChatMessageList.getNumber() == 0)
      return NULL;

   BChatMessage* pMessage = mChatMessageList[0];
   if (!pMessage)
      return NULL;

   // are we enabled? If not, expire the chat
/*
   if (!mChatsEnabled || gConfig.isDefined(cConfigDisableAllChats))
      pMessage->expire();
*/

   return pMessage;
}

//==============================================================================
//==============================================================================
BBinkVideoHandle BChatManager::getVideoHandle() const
{
   if (mChatMessageList.getNumber() == 0)
      return cInvalidVideoHandle;

   const BChatMessage* pMessage = mChatMessageList[0];
   if (!pMessage)
      return cInvalidVideoHandle;

   return pMessage->getVideoHandle();
}

//==============================================================================
//==============================================================================
void BChatManager::onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode)
{
   BChatMessage* pChat = getChat();
   if (pChat != NULL && pChat->getVideoHandle() == handle)
   {
      pChat->setVideoHandle(cInvalidVideoHandle);

      // If we had preloaded data, just kill it off here.  If for some reason we wanted the talking heads to
      // not unload after replaying, we'd pass this back to the talking head system where it could take
      // ownership of the data.
      if(preloadedData)
      {
         delete preloadedData;
         preloadedData = NULL;
      }

      // If talking head video was preloaded then unload it now
      gWorld->releaseTalkingHeadVideo(pChat->getSpeakerID());

//       if (pChat->getCueIndex()==cInvalidCueHandle)
//       {
//          // if we have more than this in the queue, then expire this message too
//          //    so the next chat can start
//          if (pChat->getAutoExpire() || mChatMessageList.getNumber() > 1)
//          {
//             pChat->setNeverExpire(false);
//             pChat->expire();
//          }
// 
//          //gGeneralEventManager.eventTrigger(BEventDefinitions::cChatCompleted, cInvalidPlayerID);
//       }
   }
}

//==============================================================================
//==============================================================================
bool BChatManager::save(BStream* pStream, int saveType)
{
   int16 msgCount = (int16)mChatMessageList.getNumber();
   GFWRITEVAR(pStream, int16, msgCount);
   GFVERIFYCOUNT(msgCount, 1000);
   for (int16 i=0; i<msgCount; i++)
   {
      BChatMessage* pMsg = mChatMessageList[i];
      if (pMsg)
      {
         GFWRITEVAR(pStream, int16, i);
         GFWRITECLASSPTR(pStream, saveType, pMsg);
      }
   }
   GFWRITEVAL(pStream, int16, -1);
   //BGeneralEventSubscriber*          mpChatCompleted;
   GFWRITEBITBOOL(pStream, mChatsEnabled);
   GFWRITEBITBOOL(pStream, mForceSubtitlesOn);
   return true;
}

//==============================================================================
//==============================================================================
bool BChatManager::load(BStream* pStream, int saveType)
{
   int16 msgCount;
   GFREADVAR(pStream, int16, msgCount);
   GFVERIFYCOUNT(msgCount, 1000);
   mChatMessageList.resize(msgCount);
   for (int16 i=0; i<msgCount; i++)
      mChatMessageList[i] = NULL;
   BChatMessage tempMsg;
   int16 loadedCount = 0;
   int16 msgIndex;
   GFREADVAR(pStream, int16, msgIndex);
   while (msgIndex != -1)
   {
      if (loadedCount >= msgCount)
      {
         GFERROR("GameFile Error: too many chats");
         return false;
      }
      BChatMessage* pMsg = new BChatMessage();
      mChatMessageList[msgIndex] = pMsg;
      if (pMsg)
         GFREADCLASSPTR(pStream, saveType, pMsg)
      else
         GFREADCLASS(pStream, saveType, tempMsg)
      loadedCount++;
      GFREADVAR(pStream, int16, msgIndex);
   }
   //BGeneralEventSubscriber*          mpChatCompleted;
   GFREADBITBOOL(pStream, mChatsEnabled);
   GFREADBITBOOL(pStream, mForceSubtitlesOn);
   return true;
}

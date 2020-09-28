//==============================================================================
// HintManager.cpp
//
// HintManager manages all user UI hints
//
// Copyright (c) 2007-2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "HintManager.h"
#include "player.h"
#include "world.h"
#include "game.h"
#include "database.h"
#include "user.h"
#include "usermanager.h"

// xvince
#include "vincehelper.h"

GFIMPLEMENTVERSION(BHintManager, 1);

//==============================================================================
// BHintMessage::BHintMessage
//==============================================================================
BHintMessage::BHintMessage(long stringID, bool allPlayers, const BPlayerIDArray& recipients, float duration) : 
mStringID(stringID),
mbAllPlayers(allPlayers),
mRecipientIDs(recipients),
mTimeToDisplay(duration),
mbIsNew(true),
mbNeverExpire(false)
{
   // look up the string index
   mStringIDIndex = gDatabase.getLocStringIndex(mStringID);

   if(duration == 0)
   {
      mbNeverExpire = true;
   }
}

//==============================================================================
// BHintMessage::updateTime
//==============================================================================
void BHintMessage::updateTime(float elapsedTime)
{
   mTimeToDisplay-=elapsedTime;
}


//==============================================================================
// BHintMessage::getHintString
//==============================================================================
const BUString& BHintMessage::getHintString()
{
   return gDatabase.getLocStringFromIndex(mStringIDIndex);
}


//==============================================================================
// BHintManager::BHintManager()
//==============================================================================
BHintManager::BHintManager()
{
}

//==============================================================================
// BHintManager::~BHintManager()
//==============================================================================
BHintManager::~BHintManager( void )
{
}

//==============================================================================
// BHintManager::init
//==============================================================================
bool BHintManager::init( void )
{
   mHintMessageList.setNumber(0);
   return( true );
}

//==============================================================================
// BHintManager::reset
//==============================================================================
void BHintManager::reset( void )
{
   for (int i=0; i<mHintMessageList.getNumber(); i++)
   {
      BHintMessage * pMessage = mHintMessageList[i];
      if (!pMessage)
         continue;

      delete pMessage;
      mHintMessageList[i] = NULL;
   }
   mHintMessageList.clear();
}

//==============================================================================
// BHintManager::addHint
//==============================================================================
void BHintManager::addHint(long stringID, bool allPlayers, const BPlayerIDArray& recipientIDs, float duration)
{
   // grab the hint that is currently at the head of the queue and expire it.
   BHintMessage * pCurrentHint = getHint();
   if(pCurrentHint != NULL)
   {
      // if we're trying to add the same string ID, just reset the duration
      if (pCurrentHint->getHintStringID() == stringID)
      {
         pCurrentHint->resetDisplayTime(duration);
         return;
      }

      pCurrentHint->expire();
      removeHint(pCurrentHint);
   }

   // add a new message, IF the player is an intended recipient.
   if (allPlayers)
   {
      BHintMessage* pMessage = new BHintMessage(stringID, allPlayers, recipientIDs, duration);
      mHintMessageList.add(pMessage);

      // record a vince event of the hint that fired
      MVinceEventAsync_HintFired(stringID, allPlayers, recipientIDs);
   }
   else
   {
//-- FIXING PREFIX BUG ID 2024
      const BUser* pPrimaryUser = gUserManager.getPrimaryUser();
//--
      if (pPrimaryUser)
      {
         BPlayerID playerID = pPrimaryUser->getPlayerID();
         //BPlayerID coopPlayerID = pPrimaryUser->getCoopPlayerID();
         if (recipientIDs.contains(playerID) /*|| recipientIDs.contains(coopPlayerID)*/)     // allow the designers to decide if this goes to the coop player by putting them in the list. 
         {
            BHintMessage* pMessage = new BHintMessage(stringID, allPlayers, recipientIDs, duration);
            mHintMessageList.add(pMessage);

            MVinceEventAsync_HintFired(stringID, allPlayers, recipientIDs);
         }
      }
   }
}

//==============================================================================
// BHintManager::removeHint
//==============================================================================
void BHintManager::removeHint( BHintMessage* hintMessage)
{
   // for right now, just remove from the front of the queue.
   int index = 0;

   // is the index
   if (index >= mHintMessageList.getNumber())
      return;

   BHintMessage* pMessage = mHintMessageList[index];
   delete pMessage;
   mHintMessageList[index]=NULL;
   mHintMessageList.removeIndex(index);
}

//==============================================================================
// BHintManager::getHint
//==============================================================================
BHintMessage* BHintManager::getHint()
{
   if (mHintMessageList.getNumber() == 0)
      return NULL;

   return mHintMessageList[0];

}

//==============================================================================
//==============================================================================
bool BHintManager::save(BStream* pStream, int saveType) const
{
   GFWRITECLASSPTRARRAY(pStream, saveType, BHintMessage, mHintMessageList, uint16, 2000);
   return true;
}

//==============================================================================
//==============================================================================
bool BHintManager::load(BStream* pStream, int saveType)
{
   GFREADCLASSPTRARRAY(pStream, saveType, BHintMessage, mHintMessageList, uint16, 2000);
   return true;
}

//==============================================================================
//==============================================================================
bool BHintMessage::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BPlayerID, mRecipientIDs, uint8, 16);
   GFWRITEVAR(pStream, long, mStringID);
   GFWRITEVAR(pStream, float, mTimeToDisplay);
   GFWRITEVAR(pStream, long, mStringIDIndex);
   GFWRITEBITBOOL(pStream, mbIsNew);
   GFWRITEBITBOOL(pStream, mbAllPlayers);
   GFWRITEBITBOOL(pStream, mbNeverExpire);
   return true;
}

//==============================================================================
//==============================================================================
bool BHintMessage::load(BStream* pStream, int saveType)
{
   GFREADARRAY(pStream, BPlayerID, mRecipientIDs, uint8, 16);
   GFREADVAR(pStream, long, mStringID);
   GFREADVAR(pStream, float, mTimeToDisplay);
   GFREADVAR(pStream, long, mStringIDIndex);
   GFREADBITBOOL(pStream, mbIsNew);
   GFREADBITBOOL(pStream, mbAllPlayers);
   GFREADBITBOOL(pStream, mbNeverExpire);
   return true;
}

//==============================================================================
// entityactionlisten.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "entityactionlisten.h"
#include "actionmanager.h"

IMPLEMENT_FREELIST(BEntityActionListen, 5, &gSimHeap);


//==============================================================================
//==============================================================================
bool BEntityActionListen::init()
{
	if (!BAction::init())
		return (false);

   mEventListeners.setNumber(0);
	return (true);
}

//==============================================================================
//==============================================================================
bool BEntityActionListen::update(float elapsed)
{
	if (mEventListeners.getNumber() <= 0)
		setState(BAction::cStateDone);

   return (true);
}

//==============================================================================
//==============================================================================
void BEntityActionListen::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
	uint numEventListeners=mEventListeners.getSize();
	for (uint i=0; i < numEventListeners; i++)
		mEventListeners[i]->notify(eventType, senderID, data, data2);
}

//==============================================================================
//==============================================================================
bool BEntityActionListen::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   return true;
}

//==============================================================================
//==============================================================================
bool BEntityActionListen::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   // ajl 10/23/08 - having lots of problems with saving/loading event the listener pointers,
   // so disabling this and having each listener re-add themselves at load time instead.
   if (BAction::mGameFileVersion < 32)
   {
      uint count;
      GFREADVAL(pStream, uint8, uint, count);
      GFVERIFYCOUNT(count, 50);
      for (uint i=0; i<count; i++)
      {
         IEventListener* pListener = NULL;
         if (!IEventListener::loadPtr(pStream, &pListener))
            return false;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BEntityActionListen::savePtr(BStream* pStream) const
{
   GFWRITEACTIONPTR(pStream, this);
   return true;
}

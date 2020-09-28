//==============================================================================
// entityactionlisten.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BEntityActionListen : public BAction, IEventListener
{
   public:   

	   BEntityActionListen() { }
	   virtual ~BEntityActionListen() { }

      //Init.
      virtual bool               init();

	   virtual bool               update(float elapsed);
	   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      void                       addEventListener(IEventListener* pListener) { mEventListeners.uniqueAdd(pListener); }
      void                       removeEventListener(IEventListener* pListener) { mEventListeners.removeValueAllInstances(pListener); }

      virtual int                getEventListenerType() const { return cEventListenerTypeAction; }
      virtual bool               savePtr(BStream* pStream) const;

      DECLARE_FREELIST(BEntityActionListen, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

	   BSmallDynamicSimArray<IEventListener*> mEventListeners;
};
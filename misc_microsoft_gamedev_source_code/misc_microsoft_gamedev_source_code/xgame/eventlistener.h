//==============================================================================
// eventlistener.h
//
// Interface that defines the notify method for passing simulation events
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#pragma once

#include "stream\stream.h"

enum
{
   cEventListenerTypeEntity,
   cEventListenerTypeSquadAI,
   cEventListenerTypeWorld,
   cEventListenerTypeAction,
   cEventListenerTypePower,
};

class IEventListener
{
public:
   virtual void notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)=0;
   virtual int getEventListenerType() const = 0;
   virtual bool savePtr(BStream* pStream) const = 0;

   static bool savePtr(BStream* pStream, const IEventListener* pListener);
   static bool loadPtr(BStream* pStream, IEventListener** ppListener);
};

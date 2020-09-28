//==============================================================================
// squadactioncloak.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Action.h"

class BGrannyInstance;
class BBitArray;

//==============================================================================
//==============================================================================
class BSquadActionCloak : public BAction
{
public:
   BSquadActionCloak() { }
   virtual ~BSquadActionCloak() { }

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);

   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

   static bool                unitMaskReset(BGrannyInstance* pGrannyInstance, const BBitArray& previousMask);

   DECLARE_FREELIST(BSquadActionCloak, 4);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   void                       cloak();
   void                       uncloak();

   void                       detect();
   void                       undetect();

   void                       updateVisuals(BSquad* pSquad, bool bEnemyOnly);

   void                       playCloakSound(bool cloak);

   DWORD                      mActivateDelay;
   DWORD                      mDetectedCountdown;
   BSmallDynamicSimArray<BEntityID> mFXAttachment;

   bool                       mFlagPermaCloaked:1;
};
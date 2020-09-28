//==============================================================================
// unitactionrevive.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

#define MAX_HEALTH_STATES 6

//==============================================================================
//==============================================================================
class BUnitActionRevive : public BAction
{
   public:

      BUnitActionRevive() { }
      virtual ~BUnitActionRevive() { }
      virtual bool connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void disconnect();
      virtual bool init();
      virtual bool setState(BActionState state);
      virtual bool update(float elapsed);
      virtual void notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);    

      void        setDamageDealt() { mDamageDealt = true; }

      DECLARE_FREELIST(BUnitActionRevive, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      void handleAnimationState();

      float       mReviveTimer;
      float       mHibernateTimer;

      long        mHealthState;
      long        mNextHealthState;
      long        mNumHealthStates;
      long        mHealthAnim[MAX_HEALTH_STATES];
      long        mShrinkAnim[MAX_HEALTH_STATES-1];
      long        mGrowAnim[MAX_HEALTH_STATES-1];
      int         mDormantCalloutID;

      bool        mDamageDealt:1;
      bool        mInTransition:1;
};
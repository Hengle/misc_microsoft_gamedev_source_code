//==============================================================================
// squadactionambientlife.h
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#pragma once
#include "Action.h"


//==============================================================================
//==============================================================================
class BSquadActionAmbientLife : public BAction
{

   typedef enum BAmbientLifeState
   {
      cStateWander,
      cStateHunt,
      cStateFlee,
      cStateIdle,
      cStateDevour
   };

public:
   BSquadActionAmbientLife() { }
   virtual ~BSquadActionAmbientLife() { }

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);

   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

   void                       fleeMap(BSquad* pEnemySquad);

   DECLARE_FREELIST(BSquadActionAmbientLife, 4);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   void                 wanderMove();
   void                 flee();
   void                 attack();
   void                 updateAmbientLifeState(float elapsed);
   void                 updateOpps(); 
   void                 setAmbientLifeState(BAmbientLifeState newState);

   BAmbientLifeState    mAmbientLifeState;
   float                mMaxWanderDistance;
   float                mMinWanderDistance;
   DWORD                mWanderTimer;  
   DWORD                mPredatorCheckTimer;
   DWORD                mDevourTimer;  // DMG NOTE: This should go away once there are actual animal corpses
   DWORD                mCurrentPreyTimer;
   BEntityID            mDangerousSquad;
   BEntityID            mPreySquad;
   BEntityID            mCurrentPreyUnit;
   BActionID            mChildActionID;
   BSimTarget           mTarget;
   bool                 mFlagFleeing:1;
   bool                 mFlagLeavingMap:1;
};
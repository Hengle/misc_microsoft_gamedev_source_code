//==============================================================================
// squadactionCryo.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Action.h"


//==============================================================================
//==============================================================================
class BSquadActionCryo : public BAction, IEventListener
{
public:
   BSquadActionCryo() { }
   virtual ~BSquadActionCryo() { }

   enum BCryoState
   {
      CryoStateNone,
      CryoStateFreezing, 
      CryoStatePendingFrozenKill,
      CryoStateFrozen,
      CryoStateThawing,

      CryoStateMax
   };

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);

   void                       addCryo(float cryoAmount);
   float                      getCryoPoints() const { return mCryoPoints; }
   void                       forceCryoFrozen() { setCryoState(CryoStateFrozen); }
   void                       cryoKillSquad(BPlayer *pPowerPlayer, BEntityID killerID);

   void                       updateFreezingThawTime(float newVal) { mFreezingThawTime = Math::Max(mFreezingThawTime, newVal); }
   void                       updateFrozenThawTime(float newVal) { mFrozenThawTime = Math::Max(mFrozenThawTime, newVal); }

   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

   virtual int                getEventListenerType() const { return cEventListenerTypeAction; }
   virtual bool               savePtr(BStream* pStream) const;

   DECLARE_FREELIST(BSquadActionCryo, 4);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);
   virtual bool postLoad(int saveType);

protected:

   void                       onUnitKilled(BEntityID unitId);
   void                       onUnitAdded(BEntityID unitId);

   void                       updateUnitCryoEffect(BEntityID unitId, float currentCryoPercent, float newCryoPercent, bool affectTexture);
   void                       updateCryoPoints(float newCryoPoints);
   void                       setCryoState(BCryoState state);

   void                       startFreezing();
   void                       endFreezing();

   void                       startFrozen();
   void                       endFrozen();

   void                       startThawing();
   void                       endThawing();

   void                       impulseAirUnits(BSquad& squad, float elapsedTime);

   BProtoObjectID             getSnowMoundProtoId(const BUnit& unit);

   float                      mCryoPoints;
   float                      mMaxCryoPoints;
   BCryoState                 mCryoState;
   float                      mTimeUntilThaw;

   float                      mFreezingThawTime;
   float                      mFrozenThawTime;

   BEntityIDArray             mSnowMounds; 
};
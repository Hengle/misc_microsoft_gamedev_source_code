//==============================================================================
// unitactionplasmashieldgen.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;
class BVisualRenderAttributes;
class BObject;
class BGrannyModel;

//==============================================================================
//==============================================================================
class BUnitActionPlasmaShieldGen : public BAction, IEventListener
{
   public:
      BUnitActionPlasmaShieldGen() { }
      virtual ~BUnitActionPlasmaShieldGen() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder *pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      bool                       isPrimary(void) const { return mShieldInfo.mFlagPrimary; }

      void                       addSecondaryShieldGen(void);
      void                       removeSecondaryShieldGen(void);

      virtual int                getEventListenerType() const { return cEventListenerTypeAction; }
      virtual bool               savePtr(BStream* pStream) const;

      // Render overrides for shield hits
      static void                renderStatic(BObject* pObject, BVisualRenderAttributes* pRenderAttributes, DWORD subUpdate, DWORD subUpdatesPerUpdate, float elapsedSubUpdateTime);

      DECLARE_FREELIST(BUnitActionPlasmaShieldGen, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);
      virtual bool postLoad(int saveType);

   protected:

      void                       attemptBringUp();

      void                       bringShieldUp();
      void                       takeShieldDown();
      void                       updateSockets();
      void                       updateSocket(BEntityID targetID);

      void                       addSubShield(BEntityID targetID);
      void                       removeSubShield(BEntityID targetID);
      void                       updateSubShield(BEntityID targetID);

      void                       updatePlayerVisibilityAndSelection(BEntityID shieldID, bool force = false);
      void                       updateCoveredObjectsVulnerability(bool hide);

      bool                       isShieldUp() const { return mShieldInfo.mFlagShieldUp; }

      // Primary/secondary shield methods
      BEntityID                  getPrimaryShieldGen();

      bool                       primaryUpdate(float elapsed);
      bool                       secondaryUpdate(float elapsed);

      void                       findNewPrimary();
      void                       updatePrimary();

      void                       upgradeShield();
      void                       downgradeShield();

      int                        findSquad(BEntityID squadID);

      typedef std::pair<BEntityID, BEntityID> BuildingShieldPair;
      typedef BSmallDynamicSimArray<BuildingShieldPair> BuildingToShieldCtnr;

      // This data is stored in the primary shield
      struct ShieldInfo
      {
         ShieldInfo() { reset(); }
         void reset();

         BProtoObjectID             mShieldProtoId;
         BEntityID                  mShieldID;
         uint                       mNumShields; // Used by the primary shield to keep track of the total number of shields currently in the base
         BuildingToShieldCtnr       mBldgToShieldMap;
         bool                       mFlagPrimary:1;
         bool                       mFlagShieldUp:1;
         bool                       mFlagRaising:1;
      };

      BEntityID                  mPrimary;
      long                       mCachedLocalPlayerId;
      float                      mAttackWaitTime;
      ShieldInfo                 mShieldInfo;
      long                       mShieldAnims[4];
      int                        mAnimIdx;

      static const float         mHitrate;
      static const DWORD         mHitTime;
      //static const DWORD         mHitBreak;
      static const float         mMaxHitIntensity;

      float                      mCurrentHitVal;
      DWORD                      mHitStartTime;
      bool                       mPlayingHit:1;
};

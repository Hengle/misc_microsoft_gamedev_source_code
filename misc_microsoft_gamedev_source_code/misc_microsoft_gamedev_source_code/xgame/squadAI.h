//==============================================================================
// squadai.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once 

#include "bitvector.h"
#include "poolable.h"
#include "opportunity.h"
#include "UnitOpportunity.h"
#include "gamefilemacros.h"

class BSquad;
class BUnit;

typedef std::pair<BEntityID,DWORD> BSquadAIIgnoreItem;
typedef BSmallDynamicSimArray<BSquadAIIgnoreItem> BSquadAIIgnoreList;

//==============================================================================
//==============================================================================
class BSquadAI : IEventListener
{
   public:
      // Enumerations added here need to be added to TriggerPropSquadMode and LogicNodeSquadModePage editor classes.
      enum
      {
         cModeNormal=0,
         cModeStandGround,
         cModeLockdown,
         cModeSniper,
         cModeHitAndRun,
         cModePassive,
         cModeCover,
         cModeAbility,
         cModeCarryingObject,
         cModePower,
         // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
         cModeScarabScan,
         cModeScarabTarget,
         cModeScarabKill,
         cNumberModes
      };
      enum { cIgnoreSquadTime = 5000, };

      BSquadAI();
      virtual ~BSquadAI();

      virtual void            init(BSquad* pSqaud);
      virtual void            notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) { }

      void                    update();
      void                    searchForWork();
      uint8                   getMode() const {return mMode;}
      void                    setMode(int mode);
      void                    setMode(uint8 mode);
      bool                    canMoveToAttack(bool allowAutoUnlock);
      bool                    isSquadIgnored(BEntityID squadID) const;
      void                    removeExpiredIgnores();
      void                    ignoreSquad(BEntityID squadID, DWORD ignoreDuration);
      bool                    doesTargetObeyLeash(BEntityID squadID) const;

      void                    setFlagCanWork(bool newVal) { mFlagCanWork = newVal; }
      void                    setFlagSearchToken(bool v) { mFlagSearchToken=v; }
      void                    setFlagWorkToken(bool v) { mFlagWorkToken=v; }

      BSquad*                 getOwner() const { return mpOwner; }

      virtual int             getEventListenerType() const { return cEventListenerTypeSquadAI; }
      virtual bool            savePtr(BStream* pStream) const;

      // Frees memory used by static squad AI data
      static void             deinit();

      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      static BOpportunityList mOpportunityList;
      BSquadAIIgnoreList      mIgnoreList;
      BSquad*                 mpOwner;
      BUnitOpp                mWorkOpp;
      uint8                   mMode;

      //-- Flags
      bool                    mFlagCanWork:1;
      bool                    mFlagSearchToken:1;
      bool                    mFlagWorkToken:1;
};

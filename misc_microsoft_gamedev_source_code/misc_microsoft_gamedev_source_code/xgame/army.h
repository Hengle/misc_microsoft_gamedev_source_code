//==============================================================================
// Army.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Entity.h"
#include "gamefilemacros.h"

class BCommand;
class BPlatoon;

// Debugging
#ifndef BUILD_FINAL
   extern BEntityID sDebugSquadTempID;
   void dbgArmyInternal(const char *format, ...);
   void dbgArmyInternalTempID(const char *format, ...);
   #define debugArmy dbgArmyInternal
#else
   #define debugArmy __noop
#endif

//==============================================================================
//==============================================================================
class BArmy : public BEntity
{
   public:
      BArmy() { }
      virtual ~BArmy() { mActions.clearActions(); }
      virtual void            onRelease();
      virtual void            init();

      //Update
      virtual bool            updatePreAsync(float elapsedTime);
      virtual bool            updateAsync(float elapsedTime);
      virtual bool            update(float elapsedTime);
      
      virtual void            kill(bool bKillImmediately);
      virtual void            destroy();

      //Children.
      long                    addChild(BPlatoon *platoon);
      bool                    removeChild(BPlatoon *platoon);
      uint                    getNumberChildren() const { return mChildren.getNumber(); }
      BEntityID               getChild(uint index) const;
      //Squads.
      bool                    addSquads(const BEntityIDArray &squads, bool groupBySpeed = true);
      // Platoons
      bool                    rePlatoon();
      bool                    checkAndReplatoon();

      //Orders.
      bool                    queueOrder(const BCommand* pCommand);
      void                    propagateAttackMove();

      // Playable Bounds
      virtual bool            isOutsidePlayableBounds(bool forceCheckWorldBoundaries=false) const;

      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      BEntityIDArray          mChildren;   // 8 bytes
};

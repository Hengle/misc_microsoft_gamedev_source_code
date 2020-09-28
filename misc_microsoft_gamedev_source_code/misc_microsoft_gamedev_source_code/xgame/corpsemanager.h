//============================================================================
// corpsemanager.h
//  
// Copyright (c) 2007 Ensemble Studios
//============================================================================
#pragma once

#include "gamefilemacros.h"

//============================================================================
// forward declarations
//============================================================================
class BUnit;

//============================================================================
// BCorpse
//============================================================================
class BCorpse
{
   public:

      BCorpse()      {}
      ~BCorpse()     {}

      void  init(BUnit* pUnit);

      void setFlagRemove()             { mFlagRemove = true; }
      void setTimeStamp();
      void setFlagIgnore()             { mFlagIgnore = true; }

      DWORD getTimeStamp() const       { return mTimeStamp; }
      BUnit* getUnit() const;
      BEntityID getUnitID() const      { return mUnitID; }
      bool getFlagIsInfantry() const   { return mFlagIsInfantry; }
      bool getFlagRemove() const       { return mFlagRemove; }
      bool getFlagIgnore() const       { return mFlagIgnore; }

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:

      DWORD       mTimeStamp;
      BEntityID   mUnitID;
      bool        mFlagIsInfantry:1;
      bool        mFlagRemove:1;
      bool        mFlagIgnore:1;
};

//============================================================================
// BCorpseManager
//============================================================================
class BCorpseManager
{
   public:

      BCorpseManager()     { reset(); }
      ~BCorpseManager()    {}

      void reset();

      void update();

      void registerCorpse(BEntityID unitID);
      void removeCorpse(BEntityID unitID, bool doEffect = true);

      uint getNumCorpses() { return (mCorpses.getSize()); }
      BEntityID getCorpseUnit(long corpseIndex);

      float getCorpseDecay(BEntityID id);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:

      void removeCorpse(long corpseIndex, bool doEffect);
      void queueRemoveCorpse(long corpseIndex);
      void queueRemoveOldestCorpse();
      long findOldestCorpse(bool queued) const;
      long findCorpse(BEntityID unitID) const;
      bool canRemoveNow() const;

      typedef BHashMap<BEntityID, uint> EntityToCorpseCtnr;

      BFreeList<BCorpse, 4>            mCorpses;
      EntityToCorpseCtnr               mEntityToCorpse;
      long                             mNumQueuedRemovals;
      long                             mRemoveCount;
      DWORD                            mTimeOfLastRemoval;

};

// gCorpseManager
extern BCorpseManager gCorpseManager;

//==============================================================================
// SimTarget.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#include "gamefilemacros.h"

//==============================================================================
//==============================================================================
class BSimTarget
{
   public:
      BSimTarget()  { reset(); }
      BSimTarget(BEntityID id)  { reset(); setID(id); }
      BSimTarget(BVector position)  { reset(); setPosition(position); }
      BSimTarget(BVector position, float range)  { reset(); setPosition(position); setRange(range); }
      BSimTarget(BEntityID id, BVector position)  { reset(); setID(id); setPosition(position); }
      BSimTarget(BEntityID id, BVector position, long abilityID)  { reset(); setID(id); setPosition(position); setAbilityID(abilityID); }
      ~BSimTarget() { }

      BEntityID                  getID() const { return (mID); }
      void                       setID(BEntityID v) { mID=v; }
      bool                       isIDValid() const { return (mID.isValid()); }
      void                       invalidateID() { mID.invalidate(); }

      BVector                    getPosition() const { return (mPosition); }
      void                       setPosition(BVector v) { mPosition=v; if ((mPosition.x > -cFloatCompareEpsilon) && (mPosition.z > -cFloatCompareEpsilon)) mPositionValid=true; else mPositionValid=false; }
      bool                       isPositionValid() const { return (mPositionValid); }
      void                       invalidatePosition() { mPosition=cInvalidVector; mPositionValid=false; }

      float                      getRange() const { return (mRange); }
      void                       setRange(float v) { mRange=v; mRangeValid=true; }
      bool                       isRangeValid() const { return (mRangeValid); }
      void                       invalidateRange() { mRange=0.0f; mRangeValid=false; }

      long                       getAbilityID() const { return (mAbilityID); }
      void                       setAbilityID(long v) { mAbilityID=v; mAbilityIDValid=true; }
      bool                       isAbilityIDValid() const { return (mAbilityIDValid); }
      void                       invalidateAbilityID() { mAbilityID=-1; mAbilityIDValid=false; }
      
      bool                       isValid() const { if (getID().isValid() || isPositionValid()) return (true); return (false); }
      void                       set(BEntityID id, BVector position) { setID(id); setPosition(position); }
      void                       reset() { mID=cInvalidObjectID; mPosition=cInvalidVector; mRange=0.0f; mAbilityID=-1; mPositionValid=false; mRangeValid=false; mAbilityIDValid=false; }
      BSimTarget&                operator=(const BSimTarget& t)
                                 {
                                    mPosition=t.mPosition;
                                    mID=t.mID;
                                    mRange=t.mRange;
                                    mAbilityID=t.mAbilityID;
                                    mPositionValid=t.mPositionValid;
                                    mRangeValid=t.mRangeValid;
                                    mAbilityIDValid=t.mAbilityIDValid;
                                    return(*this);
                                 }
      bool                       operator==(const BSimTarget& t) const
                                 {
                                    if ((mID == t.mID) &&
                                       (mPosition == t.mPosition) &&
                                       (mRange == t.mRange) &&
                                       (mAbilityID == t.mAbilityID))
                                       return(true);
                                    return(false);
                                 }
      bool                       operator!=(const BSimTarget& t) const
                                 {
                                    return (!(*this == t));
                                 }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      BVector                    mPosition;
      BEntityID                  mID;
      float                      mRange;
      long                       mAbilityID;
      
      bool                       mPositionValid:1;
      bool                       mRangeValid:1;
      bool                       mAbilityIDValid:1;
};

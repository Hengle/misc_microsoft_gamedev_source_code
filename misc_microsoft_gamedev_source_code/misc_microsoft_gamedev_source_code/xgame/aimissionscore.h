//==============================================================================
// aimissionscore.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

#include "gamefilemacros.h"

//==============================================================================
// class BAIMissionScore
//==============================================================================
class BAIMissionScore
{
public:

   // Constructor / Destructor
   BAIMissionScore() { zero(); }

   void zero()
   {
      setTotal(0.0f);
      setInstance(0.0f);
      setClass(0.0f);
      setAfford(0.0f);
      setPermission(0.0f);
   }

   // Gets
   float getTotal() const { return (mTotal); }
   float getInstance() const { return (mInstance); }
   float getClass() const { return (mClass); }
   float getAfford() const { return (mAfford); }
   float getPermission() const { return (mPermission); }

   // Sets
   void setTotal(float total) { mTotal = total; }
   void setInstance(float instance) { mInstance = instance; }
   void setClass(float newClass) { mClass = newClass; }
   void setAfford(float afford) { mAfford = afford; }
   void setPermission(float permission) { mPermission = permission; }

   void calculateTotal() { mTotal = (mInstance * mClass * mAfford) + mPermission; }

   float applyBias(float numberToModify, float min, float max, float bias);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   float mTotal;        // Total score, permission + instance*class*afford
   float mInstance;     // 0..1, how good of a target is this particular target?
   float mClass;        // 0..1, how much are we inclined to act on this class of target?
   float mAfford;       // 0..1, how appropriate is this target relative to available military, resources
   float mPermission;   // Normally 0.0.  1.0 = special request from trigger, trumps all.  2.0 = ally request, uber alles.
};
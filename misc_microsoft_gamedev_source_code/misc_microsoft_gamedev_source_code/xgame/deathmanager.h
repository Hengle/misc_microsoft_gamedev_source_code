//============================================================================
// deathmanager.h
//  
// Copyright (c) 2008 Ensemble Studios
//============================================================================
#pragma once

//============================================================================
// Includes
//============================================================================
#include "SimTypes.h"



//============================================================================
// BDeathSpecial
//============================================================================
class BDeathSpecial
{
   public:
      enum
      {
         cDeathSpecialRocket,

         cNumDeathSpecials,
      };

      BDeathSpecial();

      long           mType;
      float          mCurrentChance;
      float          mMaxChance;
      float          mMinChance;
      float          mChanceGrowth;

      void resetChance();
      void increaseChance();
};

//============================================================================
// BDeathManager
//============================================================================

class BDeathManager
{
   public:
      BDeathManager();

      bool                    checkDeathChanceSpecial(long specialType);

      BDeathSpecial*          getSpecialChanceInfo(long specialType);

      void                    init();

      void                    reset();

   protected:
      BSmallDynamicSimArray<BDeathSpecial>      mDeathSpecials;   
};


extern BDeathManager gDeathManager;
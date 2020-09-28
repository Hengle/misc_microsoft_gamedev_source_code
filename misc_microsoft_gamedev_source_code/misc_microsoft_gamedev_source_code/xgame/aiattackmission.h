//==============================================================================
// aiattackmission.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aitypes.h"


//==============================================================================
//==============================================================================
class BAIAttackMission : public BAIMission
{
public:
   BAIAttackMission()
   {
      mType = MissionType::cAttack;
   };
   ~BAIAttackMission()
   {
   }

   virtual void resetNonIDData();

protected:
};
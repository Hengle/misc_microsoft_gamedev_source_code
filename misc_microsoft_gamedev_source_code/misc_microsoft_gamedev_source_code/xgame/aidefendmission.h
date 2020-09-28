//==============================================================================
// aidefendmission.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aitypes.h"


//==============================================================================
//==============================================================================
class BAIDefendMission : public BAIMission
{
public:
   BAIDefendMission()
   {
      mType = MissionType::cDefend;
   };
   ~BAIDefendMission()
   {
   }

   virtual void resetNonIDData();

protected:
};
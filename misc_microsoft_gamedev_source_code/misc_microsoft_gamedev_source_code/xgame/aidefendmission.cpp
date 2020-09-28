//==============================================================================
// aidefendmission.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "aidefendmission.h"
#include "army.h"
#include "commands.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "EntityGrouper.h"
#include "kb.h"
#include "kbsquad.h"
#include "player.h"
#include "protoobject.h"
#include "simhelper.h"
#include "squad.h"
#include "world.h"


//==============================================================================
//==============================================================================
void BAIDefendMission::resetNonIDData()
{
   // Do the base reset.
   BAIMission::resetNonIDData();

   // Defend mission specific reset.
   mType = MissionType::cDefend;
}
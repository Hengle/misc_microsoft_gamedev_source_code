//==============================================================================
// aiattackmission.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aiattackmission.h"
#include "aidebug.h"
#include "army.h"
#include "commands.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "EntityGrouper.h"
#include "kb.h"
#include "kbbase.h"
#include "kbsquad.h"
#include "player.h"
#include "protoobject.h"
#include "simhelper.h"
#include "squad.h"
#include "world.h"


//==============================================================================
//==============================================================================
void BAIAttackMission::resetNonIDData()
{
   // Do the base reset.
   BAIMission::resetNonIDData();

   // Attack mission specific reset.
   mType = MissionType::cAttack;
}
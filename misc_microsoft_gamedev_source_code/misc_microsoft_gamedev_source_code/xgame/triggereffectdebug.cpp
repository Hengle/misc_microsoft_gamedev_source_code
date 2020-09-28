//==============================================================================
// triggereffectdebug.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "simtypes.h"
#include "triggereffect.h"
#include "triggermanager.h"


//==============================================================================
// BTriggerEffect::teDebugVarTech()
//==============================================================================
void BTriggerEffect::teDebugVarTech()
{
#ifndef BUILD_FINAL
   enum { cTech = 1, };
   getVar(cTech)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarTechStatus()
//==============================================================================
void BTriggerEffect::teDebugVarTechStatus()
{
#ifndef BUILD_FINAL
   enum { cTechStatus = 1, };
   getVar(cTechStatus)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarOperator()
//==============================================================================
void BTriggerEffect::teDebugVarOperator()
{
#ifndef BUILD_FINAL
   enum { cOperator = 1, };
   getVar(cOperator)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarProtoObject()
//==============================================================================
void BTriggerEffect::teDebugVarProtoObject()
{
#ifndef BUILD_FINAL
   enum { cProtoObject = 1, };
   getVar(cProtoObject)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarObjectType()
//==============================================================================
void BTriggerEffect::teDebugVarObjectType()
{
#ifndef BUILD_FINAL
   enum { cObjectType = 1, };
   getVar(cObjectType)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarProtoSquad()
//==============================================================================
void BTriggerEffect::teDebugVarProtoSquad()
{
#ifndef BUILD_FINAL
   enum { cProtoSquad = 1, };
   getVar(cProtoSquad)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarSound()
//==============================================================================
void BTriggerEffect::teDebugVarSound()
{
#ifndef BUILD_FINAL
   enum { cSound = 1, };
   getVar(cSound)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarDistance()
//==============================================================================
void BTriggerEffect::teDebugVarDistance()
{
#ifndef BUILD_FINAL
   enum { cDistance = 1, };
   getVar(cDistance)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarTime()
//==============================================================================
void BTriggerEffect::teDebugVarTime()
{
#ifndef BUILD_FINAL
   enum { cTime = 1, };
   getVar(cTime)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarPlayer()
//==============================================================================
void BTriggerEffect::teDebugVarPlayer()
{
#ifndef BUILD_FINAL
   enum { cPlayer = 1, };
   getVar(cPlayer)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarCount()
//==============================================================================
void BTriggerEffect::teDebugVarCount()
{
#ifndef BUILD_FINAL
   enum { cCount = 1, };
   getVar(cCount)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarLocation()
//==============================================================================
void BTriggerEffect::teDebugVarLocation()
{
#ifndef BUILD_FINAL
   enum { cLocation = 1, };
   getVar(cLocation)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarUILocation()
//==============================================================================
void BTriggerEffect::teDebugVarUILocation()
{
#ifndef BUILD_FINAL
   enum { cUILocation = 1, };
   getVar(cUILocation)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarCost()
//==============================================================================
void BTriggerEffect::teDebugVarCost()
{
#ifndef BUILD_FINAL
   enum { cCost = 1, };
   getVar(cCost)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarAnimType()
//==============================================================================
void BTriggerEffect::teDebugVarAnimType()
{
#ifndef BUILD_FINAL
   enum { cAnimType = 1, };
   getVar(cAnimType)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarPercent()
//==============================================================================
void BTriggerEffect::teDebugVarPercent()
{
#ifndef BUILD_FINAL
   enum { cPercent = 1, };
   getVar(cPercent)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarHitpoints()
//==============================================================================
void BTriggerEffect::teDebugVarHitpoints()
{
#ifndef BUILD_FINAL
   enum { cHitpoints = 1, };
   getVar(cHitpoints)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarPower()
//==============================================================================
void BTriggerEffect::teDebugVarPower()
{
#ifndef BUILD_FINAL
   enum { cPower = 1, };
   getVar(cPower)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarBool()
//==============================================================================
void BTriggerEffect::teDebugVarBool()
{
#ifndef BUILD_FINAL
   enum { cBool = 1, };
   getVar(cBool)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarFloat()
//==============================================================================
void BTriggerEffect::teDebugVarFloat()
{
#ifndef BUILD_FINAL
   enum { cFloat = 1, };
   getVar(cFloat)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarIterator()
//==============================================================================
void BTriggerEffect::teDebugVarIterator()
{
#ifndef BUILD_FINAL
   enum { cIterator = 1, };
   getVar(cIterator)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarTeam()
//==============================================================================
void BTriggerEffect::teDebugVarTeam()
{
#ifndef BUILD_FINAL
   enum { cTeam = 1, };
   getVar(cTeam)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarPlayerList()
//==============================================================================
void BTriggerEffect::teDebugVarPlayerList()
{
#ifndef BUILD_FINAL
   enum { cPlayerList = 1, };
   getVar(cPlayerList)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarTeamList()
//==============================================================================
void BTriggerEffect::teDebugVarTeamList()
{
#ifndef BUILD_FINAL
   enum { cTeamList = 1, };
   getVar(cTeamList)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarPlayerState()
//==============================================================================
void BTriggerEffect::teDebugVarPlayerState()
{
#ifndef BUILD_FINAL
   enum { cPlayerState = 1, };
   getVar(cPlayerState)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarObjective()
//==============================================================================
void BTriggerEffect::teDebugVarObjective()
{
#ifndef BUILD_FINAL
   enum { cObjective = 1, };
   getVar(cObjective)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarUnit()
//==============================================================================
void BTriggerEffect::teDebugVarUnit()
{
#ifndef BUILD_FINAL
   enum { cUnit = 1, };
   getVar(cUnit)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarUnitList()
//==============================================================================
void BTriggerEffect::teDebugVarUnitList()
{
#ifndef BUILD_FINAL
   enum { cUnitList = 1, };
   getVar(cUnitList)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarSquad()
//==============================================================================
void BTriggerEffect::teDebugVarSquad()
{
#ifndef BUILD_FINAL
   enum { cSquad = 1, };
   getVar(cSquad)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarUIUnit()
//==============================================================================
void BTriggerEffect::teDebugVarUIUnit()
{
#ifndef BUILD_FINAL
   enum { cUIUnit = 1, };
   getVar(cUIUnit)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarUISquad()
//==============================================================================
void BTriggerEffect::teDebugVarUISquad()
{
#ifndef BUILD_FINAL
   enum { cSquad = 1, };
   getVar(cSquad)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarString()
//==============================================================================
void BTriggerEffect::teDebugVarString()
{
#ifndef BUILD_FINAL
   enum { cString = 1, };
   getVar(cString)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarColor()
//==============================================================================
void BTriggerEffect::teDebugVarColor()
{
#ifndef BUILD_FINAL
   enum { cColor = 1, };
   getVar(cColor)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarProtoObjectList()
//==============================================================================
void BTriggerEffect::teDebugVarProtoObjectList()
{
#ifndef BUILD_FINAL
   enum { cProtoObjectList = 1, };
   getVar(cProtoObjectList)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarObjectTypeList()
//==============================================================================
void BTriggerEffect::teDebugVarObjectTypeList()
{
#ifndef BUILD_FINAL
   enum { cObjectTypeList = 1, };
   getVar(cObjectTypeList)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarProtoSquadList()
//==============================================================================
void BTriggerEffect::teDebugVarProtoSquadList()
{
#ifndef BUILD_FINAL
   enum { cProtoSquadList = 1, };
   getVar(cProtoSquadList)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarTechList()
//==============================================================================
void BTriggerEffect::teDebugVarTechList()
{
#ifndef BUILD_FINAL
   enum { cTechList = 1, };
   getVar(cTechList)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarMathOperator()
//==============================================================================
void BTriggerEffect::teDebugVarMathOperator()
{
#ifndef BUILD_FINAL
   enum { cMathOperator = 1, };
   getVar(cMathOperator)->outputDebugDataToXFS();
#endif
}

//==============================================================================
// Outputs the direction vector value for debug
//==============================================================================
void BTriggerEffect::teDebugDirection()
{
#ifndef BUILD_FINAL
   enum 
   { 
      cDirection = 1, 
   };

   getVar(cDirection)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarUIButton()
//==============================================================================
void BTriggerEffect::teDebugVarUIButton()
{
#ifndef BUILD_FINAL
   enum { cUIButton = 1, };
   getVar(cUIButton)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// BTriggerEffect::teDebugVarBuildingCommandState()
//==============================================================================
void BTriggerEffect::teDebugVarBuildingCommandState()
{
#ifndef BUILD_FINAL
   enum { cBuildingCommandState = 1, };
   getVar(cBuildingCommandState)->outputDebugDataToXFS();
#endif
}


//==============================================================================
// Debug output for squad list
//==============================================================================
void BTriggerEffect::teDebugVarSquadList()
{
#ifndef BUILD_FINAL
   enum 
   { 
      cSquadList = 1, 
   };

   getVar(cSquadList)->outputDebugDataToXFS();
#endif
}

//==============================================================================
// Output value of design line
//==============================================================================
void BTriggerEffect::teDebugVarDesignLine()
{
#ifndef BUILD_FINAL
   enum 
   { 
      cInDesignLine = 1, 
   };

   getVar(cInDesignLine)->outputDebugDataToXFS();
#endif
}

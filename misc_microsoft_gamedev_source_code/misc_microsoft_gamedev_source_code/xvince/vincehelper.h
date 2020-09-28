//==============================================================================
// vincehelper.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#ifndef _VINCEHELPER_H_
#define _VINCEHELPER_H_

// Macro out EVERYTHING if _VINCE_ is not defined
#ifndef _VINCE_

// Includes
#include "common.h"

void NonVince_CreateGameID(BSimString& gameIDString);

#define MVinceInitialize( pDevice, enableLog );
#define MVince_OpenNewLog();
#define MVince_CloseAndSendLog();
#define MVince_IsLogUploading() { return false; }
#define MVince_CreateGameID(gameIDString) NonVince_CreateGameID(gameIDString)

#define MVinceEventSync_GenericMessage(msg);
#define MVinceEventSync_PlayerJoinedMatch(player, pos, difficulty);
#define MVinceEventSync_MatchEnded(winningTeamID);
#define MVinceEventSync_EntityBuilt(unit);
#define MVinceEventSync_UnitKilled(unit, attackerID);
#define MVinceEventSync_TechResearched(techID, unitID); 
#define MVinceEventSync_ControlUsed( pUser, controlName );
#define MVinceEventSync_UsedLeaderPower( pProtoPower, playerID );
#define MVinceEventSync_UnitInterval();
#define MVinceEventSync_BuildingInterval();
#define MVinceEventSync_ResourceInterval();
#define MVinceEventSync_PathingInterval();
#define MVinceEventSync_ChatFired(pUser, chatID);
#define MVinceEventSync_WowFired(pUser);
#define MVinceEventSync_BattleStarted();

#define MVinceEventAsync_GenericMessage(msg);
#define MVinceEventAsync_PreGameEvent(eventType);
#define MVinceEventAsync_PreGameSettings(eventType, mapIndex, matchmakingHopper, pLocalMember);
#define MVinceEventAsync_PlayerJoinedMatch( player, posList, playerPosIndex );
#define MVinceEventAsync_MatchEnded(winningTeamID);
#define MVinceEventAsync_EntityBuilt(unit);
#define MVinceEventAsync_UnitKilled(unit, attackerID);
#define MVinceEventAsync_TechResearched(techID, unitID);
#define MVinceEventAsync_ControlUsed( pUser, controlName );
#define MVinceEventAsync_AbilityUsed( pUser, abilityID );
#define MVinceEventAsync_UsedLeaderPower( pProtoPower, playerID );
#define MVinceEventAsync_UnitInterval();
#define MVinceEventAsync_BuildingInterval();
#define MVinceEventAsync_ResourceInterval();
#define MVinceEventAsync_PathingInterval();
#define MVinceEventAsync_ChatFired(pUser, chatID);
#define MVinceEventAsync_WowFired(pUser);
#define MVinceEventAsync_BattleStarted();
#define MVinceEventAsync_HintFired(stringID, allPlayers, recipientIDs);

#endif // _VINCE_

// ***** *!*!*!* MACRO DEFINITIONS - IMPORTANT NOTES BELOW *!*!*!* *****
//
// ***** ABOUT FORSYNC vs NOTSYNC *****
// In a multiplayer game situation, Vince is running on all boxes, but because every box runs
// the whole game sim, we would end up with redundant data because every box would log everything.
// So in MOST cases, we need to check to see if we're the host and only log data if we are.
// Thus, every Vince event needs a macro called MFORSYNCVinceEvent_<yourevent>(). This Macro MUST
// be encapsulated between the HOST_CHECK_OPEN and HOST_CHECK_CLOSE macros.
//
// However, it's possible that in some cases we would want to log a piece of data on every box, or
// that a given event has usefulness in both the "only the host logs this" and "everyone logs this"
// context. Thus, every Vince event also needs a macro called MNOTSYNCVinceEvent_<yourevent>(). This
// version is NOT encapsulated in the host check macros.
//
// ***** ABOUT SEMICOLONS *****
// Why do some of these macros have semicolons and others don't? It's because of the host-checking
// (see above). When you do a check to see if this machine is the host, that's encapsulated in an
// if-block. Thus the actual function call needs a semicolon after it. However, when writing out the
// macro in the game's code files, we want to be able to put a semicolon after the macro so that it
// looks clean and doesn't confuse VisualStudio's text editor. SO: the MFORSYNC(etc) macros need
// a semicolon at the end. The others don't.

#ifdef _VINCE_

#define HOST_CHECK_OPEN                         if (gModeManager.getModeGame()->getIsHost()) {
#define HOST_CHECK_CLOSE                        }
#define MVinceInitialize( pDevice, enableLog )  XVinceHelper::Vince_Initialize( pDevice, enableLog )
#define MVince_OpenNewLog()                     XVinceHelper::Vince_OpenNewLog()
#define MVince_CloseAndSendLog()                XVinceHelper::Vince_CloseAndSendLog()
#define MVince_IsLogUploading()                 XVinceHelper::IsLogUploading()
#define MVince_CreateGameID(gameIDString)       XVinceHelper::Vince_CreateGameID(gameIDString)

// Macros used for making only the host log data
#define MVinceEventSync_GenericMessage(msg);									         HOST_CHECK_OPEN XVinceHelper::VinceEvent_GenericMessage(msg); HOST_CHECK_CLOSE
#define MVinceEventSync_PlayerJoinedMatch(player, pos, difficulty);	         HOST_CHECK_OPEN XVinceHelper::VinceEvent_PlayerJoinedMatch(player, pos, difficulty); HOST_CHECK_CLOSE
#define MVinceEventSync_MatchEnded(winningTeamID);							         HOST_CHECK_OPEN XVinceHelper::VinceEvent_MatchEnded(winningTeamID); HOST_CHECK_CLOSE
#define MVinceEventSync_EntityBuilt(unit);									         HOST_CHECK_OPEN XVinceHelper::VinceEvent_EntityBuilt(unit); HOST_CHECK_CLOSE
#define MVinceEventSync_UnitKilled(unit, attackerID);						         HOST_CHECK_OPEN XVinceHelper::VinceEvent_UnitKilled(unit, attackerID); HOST_CHECK_CLOSE
#define MVinceEventSync_TechResearched(techID, unitID);                       HOST_CHECK_OPEN XVinceHelper::VinceEvent_TechResearched(techID, unitID); HOST_CHECK_CLOSE
#define MVinceEventSync_ControlUsed( pUser, controlName );                    HOST_CHECK_OPEN XVinceHelper::VinceEvent_ControlUsed( pUser, controlName ); HOST_CHECK_CLOSE
#define MVinceEventSync_UsedLeaderPower( pProtoPower, playerID );             HOST_CHECK_OPEN XVinceHelper::VinceEvent_UsedLeaderPower( pProtoPower, playerID ); HOST_CHECK_CLOSE
#define MVinceEventSync_UnitInterval();                                       HOST_CHECK_OPEN XVinceHelper::VinceEvent_UnitInterval(); HOST_CHECK_CLOSE
#define MVinceEventSync_BuildingInterval();                                   HOST_CHECK_OPEN XVinceHelper::VinceEvent_BuildingInterval(); HOST_CHECK_CLOSE
#define MVinceEventSync_ResourceInterval();                                   HOST_CHECK_OPEN XVinceHelper::VinceEvent_ResourceInterval(); HOST_CHECK_CLOSE
#define MVinceEventSync_PathingInterval();                                    HOST_CHECK_OPEN XVinceHelper::VinceEvent_PathingInterval(); HOST_CHECK_CLOSE
#define MVinceEventSync_ChatFired(pUser, chatID);                             HOST_CHECK_OPEN XVinceHelper::VinceEvent_ChatFired(pUser, chatID); HOST_CHECK_CLOSE
#define MVinceEventSync_WowFired(pUser);                                      HOST_CHECK_OPEN XVinceHelper::VinceEvent_WowFired(pUser); HOST_CHECK_CLOSE
#define MVinceEventSync_BattleStarted();                                      HOST_CHECK_OPEN XVinceHelper::VinceEvent_BattleStarted(); HOST_CHECK_CLOSE

// Macros used for making all players log data (synchronous or otherwise)
#define MVinceEventAsync_GenericMessage(msg);								         XVinceHelper::VinceEvent_GenericMessage(msg)
#define MVinceEventAsync_PreGameEvent(eventType);									   XVinceHelper::VinceEvent_PreGameEvent(eventType)
#define MVinceEventAsync_PreGameSettings(eventType, mapIndex, matchmakingHopper, pLocalMember);	XVinceHelper::VinceEvent_PreGameSettings(eventType, mapIndex, matchmakingHopper, pLocalMember)
#define MVinceEventAsync_PlayerJoinedMatch(player, pos);	                     XVinceHelper::VinceEvent_MatchStarted()
#define MVinceEventAsync_MatchEnded(winningTeamID);					      	   XVinceHelper::VinceEvent_MatchEnded(winningTeamID)
#define MVinceEventAsync_EntityBuilt(unit);									         XVinceHelper::VinceEvent_EntityBuilt(unit)
#define MVinceEventAsync_UnitKilled(unit, attackerID);						      XVinceHelper::VinceEvent_UnitKilled(unit, attackerID)
#define MVinceEventAsync_TechResearched(techID, unitID);                      XVinceHelper::VinceEvent_TechResearched(techID, unitID)
#define MVinceEventAsync_ControlUsed( pUser, controlName );                   XVinceHelper::VinceEvent_ControlUsed( pUser, controlName )
#define MVinceEventAsync_AbilityUsed( pUser, abilityID );                     XVinceHelper::VinceEvent_AbilityUsed( pUser, abilityID )
#define MVinceEventAsync_UsedLeaderPower( pProtoPower, playerID );            XVinceHelper::VinceEvent_UsedLeaderPower( pProtoPower, playerID )
#define MVinceEventAsync_UnitInterval();                                      XVinceHelper::VinceEvent_UnitInterval()
#define MVinceEventAsync_BuildingInterval();                                  XVinceHelper::VinceEvent_BuildingInterval()
#define MVinceEventAsync_ResourceInterval();                                  XVinceHelper::VinceEvent_ResourceInterval()
#define MVinceEventAsync_PathingInterval();                                   XVinceHelper::VinceEvent_PathingInterval()
#define MVinceEventAsync_ChatFired(pUser, chatID);                            XVinceHelper::VinceEvent_ChatFired(pUser, chatID)
#define MVinceEventAsync_WowFired(pUser);                                     XVinceHelper::VinceEvent_WowFired(pUser)
#define MVinceEventAsync_BattleStarted();                                     XVinceHelper::VinceEvent_BattleStarted()
#define MVinceEventAsync_HintFired(stringID, allPlayers, recipientIDs);       XVinceHelper::VinceEvent_HintFired(stringID, allPlayers, recipientIDs)

#endif // _VINCE_

// Macro out/#undef *some* things if BUILD_DEBUG is defined
#ifdef BUILD_DEBUG
	// well, I lied... right now there's nothing... might be in the future, though
#endif // _VINCE_

#ifdef _VINCE_

// Includes
#include "common.h"
#include "entityID.h"
#include "scenario.h"
#include "modemanager.h"
#include "modegame.h"

#define NUM_RESOURCES 4
#define NUM_PLAYERS   8

// Externs
extern BModeManager gModeManager;

// forward declarations
class BUser;
class BPlayer;
class BObject;
class BUnit;
class BEntity;
class BProtoPower;
class BPartySessionPartyMember;

class XVinceHelper
{
   public:
      static void Vince_Initialize(void* pDevice, BOOL enableLog);
      static void Vince_OpenNewLog();
      static void Vince_CloseAndSendLog();
      static void Vince_CloseLog();
      static void Vince_SendLog();
      static bool IsLogUploading();
      static void Vince_CreateGameID(BSimString& gameID);

      // Vince EVENTS
      static void VinceEvent_GenericMessage(const char* msg);
      static void VinceEvent_PreGameEvent(const char* eventType);
      static void VinceEvent_PreGameSettings(const char* eventType, int mapIndex, const char* matchmakingHopper, const BPartySessionPartyMember* pLocalMember);
      static void VinceEvent_PlayerJoinedMatch(BPlayer* player, BVector pos, float difficulty);
      static void VinceEvent_MatchEnded(long winningTeamID);
      static void VinceEvent_EntityBuilt(BEntity* entity);
      static void VinceEvent_UnitKilled(BUnit* unit, long attackerID);
      static void VinceEvent_TechResearched(long techID, long unitID);
      static void VinceEvent_ControlUsed( BUser* pUser, BSimString controlName );
      static void VinceEvent_AbilityUsed( BUser* pUser, int abilityID );
      static void VinceEvent_UsedLeaderPower( BProtoPower* pProtoPower, long playerID );
      static void VinceEvent_UnitInterval();
      static void VinceEvent_BuildingInterval();
      static void VinceEvent_ResourceInterval();
      static void VinceEvent_PathingInterval();
      static void VinceEvent_ChatFired(BUser* pUser, int chatID);
      static void VinceEvent_WowFired(BUser* pUser);
      static void VinceEvent_BattleStarted();
      static void VinceEvent_HintFired(long stringID, bool allPlayers, const BPlayerIDArray& recipientIDs);

   protected:

      // Helper functions
      static void SendDateTimeEventParameters();

      // Timer Variables
      static float  mUnitInterval;
      static float  mBuildingInterval;
      static float  mResourceInterval;
      static float  mPathingInterval;
      static double mUnitTime;
      static double mBuildingTime;
      static double mResourceTime;
      static double mPathingTime;

      // Resource variables
      static float mResTotals[NUM_PLAYERS][NUM_RESOURCES];

      static int m_eventFlag;

      static bool mDisplayedText;
};

#endif // _VINCE_

#endif
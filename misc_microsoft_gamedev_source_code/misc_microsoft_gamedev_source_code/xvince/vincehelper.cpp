//==============================================================================
// vincehelper.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#ifndef _VINCE_

// common
#include "common.h"

#include <winsockx.h>

void NonVince_CreateGameID(BSimString& gameID)
{
   uint64 id = 0;
   XNetRandom((BYTE*)&id, sizeof(id));
   gameID.format("%I64u", id);
}

#endif

#ifdef _VINCE_

// ***** Includes ***** //

// XDK
#include <xtl.h>

// Vince
#include "vince.h"
#include "vincehelper.h"

// common
#include "common.h"

// xgame
#include "player.h"
#include "user.h"
#include "usermanager.h"
#include "team.h"
#include "leaders.h"
#include "civ.h"
#include "database.h"
#include "scenario.h"
#include "world.h"
#include "unit.h"
#include "squad.h"
#include "protosquad.h"
#include "protoobject.h"
#include "modemanager.h"
#include "modegame.h"
#include "object.h"
#include "settings.h"
#include "gamesettings.h"
#include "prototech.h"
#include "camera.h"
#include "protopower.h"
#include "configsgame.h"
#include "pathingLimiter.h"
#include "partySession.h"

// Timer Variables
float  XVinceHelper::mUnitInterval     = 30.0f;
float  XVinceHelper::mBuildingInterval = 30.0f;
float  XVinceHelper::mResourceInterval = 30.0f;
float  XVinceHelper::mPathingInterval  = 5.0f;
double XVinceHelper::mUnitTime         = 0.0;
double XVinceHelper::mBuildingTime     = 0.0;
double XVinceHelper::mResourceTime     = 0.0;
double XVinceHelper::mPathingTime      = 0.0;
int	   XVinceHelper::m_eventFlag	   = 0;

// Resource Variables
float XVinceHelper::mResTotals[NUM_PLAYERS][NUM_RESOURCES] = 
{
   { 0.0f, 0.0f, 0.0f, 0.0f },
   { 0.0f, 0.0f, 0.0f, 0.0f },
   { 0.0f, 0.0f, 0.0f, 0.0f },
   { 0.0f, 0.0f, 0.0f, 0.0f },
   { 0.0f, 0.0f, 0.0f, 0.0f },
   { 0.0f, 0.0f, 0.0f, 0.0f },
   { 0.0f, 0.0f, 0.0f, 0.0f },
   { 0.0f, 0.0f, 0.0f, 0.0f }
};

// change this to add more vince hooks
const int maxNumVinceEvents = 19;
static bool isVinceEnabled = false;

// this allows the specialized vince integration halowars uses to be used with the april 2007 version of vince which has been 
// refactored into a .h and a .lib without direct access to the components which were previously used by vincewrapper.h/.cpp
#define VINCE_EVENT_INIT_HACK(EventName,EventPath)	\
	static bool firstRun = true;															\
	static int thisEventFlag = -1;															\
	if (firstRun)																			\
	{																						\
		if (m_eventFlag < maxNumVinceEvents)												\
		{																					\
			Vince::InitializeEvent(m_eventFlag, EventName, EventPath, isVinceEnabled);		\
			thisEventFlag = m_eventFlag;													\
			m_eventFlag++;																	\
		}																					\
		firstRun = false;																	\
	}																						\
	BASSERT(thisEventFlag >= 0);

// ***** Initialization ***** //

// Vince_Initialize()
// Gets Vince initialized and set up
void XVinceHelper::Vince_Initialize( void* pDevice, BOOL enableLog )
{
	if (enableLog)
	{
		Vince::InitializeCore();
		Vince::InitializeEvents(maxNumVinceEvents);
	}
   isVinceEnabled = enableLog ? true : false;
}

// Vince_OpenNewLog()
// Opens a new log for writing Vince events. Should only be used in BModeGame::initGame()
void XVinceHelper::Vince_OpenNewLog()
{
	Vince::OpenLog();
}

// Vince_CloseAndSendLog()
// Closes the current game log and sends it to the Vince server. Should only be used in BModeGame::deinitGame()
void XVinceHelper::Vince_CloseAndSendLog()
{
   Vince::CloseLog();
}

void XVinceHelper::Vince_CloseLog()
{
   Vince::CloseLog();
}

void XVinceHelper::Vince_SendLog()
{
   Vince::TransmitLog();
}

bool XVinceHelper::IsLogUploading()
{
   return Vince::IsUploading();
}

// Vince_CreateGameID()
void XVinceHelper::Vince_CreateGameID(BSimString& gameID)
{
	BSimString player1Name;
	long playerNameMunge = 0;
	gDatabase.getGameSettings()->getString(BGameSettings::cPlayer1Name, player1Name);
	playerNameMunge = player1Name.asLong();

	SYSTEMTIME time;
	GetSystemTime(&time);
	playerNameMunge += time.wYear;
	playerNameMunge += time.wMonth;
	playerNameMunge += time.wDay;
	playerNameMunge += time.wHour;
	playerNameMunge += time.wMinute;
	playerNameMunge += time.wSecond;

	gameID.format("%d-%d-%d-%d-%d-%d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, playerNameMunge);

   // Set interval times
   gConfig.get( cConfigVinceUnitInterval, &mUnitInterval );
   gConfig.get( cConfigVinceBuildingInterval, &mBuildingInterval );
   gConfig.get( cConfigVinceResourceInterval, &mResourceInterval );
   gConfig.get( cConfigVincePathingInterval, &mPathingInterval );

   XVinceHelper::mUnitTime         = 0.0;
   XVinceHelper::mBuildingTime     = 0.0;
   XVinceHelper::mResourceTime     = 0.0;
   XVinceHelper::mPathingTime      = 0.0;
}


// ***** Vince Events: Generic/Testing ***** //

// VinceEvent_GenericMessage()
// Used to send any string you want as a VINCE event. Mostly used just to test basic Vince functionality
void XVinceHelper::VinceEvent_GenericMessage(const char* msg)
{
	VINCE_EVENT_INIT_HACK("GenericMessage","Generic");

	if (Vince::StartEvent(thisEventFlag) == false) return;
	Vince::SendParameter("Message", msg);
	Vince::SendEvent();
}

// ***** Vince Events: Pre-game ***** //

// VinceEvent_PreGameEvent()
void XVinceHelper::VinceEvent_PreGameEvent(const char* eventType)
{
   BSimString gamerTag;
   gamerTag.format( "(none)" );

//-- FIXING PREFIX BUG ID 760
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (pUser)
      gamerTag = pUser->getName();

   VINCE_EVENT_INIT_HACK("PreGameEvent","Default");

   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter("Event", eventType);
   Vince::SendParameter("gamertag", gamerTag.asNative());
   Vince::SendEvent();
}

// VinceEvent_PreGameSettings()
void XVinceHelper::VinceEvent_PreGameSettings(const char* eventType, int mapIndex, const char* matchmakingHopper, const BPartySessionPartyMember* pLocalMember)
{
   // set up data we'll be collecting
   BSimString gamerTag, civName, leaderName;
   gamerTag.format( "(none)" );
   civName.format( "(none)" );
   leaderName.format( "(none)" );
   if (pLocalMember)
   {
      gamerTag = pLocalMember->mGamerTag;
//-- FIXING PREFIX BUG ID 761
      const BLeader* pLeader = gDatabase.getLeader(pLocalMember->mSettings.mLeader);
//--
      if (pLeader)
         leaderName = pLeader->mName;
//-- FIXING PREFIX BUG ID 762
      const BCiv* pCiv = gDatabase.getCiv(pLocalMember->mSettings.mCiv);
//--
      if (pCiv)
         civName = pCiv->getCivName();
   }
   else
   {
//-- FIXING PREFIX BUG ID 763
      const BUser* pUser = gUserManager.getPrimaryUser();
//--
      if (pUser)
         gamerTag = pUser->getName();
   }

   BSimString mapName;
   mapName.format("(none)");
   const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapIndex);
   if (pMap)
   {
      mapName = pMap->getFilename();
      mapName.crop(0, mapName.length()-5);
   }

   // Begin logging
   VINCE_EVENT_INIT_HACK("PreGameSettings","Default");
   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter("Event", eventType);
   Vince::SendParameter("map_id", mapName.asNative());
   Vince::SendParameter("gamertag", gamerTag.asNative());
   Vince::SendParameter("civ", civName.asNative());
   Vince::SendParameter("hopper", matchmakingHopper);
   Vince::SendParameter("leader_1", leaderName.asNative());
   Vince::SendEvent();
}

// ***** Vince Events: Match Metrics ***** //

// VinceEvent_PlayerJoinedMatch()
void XVinceHelper::VinceEvent_PlayerJoinedMatch(BPlayer* player, BVector pos, float difficulty)
{
	long numPlayers = 0;

	// set up data we'll be collecting
	BSimString  gameID;
   BSimString  mapName;
	long        playerID         = -1; // long playerID = i;
	const char* gamertag         = NULL;
	long        teamID           = -1;
	long        numPlayersOnTeam = 0;
	const char* civ              = NULL;
	long        numLeaders       = -1;
	float       startPosX        = pos.x;
	float       startPosY        = pos.y;
	float       startPosZ        = pos.z;   
   bool        humanFlag        = false;

	gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );
   gDatabase.getGameSettings()->getString( BGameSettings::cMapName, mapName );

	// check player
	if( player )
	{
		// populate data
		playerID = player->getID();
		gamertag = player->getName().asNative();
		teamID   = player->getTeamID();
		if( player->getTeam() )
		{
			numPlayersOnTeam = player->getTeam()->getNumberPlayers();
		}
		civ        = player->getCiv()->getCivName().asNative();
      humanFlag  = player->isHuman();
	}

	// Begin logging
	VINCE_EVENT_INIT_HACK("PlayerJoinedMatch","Default");
	if (Vince::StartEvent(thisEventFlag) == false) return;
	XVinceHelper::SendDateTimeEventParameters();
	Vince::SendParameter( "game_id", gameID.asNative() );
	Vince::SendParameter( "map_id", mapName.asNative() );
	Vince::SendParameter( "player_id", playerID );
	Vince::SendParameter( "gamertag", gamertag );
	Vince::SendParameter( "total_numplayers", numPlayers );
	Vince::SendParameter( "team_id", teamID );
	Vince::SendParameter( "team_numplayers", numPlayersOnTeam );
	Vince::SendParameter( "civ", civ );
	Vince::SendParameter( "start_loc_x", startPosX );
	Vince::SendParameter( "start_loc_y", startPosY );
	Vince::SendParameter( "start_loc_z", startPosZ );
	Vince::SendParameter( "player_type", humanFlag ? "Human" : "AI" );

	BSimString leaderIndex;
	BSimString leaderName;
	leaderIndex.format( "leader_1" );
	leaderName.format( "(none)" );
	if( player )
	{
		long     leaderID = player->getLeaderID();
//-- FIXING PREFIX BUG ID 765
		const BLeader* leader   = gDatabase.getLeader( leaderID );
//--
		if( leader )
		{
			leaderName = leader->mName;
		}
	}
	Vince::SendParameter( leaderIndex.asNative(), leaderName.asNative() );
   Vince::SendParameter("PlayerDifficulty", difficulty);
	Vince::SendEvent();
	// End logging
}

// VinceEvent_MatchEnded()
void XVinceHelper::VinceEvent_MatchEnded(long winningTeamID)
{
	double total_length = -1.0;
	long winner = winningTeamID;

	if (gWorld)
	{
		total_length = gWorld->getGametimeFloat();
	}
	VINCE_EVENT_INIT_HACK("GameEnd","Default");
	if (Vince::StartEvent(thisEventFlag) == false) return;
	Vince::SendParameter("total_length", total_length);
	Vince::SendParameter("winner", winningTeamID);
	Vince::SendEvent();
}


// ***** Vince Events: Player Statistics ***** //

// VinceEvent_UnitBuilt()
void XVinceHelper::VinceEvent_EntityBuilt(BEntity* unit)
{
	// set default data
	BSimString gameID;
	long playerID = -1;
	const char* objectClassName = NULL;
	const char* unitTypeBuilt = NULL;
	float posX = 0.0f;
	float posY = 0.0f;
	float posZ = 0.0f;
	float resCostTotal = -1.0f;
	float reqBuildPoints = -1.0f;
	long numResourceTypes = -1;

	gDatabase.getGameSettings()->getString(BGameSettings::cGameID, gameID);

	// setup pointers
//-- FIXING PREFIX BUG ID 768
	const BSquad* squad = unit->getSquad();
//--
//-- FIXING PREFIX BUG ID 769
	const BUnit* individualUnit = unit->getUnit();
//--
	const BProtoObject* protoObject = NULL;
	const BProtoSquad* protoSquad = NULL;
	const BCost* cost = NULL;

	// check unit pointer
	if (unit)
	{
		// get protoObject or protoSquad pointer
		if (squad)
		{
			protoSquad = squad->getProtoSquad();
			if (!protoSquad)
			{
//-- FIXING PREFIX BUG ID 767
				const BUnit* tempUnit = gWorld->getUnit(squad->getChild(0));
//--
				if (tempUnit)
				{
					protoObject = tempUnit->getProtoObject();
				}
			}
		}
		if (individualUnit)
		{
			protoObject = individualUnit->getProtoObject();
		}

		// populate data only dependent on unit
		playerID = unit->getPlayerID();
		posX = unit->getPosition().x;
		posY = unit->getPosition().y;
		posZ = unit->getPosition().z;
	}



	// check protoObject pointer
	if (protoObject)
	{
		// get BCost pointer
		cost = protoObject->getCost();

		// populate data only dependent on protoObject
		objectClassName = gDatabase.getObjectClassName(protoObject->getObjectClass());
		unitTypeBuilt = protoObject->getName().asNative();
		reqBuildPoints = protoObject->getBuildPoints();
	}

	if (protoSquad)
	{
		// get BCost pointer
		cost = protoSquad->getCost();

		// populate data only dependent on protoObject
		objectClassName = "Squad";
		unitTypeBuilt = protoSquad->getName().asNative();
		reqBuildPoints = protoSquad->getBuildPoints();
	}

	// check cost pointer
	if (cost)
	{
		resCostTotal = cost->getTotal();
	}

	// Begin logging
	VINCE_EVENT_INIT_HACK("UnitBuilt","Default");
	if (Vince::StartEvent(thisEventFlag) == false) return;
	XVinceHelper::SendDateTimeEventParameters();
	Vince::SendParameter("game_id", gameID);
	Vince::SendParameter("player_id", playerID);
	Vince::SendParameter("unit_objectclass", objectClassName);
	Vince::SendParameter("unit_type_built", unitTypeBuilt);
	Vince::SendParameter("unit_built_x", posX);
	Vince::SendParameter("unit_built_y", posY);
	Vince::SendParameter("unit_built_z", posZ);
	Vince::SendParameter("res_cost_total", resCostTotal);
	Vince::SendParameter("required_build_points", reqBuildPoints);

	numResourceTypes = gDatabase.getNumberResources();
	for (long i=0; i < numResourceTypes; i++)
	{
		BSimString resourceName;
		resourceName.format("res_cost_%s", gDatabase.getResourceName(i));
		float resCost = -1.0f;
		if (cost)
		{
			resCost = cost->get(i);
		}
		Vince::SendParameter(resourceName.asNative(), resCost);
	}

	Vince::SendEvent();
	// End logging
}

// VinceEvent_UnitKilled()
void XVinceHelper::VinceEvent_UnitKilled(BUnit* unit, long attackerID)
{
	// set default data
	BSimString gameID;
	long playerID = -1;
	const char* victimTypeName = NULL;
	float victimPosX = 0.0f;
	float victimPosY = 0.0f;
	float victimPosZ = 0.0f;
	long killerPlayerID = -1;
	const char* killerTypeName = NULL;
	float killerPosX = 0.0f;
	float killerPosY = 0.0f;
	float killerPosZ = 0.0f;

	gDatabase.getGameSettings()->getString(BGameSettings::cGameID, gameID);

	// setup pointers
	const BProtoObject* victimProtoObject = NULL;
	BEntity* killerEntity = NULL;
	const BProtoObject* killerProtoObject = NULL;

	if (unit)
	{
		// get BProtoObject
		victimProtoObject = unit->getProtoObject();

		// set data
		playerID = unit->getPlayerID();
		victimPosX = unit->getPosition().x;
		victimPosY = unit->getPosition().y;
		victimPosZ = unit->getPosition().z;
	}

	if (victimProtoObject)
	{
		victimTypeName = victimProtoObject->getName().asNative();
	}

	if (gWorld)
	{
		killerEntity = gWorld->getEntity(attackerID);
	}

	if (killerEntity)
	{
		if (killerEntity->getObject())
		{
			killerProtoObject = killerEntity->getObject()->getProtoObject();
		}

		killerPlayerID = killerEntity->getPlayerID();
		killerPosX = killerEntity->getPosition().x;
		killerPosY = killerEntity->getPosition().y;
		killerPosZ = killerEntity->getPosition().z;
	}

	if (killerProtoObject)
	{
		killerTypeName = killerProtoObject->getName().asNative();
	}

	// Begin logging
	VINCE_EVENT_INIT_HACK("UnitKilled","Default");
	if (Vince::StartEvent(thisEventFlag) == false) return;
	XVinceHelper::SendDateTimeEventParameters();
	Vince::SendParameter("game_id", gameID);
	Vince::SendParameter("player_id", playerID);
	Vince::SendParameter("victim_type", victimTypeName);
	Vince::SendParameter("victim_unit_x", victimPosX);
	Vince::SendParameter("victim_unit_y", victimPosY);
	Vince::SendParameter("victim_unit_z", victimPosZ);
	Vince::SendParameter("killer_player_id", killerPlayerID);
	Vince::SendParameter("killer_unit_type", killerTypeName);
	Vince::SendParameter("killer_x", killerPosX);
	Vince::SendParameter("killer_y", killerPosY);
	Vince::SendParameter("killer_z", killerPosZ);
	Vince::SendEvent();
	// End logging
}

//==============================================================================
// XVinceHelper::VinceEvent_TechResearched
//==============================================================================
void XVinceHelper::VinceEvent_TechResearched( long techID, long unitID )
{
   // Bomb check.
   BUnit* pUnit = gWorld->getUnit( unitID );
   if( !pUnit )
   {
      return;
   }

//-- FIXING PREFIX BUG ID 770
   const BPlayer* pPlayer = pUnit->getPlayer();
//--
   if( !pPlayer )
   {
      return;
   }

//-- FIXING PREFIX BUG ID 771
   const BProtoTech* pProtoTech = gDatabase.getProtoTech( techID );
//--
   if( !pProtoTech )
   {
      return;
   }

   const BCost* pCost = pProtoTech->getCost();
   if( !pCost )
   {
      return;
   }

   const BProtoObject* pProtoObject = pUnit->getProtoObject();

   // Data we want to log out.
   BSimString gameID;
   long       playerID = -1;
   // gametype - Not established yet
   BSimString techName;
   BSimString unitName;
   // Covenant_Age - ?
   // UNSC_building_level - ?
   BSimString resName;
  
   // Get values
   gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );
   playerID = pPlayer->getID();
   techName = pProtoTech->getName();

   if( pProtoObject )
   {
      unitName = pProtoObject->getName();
   }
   else
   {
      unitName = "NA";
   }

   // Begin logging
   VINCE_EVENT_INIT_HACK("TechResearched","Default");
   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter( "game_id", gameID );
   Vince::SendParameter( "player_id", playerID );
   Vince::SendParameter( "tech_name", techName );
   Vince::SendParameter( "upgraded_unit", unitName );
   unsigned long numRes = pCost->getNumberResources();
   for( unsigned long i = 0; i < numRes; i++ )
   {
      resName.format( "res_cost_%s", gDatabase.getResourceName( i ) );
      Vince::SendParameter( resName, pCost->get( i ) );
   }
   Vince::SendEvent();
   // End logging
}

//==============================================================================
// XVinceHelper::VinceEvent_ControlUsed
//==============================================================================
void XVinceHelper::VinceEvent_ControlUsed( BUser* pUser, BSimString controlName )
{
   if( !pUser )
   {
      return;
   }

//-- FIXING PREFIX BUG ID 772
   const BPlayer* pPlayer = pUser->getPlayer();
//--
   if( !pPlayer )
   {
      return;
   }

   if( pPlayer->getPlayerState() != BPlayer::cPlayerStatePlaying )
   {
      return;
   }

   // set default data
   BSimString gameID;
   long       playerID = -1;
   float      camX     = 0.0f;
   float      camY     = 0.0f;
   float      camZ     = 0.0f;

   // set values
   gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );
   playerID = pUser->getPlayerID();

//-- FIXING PREFIX BUG ID 773
   const BCamera* pCamera = pUser->getCamera();
//--
   if( pCamera )
   {
      const BVector camPos = pCamera->getCameraLoc();
      camX                 = camPos.x;
      camY                 = camPos.y;
      camZ                 = camPos.z;
   }

   // Begin logging
   VINCE_EVENT_INIT_HACK("ControlUsed","Default");
   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter( "game_id", gameID );
   Vince::SendParameter( "player_id", playerID );
   Vince::SendParameter( "control_used", controlName );
   Vince::SendParameter( "control_x", camX );
   Vince::SendParameter( "control_y", camY );
   Vince::SendParameter( "control_z", camZ );
   Vince::SendEvent();
   // End logging
}

//==============================================================================
// XVinceHelper::VinceEvent_AbilityUsed
//==============================================================================
void XVinceHelper::VinceEvent_AbilityUsed( BUser* pUser, int abilityID )
{
   if( !pUser )
   {
      return;
   }

//-- FIXING PREFIX BUG ID 774
   const BPlayer* pPlayer = pUser->getPlayer();
//--
   if( !pPlayer )
   {
      return;
   }

   if( pPlayer->getPlayerState() != BPlayer::cPlayerStatePlaying )
   {
      return;
   }

   // set default data
   BSimString gameID;
   long       playerID = -1;
   float      camX     = 0.0f;
   float      camY     = 0.0f;
   float      camZ     = 0.0f;

   // set values
   gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );
   playerID = pUser->getPlayerID();

//-- FIXING PREFIX BUG ID 775
   const BCamera* pCamera = pUser->getCamera();
//--
   if( pCamera )
   {
      const BVector camPos = pCamera->getCameraLoc();
      camX                 = camPos.x;
      camY                 = camPos.y;
      camZ                 = camPos.z;
   }

   // Begin logging
   VINCE_EVENT_INIT_HACK("AbilityUsed","Default");
   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter( "game_id", gameID );
   Vince::SendParameter( "player_id", playerID );
   Vince::SendParameter( "ability_id", abilityID );
   Vince::SendParameter( "ability_x", camX );
   Vince::SendParameter( "ability_y", camY );
   Vince::SendParameter( "ability_z", camZ );
   Vince::SendEvent();
   // End logging
}

//==============================================================================
// XVinceHelper::VinceEvent_UsedLeaderPower
//==============================================================================
void XVinceHelper::VinceEvent_UsedLeaderPower( BProtoPower* pProtoPower, long playerID )
{
   if( !pProtoPower )
   {
      return;
   }

//-- FIXING PREFIX BUG ID 776
   const BCost* pCost = pProtoPower->getCost();
//--
   if( !pCost )
   {
      return;
   }

   // setup data
   BSimString gameID;
   BSimString powerID  = pProtoPower->getName();
   BSimString resName;

   gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );   

   // Begin logging
   VINCE_EVENT_INIT_HACK("UsedLeaderPower","Default");
   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter( "game_id", gameID );
   Vince::SendParameter( "player_id", playerID );
   Vince::SendParameter( "leader_power_used_id", powerID );
   unsigned long numRes = pCost->getNumberResources();
   for( unsigned long i = 0; i < numRes; i++ )
   {
      resName.format( "res_cost_%s", gDatabase.getResourceName( i ) );
      Vince::SendParameter( resName, pCost->get( i ) );
   }
   Vince::SendEvent();
   // End logging
}

//==============================================================================
// XVinceHelper::VinceEvent_UnitInterval
//==============================================================================
void XVinceHelper::VinceEvent_UnitInterval()
{  
   if (!gWorld || gWorld->getFlagGameOver())
      return;

   double gameTime = gWorld->getGametimeFloat();
   long   gameType = gModeManager.getModeType();
   if( ( gameType == BModeManager::cModeGame ) && ( gameTime > mUnitTime ) )
   {
      mUnitTime = gameTime + mUnitInterval;

      // setup data
      BSimString gameID;

      gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );   

      BEntityHandle handle = cInvalidObjectID;
      BUnit* pUnit         = gWorld->getNextUnit( handle );
      long   unitType      = gDatabase.getObjectType( "Unit" );

      while( pUnit )
      {
         if( pUnit->isType( unitType ) )
         {            
            // Begin logging
            VINCE_EVENT_INIT_HACK("UnitIntervals","Default");
            if (Vince::StartEvent(thisEventFlag) == false) return;
			XVinceHelper::SendDateTimeEventParameters();
            Vince::SendParameter( "game_id", gameID );
            Vince::SendParameter( "player_id", pUnit->getPlayerID() );            

            const BProtoObject* cpProtoObject = pUnit->getProtoObject();
            if( cpProtoObject )
            {
               BSimString unitName = cpProtoObject->getName();
               Vince::SendParameter( "unit_type", unitName );
            }
            else
            {
               Vince::SendParameter( "unit_type", "NA" );
            }

            BVector unitPos = pUnit->getPosition();
            Vince::SendParameter( "unit_x", unitPos.x );
            Vince::SendParameter( "unit_y", unitPos.y );
            Vince::SendParameter( "unit_z", unitPos.z );
            Vince::SendParameter( "unit_health", pUnit->getHitpoints() );                       
            Vince::SendEvent();
            // End logging
         }

         pUnit = gWorld->getNextUnit( handle );
      }
   }
}

//==============================================================================
// XVinceHelper::VinceEvent_BuildingInterval
//==============================================================================
void XVinceHelper::VinceEvent_BuildingInterval()
{
   if (!gWorld || gWorld->getFlagGameOver())
      return;

   double gameTime = gWorld->getGametimeFloat();
   long   gameType = gModeManager.getModeType();
   if( ( gameType == BModeManager::cModeGame ) && ( gameTime > mBuildingTime ) )
   {
      mBuildingTime = gameTime + mBuildingInterval;

      // setup data
      BSimString gameID;

      gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );   

      BEntityHandle handle   = cInvalidObjectID;
      BUnit*        pUnit    = gWorld->getNextUnit( handle );
      long          unitType = gDatabase.getOTIDBuilding();

      while( pUnit )
      {
         if( pUnit->isType( unitType ) )
         {
            // Begin logging
            VINCE_EVENT_INIT_HACK("BuildingIntervalsNew","Default");
            if (Vince::StartEvent(thisEventFlag) == false) return;
			XVinceHelper::SendDateTimeEventParameters();
            Vince::SendParameter( "game_id", gameID );
            Vince::SendParameter( "player_id", pUnit->getPlayerID() );            

            const BProtoObject* cpProtoObject = pUnit->getProtoObject();
            if( cpProtoObject )
            {
               BSimString unitName = cpProtoObject->getName();
               Vince::SendParameter( "building_type", unitName );
            }
            else
            {
               Vince::SendParameter( "building_type", "NA" );
            }

            BVector unitPos = pUnit->getPosition();
            Vince::SendParameter( "building_x", unitPos.x );
            Vince::SendParameter( "building_y", unitPos.y );
            Vince::SendParameter( "building_z", unitPos.z );
            Vince::SendParameter( "building_health", pUnit->getHitpoints() );                       
            Vince::SendEvent();
            // End logging
         }

         pUnit = gWorld->getNextUnit( handle );
      }
   }
}

//==============================================================================
// XVinceHelper::VinceEvent_ResourceInterval
//==============================================================================
void XVinceHelper::VinceEvent_ResourceInterval()
{
   if (!gWorld || gWorld->getFlagGameOver())
      return;

   double gameTime = gWorld->getGametimeFloat();
   long   gameType = gModeManager.getModeType();
   if( ( gameType == BModeManager::cModeGame ) && ( gameTime > mResourceTime ) )
   {
      mResourceTime = gameTime + mResourceInterval;

      // setup data
      BSimString gameID;
      BSimString resName;
      float      resTotals[NUM_RESOURCES];
      float      resDeltas[NUM_RESOURCES];

      gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );

      long numResources = gDatabase.getNumberResources();
      BASSERTM(NUM_RESOURCES >= numResources, "NUM_RESOURCES >= numResources.  Fix your hard-coded incorrect value here please.");
      if (numResources > NUM_RESOURCES)
         return;

      long numPlayers = gWorld->getNumberPlayers();
      for (long i = 0; i < numPlayers; i++)
      {
//-- FIXING PREFIX BUG ID 777
         const BPlayer* pPlayer  = gWorld->getPlayer( i );
//--
         long     playerID = pPlayer->getID();

         // Add up current available resources
         memset( resTotals, 0, sizeof( float ) * NUM_RESOURCES );
         for( long j = 0; j < NUM_RESOURCES; j++ )
         {
            if (BCost::isValidResourceID(j))
               resTotals[j] += pPlayer->getResource( j );
         }

         // Find resource deltas from last interval and set this intervals amounts
         memset( resDeltas, 0, sizeof( float ) * NUM_RESOURCES );
         for( long j = 0; j < NUM_RESOURCES; j++ )
         {
            resDeltas[j]     = resTotals[j] - mResTotals[i][j];
            mResTotals[i][j] = resTotals[j];
         }

         // Begin logging
         VINCE_EVENT_INIT_HACK("ResourceIntervals","Default");
         if (Vince::StartEvent(thisEventFlag) == false) return;
         XVinceHelper::SendDateTimeEventParameters();
         Vince::SendParameter( "game_id", gameID );
         Vince::SendParameter( "player_id", playerID );

         for (long i=0; i < numResources; ++i)
         {
            const char* cpResName = gDatabase.getResourceName(i);
            resName.format( "res_%s_available", cpResName );
            Vince::SendParameter( resName, resTotals[i] );
            resName.format( "res_%s_rate", cpResName );
            Vince::SendParameter( resName, resDeltas[i] / mResourceInterval );
         }

         Vince::SendEvent();
         // End logging
      }
   }
}

//==============================================================================
// XVinceHelper::VinceEvent_PathingInterval
//==============================================================================
void XVinceHelper::VinceEvent_PathingInterval()
{
   if (!gWorld || gWorld->getFlagGameOver())
      return;

   double gameTime = gWorld->getGametimeFloat();
   long   gameType = gModeManager.getModeType();
   if( ( gameType == BModeManager::cModeGame ) && ( gameTime > mPathingTime ) )
   {
      mPathingTime = gameTime + mPathingInterval;
      BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();

      if (pathLimiter && pathLimiter->isPathHistInitialized())
      {
         // setup data
         BSimString gameID;

         gDatabase.getGameSettings()->getString( BGameSettings::cGameID, gameID );   

         long numPlayers = gWorld->getNumberPlayers();
         for( long i = 0; i < numPlayers; i++ )
         {         
//-- FIXING PREFIX BUG ID 778
            const BPlayer* pPlayer  = gWorld->getPlayer( i );
//--
            long     playerID = pPlayer->getID();

            long maxPlatoonFramesDenied = pathLimiter->getMaxPlatoonFramesDenied(playerID);
            long maxSquadFramesDenied = pathLimiter->getMaxSquadFramesDenied(playerID);

            pathLimiter->setMaxPlatoonFramesDenied(playerID, 0);
            pathLimiter->setMaxSquadFramesDenied(playerID, 0);

            VINCE_EVENT_INIT_HACK("PathingIntervalsNew","Default");
            if (Vince::StartEvent(thisEventFlag) == false) return;
            XVinceHelper::SendDateTimeEventParameters();
            Vince::SendParameter( "game_id", gameID );         
            Vince::SendParameter( "player_id", playerID );
            Vince::SendParameter( "maxPlatoonFramesDenied", maxPlatoonFramesDenied );
            Vince::SendParameter( "maxSquadFramesDenied", maxSquadFramesDenied );

            Vince::SendEvent();
            // End logging
         }
      }
   }
}

//==============================================================================
// XVinceHelper::VinceEvent_ChatFired
//==============================================================================
void XVinceHelper::VinceEvent_ChatFired(BUser* pUser, int chatID)
{
   // set default data
   BSimString gameID;
   long       playerID = -1;
   float      camX     = 0.0f;
   float      camY     = 0.0f;
   float      camZ     = 0.0f;

   // set values
   gDatabase.getGameSettings()->getString(BGameSettings::cGameID, gameID);
   playerID = pUser->getPlayerID();

//-- FIXING PREFIX BUG ID 779
   const BCamera* pCamera = pUser->getCamera();
//--
   if (pCamera)
   {
      const BVector camPos = pCamera->getCameraLoc();
      camX                 = camPos.x;
      camY                 = camPos.y;
      camZ                 = camPos.z;
   }

   // Begin logging
   VINCE_EVENT_INIT_HACK("ChatFired","Default");
   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter("game_id", gameID);
   Vince::SendParameter("player_id", playerID);
   Vince::SendParameter("chat_id", chatID);
   Vince::SendParameter("chat_fired_x", camX);
   Vince::SendParameter("chat_fired_y", camY);
   Vince::SendParameter("chat_fired_z", camZ);
   Vince::SendEvent();
   // End logging
}

//==============================================================================
// XVinceHelper::VinceEvent_WowFired
//==============================================================================
void XVinceHelper::VinceEvent_WowFired(BUser* pUser)
{
   // set default data
   BSimString gameID;
   long       playerID = -1;
   float      camX     = 0.0f;
   float      camY     = 0.0f;
   float      camZ     = 0.0f;

   // set values
   gDatabase.getGameSettings()->getString(BGameSettings::cGameID, gameID);
   playerID = pUser->getPlayerID();

//-- FIXING PREFIX BUG ID 780
   const BCamera* pCamera = pUser->getCamera();
//--
   if (pCamera)
   {
      const BVector camPos = pCamera->getCameraLoc();
      camX                 = camPos.x;
      camY                 = camPos.y;
      camZ                 = camPos.z;
   }

   // Begin logging
   VINCE_EVENT_INIT_HACK("WowFired","Default");
   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter("game_id", gameID);
   Vince::SendParameter("player_id", playerID);
   Vince::SendParameter("wow_fired_x", camX);
   Vince::SendParameter("wow_fired_y", camY);
   Vince::SendParameter("wow_fired_z", camZ);
   Vince::SendEvent();
   // End logging
}

//==============================================================================
// XVinceHelper::VinceEvent_BattleStarted
//==============================================================================
void XVinceHelper::VinceEvent_BattleStarted()
{
   // set default data
   BSimString gameID;

   // set values
   gDatabase.getGameSettings()->getString(BGameSettings::cGameID, gameID);

   // Begin logging
   VINCE_EVENT_INIT_HACK("BattleStarted","Default");
   if (Vince::StartEvent(thisEventFlag) == false) return;
   XVinceHelper::SendDateTimeEventParameters();
   Vince::SendParameter("game_id", gameID);
   Vince::SendEvent();
   // End logging
}

//==============================================================================
// 
//==============================================================================
void XVinceHelper::VinceEvent_HintFired(long stringID, bool allPlayers, const BPlayerIDArray& recipientIDs)
{
   BSimString gameID;
   gDatabase.getGameSettings()->getString(BGameSettings::cGameID, gameID);

   double gameTime = 0.0;
   if (gWorld)
   {
      gameTime = gWorld->getGametimeFloat();
   }

   if (allPlayers)
   {
      // Begin logging
      VINCE_EVENT_INIT_HACK("HintFired","Default");
      if (Vince::StartEvent(thisEventFlag) == false) return;
      XVinceHelper::SendDateTimeEventParameters();
      Vince::SendParameter("game_id", gameID);
      Vince::SendParameter("game_time", gameTime);
      Vince::SendParameter("string_id", stringID);
      Vince::SendParameter("player_id", -1);
      Vince::SendEvent();
      // End logging
   }
   else
   {
      // loop on the recipientIDs
      uint count = recipientIDs.getSize();
      for (uint i = 0; i < count; i++)
      {
         // Begin logging
         VINCE_EVENT_INIT_HACK("HintFired","Default");
         if (Vince::StartEvent(thisEventFlag) == false) return;
         XVinceHelper::SendDateTimeEventParameters();
         Vince::SendParameter("game_id", gameID);
         Vince::SendParameter("game_time", gameTime);
         Vince::SendParameter("string_id", stringID);
         Vince::SendParameter("player_id", recipientIDs[i]);
         Vince::SendEvent();
         // End logging
      }
   }
}

// ***** Helper Functions ***** //

// SendDateTimeEventParameters()
void XVinceHelper::SendDateTimeEventParameters()
{
	if (isVinceEnabled == false)
		return;

	SYSTEMTIME datetime;
	GetSystemTime(&datetime);
	Vince::SendParameter("datetime_year", datetime.wYear);
	Vince::SendParameter("datetime_month", datetime.wMonth);
	Vince::SendParameter("datetime_day", datetime.wDay);
	Vince::SendParameter("datetime_hour", datetime.wHour);
	Vince::SendParameter("datetime_minute", datetime.wMinute);
	Vince::SendParameter("datetime_second", datetime.wSecond);
}

#endif // _VINCE_
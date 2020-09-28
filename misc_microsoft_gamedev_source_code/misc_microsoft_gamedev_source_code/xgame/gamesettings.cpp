//==============================================================================
// gamesettings.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "gamesettings.h"
#include "config.h"
#include "configsgame.h"
#include "dataentry.h"
#include "gamedirectories.h"

//============================================================================
// gGameSettings
//============================================================================
BSettingDef gGameSettings[BGameSettings::cSettingCount]=
{
   { BGameSettings::cPlayerCount,               B("PlayerCount"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cMaxPlayers,                B("MaxPlayers"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cMapName,                   B("MapName"),                 BDataEntry::cTypeString,   0 },
   { BGameSettings::cMapIndex,                  B("MapIndex"),                BDataEntry::cTypeLong,     0 },
   { BGameSettings::cNetworkType,               B("NetworkType"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cGameType,                  B("GameType"),                BDataEntry::cTypeLong,     0 },
   { BGameSettings::cGameStartContext,          B("GameStartContext"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cGameMode,                  B("GameMode"),                BDataEntry::cTypeLong,     0 },
   { BGameSettings::cRandomSeed,                B("RandomSeed"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cGameID,                    B("GameID"),                  BDataEntry::cTypeString,   0 },
   { BGameSettings::cCoop,                      B("Coop"),                    BDataEntry::cTypeBool,     0 },
   { BGameSettings::cRecordGame,                B("RecordGame"),              BDataEntry::cTypeBool,     0 },
   { BGameSettings::cLoadType,                  B("LoadType"),                BDataEntry::cTypeByte,     0 },
   { BGameSettings::cLoadName,                  B("LoadName"),                BDataEntry::cTypeString,   0 },

   { BGameSettings::cPlayerName,                B("PlayerName"),              BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayerXUID,                B("PlayerXUID"),              BDataEntry::cTypeInt64,    0 }, 
   { BGameSettings::cPlayerType,                B("PlayerType"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayerTeam,                B("PlayerTeam"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayerCiv,                 B("PlayerCiv"),               BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayerLeader,              B("PlayerLeader"),            BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayerReady,               B("PlayerReady"),             BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayerDifficulty,          B("PlayerDifficulty"),        BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayerDifficultyType,      B("PlayerDifficultyType"),    BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayerSkullBits,           B("PlayerSkullBits"),         BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayerRank,                B("PlayerRank"),              BDataEntry::cTypeShort,    0 },

   { BGameSettings::cPlayer1Name,               B("Player1Name"),             BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayer1XUID,               B("Player1XUID"),             BDataEntry::cTypeInt64,    0 },
   { BGameSettings::cPlayer1Type,               B("Player1Type"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer1Team,               B("Player1Team"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer1Civ,                B("Player1Civ"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer1Leader,             B("Player1Leader"),           BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer1Ready,              B("Player1Ready"),            BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayer1Difficulty,         B("Player1Difficulty"),       BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayer1DifficultyType,     B("Player1DifficultyType"),   BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer1SkullBits,          B("Player1SkullBits"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer1Rank,               B("Player1Rank"),             BDataEntry::cTypeShort,    0 },

   { BGameSettings::cPlayer2Name,               B("Player2Name"),             BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayer2XUID,               B("Player2XUID"),             BDataEntry::cTypeInt64,    0 },
   { BGameSettings::cPlayer2Type,               B("Player2Type"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer2Team,               B("Player2Team"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer2Civ,                B("Player2Civ"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer2Leader,             B("Player2Leader"),           BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer2Ready,              B("Player2Ready"),            BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayer2Difficulty,         B("Player2Difficulty"),       BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayer2DifficultyType,     B("Player2DifficultyType"),   BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer2SkullBits,          B("Player2SkullBits"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer2Rank,               B("Player2Rank"),             BDataEntry::cTypeShort,    0 },

   { BGameSettings::cPlayer3Name,               B("Player3Name"),             BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayer3XUID,               B("Player3XUID"),             BDataEntry::cTypeInt64,    0 },
   { BGameSettings::cPlayer3Type,               B("Player3Type"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer3Team,               B("Player3Team"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer3Civ,                B("Player3Civ"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer3Leader,             B("Player3Leader"),           BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer3Ready,              B("Player3Ready"),            BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayer3Difficulty,         B("Player3Difficulty"),       BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayer3DifficultyType,     B("Player3DifficultyType"),   BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer3SkullBits,          B("Player3SkullBits"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer3Rank,               B("Player3Rank"),             BDataEntry::cTypeShort,    0 },

   { BGameSettings::cPlayer4Name,               B("Player4Name"),             BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayer4XUID,               B("Player4XUID"),             BDataEntry::cTypeInt64,    0 },
   { BGameSettings::cPlayer4Type,               B("Player4Type"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer4Team,               B("Player4Team"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer4Civ,                B("Player4Civ"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer4Leader,             B("Player4Leader"),           BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer4Ready,              B("Player4Ready"),            BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayer4Difficulty,         B("Player4Difficulty"),       BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayer4DifficultyType,     B("Player4DifficultyType"),   BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer4SkullBits,          B("Player4SkullBits"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer4Rank,               B("Player4Rank"),             BDataEntry::cTypeShort,    0 },

   { BGameSettings::cPlayer5Name,               B("Player5Name"),             BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayer5XUID,               B("Player5XUID"),             BDataEntry::cTypeInt64,    0 },
   { BGameSettings::cPlayer5Type,               B("Player5Type"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer5Team,               B("Player5Team"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer5Civ,                B("Player5Civ"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer5Leader,             B("Player5Leader"),           BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer5Ready,              B("Player5Ready"),            BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayer5Difficulty,         B("Player5Difficulty"),       BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayer5DifficultyType,     B("Player5DifficultyType"),   BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer5SkullBits,          B("Player5SkullBits"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer5Rank,               B("Player5Rank"),             BDataEntry::cTypeShort,    0 },

   { BGameSettings::cPlayer6Name,               B("Player6Name"),             BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayer6XUID,               B("Player6XUID"),             BDataEntry::cTypeInt64,    0 },
   { BGameSettings::cPlayer6Type,               B("Player6Type"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer6Team,               B("Player6Team"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer6Civ,                B("Player6Civ"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer6Leader,             B("Player6Leader"),           BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer6Ready,              B("Player6Ready"),            BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayer6Difficulty,         B("Player6Difficulty"),       BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayer6DifficultyType,     B("Player6DifficultyType"),   BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer6SkullBits,          B("Player6SkullBits"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer6Rank,               B("Player6Rank"),             BDataEntry::cTypeShort,    0 },

   { BGameSettings::cPlayer7Name,               B("Player7Name"),             BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayer7XUID,               B("Player7XUID"),             BDataEntry::cTypeInt64,    0 },
   { BGameSettings::cPlayer7Type,               B("Player7Type"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer7Team,               B("Player7Team"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer7Civ,                B("Player7Civ"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer7Leader,             B("Player7Leader"),           BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer7Ready,              B("Player7Ready"),            BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayer7Difficulty,         B("Player7Difficulty"),       BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayer7DifficultyType,     B("Player7DifficultyType"),   BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer7SkullBits,          B("Player7SkullBits"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer7Rank,               B("Player7Rank"),             BDataEntry::cTypeShort,    0 },

   { BGameSettings::cPlayer8Name,               B("Player8Name"),             BDataEntry::cTypeString,   0 },
   { BGameSettings::cPlayer8XUID,               B("Player8XUID"),             BDataEntry::cTypeInt64,    0 },
   { BGameSettings::cPlayer8Type,               B("Player8Type"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer8Team,               B("Player8Team"),             BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer8Civ,                B("Player8Civ"),              BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer8Leader,             B("Player8Leader"),           BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer8Ready,              B("Player8Ready"),            BDataEntry::cTypeBool,     0 },
   { BGameSettings::cPlayer8Difficulty,         B("Player8Difficulty"),       BDataEntry::cTypeFloat,    0 },
   { BGameSettings::cPlayer8DifficultyType,     B("Player8DifficultyType"),   BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer8SkullBits,          B("Player8SkullBits"),        BDataEntry::cTypeLong,     0 },
   { BGameSettings::cPlayer8Rank,               B("Player8Rank"),             BDataEntry::cTypeShort,    0 },
};

//==============================================================================
// BGameSettings::BGameSettings
//==============================================================================
BGameSettings::BGameSettings() :
   BSettings(gGameSettings, cSettingCount)
{
}

//==============================================================================
// BGameSettings::BGameSettings
//==============================================================================
BGameSettings::BGameSettings(BGameSettings* copyFrom) :
   BSettings(copyFrom, cSettingCount)
{
}

//==============================================================================
// BGameSettings::setup
//==============================================================================
bool BGameSettings::setup()
{
   if(!loadFromFile(cDirData, B("gamesettings.xml")))
      return false;

   // Override record game value from config
   bool record=gConfig.isDefined(cConfigRecordGames);
   setBool(cRecordGame, record);

   return true;
}

//==============================================================================
// BGameSettings::resetPlayer
//==============================================================================
void  BGameSettings::resetPlayer( long playerID )
{
   //TODO - this is a hack, it duplicates the default values from above
   //  We need a better system here that just has ONE set of default values
   setString( PSINDEX( playerID, cPlayerName), sEmptySimString);
   setUInt64( PSINDEX( playerID, cPlayerXUID), 0);
   setLong( PSINDEX( playerID, cPlayerType), 0);
   setLong( PSINDEX( playerID, cPlayerTeam), 0);
   setLong( PSINDEX( playerID, cPlayerCiv), 0);
   setLong( PSINDEX( playerID, cPlayerLeader), -1);
   setLong( PSINDEX( playerID, cPlayerReady), 0);
   setFloat( PSINDEX( playerID, cPlayerDifficulty), 0.375);
   setLong( PSINDEX( playerID, cPlayerDifficultyType), 0);
   setLong( PSINDEX( playerID, cPlayerSkullBits), 0);
   setWORD( PSINDEX( playerID, cPlayerRank), 0);
}


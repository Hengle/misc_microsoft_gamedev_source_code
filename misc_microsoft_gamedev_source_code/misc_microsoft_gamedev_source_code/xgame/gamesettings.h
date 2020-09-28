//==============================================================================
// gamesettings.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "settings.h"

//==============================================================================
// BGameSettings
//==============================================================================
class BGameSettings : public BSettings
{
   public:
      enum
      {
         // Note: these are the valid values of cPlayerXType
         cPlayerNotDefined = 0,
         cPlayerComputer = 1,
         cPlayerHuman = 2,
      };

      enum
      {
         // Note: these are the valid values for cGameType
         cGameTypeSkirmish=0,
         cGameTypeCampaign,
         cGameTypeScenario,
         cNumberGameTypes
      };

      enum
      {
         cGameStartContextNone=0,
         cGameStartContextBasicTutorialFromMainMenu,
         cGameStartContextBasicTutorialFromSPCMenu,
         cGameStartContextBasicTutorialFromNewSPC,
         cGameStartContextAdvancedTutorialFromMainMenu,
         cGameStartContextAdvancedTutorialFromSPCMenu,
         cGameStartContextPartyCampaign,
         cGameStartContextPartySkirmish,
         cGameStartContextPartyMatchmaking,

      };

      enum
      {
         cLoadTypeNone=0,
         cLoadTypeRecord,
         cLoadTypeSave,
      };

      enum
      {
         cNetworkTypeLocal=0,
         cNetworkTypeLan,
         cNetworkTypeLive,
         cNumberNetworkTypes
      };

      // !! WARNING !!
      // If you change the meaning or name of any of the game settings then you need to 
      // modify the record game code where we read in the game settings and update the 
      // record game version and provide backwards compatibility if you can.
      enum
      {
         cPlayerCount,                       // 0
         cMaxPlayers,                        // 1
         cMapName,
         cMapIndex,  // index into scenariodescriptions.xml if set
         cNetworkType,                       // local, lan, live.
         cGameType,
         cGameStartContext,
         cGameMode,
         cRandomSeed,
         cGameID,
         cCoop,
         cRecordGame,
         cLoadType,
         cLoadName,                          // 11

         cPlayerFirstEntry,                  // 12
         cPlayerName = cPlayerFirstEntry,    // 12
         cPlayerXUID,                        // 13
         cPlayerType,
         cPlayerTeam,
         cPlayerCiv,
         cPlayerLeader,
         cPlayerReady,
         cPlayerDifficulty,
         cPlayerDifficultyType,
         cPlayerSkullBits,
         cPlayerRank,
         cPlayerLastEntry,                   // 24

         cPlayer1Name = cPlayerLastEntry,    
         cPlayer1XUID,                       
         cPlayer1Type,
         cPlayer1Team,
         cPlayer1Civ,
         cPlayer1Leader,
         cPlayer1Ready,
         cPlayer1Difficulty,                 
         cPlayer1DifficultyType,
         cPlayer1SkullBits,
         cPlayer1Rank,

         cPlayer2Name,                      
         cPlayer2XUID,
         cPlayer2Type,
         cPlayer2Team,
         cPlayer2Civ,
         cPlayer2Leader,
         cPlayer2Ready,
         cPlayer2Difficulty,                 
         cPlayer2DifficultyType,
         cPlayer2SkullBits,
         cPlayer2Rank,

         cPlayer3Name,                       
         cPlayer3XUID,
         cPlayer3Type,
         cPlayer3Team,
         cPlayer3Civ,
         cPlayer3Leader,
         cPlayer3Ready,
         cPlayer3Difficulty,                
         cPlayer3DifficultyType,
         cPlayer3SkullBits,
         cPlayer3Rank,

         cPlayer4Name,                       
         cPlayer4XUID,                      
         cPlayer4Type,                     
         cPlayer4Team,                      
         cPlayer4Civ,                        
         cPlayer4Leader,                     
         cPlayer4Ready,                      
         cPlayer4Difficulty,               
         cPlayer4DifficultyType,
         cPlayer4SkullBits,
         cPlayer4Rank,

         cPlayer5Name,
         cPlayer5XUID,
         cPlayer5Type,
         cPlayer5Team,
         cPlayer5Civ,
         cPlayer5Leader,
         cPlayer5Ready,
         cPlayer5Difficulty,
         cPlayer5DifficultyType,
         cPlayer5SkullBits,
         cPlayer5Rank,

         cPlayer6Name,
         cPlayer6XUID,
         cPlayer6Type,
         cPlayer6Team,
         cPlayer6Civ,
         cPlayer6Leader,
         cPlayer6Ready,
         cPlayer6Difficulty,
         cPlayer6DifficultyType,
         cPlayer6SkullBits,
         cPlayer6Rank,

         cPlayer7Name,
         cPlayer7XUID,
         cPlayer7Type,
         cPlayer7Team,
         cPlayer7Civ,
         cPlayer7Leader,
         cPlayer7Ready,
         cPlayer7Difficulty,
         cPlayer7DifficultyType,
         cPlayer7SkullBits,
         cPlayer7Rank,

         cPlayer8Name,
         cPlayer8XUID,
         cPlayer8Type,
         cPlayer8Team,
         cPlayer8Civ,
         cPlayer8Leader,
         cPlayer8Ready,
         cPlayer8Difficulty,
         cPlayer8DifficultyType,
         cPlayer8SkullBits,
         cPlayer8Rank,

         cSettingCount,
      };

      BGameSettings();
      BGameSettings(BGameSettings* copyFrom);
      
      virtual ~BGameSettings() { }

      bool  setup();
      
      //Added these as a way to set a particular player's settings to their default values - ewb
      void  resetPlayer( long playerID );

};

#define PSINDEX(playerID, index) (((playerID)*(BGameSettings::cPlayerLastEntry-BGameSettings::cPlayerFirstEntry))+(index))

extern BSettingDef gGameSettings[BGameSettings::cSettingCount];
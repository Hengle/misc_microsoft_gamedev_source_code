//==============================================================================
// configssound.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "configssound.h"

//==============================================================================
// Defines
//==============================================================================
DEFINE_CONFIG(cConfigNoSound);
DEFINE_CONFIG(cConfigNoMusic);
DEFINE_CONFIG(cConfigNoExistSound);
DEFINE_CONFIG(cConfigDisplayListener);
DEFINE_CONFIG(cConfigDisplaySounds);
DEFINE_CONFIG(cConfigCatchWorldSoundLimiting);
DEFINE_CONFIG(cConfigFixListenerHeight);
DEFINE_CONFIG(cConfigEnableBattleAmbience);
DEFINE_CONFIG(cConfigEnableBattleStatus);
DEFINE_CONFIG(cConfigUseWin32SoundIO);
DEFINE_CONFIG(cConfigEnableMusic);
DEFINE_CONFIG(cConfigUIWorldSounds);
DEFINE_CONFIG(cConfigMusicVolume);
DEFINE_CONFIG(cConfigSoundFXVolume);
DEFINE_CONFIG(cConfigVoiceVolume);
DEFINE_CONFIG(cConfigDebugSound);

//==============================================================================
// BConfigsInput::registerConfigs
//==============================================================================
static bool registerSoundConfigs(bool)
{
   DECLARE_CONFIG(cConfigNoSound,                  "NoSound", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoMusic,                  "NoMusic", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoExistSound,             "NoExistSound", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisplayListener,          "DisplayListener", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisplaySounds,            "DisplaySounds", "", 0, NULL);   
   DECLARE_CONFIG(cConfigCatchWorldSoundLimiting,  "CatchWorldSoundLimiting", "", 0, NULL);   
   DECLARE_CONFIG(cConfigFixListenerHeight,        "FixListenerHeight", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableBattleStatus,       "EnableBattleStatus", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableBattleAmbience,     "EnableBattleAmbience", "", 0, NULL);
   DECLARE_CONFIG(cConfigUseWin32SoundIO,          "EnableWin32SoundIO", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableMusic,              "EnableMusic", "", 0, NULL); //-- Temp
   DECLARE_CONFIG(cConfigUIWorldSounds,            "UIWorldSounds", "", 0, NULL); //-- Temp
   DECLARE_CONFIG(cConfigMusicVolume,              "MusicVolume", "", 0, NULL);
   DECLARE_CONFIG(cConfigSoundFXVolume,            "SoundFXVolume", "", 0, NULL);
   DECLARE_CONFIG(cConfigVoiceVolume,              "VoiceVolume", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugSound,               "DebugSound", "", 0, NULL);

   return true;
}

// This causes xcore to call registerSoundConfigs() after it initializes.
#pragma data_seg(".ENS$XIU") 
BXCoreInitFuncPtr gpRegisterSoundConfigs[] = { registerSoundConfigs };
#pragma data_seg() 

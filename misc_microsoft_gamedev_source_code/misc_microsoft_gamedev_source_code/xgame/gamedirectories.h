//==============================================================================
// gamedirectories.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#pragma once

// These "constants" are used as indices when calling BFileManager::getWorkingDirectory.

extern long cDirExec;                  // where the game was executed from

// Shipping/production directories
extern long cDirData;
extern long cDirArt;
extern long cDirTalkingHead;
extern long cDirScenario;
extern long cDirSound;
extern long cDirFonts;
extern long cDirTerrain;
extern long cDirPhysics;
extern long cDirRenderEffects;
extern long cDirTactics;
extern long cDirPlacementRules;
extern long cDirTriggerScripts;
extern long cDirRecordGame;
extern long cDirSaveGame;
extern long cDirCampaign;
extern long cDirAIData;

// Functions
bool setupGameDirectories();

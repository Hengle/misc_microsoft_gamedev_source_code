//==============================================================================
// configsgame.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#pragma once

#include "config.h"

//==============================================================================
// Config constants
//==============================================================================

// Commands
extern const long& cConfigDontMeterWorkCommands;

// Multiplayer syncing
extern const long& cConfigNoSync;
extern const long& cConfigDontGoOOS;
extern const long& cConfigSyncUpdateInterval;
extern const long& cConfigFullAllSync;
extern const long& cConfigDisableAllSync;
extern const long& cConfigXORAllSync;
extern const long& cConfigRandSync;
extern const long& cConfigPlayerSync;
extern const long& cConfigTeamSync;
extern const long& cConfigUnitGroupSync;
extern const long& cConfigUnitSync;
extern const long& cConfigUnitDetailSync;
extern const long& cConfigUnitActionSync;
extern const long& cConfigSquadSync;
extern const long& cConfigProjectileSync;
extern const long& cConfigWorldSync;
extern const long& cConfigTechSync;
extern const long& cConfigCommandSync;
extern const long& cConfigPathingSync;
extern const long& cConfigMovementSync;
extern const long& cConfigVisibilitySync;
extern const long& cConfigFinalReleaseSync;
extern const long& cConfigFinalDetailSync;
extern const long& cConfigChecksumSync;
extern const long& cConfigTriggerSync;
extern const long& cConfigTriggerVarSync;
extern const long& cConfigAnimSync;
extern const long& cConfigDoppleSync;
extern const long& cConfigCommSync;
extern const long& cConfigPlatoonSync;

// Comm logs
extern const long& cConfigCommLogging;
extern const long& cConfigCommLogHistorySize;
extern const long& cConfigSessionCL;
extern const long& cConfigTransportCL;
extern const long& cConfigTimeSyncCL;
extern const long& cConfigGameTimingCL;
extern const long& cConfigTimingCL;
extern const long& cConfigSyncCL;
extern const long& cConfigLowLevelSyncCL;
extern const long& cConfigBroadcasterCL;
extern const long& cConfigReceiverCL;
extern const long& cConfigConnectivityCL;
extern const long& cConfigBandwidthCL;
extern const long& cConfigPerfCL;
extern const long& cConfigMPGameCL;
extern const long& cConfigMPGameSettingsCL;
extern const long& cConfigReliableConnCL;
extern const long& cConfigMPStorageCL;
extern const long& cConfigMPMatchmakingCL;
extern const long& cConfigSessionConnAddrCL;
extern const long& cConfigSessionConnJoinCL;
extern const long& cConfigFileTransferCL;

// Gamepad scrolling
extern const long& cConfigDelayScrollingAngle;
extern const long& cConfigDelayScrollingTime;
extern const long& cConfigScrollRateBase;
extern const long& cConfigScrollRateGrow;
extern const long& cConfigScrollFastSpeed;
extern const long& cConfigScrollMaxSpeed;
extern const long& cConfigScrollRampPoint;
extern const long& cConfigScrollRampSpeed;
extern const long& cConfigScrollExp;

// Sticky reticle
extern const long& cConfigStickyReticle;
extern const long& cConfigStickyReticleSensitivity;
extern const long& cConfigStickyReticleFollow;
extern const long& cConfigStickyReticleJumpCamera;
extern const long& cConfigStickyReticleJumpCameraSpeed;
extern const long& cConfigStickyReticleJumpStep;
extern const long& cConfigStickyReticleJumpForwardMinSpeed;
extern const long& cConfigStickyReticleJumpForwardMaxSpeed;
extern const long& cConfigStickyReticleJumpForwardMinDist;
extern const long& cConfigStickyReticleJumpForwardMaxDist;
extern const long& cConfigStickyReticleJumpBackMinSpeed;
extern const long& cConfigStickyReticleJumpBackMaxSpeed;
extern const long& cConfigStickyReticleJumpBackMinDist;
extern const long& cConfigStickyReticleJumpBackMaxDist;
extern const long& cConfigStickyReticleJumpToAngleTolerance;
extern const long& cConfigStickyReticleJumpMaxSpeed;
extern const long& cConfigStickyReticleJumpMinSpeed;
extern const long& cConfigStickyReticleJumpMaxTime;
extern const long& cConfigStickyReticleJumpMinTime;
extern const long& cConfigStickyReticleJumpMinSearchScale;
extern const long& cConfigStickyReticleJumpMaxSearchScale;
extern const long& cConfigStickyReticleAverageSpeedInterval;

// Circle selection
extern const long& cConfigCircleSelection;
extern const long& cConfigCircleSelectSize;
extern const long& cConfigCircleSelectMaxSize;
extern const long& cConfigCircleSelectMaxResize;
extern const long& cConfigCircleSelectClickSize;
extern const long& cConfigCircleSelectDelayTime;
extern const long& cConfigCircleSelectRate;
extern const long& cConfigCircleSelectRateAccel;
extern const long& cConfigCircleSelectMaxRate;
extern const long& cConfigCircleSelectDrawMax;
extern const long& cConfigCircleSelectResizeRate;
extern const long& cConfigCircleSelectHover;
extern const long& cConfigCircleSelectFadeInAccel;
extern const long& cConfigCircleSelectEventSize;

// Camera
extern const long& cConfigCameraPitch;
extern const long& cConfigCameraYaw;
extern const long& cConfigCameraZoom;
extern const long& cConfigCameraFOV;
extern const long& cConfigCameraZoomRate;
extern const long& cConfigCameraZoomFastFactor;
extern const long& cConfigCameraZoomMin;
extern const long& cConfigCameraZoomMax;
extern const long& cConfigCameraRotateRate;
extern const long& cConfigCameraRotateFastFactor;
extern const long& cConfigCameraPitchRate;
extern const long& cConfigCameraPitchFastFactor;
extern const long& cConfigCameraPitchMin;
extern const long& cConfigCameraPitchMax;
extern const long& cConfigCameraPitchInvert;
extern const long& cConfigCameraFOVRate;
extern const long& cConfigCameraAutoZoomInstant;
extern const long& cConfigCameraAutoZoomOutRate;
extern const long& cConfigCameraAutoZoomOutFastFactor;
extern const long& cConfigCameraAutoZoomOutDelay;
extern const long& cConfigCameraAutoZoomInRate;
extern const long& cConfigCameraAutoZoomInFastFactor;
extern const long& cConfigCameraAutoZoomInDelay;
extern const long& cConfigCameraPitchOnZoom;
extern const long& cConfigCameraPitchOnZoomMin;
extern const long& cConfigCameraPitchOnZoomMax;
extern const long& cConfigCameraLimits;
extern const long& cConfigLockCameraToWorld;

// Reticle
extern const long& cConfigReticleOffset;
extern const long& cConfigReticleAdjustOnZoomMin;
extern const long& cConfigReticleAdjustOnZoomMax;

// Record Games
extern const long& cConfigRecordToCacheDrive;
extern const long& cConfigRecordingMaxSize;

// Debug
extern const long& cConfigDebugSelectionPicking;
extern const long& cConfigObstructionRenderMode;
extern const long& cConfigSpanListSpew;
extern const long& cConfigRenderPathingData;
extern const long& cConfigRenderSquadPlotter;
extern const long& cConfigDisplayCombatLog;
extern const long& cConfigNoExeCrc;
extern const long& cConfigRecordGameSync;
extern const long& cConfigDebugAttachmentRotation;
extern const long& cConfigDebugVisualSync;
extern const long& cConfigTechLog;
extern const long& cConfigDebugRenderShape;
extern const long& cConfigRenderTerrainSimRep;
extern const long& cConfigNoTerrainSkirt;
extern const long& cConfigRenderBattles;
extern const long& cConfigRenderAlerts;
extern const long& cConfigDebugProjectiles;
extern const long& cConfigAIDisable;
extern const long& cConfigAIShadow;
extern const long& cConfigAICamera;
extern const long& cConfigAIStrength;
extern const long& cConfigAIOrderLogging;
extern const long& cConfigAIUseSPCDefault;
extern const long& cConfigDisplayStatPage;
extern const long& cConfigDisplayRenderStats;
extern const long& cConfigDisableWow;
extern const long& cConfigNoCull;
extern const long& cConfigCircleDebugData;
extern const long& cConfigDebugStickyReticle;
extern const long& cConfigDebugPlatoonFormations;
extern const long& cConfigEnableMotionExtraction;
extern const long& cConfigEnableImpactLimits;
extern const long& cConfigWowRecord;
extern const long& cConfigWowPlay;
extern const long& cConfigWowDownsample;
extern const long& cConfigDrawHardpoints;
extern const long& cConfigReloadScenarioIgnoreTime;
extern const long& cConfigReloadScenarioWaitTime;
extern const long& cConfigClassicPlatoonGrouping;
extern const long& cConfigNoSquadSlowdown;
extern const long& cConfigPreemptSpeedFactor;
extern const long& cConfigDecUpdTraceStart;
extern const long& cConfigDecUpdTraceEnd;
extern const long& cConfigDecUpdTracePct;
extern const long& cConfigDecUpdTraceObj;
extern const long& cConfigSubUpdTrace;
extern const long& cConfigMinSimTime;
extern const long& cConfigWriteFPSLog;
extern const long& cConfigWriteMemLog;
extern const long& cConfigAutoCopyFPSLog;

// Vismap
extern const long& cConfigNoVismap;
extern const long& cConfigUseVismapFile;
extern const long& cConfigSaveVismapFile;

// Skirmish game
extern const long& cConfigDefaultMap;

// Sim
extern const long& cConfigAutoScenarioLoad;
extern const long& cConfigAutoScenarioPlayers;
extern const long& cConfigAutoScenarioSkirmish;
extern const long& cConfigAutoScenarioGameMode;
extern const long& cConfigAutoScenarioCoop;
extern const long& cConfigAutoRecordGameLoad;
extern const long& cConfigAutoSaveGameLoad;
extern const long& cConfigAutoSave;
extern const long& cConfigNoFogMask;
extern const long& cConfigNoVictoryCondition;
extern const long& cConfigNoDamage;
extern const long& cConfigNoShieldDamage;
extern const long& cConfigGameSpeed;
extern const long& cConfigPassThroughOwnVehicles;
extern const long& cConfigNoRandomPlayerPlacement;
extern const long& cConfigDisableOneBuilding;
extern const long& cConfigBuildingQueue;
extern const long& cConfigRecordGames;
extern const long& cConfigUseTestLeaders;
extern const long& cConfigEnableCapturePointResourceSharing;
extern const long& cConfigEnableFlight;
extern const long& cConfigEnableFPSLock;
extern const long& cConfigFPSLockRate;
extern const long& cConfigAINoAttack;
extern const long& cConfigNoBirthAnims;
extern const long& cConfigVeterancy;
extern const long& cConfigTrueLOS;
extern const long& cConfigDebugTrueLOS;
extern const long& cConfigNoDestruction;
extern const long& cConfigCoopSharedResources;
extern const long& cConfigCoopSharedPop;
extern const long& cConfigMaxProjectileHeightForDecal;
extern const long& cConfigEnableSubbreakage;
extern const long& cConfigEnableThrowPart;
extern const long& cConfigAllowAnimIsDirty;
extern const long& cConfigQuickBuild;
extern const long& cConfigOverrideLeaderPower;
extern const long& cConfigNoProtoObjectOptimization;
extern const long& cConfigEnableSubUpdating;
extern const long& cConfigMPSubUpdating;
extern const long& cConfigAlternateSubUpdating;
extern const long& cConfigDynamicSubUpdateTime;
extern const long& cConfigDecoupledUpdate;
extern const long& cConfigDecoupledRenderTime;
extern const long& cConfigDecoupledOutsideTimePercent;
extern const long& cConfigDecoupledOutsideTimeAmount;
extern const long& cConfigSkipDecoupledRenders;
extern const long& cConfigMaxSubUpdateTime;
extern const long& cConfigDynamicSubUpdating;
extern const long& cConfigSubUpdateAvgUpdOn;
extern const long& cConfigSubUpdateAvgUpdOff;
extern const long& cConfigMinSubUpdateEnabledTime;

// Floaty Text
extern const long& cConfigRenderFloatyTextXVelocity;
extern const long& cConfigRenderFloatyTextYVelocity;
extern const long& cConfigRenderFloatyTextDuration;
extern const long& cConfigRenderFloatyTextFadeOutTime;

// Powers
extern const long& cConfigTestPowersMode;

// UI
extern const long& cConfigDefaultOptions;
extern const long& cConfigDisableUI;
extern const long& cConfigDisableOldUITextures;
extern const long& cConfigNoHelpUI;
extern const long& cConfigBuildingMenuOnSelect;
extern const long& cConfigExitSubSelectOnCancel;
extern const long& cConfigExitTargetSelectOnScroll;
extern const long& cConfigDisableFlash;
extern const long& cConfigFlashGameUI;
extern const long& cConfigFlashMinimap;
extern const long& cConfigCrowdNeighborDistance;
extern const long& cConfigGotoItemTimeout;
extern const long& cConfigGotoBaseDistance;
extern const long& cConfigGotoSlideAwaySpeed;
extern const long& cConfigGotoSlideTowardsDistance;
extern const long& cConfigGotoSlideTime;
extern const long& cConfigGroupCreateTime;
extern const long& cConfigGroupGotoTime;
extern const long& cConfigShowUnavailIcons;
extern const long& cConfigBaseDecalIntensity;
extern const long& cConfigSelectionDecalIntensity;
extern const long& cConfigSubSelectDecalAlpha;
extern const long& cConfigShowScoreUI;
extern const long& cConfigNoHPBar;
extern const long& cConfigNoEnemySelectionDecals;
extern const long& cConfigEnableCampaign;
extern const long& cConfigPlayer1AI;
extern const long& cConfigAIAutoDifficulty;
extern const long& cConfigShowReticuleHelp;
extern const long& cConfigObjectivesDisplay;
extern const long& cConfigRenderStrength;
extern const long& cConfigNoWorldBorder;
extern const long& cConfigRenderCameraBoundaryLines;
extern const long& cConfigHUDAttackNotification;
extern const long& cConfigShowDebugMenu;
extern const long& cConfigShowFlashObjWidget;
extern const long& cConfigShowFlashChat;
extern const long& cConfigShowPostGame;
extern const long& cConfigShowDevMaps;
extern const long& cConfigEnableBackgroundPlayer;
extern const long& cConfigNoIntroCinematics;
extern const long& cConfigIntroCinematic;
extern const long& cConfigMinIntroTime;
extern const long& cConfigUIBackgroundMovie;
extern const long& cConfigCreditsMovie;
extern const long& cConfigCreditsSubtitles;
extern const long& cConfigUIBackgroundMovieMain;
extern const long& cConfigUITimelineScreenMovie;
extern const long& cConfigUIBackgroundImg;
extern const long& cConfigUIBackgroundMovieCampaign;
extern const long& cConfigNoUIDebug;
extern const long& cConfigSplitScreen;
extern const long& cConfigVerticalSplit;
extern const long& cConfigShowCampaignPostGame;
extern const long& cConfigDisableAllChats;
extern const long& cConfigNewPartyMode;
extern const long& cConfigAiInMP;
extern const long& cConfigSplitScreenInMP;
extern const long& cConfigDebugCircleSelect;
extern const long& cConfigShowCommands;
extern const long& cConfigCircleSelectIntensity;
extern const long& cConfigCircleSelectSizeFactor;
extern const long& cConfigCircleSelectOffset;
extern const long& cConfigHoverDecalIntensity;
extern const long& cConfigHoverDecalSize;
extern const long& cConfigHoverDecalOffset;
extern const long& cConfigShowGameTime;
extern const long& cConfigCircleMenuBlurFactor;
extern const long& cConfigCircleMenuBlurSaturation;
extern const long& cConfigCircleMenuBlurDuration;
extern const long& cConfigSelectAllIgnoreWorking;
extern const long& cConfigObjectives2;
extern const long& cConfigRecoverEffectOffset;
extern const long& cConfigBillboardRecoverEffect;
extern const long& cConfigNoSelectionHighlight;
extern const long& cConfigRenderGameStateMessages;

extern const long& cConfigNoUpdatePathingQuad;

extern const long& cConfigForceAuth;

extern const long& cConfigUseLAN;
extern const long& cConfigPlayerMenu;
extern const long& cConfigUINotReadyStuff;
extern const long& cConfigEnableAttractMode;
extern const long& cConfigRejoinParty;
extern const long& cConfigAllowSplitScreenMP;
extern const long& cConfigShowSafeArea;
extern const long& cConfigUseGraphTestData;

extern const long& cConfigShowESRBNotice;

#ifndef BUILD_FINAL
extern const long& cConfigLocIdInString;
extern const long& cConfigTestFonts;
#endif

extern const long& cConfigLocaleLanguage;

extern const long& cConfigDisableCinematicBars;
extern const long& cConfigGameStartFadeTime;
extern const long& cConfigGameStartFadeDelay;

// VINCE
#if defined( _VINCE_ )
// game
extern const long& cConfigVinceUnitInterval;
extern const long& cConfigVinceBuildingInterval;
extern const long& cConfigVinceResourceInterval;
extern const long& cConfigVincePathingInterval;

// user
extern const long& cConfigVinceEnableLog;
#endif

// Demo
extern const long& cConfigDemo;
extern const long& cConfigDemo2;
extern const long& cConfigTutorialMap;
extern const long& cConfigDemoMovie;

// Triggers
extern const long& cEnableTriggerDebugs;
extern const long& cEnableTriggerDebugsStatusText;

//Hint system
extern const long& cEnableHintSystem;
extern const long& cHintSystemResetProfile;


// Campaign
extern const long& cConfigUseDesignFolder;

//Sim debugging.
#ifndef BUILD_FINAL
extern const long& cConfigRenderGrouper;
extern const long& cConfigRenderSimDebug;
extern const long& cConfigRenderSimDebugNoBoxes;
extern const long& cConfigRenderSimDebugSquadCircles;
extern const long& cConfigStartPauseButton;
extern const long& cConfigRemakePlatoonFormations;
extern const long& cConfigRemakeSquadFormations;
extern const long& cConfigPauseOnUpdate;
extern const long& cConfigDebugDodge;
extern const long& cConfigRenderPathingQuad;
extern const long& cConfigDebugMinigame;
extern const long& cConfigPIXTraceWorldUpdate;
extern const long& cConfigPIXTraceWorldLoad;
extern const long& cConfigTimingAlertsOFF;
extern const long& cConfigTimingAlertsThresh;
extern const long& cConfigEnableTriggerPerfAsserts;
extern const long& cConfigDisableTalkingHeads;
extern const long& cConfigDebugPhysicsImpulseTags;
extern const long& cConfigDebugIK;
extern const long& cConfigRenderSquadState;
extern const long& cConfigRenderPredictedPaths;
extern const long& cConfigRenderLineToFormationPos;
extern const long& cConfigRenderInterpolationPercent;
#endif

//Sim.
extern const long& cConfigSlaveUnitPosition;
extern const long& cConfigTurning;
extern const long& cConfigHumanAttackMove;
extern const long& cConfigPlatoonRadius;
extern const long& cConfigProjectionTime;
extern const long& cConfigMoreNewMovement3;
extern const long& cConfigSyncSimAndRenderFrames;
extern const long& cConfigOverrideGroundIK;
extern const long& cConfigOverrideGroundIKRange;
extern const long& cConfigOverrideGroundIKTiltFactor;
extern const long& cConfigDriveWarthog;
extern const long& cConfigDisplayWarthogVelocity;
extern const long& cConfigEnableCorpses;
extern const long& cConfigBowling;
extern const long& cConfigDisablePathingLimits;
extern const long& cConfigDisableVelocityMatchingBySquadType;
extern const long& cConfigActiveAbilities;
extern const long& cConfigExplorationRegionShowTime;
extern const long& cConfigDisableTrails;
extern const long& cConfigPercentFadeTimeCorpseSink;
extern const long& cConfigCorpseSinkSpeed;
extern const long& cConfigCorpseMinScale;
extern const long& cConfigBlockOutsideBounds;
extern const long& cConfigIgnoreAllPlatoonmates;
extern const long& cConfigDisplayUnjoinMaxTeleDist;

//Achievements
extern const long& cConfigFakeAchievements;

// Flash Fonts
extern const long& cConfigEnableFlashFonts;

// Game Stats
extern const long& cConfigEnableGameStats;
extern const long& cConfigServiceRecordShowAll;
#ifndef BUILD_FINAL
extern const long& cConfigForceGameStats;
#endif

// XLSP
extern const long& cConfigLSPServerFilter;
extern const long& cConfigLSPServiceID;
extern const long& cConfigLSPEnableAuth;
extern const long& cConfigLSPEnableFileUpload;
extern const long& cConfigLSPAuthPort;
extern const long& cConfigLSPEnableTicker;
extern const long& cConfigLSPTickerPort;
extern const long& cConfigLSPFileUploadPort;
extern const long& cConfigLSPEnableMediaTransfer;
extern const long& cConfigLSPMediaTransferPort;
extern const long& cConfigLSPEnableConfigData;
extern const long& cConfigLSPConfigDataPort;
extern const long& cConfigLSPEnablePerfReporting;
extern const long& cConfigLSPPerfReportingPort;
extern const long& cConfigLSPEnableServiceRecord;
extern const long& cConfigLSPServiceRecordPort;
extern const long& cConfigLSPAuthServiceID;
extern const long& cConfigLSPTickerServiceID;
extern const long& cConfigLSPConfigServiceID;
extern const long& cConfigLSPPerfServiceID;
extern const long& cConfigLSPStatsServiceID;
extern const long& cConfigLSPMediaServiceID;
extern const long& cConfigLSPDebugServiceID;
extern const long& cConfigLSPServiceRecordServiceID;
extern const long& cConfigLSPDefaultAuthTTL;
extern const long& cConfigLSPDefaultConfigTTL;
extern const long& cConfigLSPDefaultMediaTTL;
extern const long& cConfigLSPDefaultServiceRecordTTL;
extern const long& cConfigLSPDefaultTCPTimeout;
extern const long& cConfigLSPDefaultSGTimeout;
extern const long& cConfigLSPDefaultSGConnectTimeout;
//extern const long& cConfigLSPDefaultTCPReconnects;
extern const long& cConfigLSPDefaultSGTTL;
extern const long& cConfigLSPDefaultSGFailTTL;
extern const long& cConfigLSPConfigDataPacketVersion2;

#ifndef BUILD_FINAL
extern const long& cConfigDisableObscuredUnits;
#endif

#ifndef BUILD_FINAL
extern const long& cConfigForceSPCColors;
#endif

// Pather
#ifndef BUILD_FINAL 
extern const long& cConfigShowPaths;
extern const long& cConfigDebugPather;
extern const long& cConfigDebugSpecificSquad;
//extern const long& cConfigDebugSpecificUnit;
//extern const long& cConfigDebugSpecificUnitGroup;
//extern const long& cConfigTrackPathing;
//extern const long& cConfigRenderTrackedPathing;
//extern const long& cConfigRenderTrackedPathingUpdate;
//extern const long& cConfigRenderTrackedPathingWaypoints;
//extern const long& cConfigRenderTrackedPathingTiming;
#endif

// Temp
extern const long& cConfigNoCovenantArchive;
extern const long& cConfigAsyncWorldUpdate;
extern const long& cConfigNoArchiveTexture;

// Options
extern const long& cConfigDefaultUserZoom;
extern const long& cConfigScrollUserSpeedAdjustment;
extern const long& cConfigSelectionSpeed;
extern const long& cConfigUserGamma;
extern const long& cConfigFriendOrFoe;
extern const long& cConfigSubTitles;

extern const long& cConfigRecomputeFatalityData;
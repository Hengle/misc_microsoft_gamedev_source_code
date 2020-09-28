//==============================================================================
// configsgame.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "configsgame.h"
#include "world.h"
#include "player.h"

//==============================================================================
// Defines
//==============================================================================

// Commands
DEFINE_CONFIG(cConfigDontMeterWorkCommands);

// Multiplayer syncing
DEFINE_CONFIG(cConfigNoSync);
DEFINE_CONFIG(cConfigDontGoOOS);
DEFINE_CONFIG(cConfigSyncUpdateInterval);
DEFINE_CONFIG(cConfigFullAllSync);
DEFINE_CONFIG(cConfigDisableAllSync);
DEFINE_CONFIG(cConfigXORAllSync);
DEFINE_CONFIG(cConfigRandSync);
DEFINE_CONFIG(cConfigPlayerSync);
DEFINE_CONFIG(cConfigTeamSync);
DEFINE_CONFIG(cConfigUnitGroupSync);
DEFINE_CONFIG(cConfigUnitSync);
DEFINE_CONFIG(cConfigUnitDetailSync);
DEFINE_CONFIG(cConfigUnitActionSync);
DEFINE_CONFIG(cConfigSquadSync);
DEFINE_CONFIG(cConfigProjectileSync);
DEFINE_CONFIG(cConfigWorldSync);
DEFINE_CONFIG(cConfigTechSync);
DEFINE_CONFIG(cConfigCommandSync);
DEFINE_CONFIG(cConfigPathingSync);
DEFINE_CONFIG(cConfigMovementSync);
DEFINE_CONFIG(cConfigVisibilitySync);
DEFINE_CONFIG(cConfigFinalReleaseSync);
DEFINE_CONFIG(cConfigFinalDetailSync);
DEFINE_CONFIG(cConfigChecksumSync);
DEFINE_CONFIG(cConfigTriggerSync);
DEFINE_CONFIG(cConfigTriggerVarSync);
DEFINE_CONFIG(cConfigAnimSync);
DEFINE_CONFIG(cConfigDoppleSync);
DEFINE_CONFIG(cConfigCommSync);
DEFINE_CONFIG(cConfigPlatoonSync);

// Comm logs
DEFINE_CONFIG(cConfigCommLogging);
DEFINE_CONFIG(cConfigCommLogHistorySize);
DEFINE_CONFIG(cConfigSessionCL);
DEFINE_CONFIG(cConfigTransportCL);
DEFINE_CONFIG(cConfigTimeSyncCL);
DEFINE_CONFIG(cConfigGameTimingCL);
DEFINE_CONFIG(cConfigTimingCL);
DEFINE_CONFIG(cConfigSyncCL);
DEFINE_CONFIG(cConfigLowLevelSyncCL);
DEFINE_CONFIG(cConfigBroadcasterCL);
DEFINE_CONFIG(cConfigReceiverCL);
DEFINE_CONFIG(cConfigConnectivityCL);
DEFINE_CONFIG(cConfigBandwidthCL);
DEFINE_CONFIG(cConfigPerfCL);
DEFINE_CONFIG(cConfigMPGameCL);
DEFINE_CONFIG(cConfigMPGameSettingsCL);
DEFINE_CONFIG(cConfigReliableConnCL);
DEFINE_CONFIG(cConfigMPStorageCL);
DEFINE_CONFIG(cConfigMPMatchmakingCL);
DEFINE_CONFIG(cConfigSessionConnAddrCL);
DEFINE_CONFIG(cConfigSessionConnJoinCL);
DEFINE_CONFIG(cConfigFileTransferCL);

// Gamepad scrolling
DEFINE_CONFIG(cConfigDelayScrollingAngle);
DEFINE_CONFIG(cConfigDelayScrollingTime);
DEFINE_CONFIG(cConfigScrollRateBase);
DEFINE_CONFIG(cConfigScrollRateGrow);
DEFINE_CONFIG(cConfigScrollFastSpeed);
DEFINE_CONFIG(cConfigScrollMaxSpeed);
DEFINE_CONFIG(cConfigScrollRampPoint);
DEFINE_CONFIG(cConfigScrollRampSpeed);
DEFINE_CONFIG(cConfigScrollExp);

// Sticky reticle
DEFINE_CONFIG(cConfigStickyReticle);
DEFINE_CONFIG(cConfigStickyReticleSensitivity);
DEFINE_CONFIG(cConfigStickyReticleFollow);
DEFINE_CONFIG(cConfigStickyReticleJumpCamera);
DEFINE_CONFIG(cConfigStickyReticleJumpCameraSpeed);
DEFINE_CONFIG(cConfigStickyReticleJumpStep);
DEFINE_CONFIG(cConfigStickyReticleJumpForwardMinSpeed);
DEFINE_CONFIG(cConfigStickyReticleJumpForwardMaxSpeed);
DEFINE_CONFIG(cConfigStickyReticleJumpForwardMinDist);
DEFINE_CONFIG(cConfigStickyReticleJumpForwardMaxDist);
DEFINE_CONFIG(cConfigStickyReticleJumpBackMinSpeed);
DEFINE_CONFIG(cConfigStickyReticleJumpBackMaxSpeed);
DEFINE_CONFIG(cConfigStickyReticleJumpBackMinDist);
DEFINE_CONFIG(cConfigStickyReticleJumpBackMaxDist);
DEFINE_CONFIG(cConfigStickyReticleJumpToAngleTolerance);
DEFINE_CONFIG(cConfigStickyReticleJumpMaxSpeed);
DEFINE_CONFIG(cConfigStickyReticleJumpMinSpeed);
DEFINE_CONFIG(cConfigStickyReticleJumpMaxTime);
DEFINE_CONFIG(cConfigStickyReticleJumpMinTime);
DEFINE_CONFIG(cConfigStickyReticleJumpMinSearchScale);
DEFINE_CONFIG(cConfigStickyReticleJumpMaxSearchScale);
DEFINE_CONFIG(cConfigStickyReticleAverageSpeedInterval);

// Circle selection
DEFINE_CONFIG(cConfigCircleSelection);
DEFINE_CONFIG(cConfigCircleSelectSize);
DEFINE_CONFIG(cConfigCircleSelectMaxSize);
DEFINE_CONFIG(cConfigCircleSelectMaxResize);
DEFINE_CONFIG(cConfigCircleSelectClickSize);
DEFINE_CONFIG(cConfigCircleSelectDelayTime);
DEFINE_CONFIG(cConfigCircleSelectRate);
DEFINE_CONFIG(cConfigCircleSelectRateAccel);
DEFINE_CONFIG(cConfigCircleSelectMaxRate);
DEFINE_CONFIG(cConfigCircleSelectDrawMax);
DEFINE_CONFIG(cConfigCircleSelectResizeRate);
DEFINE_CONFIG(cConfigCircleSelectHover);
DEFINE_CONFIG(cConfigCircleSelectFadeInAccel);
DEFINE_CONFIG(cConfigCircleSelectEventSize);

// Camera
DEFINE_CONFIG(cConfigCameraPitch);
DEFINE_CONFIG(cConfigCameraYaw);
DEFINE_CONFIG(cConfigCameraZoom);
DEFINE_CONFIG(cConfigCameraFOV);
DEFINE_CONFIG(cConfigCameraZoomRate);
DEFINE_CONFIG(cConfigCameraZoomFastFactor);
DEFINE_CONFIG(cConfigCameraZoomMin);
DEFINE_CONFIG(cConfigCameraZoomMax);
DEFINE_CONFIG(cConfigCameraRotateRate);
DEFINE_CONFIG(cConfigCameraRotateFastFactor);
DEFINE_CONFIG(cConfigCameraPitchRate);
DEFINE_CONFIG(cConfigCameraPitchFastFactor);
DEFINE_CONFIG(cConfigCameraPitchMin);
DEFINE_CONFIG(cConfigCameraPitchMax);
DEFINE_CONFIG(cConfigCameraPitchInvert);
DEFINE_CONFIG(cConfigCameraFOVRate);
DEFINE_CONFIG(cConfigCameraAutoZoomInstant);
DEFINE_CONFIG(cConfigCameraAutoZoomOutRate);
DEFINE_CONFIG(cConfigCameraAutoZoomOutFastFactor);
DEFINE_CONFIG(cConfigCameraAutoZoomOutDelay);
DEFINE_CONFIG(cConfigCameraAutoZoomInRate);
DEFINE_CONFIG(cConfigCameraAutoZoomInFastFactor);
DEFINE_CONFIG(cConfigCameraAutoZoomInDelay);
DEFINE_CONFIG(cConfigCameraPitchOnZoom);
DEFINE_CONFIG(cConfigCameraPitchOnZoomMin);
DEFINE_CONFIG(cConfigCameraPitchOnZoomMax);
DEFINE_CONFIG(cConfigCameraLimits);
DEFINE_CONFIG(cConfigLockCameraToWorld);

// Reticle
DEFINE_CONFIG(cConfigReticleOffset);
DEFINE_CONFIG(cConfigReticleAdjustOnZoomMin);
DEFINE_CONFIG(cConfigReticleAdjustOnZoomMax);

// Record Games
DEFINE_CONFIG(cConfigRecordToCacheDrive);
DEFINE_CONFIG(cConfigRecordingMaxSize);

// Debug
DEFINE_CONFIG(cConfigDebugSelectionPicking);
DEFINE_CONFIG(cConfigObstructionRenderMode);
DEFINE_CONFIG(cConfigSpanListSpew);
DEFINE_CONFIG(cConfigRenderPathingData);
DEFINE_CONFIG(cConfigRenderSquadPlotter);
DEFINE_CONFIG(cConfigDisplayCombatLog);
DEFINE_CONFIG(cConfigNoExeCrc);
DEFINE_CONFIG(cConfigRecordGameSync);
DEFINE_CONFIG(cConfigDebugAttachmentRotation);
DEFINE_CONFIG(cConfigDebugVisualSync);
DEFINE_CONFIG(cConfigDebugRenderShape);
DEFINE_CONFIG(cConfigTechLog);
DEFINE_CONFIG(cConfigRenderTerrainSimRep);
DEFINE_CONFIG(cConfigNoTerrainSkirt);
DEFINE_CONFIG(cConfigRenderBattles);
DEFINE_CONFIG(cConfigRenderAlerts);
DEFINE_CONFIG(cConfigDebugProjectiles);
DEFINE_CONFIG(cConfigAIDisable);
DEFINE_CONFIG(cConfigAIShadow);
DEFINE_CONFIG(cConfigAICamera);
DEFINE_CONFIG(cConfigAIStrength);
DEFINE_CONFIG(cConfigAIOrderLogging);
DEFINE_CONFIG(cConfigAIUseSPCDefault);
DEFINE_CONFIG(cConfigDisplayStatPage);
DEFINE_CONFIG(cConfigDisplayRenderStats);
DEFINE_CONFIG(cConfigDisableWow);
DEFINE_CONFIG(cConfigNoCull);
DEFINE_CONFIG(cConfigCircleDebugData);
DEFINE_CONFIG(cConfigDebugStickyReticle);
DEFINE_CONFIG(cConfigDebugPlatoonFormations);
DEFINE_CONFIG(cConfigEnableMotionExtraction);
DEFINE_CONFIG(cConfigEnableImpactLimits);
DEFINE_CONFIG(cConfigWowRecord);
DEFINE_CONFIG(cConfigWowPlay);
DEFINE_CONFIG(cConfigWowDownsample);
DEFINE_CONFIG(cConfigDrawHardpoints);
DEFINE_CONFIG(cConfigReloadScenarioIgnoreTime);
DEFINE_CONFIG(cConfigReloadScenarioWaitTime);
DEFINE_CONFIG(cConfigClassicPlatoonGrouping);
DEFINE_CONFIG(cConfigNoSquadSlowdown);
DEFINE_CONFIG(cConfigPreemptSpeedFactor);
DEFINE_CONFIG(cConfigDecUpdTraceStart);
DEFINE_CONFIG(cConfigDecUpdTraceEnd);
DEFINE_CONFIG(cConfigDecUpdTracePct);
DEFINE_CONFIG(cConfigDecUpdTraceObj);
DEFINE_CONFIG(cConfigSubUpdTrace);
DEFINE_CONFIG(cConfigMinSimTime);
DEFINE_CONFIG(cConfigWriteFPSLog);
DEFINE_CONFIG(cConfigWriteMemLog);
DEFINE_CONFIG(cConfigAutoCopyFPSLog);



// Vismap
DEFINE_CONFIG(cConfigNoVismap);
DEFINE_CONFIG(cConfigUseVismapFile);
DEFINE_CONFIG(cConfigSaveVismapFile);

// Skirmish game
DEFINE_CONFIG(cConfigDefaultMap);

// Sim
DEFINE_CONFIG(cConfigAutoScenarioLoad);
DEFINE_CONFIG(cConfigAutoScenarioPlayers);
DEFINE_CONFIG(cConfigAutoScenarioSkirmish);
DEFINE_CONFIG(cConfigAutoScenarioGameMode);
DEFINE_CONFIG(cConfigAutoScenarioCoop);
DEFINE_CONFIG(cConfigAutoRecordGameLoad);
DEFINE_CONFIG(cConfigAutoSaveGameLoad);
DEFINE_CONFIG(cConfigAutoSave);
DEFINE_CONFIG(cConfigNoFogMask);
DEFINE_CONFIG(cConfigNoVictoryCondition);
DEFINE_CONFIG(cConfigNoDamage);
DEFINE_CONFIG(cConfigNoShieldDamage);
DEFINE_CONFIG(cConfigGameSpeed);
DEFINE_CONFIG(cConfigPassThroughOwnVehicles);
DEFINE_CONFIG(cConfigNoRandomPlayerPlacement);
DEFINE_CONFIG(cConfigDisableOneBuilding);
DEFINE_CONFIG(cConfigBuildingQueue);
DEFINE_CONFIG(cConfigRecordGames);
DEFINE_CONFIG(cConfigUseTestLeaders);
DEFINE_CONFIG(cConfigEnableCapturePointResourceSharing);
DEFINE_CONFIG(cConfigEnableFlight);
DEFINE_CONFIG(cConfigEnableFPSLock);
DEFINE_CONFIG(cConfigFPSLockRate);
DEFINE_CONFIG(cConfigAINoAttack);
DEFINE_CONFIG(cConfigNoBirthAnims);
DEFINE_CONFIG(cConfigVeterancy);
DEFINE_CONFIG(cConfigTrueLOS);
DEFINE_CONFIG(cConfigDebugTrueLOS);
DEFINE_CONFIG(cConfigNoDestruction);
DEFINE_CONFIG(cConfigCoopSharedResources);
DEFINE_CONFIG(cConfigCoopSharedPop);
DEFINE_CONFIG(cConfigMaxProjectileHeightForDecal);
DEFINE_CONFIG(cConfigEnableSubbreakage);
DEFINE_CONFIG(cConfigEnableThrowPart);
DEFINE_CONFIG(cConfigAllowAnimIsDirty);
DEFINE_CONFIG(cConfigQuickBuild);
DEFINE_CONFIG(cConfigOverrideLeaderPower);
DEFINE_CONFIG(cConfigNoProtoObjectOptimization);
DEFINE_CONFIG(cConfigEnableSubUpdating);
DEFINE_CONFIG(cConfigMPSubUpdating);
DEFINE_CONFIG(cConfigAlternateSubUpdating);
DEFINE_CONFIG(cConfigDynamicSubUpdateTime);
DEFINE_CONFIG(cConfigDecoupledUpdate);
DEFINE_CONFIG(cConfigDecoupledRenderTime);
DEFINE_CONFIG(cConfigDecoupledOutsideTimePercent);
DEFINE_CONFIG(cConfigDecoupledOutsideTimeAmount);
DEFINE_CONFIG(cConfigSkipDecoupledRenders);
DEFINE_CONFIG(cConfigMaxSubUpdateTime);
DEFINE_CONFIG(cConfigDynamicSubUpdating);
DEFINE_CONFIG(cConfigSubUpdateAvgUpdOn);
DEFINE_CONFIG(cConfigSubUpdateAvgUpdOff);
DEFINE_CONFIG(cConfigMinSubUpdateEnabledTime);

// Floaty Text
DEFINE_CONFIG(cConfigRenderFloatyTextXVelocity);
DEFINE_CONFIG(cConfigRenderFloatyTextYVelocity);
DEFINE_CONFIG(cConfigRenderFloatyTextDuration);
DEFINE_CONFIG(cConfigRenderFloatyTextFadeOutTime);

// Powers
DEFINE_CONFIG(cConfigTestPowersMode);

// UI
DEFINE_CONFIG(cConfigDefaultOptions);
DEFINE_CONFIG(cConfigDisableUI);
DEFINE_CONFIG(cConfigDisableOldUITextures);
DEFINE_CONFIG(cConfigNoHelpUI);
DEFINE_CONFIG(cConfigBuildingMenuOnSelect);
DEFINE_CONFIG(cConfigExitSubSelectOnCancel);
DEFINE_CONFIG(cConfigExitTargetSelectOnScroll);
DEFINE_CONFIG(cConfigDisableFlash);
DEFINE_CONFIG(cConfigFlashGameUI);
DEFINE_CONFIG(cConfigFlashMinimap);
DEFINE_CONFIG(cConfigCrowdNeighborDistance);
DEFINE_CONFIG(cConfigGotoItemTimeout);
DEFINE_CONFIG(cConfigGotoBaseDistance);
DEFINE_CONFIG(cConfigGotoSlideAwaySpeed);
DEFINE_CONFIG(cConfigGotoSlideTowardsDistance);
DEFINE_CONFIG(cConfigGotoSlideTime);
DEFINE_CONFIG(cConfigGroupCreateTime);
DEFINE_CONFIG(cConfigGroupGotoTime);
DEFINE_CONFIG(cConfigShowUnavailIcons);
DEFINE_CONFIG(cConfigBaseDecalIntensity);
DEFINE_CONFIG(cConfigSelectionDecalIntensity);
DEFINE_CONFIG(cConfigSubSelectDecalAlpha);
DEFINE_CONFIG(cConfigShowScoreUI);
DEFINE_CONFIG(cConfigNoHPBar);
DEFINE_CONFIG(cConfigNoEnemySelectionDecals);
DEFINE_CONFIG(cConfigEnableCampaign);
DEFINE_CONFIG(cConfigPlayer1AI);
DEFINE_CONFIG(cConfigAIAutoDifficulty);
DEFINE_CONFIG(cConfigShowReticuleHelp);
DEFINE_CONFIG(cConfigObjectivesDisplay);
DEFINE_CONFIG(cConfigRenderStrength);
DEFINE_CONFIG(cConfigNoWorldBorder);
DEFINE_CONFIG(cConfigRenderCameraBoundaryLines);
DEFINE_CONFIG(cConfigHUDAttackNotification);
DEFINE_CONFIG(cConfigShowDebugMenu);
DEFINE_CONFIG(cConfigShowFlashObjWidget);
DEFINE_CONFIG(cConfigShowFlashChat);
DEFINE_CONFIG(cConfigShowPostGame);
DEFINE_CONFIG(cConfigShowDevMaps);
DEFINE_CONFIG(cConfigEnableBackgroundPlayer);
DEFINE_CONFIG(cConfigNoIntroCinematics);
DEFINE_CONFIG(cConfigIntroCinematic);
DEFINE_CONFIG(cConfigMinIntroTime);
DEFINE_CONFIG(cConfigUIBackgroundMovie);
DEFINE_CONFIG(cConfigCreditsMovie);
DEFINE_CONFIG(cConfigCreditsSubtitles);
DEFINE_CONFIG(cConfigUIBackgroundMovieMain);
DEFINE_CONFIG(cConfigUITimelineScreenMovie);
DEFINE_CONFIG(cConfigUIBackgroundImg);
DEFINE_CONFIG(cConfigUIBackgroundMovieCampaign);
DEFINE_CONFIG(cConfigNoUIDebug);
DEFINE_CONFIG(cConfigSplitScreen);
DEFINE_CONFIG(cConfigVerticalSplit);
DEFINE_CONFIG(cConfigShowCampaignPostGame);
DEFINE_CONFIG(cConfigDisableAllChats);
DEFINE_CONFIG(cConfigNewPartyMode);
DEFINE_CONFIG(cConfigAiInMP);
DEFINE_CONFIG(cConfigSplitScreenInMP)
DEFINE_CONFIG(cConfigDebugCircleSelect);
DEFINE_CONFIG(cConfigShowCommands);
DEFINE_CONFIG(cConfigCircleSelectIntensity);
DEFINE_CONFIG(cConfigCircleSelectSizeFactor);
DEFINE_CONFIG(cConfigCircleSelectOffset);
DEFINE_CONFIG(cConfigHoverDecalIntensity);
DEFINE_CONFIG(cConfigHoverDecalSize);
DEFINE_CONFIG(cConfigHoverDecalOffset);
DEFINE_CONFIG(cConfigShowGameTime);
DEFINE_CONFIG(cConfigCircleMenuBlurFactor);
DEFINE_CONFIG(cConfigCircleMenuBlurSaturation);
DEFINE_CONFIG(cConfigCircleMenuBlurDuration);
DEFINE_CONFIG(cConfigSelectAllIgnoreWorking);
DEFINE_CONFIG(cConfigObjectives2);
DEFINE_CONFIG(cConfigRecoverEffectOffset);
DEFINE_CONFIG(cConfigBillboardRecoverEffect);
DEFINE_CONFIG(cConfigNoSelectionHighlight);
DEFINE_CONFIG(cConfigRenderGameStateMessages);

DEFINE_CONFIG(cConfigNoUpdatePathingQuad);

DEFINE_CONFIG(cConfigForceAuth);

DEFINE_CONFIG(cConfigUseLAN);
DEFINE_CONFIG(cConfigPlayerMenu);
DEFINE_CONFIG(cConfigRejoinParty);
DEFINE_CONFIG(cConfigAllowSplitScreenMP);

DEFINE_CONFIG(cConfigUINotReadyStuff);
DEFINE_CONFIG(cConfigEnableAttractMode);


#ifndef BUILD_FINAL
DEFINE_CONFIG(cConfigShowSafeArea);
DEFINE_CONFIG(cConfigLocIdInString);
DEFINE_CONFIG(cConfigTestFonts);
DEFINE_CONFIG(cConfigUseGraphTestData);
#endif

DEFINE_CONFIG(cConfigShowESRBNotice);

DEFINE_CONFIG(cConfigLocaleLanguage);

DEFINE_CONFIG(cConfigDisableCinematicBars);
DEFINE_CONFIG(cConfigGameStartFadeTime);
DEFINE_CONFIG(cConfigGameStartFadeDelay);

// VINCE
#if defined( _VINCE_ )
DEFINE_CONFIG( cConfigVinceUnitInterval );
DEFINE_CONFIG( cConfigVinceBuildingInterval );
DEFINE_CONFIG( cConfigVinceResourceInterval );
DEFINE_CONFIG( cConfigVincePathingInterval );

DEFINE_CONFIG( cConfigVinceEnableLog );
#endif

// Demo
DEFINE_CONFIG(cConfigDemo);
DEFINE_CONFIG(cConfigDemo2);
DEFINE_CONFIG(cConfigTutorialMap);
DEFINE_CONFIG(cConfigDemoMovie);

// Triggers
DEFINE_CONFIG( cEnableTriggerDebugs );
DEFINE_CONFIG( cEnableTriggerDebugsStatusText );

//Hints
DEFINE_CONFIG( cEnableHintSystem );
DEFINE_CONFIG( cHintSystemResetProfile );

// Campaign
#if !defined(BUILD_FINAL)
   DEFINE_CONFIG(cConfigUseDesignFolder);
#endif

//Sim Debugging.
#ifndef BUILD_FINAL
DEFINE_CONFIG(cConfigRenderGrouper);
DEFINE_CONFIG(cConfigRenderSimDebug);
DEFINE_CONFIG(cConfigRenderSimDebugNoBoxes);
DEFINE_CONFIG(cConfigRenderSimDebugSquadCircles);
DEFINE_CONFIG(cConfigStartPauseButton);
DEFINE_CONFIG(cConfigRemakePlatoonFormations);
DEFINE_CONFIG(cConfigRemakeSquadFormations);
DEFINE_CONFIG(cConfigPauseOnUpdate);
DEFINE_CONFIG(cConfigDebugDodge);
DEFINE_CONFIG(cConfigRenderPathingQuad);
DEFINE_CONFIG(cConfigDebugMinigame);
DEFINE_CONFIG(cConfigPIXTraceWorldUpdate);
DEFINE_CONFIG(cConfigPIXTraceWorldLoad);
DEFINE_CONFIG(cConfigTimingAlertsOFF);
DEFINE_CONFIG(cConfigTimingAlertsThresh);
DEFINE_CONFIG(cConfigEnableTriggerPerfAsserts);
DEFINE_CONFIG(cConfigDisableTalkingHeads);
DEFINE_CONFIG(cConfigDebugPhysicsImpulseTags);
DEFINE_CONFIG(cConfigDebugIK);
DEFINE_CONFIG(cConfigRenderSquadState);
DEFINE_CONFIG(cConfigRenderPredictedPaths);
DEFINE_CONFIG(cConfigRenderLineToFormationPos);
DEFINE_CONFIG(cConfigRenderInterpolationPercent);
#endif

//Sim.
DEFINE_CONFIG(cConfigSlaveUnitPosition);
DEFINE_CONFIG(cConfigTurning);
DEFINE_CONFIG(cConfigHumanAttackMove);
DEFINE_CONFIG(cConfigPlatoonRadius);
DEFINE_CONFIG(cConfigProjectionTime);
DEFINE_CONFIG(cConfigMoreNewMovement3);
DEFINE_CONFIG(cConfigSyncSimAndRenderFrames);
DEFINE_CONFIG(cConfigOverrideGroundIK);
DEFINE_CONFIG(cConfigOverrideGroundIKRange);
DEFINE_CONFIG(cConfigOverrideGroundIKTiltFactor);
DEFINE_CONFIG(cConfigDriveWarthog);
DEFINE_CONFIG(cConfigDisplayWarthogVelocity);
DEFINE_CONFIG(cConfigEnableCorpses);
DEFINE_CONFIG(cConfigBowling);
DEFINE_CONFIG(cConfigDisablePathingLimits);
DEFINE_CONFIG(cConfigDisableVelocityMatchingBySquadType);
DEFINE_CONFIG(cConfigActiveAbilities);
DEFINE_CONFIG(cConfigExplorationRegionShowTime);
DEFINE_CONFIG(cConfigDisableTrails);
DEFINE_CONFIG(cConfigPercentFadeTimeCorpseSink);
DEFINE_CONFIG(cConfigCorpseSinkSpeed);
DEFINE_CONFIG(cConfigCorpseMinScale);
DEFINE_CONFIG(cConfigBlockOutsideBounds);
DEFINE_CONFIG(cConfigIgnoreAllPlatoonmates);
DEFINE_CONFIG(cConfigDisplayUnjoinMaxTeleDist);

//Achievements
DEFINE_CONFIG(cConfigFakeAchievements);

// Flash font localization code
DEFINE_CONFIG(cConfigEnableFlashFonts);

// Game Stats
DEFINE_CONFIG(cConfigEnableGameStats);
DEFINE_CONFIG(cConfigServiceRecordShowAll);
#ifndef BUILD_FINAL
DEFINE_CONFIG(cConfigForceGameStats);
#endif

// XLSP
DEFINE_CONFIG(cConfigLSPServerFilter);
DEFINE_CONFIG(cConfigLSPServiceID);
DEFINE_CONFIG(cConfigLSPEnableAuth);
DEFINE_CONFIG(cConfigLSPEnableFileUpload);
DEFINE_CONFIG(cConfigLSPAuthPort);
DEFINE_CONFIG(cConfigLSPFileUploadPort);
DEFINE_CONFIG(cConfigLSPEnableConfigData);
DEFINE_CONFIG(cConfigLSPConfigDataPort);
DEFINE_CONFIG(cConfigLSPEnablePerfReporting);
DEFINE_CONFIG(cConfigLSPPerfReportingPort);

DEFINE_CONFIG(cConfigLSPEnableMediaTransfer);
DEFINE_CONFIG(cConfigLSPMediaTransferPort);

DEFINE_CONFIG(cConfigLSPEnableServiceRecord);
DEFINE_CONFIG(cConfigLSPServiceRecordPort);

DEFINE_CONFIG(cConfigLSPAuthServiceID);
DEFINE_CONFIG(cConfigLSPConfigServiceID);
DEFINE_CONFIG(cConfigLSPPerfServiceID);
DEFINE_CONFIG(cConfigLSPStatsServiceID);
DEFINE_CONFIG(cConfigLSPMediaServiceID);
DEFINE_CONFIG(cConfigLSPDebugServiceID);
DEFINE_CONFIG(cConfigLSPTickerServiceID);
DEFINE_CONFIG(cConfigLSPServiceRecordServiceID);

DEFINE_CONFIG(cConfigLSPEnableTicker);
DEFINE_CONFIG(cConfigLSPTickerPort);

DEFINE_CONFIG(cConfigLSPDefaultAuthTTL);
DEFINE_CONFIG(cConfigLSPDefaultConfigTTL);
DEFINE_CONFIG(cConfigLSPDefaultMediaTTL);
DEFINE_CONFIG(cConfigLSPDefaultServiceRecordTTL);

DEFINE_CONFIG(cConfigLSPDefaultTCPTimeout);
DEFINE_CONFIG(cConfigLSPDefaultSGTimeout);
DEFINE_CONFIG(cConfigLSPDefaultSGConnectTimeout);
//DEFINE_CONFIG(cConfigLSPDefaultTCPReconnects);
DEFINE_CONFIG(cConfigLSPDefaultSGTTL);
DEFINE_CONFIG(cConfigLSPDefaultSGFailTTL);
DEFINE_CONFIG(cConfigLSPConfigDataPacketVersion2);

#ifndef BUILD_FINAL
DEFINE_CONFIG(cConfigDisableObscuredUnits);
#endif

#ifndef BUILD_FINAL
DEFINE_CONFIG(cConfigForceSPCColors);
#endif

// Pather
#ifndef BUILD_FINAL
DEFINE_CONFIG(cConfigShowPaths);
DEFINE_CONFIG(cConfigDebugPather);
DEFINE_CONFIG(cConfigDebugSpecificSquad);
//DEFINE_CONFIG(cConfigDebugSpecificUnit);
//DEFINE_CONFIG(cConfigDebugSpecificUnitGroup);
//DEFINE_CONFIG(cConfigTrackPathing);
//DEFINE_CONFIG(cConfigRenderTrackedPathing);
//DEFINE_CONFIG(cConfigRenderTrackedPathingUpdate);
//DEFINE_CONFIG(cConfigRenderTrackedPathingWaypoints);
//DEFINE_CONFIG(cConfigRenderTrackedPathingTiming);
#endif
 
// Temp
DEFINE_CONFIG(cConfigNoCovenantArchive);
DEFINE_CONFIG(cConfigAsyncWorldUpdate);
DEFINE_CONFIG(cConfigNoArchiveTexture);

// Options
DEFINE_CONFIG(cConfigDefaultUserZoom);
DEFINE_CONFIG(cConfigScrollUserSpeedAdjustment);
DEFINE_CONFIG(cConfigSelectionSpeed);
DEFINE_CONFIG(cConfigUserGamma);
DEFINE_CONFIG(cConfigFriendOrFoe);
DEFINE_CONFIG(cConfigSubTitles);

DEFINE_CONFIG(cConfigRecomputeFatalityData);

//==============================================================================
// BConfigsGame::registerConfigs
//==============================================================================
static bool registerGameConfigs(bool)
{
   // Commands
   DECLARE_CONFIG(cConfigDontMeterWorkCommands,    "dontMeterWorkCommands", "", (1<<BConfigData::cFlagRestricted), NULL);

   // Multiplayer syncing
   DECLARE_CONFIG(cConfigNoSync,                   "NoSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigDontGoOOS,                "DontGoOOS", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigSyncUpdateInterval,       "syncUpdateInterval", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigFullAllSync,              "fullAllSync", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisableAllSync,           "disableAllSync", "", 0, NULL);
   DECLARE_CONFIG(cConfigXORAllSync,               "xorAllSync", "", 0, NULL);
   DECLARE_CONFIG(cConfigRandSync,                 "randSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigPlayerSync,               "playerSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigTeamSync,                 "teamSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigUnitGroupSync,            "unitGroupSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigUnitSync,                 "unitSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigUnitDetailSync,           "unitDetailSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigUnitActionSync,           "unitActionSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigSquadSync,                "squadSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigProjectileSync,           "projectileSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigWorldSync,                "worldSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigTechSync,                 "techSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigCommandSync,              "commandSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigPathingSync,              "pathingSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigMovementSync,             "movementSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigVisibilitySync,           "visibilitySync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigFinalReleaseSync,         "FinalReleaseSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigFinalDetailSync,          "FinalDetailSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigChecksumSync,             "ChecksumSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigTriggerSync,              "triggerSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigTriggerVarSync,           "triggerVarSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigAnimSync,                 "AnimSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigDoppleSync,               "DoppleSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigCommSync,                 "CommSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);
   DECLARE_CONFIG(cConfigPlatoonSync,              "PlatoonSync", "", (1<<BConfigData::cFlagMultiplayer), NULL);

   // Comm logs
   DECLARE_CONFIG(cConfigCommLogging,              "CommLogging", "", 0, NULL);
   DECLARE_CONFIG(cConfigCommLogHistorySize,       "commLogHistorySize", "", 0, NULL);
   DECLARE_CONFIG(cConfigSessionCL,                "sessionCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigTransportCL,              "transportCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigTimeSyncCL,               "timeSyncCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigGameTimingCL,             "gameTimingCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigTimingCL,                 "timingCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigSyncCL,                   "syncCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigLowLevelSyncCL,           "lowLevelSyncCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigBroadcasterCL,            "broadcasterCL", "", 0, NULL);   
   DECLARE_CONFIG(cConfigReceiverCL,               "receiverCL", "", 0, NULL);   
   DECLARE_CONFIG(cConfigConnectivityCL,           "connectivityCL", "", 0, NULL);   
   DECLARE_CONFIG(cConfigBandwidthCL,              "bandwidthCL", "", 0, NULL);   
   DECLARE_CONFIG(cConfigPerfCL,                   "perfCL", "", 0, NULL);   
   DECLARE_CONFIG(cConfigMPGameCL,                 "mpgameCL", "", 0, NULL);   
   DECLARE_CONFIG(cConfigMPGameSettingsCL,         "mpgamesettingsCL", "", 0, NULL);   
   DECLARE_CONFIG(cConfigReliableConnCL,           "reliableConnCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigMPStorageCL,              "mpStorageCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigMPMatchmakingCL,          "mpMatchmakingCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigSessionConnAddrCL,        "sessionConnAddrCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigSessionConnJoinCL,        "sessionConnJoinCL", "", 0, NULL);
   DECLARE_CONFIG(cConfigFileTransferCL,           "fileTransferCL", "", 0, NULL);

   // Gamepad scrolling
   DECLARE_CONFIG(cConfigDelayScrollingAngle,      "DelayScrollingAngle", "The angle difference you have to push the gamepad stick to break the scroll delay.", 0, NULL);
   DECLARE_CONFIG(cConfigDelayScrollingTime,       "DelayScrollingTime", "The amount of time before the scroll delay is cancelled.", 0, NULL);
   DECLARE_CONFIG(cConfigScrollRateBase,           "ScrollRateBase", "Starting gamepad scroll rate as a percentage from 0.0 to 1.0.", 0, NULL);
   DECLARE_CONFIG(cConfigScrollRateGrow,           "ScrollRateGrow", "Rate per second the gamepad scroll rate grows per second.", 0, NULL);
   DECLARE_CONFIG(cConfigScrollFastSpeed,          "ScrollFastSpeed", "Fast gamepad scrolling speed.", 0, NULL);
   DECLARE_CONFIG(cConfigScrollMaxSpeed,           "ScrollMaxSpeed", "Max regular gamepad scrolling speed.", 0, NULL);
   DECLARE_CONFIG(cConfigScrollRampPoint,          "ScrollRampPoint", "The point to start ramping up the scroll rate to it's max speed (0.0 to 1.0).", 0, NULL);
   DECLARE_CONFIG(cConfigScrollRampSpeed,          "ScrollRampSpeed", "The scroll speed ramp up rate per second.", 0, NULL);
   DECLARE_CONFIG(cConfigScrollExp,                "ScrollExp", "Scroll speed ramp up value.", 0, NULL);
 
   // Sticky reticle
   DECLARE_CONFIG(cConfigStickyReticle,                     "StickyReticle", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleSensitivity,          "StickyReticleSensitivity", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleFollow,               "StickyReticleFollow", "", 0,  NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpCamera,           "StickyReticleJumpCamera", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpCameraSpeed,      "StickyReticleJumpCameraSpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpStep,             "StickyReticleJumpStep", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpForwardMinSpeed,  "StickyReticleJumpForwardMinSpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpForwardMaxSpeed,  "StickyReticleJumpForwardMaxSpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpForwardMinDist,   "StickyReticleJumpForwardMinDist", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpForwardMaxDist,   "StickyReticleJumpForwardMaxDist", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpBackMinSpeed,     "StickyReticleJumpBackMinSpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpBackMaxSpeed,     "StickyReticleJumpBackMaxSpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpBackMinDist,      "StickyReticleJumpBackMinDist", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpBackMaxDist,      "StickyReticleJumpBackMaxDist", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpToAngleTolerance, "StickyReticleJumpToAngleTolerance", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpMaxSpeed,         "StickyReticleJumpMaxSpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpMinSpeed,         "StickyReticleJumpMinSpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpMaxTime,          "StickyReticleJumpMaxTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpMinTime,          "StickyReticleJumpMinTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpMinSearchScale,   "StickyReticleJumpMinSearchScale", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleJumpMaxSearchScale,   "StickyReticleJumpMaxSearchScale", "", 0, NULL);
   DECLARE_CONFIG(cConfigStickyReticleAverageSpeedInterval, "StickyReticleAverageSpeedInterval", "", 0, NULL);

   // Circle selection
   DECLARE_CONFIG(cConfigCircleSelectSize,         "CircleSelectSize", "Base circle selection size.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectMaxSize,      "CircleSelectMaxSize", "Max circle selection size.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectMaxResize,    "CircleSelectMaxResize", "Max resize of circle selection.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectClickSize,    "CircleSelectClickSize", "Circle selection click size.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectDelayTime,    "CircleSelectDelayTime", "Circle selection delay time.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectRate,         "CircleSelectRate", "Circle selection grow rate.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectRateAccel,    "CircleSelectRateAccel", "Circle selection grow acceleration rate.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectMaxRate,      "CircleSelectMaxRate", "Circle selection max grow rate.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectDrawMax,      "CircleSelectDrawMax", "Draw max circle selection size.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectResizeRate,   "CircleSelectResizeRate", "Cirle selection resize rate.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectHover,        "CircleSelectHover", "Use circle selection circle to calculate hover object.", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectFadeInAccel,  "CircleSelectFadeInAccel", "Circle Selection Opacity Fade acceleration rate", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectEventSize,    "CircleSelectEventSize", "", 0, NULL);
   
   // Camera
   DECLARE_CONFIG(cConfigCameraPitch,                 "CameraPitch", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraYaw,                   "CameraYaw", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraZoom,                  "CameraZoom", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraFOV,                   "CameraFOV", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraZoomRate,              "CameraZoomRate", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraZoomFastFactor,        "CameraZoomFastFactor", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraZoomMin,               "CameraZoomMin", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraZoomMax,               "CameraZoomMax", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraRotateRate,            "CameraRotateRate", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraRotateFastFactor,      "CameraRotateFastFactor", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraPitchRate,             "CameraPitchRate", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraPitchFastFactor,       "CameraPitchFastFactor", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraPitchMin,              "CameraPitchMin", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraPitchMax,              "CameraPitchMax", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraPitchInvert,           "CameraPitchInvert", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraFOVRate,               "CameraFOVRate", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraAutoZoomInstant,       "CameraAutoZoomInstant", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraAutoZoomOutRate,       "CameraAutoZoomOutRate", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraAutoZoomOutFastFactor, "CameraAutoZoomOutFastFactor", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraAutoZoomOutDelay,      "CameraAutoZoomOutDelay", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraAutoZoomInRate,        "CameraAutoZoomInRate", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraAutoZoomInFastFactor,  "CameraAutoZoomInFastFactor", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraAutoZoomInDelay,       "CameraAutoZoomInDelay", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraPitchOnZoom,           "CameraPitchOnZoom", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraPitchOnZoomMin,        "CameraPitchOnZoomMin", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraPitchOnZoomMax,        "CameraPitchOnZoomMax", "", 0, NULL);
   DECLARE_CONFIG(cConfigCameraLimits,                "CameraLimits", "", 0, NULL);
   DECLARE_CONFIG(cConfigLockCameraToWorld,           "LockCameraToWorld", "", 0, NULL);

   // Reticle
   DECLARE_CONFIG(cConfigReticleOffset,               "ReticleOffset", "", 0, NULL);
   DECLARE_CONFIG(cConfigReticleAdjustOnZoomMin,      "ReticleAdjustOnZoomMin", "", 0, NULL);
   DECLARE_CONFIG(cConfigReticleAdjustOnZoomMax,      "ReticleAdjustOnZoomMax", "", 0, NULL);   

   // Record Games
   DECLARE_CONFIG(cConfigRecordToCacheDrive,       "RecordToCacheDrive", "Save record games to the cache drive, if XFS is disabled or it's a final build this is on by default.", 0, NULL);
   DECLARE_CONFIG(cConfigRecordingMaxSize,         "RecordingMaxSize", "Maximum recording size.  Default 400k, Max 1MB", 0, NULL);

   // Debug
   DECLARE_CONFIG(cConfigDebugSelectionPicking,    "DebugSelectionPicking", "", 0, NULL);
   DECLARE_CONFIG(cConfigObstructionRenderMode,    "ObstructionRenderMode", "ObstructionRenderMode <mode> where mode can be 0, 1, 2, 3, or 4.", 0, NULL);
   DECLARE_CONFIG(cConfigSpanListSpew,             "SpanListSpew", "Enable span list spew.", 0, NULL);
   DECLARE_CONFIG(cConfigRenderPathingData,        "RenderPathingData", "Render paths", 0, NULL);
   DECLARE_CONFIG(cConfigRenderSquadPlotter,       "RenderSquadPlotter", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisplayCombatLog,         "DisplayCombatLog", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoExeCrc,                 "NoExeCrc", "", 0, NULL);
   DECLARE_CONFIG(cConfigRecordGameSync,           "RecordGameSync", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugAttachmentRotation,  "DebugAttachmentRotation", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugVisualSync,          "DebugVisualSync", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugRenderShape,         "DebugRenderShape", "", 0, NULL);
   DECLARE_CONFIG(cConfigTechLog,                  "TechLog", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderTerrainSimRep,      "RenderTerrainSimRep", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoTerrainSkirt,           "NoTerrainSkirt", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderBattles,            "RenderBattles", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderAlerts,             "RenderAlerts", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugProjectiles,         "DebugProjectiles", "", 0, NULL);
   DECLARE_CONFIG(cConfigAIDisable,                "AIDisable", "", 0, NULL);
   DECLARE_CONFIG(cConfigAIShadow,                 "AIShadow", "", 0, NULL);
   DECLARE_CONFIG(cConfigAICamera,                 "AICamera", "", 0, NULL);
   DECLARE_CONFIG(cConfigAIStrength,               "AIStrength", "", 0, NULL);
   DECLARE_CONFIG(cConfigAIOrderLogging,           "AIOrderLogging", "", 0, NULL);
   DECLARE_CONFIG(cConfigAIUseSPCDefault,          "AIUseSPCDefault", "", 0, NULL);   
   DECLARE_CONFIG(cConfigDisplayStatPage,          "DisplayStatPage", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisplayRenderStats,       "DisplayRenderStats", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisableWow,               "DisableWow", "Disables ingame cutscenes", 0, NULL);
   DECLARE_CONFIG(cConfigNoCull,                   "NoCull", "", 0, NULL);
   DECLARE_CONFIG(cConfigCircleDebugData,          "CircleDebugData", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugStickyReticle,       "DebugStickyReticle", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugPlatoonFormations,   "DebugPlatoonFormations", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableMotionExtraction,   "EnableMotionExtraction", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableImpactLimits,       "EnableImpactLimits", "", 0, NULL);
   DECLARE_CONFIG(cConfigWowRecord,                "WowRecord", "", 0, NULL);
   DECLARE_CONFIG(cConfigWowPlay,                  "WowPlay", "", 0, NULL);
   DECLARE_CONFIG(cConfigWowDownsample,            "WowDownsample", "", 0, NULL);
   DECLARE_CONFIG(cConfigDrawHardpoints,           "DrawHardpoints", "", 0, NULL);
   DECLARE_CONFIG(cConfigReloadScenarioIgnoreTime, "ReloadScenarioIgnoreTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigReloadScenarioWaitTime,   "ReloadScenarioWaitTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigClassicPlatoonGrouping,   "ClassicPlatoonGrouping", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoSquadSlowdown,          "NoSquadSlowdown", "", 0, NULL);
   DECLARE_CONFIG(cConfigPreemptSpeedFactor,       "PreemptSpeedFactor", "", 0, NULL);
   DECLARE_CONFIG(cConfigDecUpdTraceStart,         "DecUpdTraceStart", "", 0, NULL);
   DECLARE_CONFIG(cConfigDecUpdTraceEnd,           "DecUpdTraceEnd", "", 0, NULL);
   DECLARE_CONFIG(cConfigDecUpdTracePct,           "DecUpdTracePct", "", 0, NULL);
   DECLARE_CONFIG(cConfigDecUpdTraceObj,           "DecUpdTraceObj", "", 0, NULL);
   DECLARE_CONFIG(cConfigSubUpdTrace,              "SubUpdTrace", "", 0, NULL);
   DECLARE_CONFIG(cConfigMinSimTime,               "MinSimTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigWriteFPSLog,              "WriteFPSLog", "", 0, NULL);
   DECLARE_CONFIG(cConfigWriteMemLog,              "WriteMemLog", "", 0, NULL);
   DECLARE_CONFIG(cConfigAutoCopyFPSLog,           "AutoCopyFPSLog", "", 0, NULL);
   
   // Vismap
   DECLARE_CONFIG(cConfigNoVismap,                 "NoVismap", "", 0, NULL);
   DECLARE_CONFIG(cConfigUseVismapFile,            "UseVismapFile", "", 0, NULL);
   DECLARE_CONFIG(cConfigSaveVismapFile,           "SaveVismapFile", "", 0, NULL);

   // Skirmish game
   DECLARE_CONFIG(cConfigDefaultMap,               "DefaultMap", "", 0, NULL);

   // Sim
   DECLARE_CONFIG(cConfigAutoScenarioLoad,         "AutoScenarioLoad", "Automatically loads the specified scenario on game start", 0, NULL);
   DECLARE_CONFIG(cConfigAutoScenarioPlayers,      "AutoScenarioPlayers", "Number of players to set for the auto loaded scenario", 0, NULL);
   DECLARE_CONFIG(cConfigAutoScenarioSkirmish,     "AutoScenarioSkirmish", "Set the game type to Skirmish for the auto loaded scenario", 0, NULL);
   DECLARE_CONFIG(cConfigAutoScenarioGameMode,     "AutoScenarioGameMode", "", 0, NULL);
   DECLARE_CONFIG(cConfigAutoScenarioCoop,         "AutoScenarioCoop", "", 0, NULL);
   DECLARE_CONFIG(cConfigAutoRecordGameLoad,       "AutoRecordGameLoad", "Automatically loads the specified record game on game start", 0, NULL);
   DECLARE_CONFIG(cConfigAutoSaveGameLoad,         "AutoSaveGameLoad", "", 0, NULL);
   DECLARE_CONFIG(cConfigAutoSave,                 "AutoSave", "Auto save the game every X seconds", 0, NULL);
   DECLARE_CONFIG(cConfigNoFogMask,                "NoFogMask", "Disable fog mask", 0, NULL);
   DECLARE_CONFIG(cConfigNoVictoryCondition,       "NoVictoryCondition", "Disable the Victory Condition checks", 0, NULL);
   DECLARE_CONFIG(cConfigNoDamage,                 "NoDamage", "Disable Damage for all Weapons for Testing", 0, NULL);
   DECLARE_CONFIG(cConfigNoShieldDamage,           "NoShieldDamage", "Disable Damage to Shields for all Weapons for Testing", 0, NULL);
   DECLARE_CONFIG(cConfigGameSpeed,                "GameSpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigPassThroughOwnVehicles,   "PassThroughOwnVehicles", "Simplify vehicle pathing - dev only", 0, NULL);
   DECLARE_CONFIG(cConfigNoRandomPlayerPlacement,  "NoRandomPlayerPlacement", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisableOneBuilding,       "DisableOneBuilding", "", 0, NULL);
   DECLARE_CONFIG(cConfigBuildingQueue,            "BuildingQueue", "", 0, NULL);
   DECLARE_CONFIG(cConfigRecordGames,              "RecordGames", "", 0, NULL);
   DECLARE_CONFIG(cConfigUseTestLeaders,           "UseTestLeaders", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableCapturePointResourceSharing, "EnableCapturePointResourceSharing", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableFlight, "EnableFlight", "Enables unit air movement action", 0, NULL);
   DECLARE_CONFIG(cConfigEnableFPSLock, "EnableFPSLock", "Forces the whole game to run at a certian FPS", 0, NULL);
   DECLARE_CONFIG(cConfigFPSLockRate, "FPSLockRate", "FPS Rate to force the whole game to run at", 0, NULL);
   DECLARE_CONFIG(cConfigAINoAttack, "AINoAttack", "Prevents the AI from launching attack missions", 0, NULL);   
   DECLARE_CONFIG(cConfigNoBirthAnims, "NoBirthAnims", "Disables unit birth animations", 0, NULL);
   DECLARE_CONFIG(cConfigVeterancy, "Veterancy", "", 0, NULL);
   DECLARE_CONFIG(cConfigTrueLOS, "TrueLOS", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugTrueLOS, "DebugTrueLOS", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoDestruction, "NoDestruction", "", 0, NULL);
   DECLARE_CONFIG(cConfigCoopSharedResources, "CoopSharedResources", "", 0, NULL);
   DECLARE_CONFIG(cConfigCoopSharedPop, "CoopSharedPop", "", 0, NULL);
   DECLARE_CONFIG(cConfigMaxProjectileHeightForDecal, "MaxProjectileHeightForDecal", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableSubbreakage, "EnableSubbreakage", "", 0, NULL);   
   DECLARE_CONFIG(cConfigEnableThrowPart, "EnableThrowPart", "", 0, NULL);   
   DECLARE_CONFIG(cConfigAllowAnimIsDirty, "AllowAnimIsDirty", "", 0, NULL);
   DECLARE_CONFIG(cConfigQuickBuild, "QuickBuild", "", 0, NULL);
   DECLARE_CONFIG(cConfigOverrideLeaderPower, "OverrideLeaderPower", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoProtoObjectOptimization, "NoProtoObjectOptimization", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableSubUpdating,         "EnableSubUpdating", "", 0, NULL);
   DECLARE_CONFIG(cConfigMPSubUpdating,             "MPSubUpdating", "", 0, NULL);
   DECLARE_CONFIG(cConfigAlternateSubUpdating,      "AlternateSubUpdating", "", 0, NULL);
   DECLARE_CONFIG(cConfigDynamicSubUpdateTime,      "DynamicSubUpdateTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigDecoupledUpdate,           "DecoupledUpdate", "", 0, NULL);
   DECLARE_CONFIG(cConfigDecoupledRenderTime,       "DecoupledRenderTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigDecoupledOutsideTimePercent, "DecoupledOutsideTimePercent", "", 0, NULL);
   DECLARE_CONFIG(cConfigDecoupledOutsideTimeAmount, "DecoupledOutsideTimeAmount", "", 0, NULL);
   DECLARE_CONFIG(cConfigSkipDecoupledRenders,      "SkipDecoupledRenders", "", 0, NULL);
   DECLARE_CONFIG(cConfigMaxSubUpdateTime,          "MaxSubUpdateTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigDynamicSubUpdating,        "DynamicSubUpdating", "", 0, NULL);
   DECLARE_CONFIG(cConfigSubUpdateAvgUpdOn,         "SubUpdateAvgUpdOn", "", 0, NULL);
   DECLARE_CONFIG(cConfigSubUpdateAvgUpdOff,        "SubUpdateAvgUpdOff", "", 0, NULL);
   DECLARE_CONFIG(cConfigMinSubUpdateEnabledTime,   "MinSubUpdateEnabledTime", "", 0, NULL);
   
   // Floaty Text
   DECLARE_CONFIG(cConfigRenderFloatyTextXVelocity,     "RenderFloatyTextXVelocity", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderFloatyTextYVelocity,     "RenderFloatyTextYVelocity", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderFloatyTextDuration,      "RenderFloatyTextDuration", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderFloatyTextFadeOutTime,   "RenderFloatyTextFadeOutTime", "", 0, NULL);

   // Powers
   DECLARE_CONFIG(cConfigTestPowersMode, "TestPowersMode", "", 0, NULL);

   // UI
   DECLARE_CONFIG(cConfigDefaultOptions,           "DefaultOptions", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisableUI,                "DisableUI", "UI display", 0, NULL);
   DECLARE_CONFIG(cConfigDisableOldUITextures,     "DisableOldUITextures", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoHelpUI,                 "NoHelpUI", "", 0, NULL);
   DECLARE_CONFIG(cConfigBuildingMenuOnSelect,     "BuildingMenuOnSelect", "", 0, NULL);
   DECLARE_CONFIG(cConfigExitSubSelectOnCancel,    "ExitSubSelectOnCancel", "", 0, NULL);
   DECLARE_CONFIG(cConfigExitTargetSelectOnScroll, "ExitTargetSelectOnScroll", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisableFlash,             "DisableFlash", "", 0, NULL);
   DECLARE_CONFIG(cConfigFlashGameUI,              "FlashGameUI", "", 0, NULL);
   DECLARE_CONFIG(cConfigFlashMinimap,             "FlashMinimap", "", 0, NULL);   
   DECLARE_CONFIG(cConfigCrowdNeighborDistance,    "CrowdNeighborDistance", "Max distance between neighbors within a crowd.", 0, NULL);
   DECLARE_CONFIG(cConfigGotoItemTimeout,          "GotoItemTimeout", "", 0, NULL);
   DECLARE_CONFIG(cConfigGotoBaseDistance,         "GotoBaseDistance", "", 0, NULL);
   DECLARE_CONFIG(cConfigGotoSlideAwaySpeed,       "GotoSlideAwaySpeed", "", 0, NULL);
   DECLARE_CONFIG(cConfigGotoSlideTowardsDistance, "GotoSlideTowardsDistance", "", 0, NULL);
   DECLARE_CONFIG(cConfigGotoSlideTime,            "GotoSlideTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigGroupCreateTime,          "GroupCreateTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigGroupGotoTime,            "GroupGotoTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowUnavailIcons,         "ShowUnavailIcons", "", 0, NULL);
   DECLARE_CONFIG(cConfigBaseDecalIntensity,       "BaseDecalIntensity", "", 0, NULL);
   DECLARE_CONFIG(cConfigSelectionDecalIntensity,  "SelectionDecalIntensity", "", 0, NULL);
   DECLARE_CONFIG(cConfigSubSelectDecalAlpha,      "SubSelectDecalAlpha", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowScoreUI,              "ShowScoreUI", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoHPBar,                  "NoHPBar", "Disable HP bars", 0, NULL);
   DECLARE_CONFIG(cConfigNoEnemySelectionDecals,   "NoEnemySelectionDecals", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableCampaign,           "ShowCampaignUI", "", 0, NULL);
   DECLARE_CONFIG(cConfigPlayer1AI,                "Player1AI", "", 0, NULL);
   DECLARE_CONFIG(cConfigAIAutoDifficulty,         "AIAutoDifficulty", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowReticuleHelp,         "ShowReticuleHelp", "", 0, NULL);
   DECLARE_CONFIG(cConfigObjectivesDisplay,        "ObjectivesDisplay", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderStrength,           "RenderStrength", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoWorldBorder,            "NoWorldBorder", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderCameraBoundaryLines,"RenderCameraBoundaryLines", "", 0, NULL);
   DECLARE_CONFIG(cConfigHUDAttackNotification,    "HUDAttackNotification", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowDebugMenu,            "ShowDebugMenuItems", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowFlashObjWidget,       "ShowFlashObjWidget", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowFlashChat,            "ShowFlashChat", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowPostGame,             "ShowPostGame", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowDevMaps,              "ShowDevMaps", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableBackgroundPlayer,   "EnableBackgroundPlayer", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoIntroCinematics,        "NoIntroCinematics", "", 0, NULL);
   DECLARE_CONFIG(cConfigIntroCinematic,           "IntroCinematic", "", 0, NULL);
   DECLARE_CONFIG(cConfigMinIntroTime,             "MinIntroTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigUIBackgroundMovie,        "UIBackgroundMovie", "", 0, NULL);
   DECLARE_CONFIG(cConfigCreditsMovie,             "CreditsMovie", "", 0, NULL);
   DECLARE_CONFIG(cConfigCreditsSubtitles,         "CreditsSubtitles", "", 0, NULL);
   DECLARE_CONFIG(cConfigUIBackgroundMovieMain,    "UIBackgroundMovieMain", "", 0, NULL);
   DECLARE_CONFIG(cConfigUITimelineScreenMovie,    "UITimelineScreenMovie", "", 0, NULL);
   DECLARE_CONFIG(cConfigUIBackgroundImg,          "UIBackgroundImg", "", 0, NULL);
   DECLARE_CONFIG(cConfigUIBackgroundMovieCampaign, "UIBackgroundMovieCampaign", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoUIDebug,                "NoUIDebug", "", 0, NULL);
   DECLARE_CONFIG(cConfigSplitScreen,              "SplitScreen", "", 0, NULL);
   DECLARE_CONFIG(cConfigVerticalSplit,            "VerticalSplit", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowCampaignPostGame,     "ShowCampaignPostGame", "", 0, NULL);
   DECLARE_CONFIG(cConfigDisableAllChats,          "DisableChats", "", 0, NULL);
   DECLARE_CONFIG(cConfigNewPartyMode,             "NewPartyMode_1", "", 0, NULL);
   DECLARE_CONFIG(cConfigAiInMP,                   "AiInMP", "", 0, NULL);
   DECLARE_CONFIG(cConfigSplitScreenInMP,          "SplitScreenInMP", "", 0, NULL);
   DECLARE_CONFIG(cConfigDebugCircleSelect,        "DebugCircleSelect", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowCommands,             "ShowCommands", "", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectIntensity,    "CircleSelectIntensity", "", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectSizeFactor,   "CircleSelectSizeFactor", "", 0, NULL);
   DECLARE_CONFIG(cConfigCircleSelectOffset,       "CircleSelectOffset", "", 0, NULL);
   DECLARE_CONFIG(cConfigHoverDecalIntensity,      "HoverDecalIntensity", "", 0, NULL);
   DECLARE_CONFIG(cConfigHoverDecalSize,           "HoverDecalSize", "", 0, NULL);
   DECLARE_CONFIG(cConfigHoverDecalOffset,         "HoverDecalOffset", "", 0, NULL);
   DECLARE_CONFIG(cConfigShowGameTime,             "ShowGameTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigCircleMenuBlurFactor,     "CircleMenuBlurFactor", "", 0, NULL);
   DECLARE_CONFIG(cConfigCircleMenuBlurSaturation, "CircleMenuBlurSaturation", "", 0, NULL);
   DECLARE_CONFIG(cConfigCircleMenuBlurDuration,   "CircleMenuBlurDuration", "", 0, NULL);
   DECLARE_CONFIG(cConfigSelectAllIgnoreWorking,   "SelectAllIgnoreWorking", "", 0, NULL);
   DECLARE_CONFIG(cConfigObjectives2,              "Objectives2", "", 0, NULL);
   DECLARE_CONFIG(cConfigRecoverEffectOffset,      "RecoverEffectOffset", "", 0, NULL);
   DECLARE_CONFIG(cConfigBillboardRecoverEffect,   "BillboardRecoverEffect", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoSelectionHighlight,     "NoSelectionHighlight", "", 0, NULL);
   DECLARE_CONFIG(cConfigRenderGameStateMessages,  "RenderGameStateMessages", "", 0, NULL);

   DECLARE_CONFIG(cConfigNoUpdatePathingQuad,      "NoUpdatePathingQuad", "Speed up scenario loads - for artists", 0, NULL);

   DECLARE_CONFIG(cConfigForceAuth,                "ForceAuth", "", 0, NULL);

   DECLARE_CONFIG(cConfigUseLAN,                   "UseLan", "", 0, NULL);
   DECLARE_CONFIG(cConfigPlayerMenu,               "UsePlayerMenu", "", 0, NULL);
   DECLARE_CONFIG(cConfigUINotReadyStuff,          "ShowUINotReadyStuff", "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableAttractMode,        "EnableAttractMode", "", 0, NULL);
   DECLARE_CONFIG(cConfigRejoinParty,              "RejoinParty", "", 0, NULL);
   DECLARE_CONFIG(cConfigAllowSplitScreenMP,       "AllowSplitScreenMP", "", 0, NULL);

#ifndef BUILD_FINAL
   DECLARE_CONFIG(cConfigShowSafeArea,             "ShowSafeArea", "", 0, NULL);
   DECLARE_CONFIG(cConfigLocIdInString,            "LocIdInString", "", 0, NULL);
   DECLARE_CONFIG(cConfigTestFonts,                "TestFonts", "", 0, NULL);
   DECLARE_CONFIG(cConfigUseGraphTestData,         "UseGraphTestData", "", 0, NULL);   
#endif
   DECLARE_CONFIG(cConfigShowESRBNotice,           "ShowESRBNotice", "", 0, NULL);

   DECLARE_CONFIG(cConfigLocaleLanguage,           "LocaleLanguage", "", 0, NULL);

   DECLARE_CONFIG(cConfigDisableCinematicBars,     "DisableCinematicBars", "", 0, NULL);
   DECLARE_CONFIG(cConfigGameStartFadeTime,        "GameStartFadeTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigGameStartFadeDelay,       "GameStartFadeDelay", "", 0, NULL);

// VINCE
#if defined( _VINCE_ )
   // game
   DECLARE_CONFIG( cConfigVinceUnitInterval, "VinceUnitInterval", "Interval in seconds unit stats are logged with VINCE", 0, NULL );
   DECLARE_CONFIG( cConfigVinceBuildingInterval, "VinceBuildingInterval", "Interval in seconds building stats are logged with VINCE", 0, NULL );
   DECLARE_CONFIG( cConfigVinceResourceInterval, "VinceResourceInterval", "Interval in seconds resource stats are logged with VINCE", 0, NULL );
   DECLARE_CONFIG( cConfigVincePathingInterval, "VincePathingInterval", "Interval in seconds pathing stats are logged with VINCE", 0, NULL );

   // user
   DECLARE_CONFIG( cConfigVinceEnableLog, "VinceEnableLog", "If defined VINCE logging will be enabled", 0, NULL );
#endif

   DECLARE_CONFIG(cConfigDemo, "Demo", "", 0, NULL );
   DECLARE_CONFIG(cConfigDemo2, "Demo2", "", 0, NULL );   
   DECLARE_CONFIG(cConfigTutorialMap, "TutorialMap", "", 0, NULL);
   DECLARE_CONFIG(cConfigDemoMovie, "DemoMovie", "", 0, NULL);   

   // Triggers
   DECLARE_CONFIG( cEnableTriggerDebugs, "EnableTriggerDebugs", "", 0, NULL );
   DECLARE_CONFIG( cEnableTriggerDebugsStatusText, "EnableTriggerDebugsStatusText", "", 0, NULL );

   // Hint system
   DECLARE_CONFIG( cEnableHintSystem, "EnableHintSystem", "", 0, NULL );
   DECLARE_CONFIG( cHintSystemResetProfile, "HintSystemResetProfile", "", 0, NULL );

   // Campaign
   #if !defined(BUILD_FINAL)
      DECLARE_CONFIG(cConfigUseDesignFolder, "UseDesignFolder", "", 0, NULL);
   #endif

   //Sim Debugging.
   #ifndef BUILD_FINAL
   DECLARE_CONFIG(cConfigRenderGrouper,             "RenderGrouper", "Turns entity grouper rendering on/off", 0, NULL);
   DECLARE_CONFIG(cConfigRenderSimDebug,            "RenderSimDebug", "Turns the generic render sim data for selected things on/off", 0, NULL);
   DECLARE_CONFIG(cConfigRenderSimDebugNoBoxes,     "RenderSimDebugNoBoxes", "Turns off sim entity box rendering", 0, NULL);
   DECLARE_CONFIG(cConfigRenderSimDebugSquadCircles,"RenderSimDebugSquadCircles", "Turns on sim squad circle rendering", 0, NULL);
   DECLARE_CONFIG(cConfigStartPauseButton,          "StartPauseButton", "Turns the start button into a pause button", 0, NULL);
   DECLARE_CONFIG(cConfigRemakePlatoonFormations,   "RemakePlatoonFormations", "Forces the Platoon to remake the squad formations when it gets an order", 0, NULL);
   DECLARE_CONFIG(cConfigRemakeSquadFormations,     "RemakeSquadFormations", "Forces the Squad to remake the unit formations when it gets an order", 0, NULL);
   DECLARE_CONFIG(cConfigPauseOnUpdate,             "PauseOnUpdate", "Pauses the sim right BEFORE this update is done", 0, NULL);
   DECLARE_CONFIG(cConfigDebugDodge,                "DebugDodge", "Renders debug information for dodging ability", 0, NULL);
   DECLARE_CONFIG(cConfigRenderPathingQuad,         "RenderPathingQuad", "Renders the pathing quads", 0, NULL);
   DECLARE_CONFIG(cConfigDebugMinigame,             "DebugMinigame", "Renders debug minigame info", 0, NULL);
   DECLARE_CONFIG(cConfigPIXTraceWorldUpdate,       "TraceWorldUpdate", "Dumps a trace file for world::update", 0, NULL);
   DECLARE_CONFIG(cConfigPIXTraceWorldLoad,         "TraceWorldLoad", "Dumps a trace file for world::load", 0, NULL);
   DECLARE_CONFIG(cConfigTimingAlertsOFF,           "TimingAlertsOFF", "Disables Timing Alerts", 0, NULL);
   DECLARE_CONFIG(cConfigTimingAlertsThresh,        "TimingAlertsMax", "Timing Alert threshold", 0, NULL);
   DECLARE_CONFIG(cConfigEnableTriggerPerfAsserts,  "EnableTriggerPerfAsserts", "Enables trigger system perf asserts", 0, NULL);
   DECLARE_CONFIG(cConfigDisableTalkingHeads,       "DisableTalkingHeads", "Enables talking heads in the PlayChat trigger effect", 0, NULL);
   DECLARE_CONFIG(cConfigDebugPhysicsImpulseTags,   "DebugPhysicsImpulseTags", "Enables debug rendering for physics impulses on animtags", 0, NULL);
   DECLARE_CONFIG(cConfigDebugIK,                   "DebugIK", "Enables debug rendering for IK", 0, NULL);
   DECLARE_CONFIG(cConfigRenderSquadState,          "RenderSquadState", "ctrl-a display will show text about squads' states", 0, NULL);
   DECLARE_CONFIG(cConfigRenderPredictedPaths,      "RenderPredictedPaths", "ctrl-a display will show predicted paths", 0, NULL);
   DECLARE_CONFIG(cConfigRenderLineToFormationPos,  "RenderLineToFormationPos", "ctrl-a display will show lines between squads and their formation positions", 0, NULL);
   DECLARE_CONFIG(cConfigRenderInterpolationPercent,"RenderInterpolationPercent", "show interpolation percentages being used", 0, NULL);

   
   #endif

   //Sim.
   DECLARE_CONFIG(cConfigSlaveUnitPosition,         "SlaveUnitPosition", "Slaves unit positions to squad positions", 0, NULL);
   DECLARE_CONFIG(cConfigTurning,                   "Turning","Turns the turning test code on", 0, NULL);
   DECLARE_CONFIG(cConfigHumanAttackMove,           "HumanAttackMove", "Turns on attack move processing for humans", 0, NULL);
   DECLARE_CONFIG(cConfigPlatoonRadius,             "PlatoonRadius", "The radial size of a platoon", 0, NULL);
   DECLARE_CONFIG(cConfigProjectionTime,            "ProjectionTime", "How far into the future movement projects", 0, NULL);
   DECLARE_CONFIG(cConfigMoreNewMovement3,          "MoreNewMovement3", "Config to turn on/off all the new, new, new movement changes", 0, NULL);
   DECLARE_CONFIG(cConfigSyncSimAndRenderFrames,    "SyncSimAndRenderFrames", "Syncs all processors so renderer draws results of current frame's game update", 0, NULL);
   DECLARE_CONFIG(cConfigOverrideGroundIK,          "OverrideGroundIK", "Enable this to override ground IK settings", 0, NULL);
   DECLARE_CONFIG(cConfigOverrideGroundIKRange,     "OverrideGroundIKRange", "Ground IK range", 0, NULL);
   DECLARE_CONFIG(cConfigOverrideGroundIKTiltFactor,"OverrideGroundIKTiltFactor", "Ground IK tilt factor", 0, NULL);
   DECLARE_CONFIG(cConfigDriveWarthog,              "DriveWarthog", "Enable to drive the physics warthog with the second controller", 0, NULL);
   DECLARE_CONFIG(cConfigDisplayWarthogVelocity,    "DisplayWarthogVelocity", "Show velocity of physics warthog", 0, NULL);
   DECLARE_CONFIG(cConfigEnableCorpses,             "EnableCorpses", "Enables corpse manager", 0, NULL);
   DECLARE_CONFIG(cConfigBowling,                   "Bowling", "Enables warthog bowling", 0, NULL);
   DECLARE_CONFIG(cConfigDisablePathingLimits,      "DisablePathingLimits", "Disables pathing limits", 0, NULL);
   DECLARE_CONFIG(cConfigDisableVelocityMatchingBySquadType,   "DisableVelocityMatchingBySquadType", "Disables velocity matching for similar squad types", 0, NULL);
   DECLARE_CONFIG(cConfigActiveAbilities,           "ActiveAbilities", "Turn on active abilities", 0, NULL);
   DECLARE_CONFIG(cConfigExplorationRegionShowTime, "ExplorationRegionShowTime", "Amount of time in seconds to show an exploration region", 0, NULL);
   DECLARE_CONFIG(cConfigDisableTrails,             "DisableTrails", "Disables the drawing of trails", 0, NULL);
   DECLARE_CONFIG(cConfigPercentFadeTimeCorpseSink,     "PercentFadeTimeCorpseSink", "What percent of the fading of a corpse to start sinking it", 0, NULL);
   DECLARE_CONFIG(cConfigCorpseSinkSpeed,           "CorpseSinkSpeed", "How fast to sink a corpse", 0, NULL);
   DECLARE_CONFIG(cConfigCorpseMinScale,            "CorpseMinScale", "Min scale for a corpse", 0, NULL);
   DECLARE_CONFIG(cConfigBlockOutsideBounds,        "BlockOutsideBounds", "Block LOS outside of playable bounds", 0, NULL);
   DECLARE_CONFIG(cConfigIgnoreAllPlatoonmates,     "IgnoreAllPlatoonmates", "Allow units to pass through all of their platoonmates when moving", 0, NULL);
   DECLARE_CONFIG(cConfigDisplayUnjoinMaxTeleDist,  "DisplayUnjoinMaxTeleDist", "Displays the max distance a spartan will be teleported over unpathable terrain.", 0, NULL);

   //Achievements
   DECLARE_CONFIG(cConfigFakeAchievements,          "FakeAchievements", "The real achievement system is disabled when this is defined.", 0, NULL);

   //Flash Fonts
   DECLARE_CONFIG(cConfigEnableFlashFonts,          "EnableFlashFonts", "Enable the Flash font localization code", 0, NULL);

   // Game Stats
   DECLARE_CONFIG(cConfigEnableGameStats,           "EnableGameStats", "The game stats system is enabled when this is defined.", 0, NULL);
   DECLARE_CONFIG(cConfigServiceRecordShowAll,      "ServiceRecordShowAll", "All entries of the service record are shown when this is defined.", 0, NULL);
#ifndef BUILD_FINAL
   DECLARE_CONFIG(cConfigForceGameStats,            "ForceGameStats", "Force games stats to be uploaded to our servers.  Useful for non-Live games.", 0, NULL);
#endif

   // XLSP
   DECLARE_CONFIG(cConfigLSPServerFilter,           "LSPServerFilter", "Filter for available SG XLSP servers.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPServiceID,              "LSPServiceID", "MGS assigned Service ID.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPEnableAuth,             "LSPEnableAuth", "If defined, the LSP Authorization service will be enabled.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPAuthPort,               "LSPAuthPort", "Specify the Authorization service port.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPEnableFileUpload,       "LSPEnableFileUpload", "If defined, the LSP File Upload service will be enabled.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPFileUploadPort,         "LSPFileUploadPort", "Specify the File Upload service port.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPEnableMediaTransfer,    "LSPEnableMediaTransfer", "If defined, the LSP Media Transfer service will be enabled.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPMediaTransferPort,      "LSPMediaTransferPort", "Specify the Media Transfer service port.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPEnableConfigData,       "LSPEnableConfig", "If defined, the LSP config data service is enables and queried", 0, NULL);
   DECLARE_CONFIG(cConfigLSPConfigDataPort,         "LSPConfigDataPort", "Specify the config data service port.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPEnablePerfReporting,    "LSPEnablePerfReport", "If defined, the LSP performance reporting (matchmaking) is enabled", 0, NULL);
   DECLARE_CONFIG(cConfigLSPPerfReportingPort,      "LSPPerfReportingPort", "Specify the perf reporting service port.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPEnableServiceRecord,    "LSPEnableServiceRecord", "If defined, the LSP Service Record updates are enabled", 0, NULL);
   DECLARE_CONFIG(cConfigLSPServiceRecordPort,      "LSPServiceRecordPort", "Specify the Service Record service port.", 0, NULL);

   DECLARE_CONFIG(cConfigLSPEnableTicker,           "LSPEnableTicker", "If defined, the LSP Ticker service will be enabled.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPTickerPort,             "LSPTickerPort", "Specify the Ticker service port.", 0, NULL);

   DECLARE_CONFIG(cConfigLSPAuthServiceID,          "LSPAuthServiceID", "Override the default ServiceID for the Auth Service.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPConfigServiceID,        "LSPConfigServiceID", "Override the default ServiceID for the Config Service.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPPerfServiceID,          "LSPPerfServiceID", "Override the default ServiceID for the Perf Service.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPStatsServiceID,         "LSPStatsServiceID", "Override the default ServiceID for the Stats Service.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPMediaServiceID,         "LSPMediaServiceID", "Override the default ServiceID for the Media Service.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPDebugServiceID,         "LSPDebugServiceID", "Override the default ServiceID for the Debug Service.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPTickerServiceID,        "LSPTickerServiceID", "Override the default ServiceID for the Ticker Service.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPServiceRecordServiceID, "LSPServiceRecordServiceID", "Override the default ServiceID for the Service Record Service.", 0, NULL);

   DECLARE_CONFIG(cConfigLSPDefaultAuthTTL,         "LSPDefaultAuthTTL", "Override the default Auth TTL.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPDefaultConfigTTL,       "LSPDefaultConfigTTL", "Override the default Config TTL.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPDefaultMediaTTL,        "LSPDefaultMediaTTL", "Override the default Media TTL.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPDefaultServiceRecordTTL,"LSPDefaultServiceRecordTTL", "Override the default Service Record TTL.", 0, NULL);

   DECLARE_CONFIG(cConfigLSPDefaultTCPTimeout,      "LSPDefaultTCPTimeout", "Override the default LSP TCP Timeout.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPDefaultSGTimeout,       "LSPDefaultSGTimeout", "Override the default SG Timeout.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPDefaultSGConnectTimeout,"LSPDefaultSGConnectTimeout", "Override the default SG Connect Timeout.", 0, NULL);
   //DECLARE_CONFIG(cConfigLSPDefaultTCPReconnects,   "LSPDefaultTCPReconnects", "Override the default LSP TCP reconnect attempts.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPDefaultSGTTL,           "LSPDefaultSGTTL", "Override the default LSP SG TTL.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPDefaultSGFailTTL,       "LSPDefaultSGFailTTL", "Override the default LSP SG Failure TTL.", 0, NULL);
   DECLARE_CONFIG(cConfigLSPConfigDataPacketVersion2,"LSPConfigDataPacketVersion2", "Use the new version of the config data", 0, NULL );

#ifndef BUILD_FINAL
   DECLARE_CONFIG(cConfigDisableObscuredUnits,      "DisableObscuredUnits", "Disable obscured unit rendering.", 0, NULL);
#endif

#ifndef BUILD_FINAL
   DECLARE_CONFIG(cConfigForceSPCColors,          "ForceSPCColors", "Force SPC colors on rendered objects.", 0, NULL);
#endif

   // Pather
#ifndef BUILD_FINAL
      DECLARE_CONFIG(cConfigShowPaths,               "showPaths",               "set paths to show", 0, NULL);   
      DECLARE_CONFIG(cConfigDebugPather,             "debugPather",             "turns any pathing debug logging on", 0, NULL);   
      DECLARE_CONFIG(cConfigDebugSpecificSquad,      "debugSpecificSquad",      "turns unit debug logging on for the specific squad EntityID", 0, NULL);
      //DECLARE_CONFIG(cConfigDebugSpecificUnit,       "debugSpecificUnit",       "turns unit debug logging on for the specific unit ID", 0, NULL);
      //DECLARE_CONFIG(cConfigDebugSpecificUnitGroup,  "debugSpecificUnitGroup",  "turns unit group debug logging on for the specific unit group ID", 0, NULL);

      //DECLARE_CONFIG(cConfigTrackPathing,             "TrackPathing", "Enables tracking of details about what the pather is doing.", 0, NULL);
      //DECLARE_CONFIG(cConfigRenderTrackedPathing,     "RenderTrackedPathing", "Turns on rendering of tracked pathing info.", 0, NULL);
      //DECLARE_CONFIG(cConfigRenderTrackedPathingUpdate, "RenderTrackedPathingUpdate", "Sets the update to see tracked pathing info for (undef/-1 means current update).", 0, NULL);
      //DECLARE_CONFIG(cConfigRenderTrackedPathingWaypoints, "RenderTrackedPathingWaypoints", "Turns on rendering of waypoints for a path.", 0, NULL);
      //DECLARE_CONFIG(cConfigRenderTrackedPathingTiming, "RenderTrackedPathingTiming", "Turns on rendering of timings for paths.", 0, NULL);
#endif

   // Temp
   DECLARE_CONFIG(cConfigNoCovenantArchive, "NoCovenantArchive", "", 0, NULL);
   DECLARE_CONFIG(cConfigAsyncWorldUpdate, "AsyncWorldUpdate", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoArchiveTexture, "NoTextureArchives", "", 0, NULL);
   

   // Options
   DECLARE_CONFIG(cConfigDefaultUserZoom,             "DefaultUserZoom",             "", 0, NULL);
   DECLARE_CONFIG(cConfigScrollUserSpeedAdjustment,   "ScrollSpeedAdjustment",       "", 0, NULL);
   DECLARE_CONFIG(cConfigSelectionSpeed,              "SelectionSpeed",              "", 0, NULL);
   DECLARE_CONFIG(cConfigUserGamma,                "UserGamma", "", 0, NULL);
   DECLARE_CONFIG(cConfigFriendOrFoe,              "FriendOrFoe", "", 0, NULL);
   DECLARE_CONFIG(cConfigSubTitles,                "SubTitles", "", 0, NULL);


   DECLARE_CONFIG(cConfigRecomputeFatalityData,              "RecomputeFatalityData", "", 0, NULL);

   return true;
}

// This causes xcore to call registerGameConfigs() after it initializes.
#pragma data_seg(".ENS$XIU") 
BXCoreInitFuncPtr gpRegisterGameConfigs[] = { registerGameConfigs };
#pragma data_seg() 

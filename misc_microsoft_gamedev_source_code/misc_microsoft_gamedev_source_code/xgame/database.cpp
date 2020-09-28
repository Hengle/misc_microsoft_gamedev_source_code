//==============================================================================
// database.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "database.h"
#include "string\convertToken.h"

// xgame
#include "ability.h"
#include "ChatManager.h"
#include "civ.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "leaders.h"
#include "placementrules.h"
#include "protoobject.h"
#include "protopower.h"
#include "prototech.h"
#include "protosquad.h"
#include "protovisual.h"
#include "squadai.h"
#include "tactic.h"
#include "triggermanager.h"
#include "ui.h"
#include "visualmanager.h"
#include "weapontype.h"
#include "config.h"
#include "configsgame.h"
#include "HPBar.h"
#include "unit.h"
#include "squad.h"
#include "uigame.h"
#include "protoimpacteffect.h"
#include "techtree.h"
#include "ScenarioList.h"
#include "archiveManager.h"
#include "techeffect.h"
#include "gamemode.h"
#include "econfigenum.h"
#include "user.h"
#include "world.h"
#include "terraineffectmanager.h"
#include "terraineffect.h"
#include "physics.h"
#include "physicsobjectblueprintmanager.h"
#include "physicsinfomanager.h"
#include "hintengine.h"

// xsound
#include "soundmanager.h"

// xsystem
#include "xmlreader.h"
#include "xmlwriter.h"

// xparticles
#include "particlegateway.h"

//#define CONVERT_XML_TO_VIS
//#define DEBUG_SAVE_OBJECTS

#ifdef CONVERT_XML_TO_VIS
   #include "xvisual.h"
#endif


// Globals
BDatabase gDatabase;
static BPlayerColor gInvalidPlayerColor;

extern BConceptDatabase gBConceptDatabase;

//==============================================================================
// BDatabase::BDatabase
//==============================================================================
BDatabase::BDatabase() :
   mObjectTypes(),
   mSurfaceTypes(),
   mNumberSurfaceTypes(0),
   mSurfaceTypeImpactEffect(-1),
   mCorpseDeathEffect(-1),
   mNumberBaseObjects(0),
   mNumberObjectTypes(0),
   mNumberAbstractObjectTypes(0),
   mResources(),
   mRates(),
   mPops(),
   mRefCountTypes(),   
   mPlayerStates(),
   mLocStringTable(),
   mLocStrings(),
   mAbilities(),
   mDamageTypes(),
   mAttackRatingDamageTypes(),
   mCivs(),
   mProtoObjects(),
   mProtoPowers(),
   mProtoSquads(),
   mProtoTechs(),
   mPlacementRules(),
   mTechTable(),
   mNumberTechUnitDependecies(0),
   mpTechUnitDependecies(NULL),
   mBaseGameSettings(NULL),
   mCurrentGameSettings(NULL),
   mPOIDRevealer(cInvalidProtoObjectID),
   mPOIDBlocker(cInvalidProtoObjectID),
   mPOIDUnitStart(cInvalidProtoObjectID),
   mPOIDRallyStart(cInvalidProtoObjectID),
   mPOIDAIEyeIcon(cInvalidProtoObjectID),
   mPOIDPhysicsThrownObject(cInvalidProtoObjectID),
   mPOIDScn07Scarab(cInvalidProtoObjectID),
   mPSIDSkirmishScarab(cInvalidProtoObjectID),
   mPSIDCobra(cInvalidProtoObjectID),
   mPSIDVampire(cInvalidProtoObjectID),
   mPOIDLeaderPowerCaster(cInvalidProtoObjectID),
   mPSIDLeaderPowerCaster(cInvalidProtoSquadID),
   mPOIDForge(cInvalidProtoObjectID),
   mPOIDForgeWarthog(cInvalidProtoObjectID),
   mPOIDAnders(cInvalidProtoObjectID),
   mPOIDSpartanRocket(cInvalidProtoObjectID),
   mPOIDSpartanMG(cInvalidProtoObjectID),
   mPOIDSpartanSniper(cInvalidProtoObjectID),
   mBuildingSelfDestructTime(0),
   mTributeAmount(500.0f),
   mTributeCost(0.0f),
   mUnscSupplyPadBonus(0.0f),
   mUnscSupplyPadBreakEvenPoint(0.0f),
   mCovSupplyPadBonus(0.0f),
   mCovSupplyPadBreakEvenPoint(0.0f),
   mLeaderPowerChargeResourceID(-1),
   mLeaderPowerChargeRateID(-1),
   mDamageReceivedXPFactor(0.0f),
   mAttackRevealerLOS(0.0f),
   mAttackRevealerLifespan(0),
   mAttackedRevealerLOS(0.0f),
   mAttackedRevealerLifespan(00),
   mMinimumRevealerSize(0.0f),
   mAttackRatingMultiplier(20.0f),
   mDefenseRatingMultiplier(10.0f),
   mGoodAgainstMinAttackGrade(3),
   mOTIDBuilding(cInvalidObjectTypeID),
   mOTIDBuildingSocket(cInvalidObjectTypeID),
   mOTIDTurretSocket(cInvalidObjectTypeID),
   mOTIDTurretBuilding(cInvalidObjectTypeID),   
   mOTIDSettlement(cInvalidObjectTypeID),
   mOTIDBase(cInvalidObjectTypeID),
   mOTIDIcon(cInvalidObjectTypeID),
   mOTIDGatherable(cInvalidObjectTypeID),
   mOTIDInfantry(cInvalidObjectTypeID),
   mPPIDRepair(cInvalidProtoPowerID),
   mPPIDRallyPoint(cInvalidProtoPowerID),
   mPPIDHookRepair(cInvalidProtoPowerID),
   mPPIDUnscOdstDrop(cInvalidProtoPowerID),
   mOTIDTransporter(cInvalidObjectTypeID),
   mOTIDTransportable(cInvalidObjectTypeID),
   mOTIDFlood(cInvalidObjectTypeID),
   mOTIDCover(cInvalidObjectTypeID),
   mOTIDLOSObstructable(cInvalidObjectTypeID),
   mOTIDObjectiveArrow(cInvalidObjectTypeID),
   mOTIDCampaignHero(cInvalidObjectTypeID),
   mOTIDHero(cInvalidObjectTypeID),
   mOTIDHeroDeath(cInvalidObjectTypeID),
   mOTIDHotDropPickup(cInvalidObjectTypeID),
   mOTIDTeleportDropoff(cInvalidObjectTypeID),
   mOTIDTeleportPickup(cInvalidObjectTypeID),
   mOTIDGroundVehicle(cInvalidObjectTypeID),
   mOTIDGarrison(cInvalidObjectTypeID),
   mOTIDUnscSupplyPad(cInvalidObjectTypeID),
   mOTIDCovSupplyPad(cInvalidObjectTypeID),
   mOTIDStun(cInvalidObjectTypeID),
   mOTIDEmp(cInvalidObjectTypeID),
   mOTIDWallShield(cInvalidObjectTypeID),
   mOTIDBaseShield(cInvalidObjectTypeID),
   mOTIDLeader(cInvalidObjectTypeID),
   mOTIDCovenant(cInvalidObjectTypeID),
   mOTIDUnsc(cInvalidObjectTypeID),
   mOTIDBlackBox(cInvalidObjectTypeID),
   mOTIDSkull(cInvalidObjectTypeID),
   mOTIDBirthOnTop(cInvalidObjectTypeID),
   mOTIDCanCryo(cInvalidObjectTypeID),
   mOTIDHook(cInvalidObjectTypeID),
   mSmallSnowMoundPOID(cInvalidObjectTypeID),
   mMediumSnowMoundPOID(cInvalidObjectTypeID),
   mLargeSnowMoundPOID(cInvalidObjectTypeID),
   mHotdropGlowLargePOID(cInvalidObjectTypeID),
   mHotdropGlowSmallPOID(cInvalidObjectTypeID),
   mHotdropPadBeamPOID(cInvalidObjectTypeID),
   mSmallDotSize(0.0f),
   mMediumDotSize(0.0f),
   mGarrisonDamageMultiplier(1.0f),
   mConstructionDamageMultiplier(1.0f),
   mCaptureDecayRate(0.0f),
   mSquadLeashLength(0.0f),
   mSquadAggroLength(0.0f),
   mUnitLeashLength(0.0f),
   mCloakingDelay(0),
   mReCloakingDelay(0),
   mCloakDetectFrequency(0),
   mShieldRegenDelay(0),
   mShieldRegenTime(0),
   mHeightBonusDamage(0.0f),
   mShieldRegenRate(0.0f),
   mMaxNumCorpses(0),
   mInfantryCorpseDecayTime(0.0f),
   mCorpseSinkingSpacing(0.0f),
   mMaxCorpseDisposalCount(0),
   mProjectileGravity(0.0f),
   mProjectileTumbleRate(0.0f),
   mTrackInterceptDistance(0.0f),
   mStationaryTargetAttackToleranceAngle(0.0f),
   mMovingTargetAttackToleranceAngle(0.0f),
   mMovingTargetTrackingAttackToleranceAngle(0.0f),
   mMovingTargetRangeMultiplier(1.0f),
   mShieldBarColor(0),
   mAmmoBarColor(0),
   mOpportunityDistPriFactor(1.0f),
   mOpportunityBeingAttackedPriBonus(0.0f),
   mChanceToRocket(0.0f),
   mDamageBankTimer(0.0f),
   mMaxDamageBankPctAdjust(0.0f),
   mPTIDShieldUpgrade(cInvalidTechID),
   mPTIDShieldDowngrade(cInvalidTechID),
   mAIDCommand(cInvalidAbilityID),
   mAIDUngarrison(cInvalidAbilityID),
   mAIDUnhitch(cInvalidAbilityID),
   mAIDUnscRam(cInvalidAbilityID),
   mAIDUnscMarineRockets(cInvalidAbilityID),
   mAIDUnscWolverineBarrage(cInvalidAbilityID),
   mAIDUnscBarrage(cInvalidAbilityID),
   mAIDUnscFlashBang(cInvalidAbilityID),
   mAIDUnscHornetSpecial(cInvalidAbilityID),
   mAIDUnscScorpionSpecial(cInvalidAbilityID),
   mAIDCovGruntGrenade(cInvalidAbilityID),
   mAIDUnscLockdown(cInvalidAbilityID),
   mAIDUnscCyclopsThrow(cInvalidAbilityID),
   mAIDUnscSpartanTakeOver(cInvalidAbilityID),
   mAIDUnscGremlinSpecial(cInvalidAbilityID),
   mAIDCovCloak(cInvalidAbilityID),
   mAIDCovArbCloak(cInvalidAbilityID),
   mAIDCovLeaderGlassing(cInvalidAbilityID),
   mAIDCovGhostRam(cInvalidAbilityID),
   mAIDCovChopperRunOver(cInvalidAbilityID),
   mAIDCovGruntSuicideExplode(cInvalidAbilityID),
   mAIDCovStasis(cInvalidAbilityID),
   mAIDCovLocustOverburn(cInvalidAbilityID),
   mAIDCovJumppack(cInvalidAbilityID),
   mAIDCovWraithSpecial(cInvalidAbilityID),
   mAIDUnscUnload(cInvalidAbilityID),
   mAIDUnscCobraLockdown(cInvalidAbilityID),
   mRIDSupplies(cInvalidResourceID),
   mRIDPower(cInvalidResourceID),
   mRIDLeaderPowerCharge(cInvalidResourceID),
   mRCTRegen(BEntityRef::cTypeNone),
   mDamageTypeLight(cInvalidDamageTypeID),
   mDamageTypeLightInCover(cInvalidDamageTypeID),
   mDamageTypeLightArmored(cInvalidDamageTypeID),
   mDamageTypeLightArmoredInCover(cInvalidDamageTypeID),
   mDamageTypeMedium(cInvalidDamageTypeID),
   mDamageTypeMediumAir(cInvalidDamageTypeID),
   mDamageTypeHeavy(cInvalidDamageTypeID),
   mDamageTypeBuilding(cInvalidDamageTypeID),
   mDamageTypeShielded(cInvalidDamageTypeID),
   mDamageTypeSmallAnimal(cInvalidDamageTypeID),
   mDamageTypeBigAnimal(cInvalidDamageTypeID),
   mDefaultBurningEffectLimit(1),
   mAirStrikeLoiterTime(0.0f),
   mMaxSquadPathingCallsPerFrame(10),
   mMaxPlatoonPathingCallsPerFrame(10),
   mFatalityTransitionScale(0.0f),
   mFatalityMaxTransitionTime(0.0f),
   mFatalityPositionOffsetTolerance(0.0f),
   mFatalityOrientationOffsetTolerance(0.0f),
   mFatalityExclusionRange(0.0f),
   mGameOverDelay(0.0f),
   mSkirmishEmptyBaseObject(-1),
   mRecyleRefundRate(1.0f),
   mBaseRebuildTimer(0.0f),
   mDifficultyEasy(0.0f),
   mDifficultyNormal(0.34f),
   mDifficultyHard(0.67f),
   mDifficultyLegendary(1.0f),
   mDifficultyDefault(0.4f),
   mDifficultySPCAIDefault(0.5f),
   mPOIDObjectiveArrow(cInvalidProtoObjectID),
   mPOIDObjectiveLocArrow(cInvalidProtoObjectID),
   //mPOIDObjectiveGroundFX(cInvalidProtoObjectID),
   mObjectiveArrowRadialOffset(0.0f),
   mObjectiveArrowSwitchOffset(0.0f),
   mObjectiveArrowYOffset(0.0f),
   mObjectiveArrowMaxIndex(0),
   mOverrunMinVel(0),
   mOverrunJumpForce(0),
   mOverrunDistance(0),
   mCoopResourceSplitRate(1.0f),
   mHeroDownedLOS(0.0f),
   mHeroHPRegenTime(0.0f),
   mHeroRevivalDistance(0.0f),
   mHeroPercentHPRevivalThreshhold(0.0f),
   mHeroMaxDeadTransportDist(0.0f),
   mTransportClearRadiusScale(1.0f),
   mTransportMaxSearchRadiusScale(1.0f),
   mTransportMaxSearchLocations(1),
   mTransportBlockTime(0),
   mTransportLoadBlockTime(0),
   mTransportMaxBlockAttempts(1),
   mTransportIncomingHeight(60.0f),
   mTransportIncomingOffset(40.0f),
   mTransportOutgoingHeight(60.0f),
   mTransportOutgoingOffset(40.0f),
   mTransportPickupHeight(12.0f),
   mTransportDropoffHeight(12.0f),
   mTransportMax(3),
   mHitchOffset(8.0f),
   mDefaultShieldID(cInvalidProtoObjectID),
   mALMaxWanderFrequency(0),
   mALPredatorCheckFrequency(0),
   mALPreyCheckFrequency(0),
   mALOppCheckRadius(0.0f),
   mALFleeDistance(0.0f),
   mALFleeMovementModifier(0.0f),
   mALMinWanderDistance(0.0f),
   mALMaxWanderDistance(0.0f),
   mALSpawnerCheckFrequency(0.0f),
   mpGameDataReader(NULL),
   mpObjectsReader(NULL),
   mpSquadsReader(NULL),
   mpTechsReader(NULL),
   mpLeadersReader(NULL),
   mpCivsReader(NULL),
   mpPowersReader(NULL),
   mpSharedProtoObjectStaticData(NULL),
   mNumBaseProtoSquads(0),
   mPostProcessProtoVisualLoads(false)

#if !defined(BUILD_FINAL) && defined(ENABLE_RELOAD_MANAGER)
   ,
   mpFileWatcher(NULL),
   mPathStringTable(-1),
   mPathPowers(-1),
   mPathAbilities(-1),
   mPathTrigerScripts(-1),
   mPathRumble(-1),
   mPlayerColorPathHandle(-1),
   mPowersHandle(-1)
#endif
#ifndef BUILD_FINAL
   ,
   mPlayerColorLoadCount(0)
#endif
{
   mStringNotFoundString.set(L"");

   // default values
   for (uint i=0; i<cNumReticleAttackGrades; i++)
      mGoodAgainstGrades[i]=0;

   for (uint i=0; i<cMaximumSupportedCivs; i++)
   {
      for (uint j=0; j<cMaximumSupportedPlayers; j++)
         mCivPlayerColors[i][j]=j;
   }

   for (uint i=0; i<BRumbleEvent::cNumEvents; i++)
      mRumbleEventIDs[i]=-1;
}

//==============================================================================
// BDatabase::~BDatabase
//==============================================================================
BDatabase::~BDatabase()
{
}

//==============================================================================
// BDatabase::discardDatabaseFiles
//==============================================================================
void BDatabase::discardDatabaseFiles()
{
   const long dirsToDiscard[] = { cDirTactics, cDirPlacementRules };
   
   uint totalFilesDiscarded = 0;
   for (uint i = 0; i < sizeof(dirsToDiscard)/sizeof(dirsToDiscard[0]); i++)
      gFileManager.discardFiles(dirsToDiscard[i], "*.xmb", true, NULL, &totalFilesDiscarded);

   gFileManager.discardFiles(cDirPhysics, "*.physics.xmb", true, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "*.blueprint.xmb", true, NULL, &totalFilesDiscarded);

   // Re-discard data that was originally discarded since the root archive gets fully reloaded 
   // after the player finishes a game and returns to the main menu.
   gFileManager.discardFiles(cDirPhysics, "*.vis.xmb", true, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "achievements.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "HaloWars.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "gamedata.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "impacteffects.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "mpactsounds.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "rumble.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "hpbars.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "stringtable.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "terrainTileTypes.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "soundTable.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "objects.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "objecttypes.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "squads.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "techs.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "leaders.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "civs.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "powers.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "damagetypes.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "powers.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "abilities.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "playercolors.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "weapontypes.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "gamemodes.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "deathmanager.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "fatalityData.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "skulls.xml.xmb", false, NULL, &totalFilesDiscarded);
   gFileManager.discardFiles(cDirPhysics, "preloadxmlfiles.xml.xmb", false, NULL, &totalFilesDiscarded);
   //*.tfx
   gFileManager.discardFiles(cDirPhysics, "concepts.xml.xmb", false, NULL, &totalFilesDiscarded);
   //userclasses.xml
   //C:\x\code\xgame\triggeruserdata.cpp(227):   if (!reader.load(cDirAIData, filename, XML_READER_LOAD_DISCARD_ON_CLOSE))
   //C:\x\code\xgameRender\worldVisibility.cpp(88):   if (!xmlReader.load(mDataDirID, pFilename, XML_READER_LOAD_DISCARD_ON_CLOSE))
   //C:\x\code\terrain\TerrainFoliage.cpp(1041):   if (!reader.load(cDirArt, pathXML, XML_READER_LOAD_DISCARD_ON_CLOSE))

   trace("BDatabase::discardDatabaseFiles: Discarded %u files\n", totalFilesDiscarded);
}

//==============================================================================
// BDatabase::setup
//==============================================================================
bool BDatabase::setup()
{
   SCOPEDSAMPLE(BDatabase_setup)
   gVisualManager.addProtoVisualHandler(this);

   mLoadedProtoVisuals.clear();
   mPostProcessProtoVisualLoads = true;

   // Game data
   if (!setupGameData())
      return false;

   // Setup data that other data is dependent on
   if(!setupDependentData())
      return false;

   // Read all vis files
   preloadVisFiles();

   // Read all tfx files
   preloadTfxFiles();

   // Rumble (controller vibration)
   setupRumble();

   // Placement rules
   if(!setupPlacementRules())
      return false;

   // Load HP Bars
   if (!setupHPBars())
      return false;

   // Load impact effects
   setupImpactEffects();

   // Load localization strings
   setupLocStrings(false);

   // Load Weapon Types
   setupWeaponTypes();

   // Proto power data.
   setupProtoPowers();

   // Ability data
   setupAbilities();

   // Read all physics files
   preloadPhysicsFiles();

   // Object data
   if (!setupProtoObjects())
   {
      BASSERTM(false, "setupProtoObjects() failed.");
      return false;
   }

   setupDamageTypeExemplars();
   
   setupTacticAttackRatings();

   // Setup decomposed object types.
   setupDecomposedObjectTypes();

   // Squad data
   if (!setupProtoSquads())
   {
      BASSERTM(false, "setupProtoSquads() failed.");
      return false;
   }

   // Tech data
   if (!setupProtoTechs())
   {
      BASSERTM(false, "setupProtoTechs() failed.");
      return false;
   }

   // Civ data
   if (!setupCivs())
   {
      BASSERTM(false, "setupCivs() failed.");
      return false;
   }

   // Leaders data
   if (!setupLeaders())
   {
      BASSERTM(false, "setupLeaders() failed.");
      return false;
   }

   // Placement tracking
   if(!setupPlacementTracking())
   {
      BASSERTM(false, "setupPlacementTracking() failed.");
      return false;
   }

   // Player colors
   setupPlayerColors();

   // Impact Sounds
   setupImpactSounds();

   // Game types
   if (!setupGameModes())
      return false;

   // Game settings
   mBaseGameSettings=new BGameSettings();
   if(!mBaseGameSettings || !mBaseGameSettings->setup())
      return false;
   mCurrentGameSettings=new BGameSettings(mBaseGameSettings);
   if(!mCurrentGameSettings)
      return false;

   if (!setupDBIDs())
   {
      BASSERTM(false, "setupDBIDs() failed.");
      return (false);
   }

   // initialize the scenario database
   if (!mScenarioList.load())
      return false;

   // load up the tips
   if (!mTips.load())
      return false;

   gTriggerManager.init();

   // load fatality asset data
   bool recomputeFatalityData = gConfig.isDefined(cConfigRecomputeFatalityData);
   if (!recomputeFatalityData)
      gFatalityManager.loadFatalityAssets();


   gBConceptDatabase.load();

   setupPreloadXmlFiles();

#if !defined(BUILD_FINAL) && defined(ENABLE_RELOAD_MANAGER)
   // Data reloading
   setupDataReloading();
#endif

   mPostProcessProtoVisualLoads = false;
   int numVis = mLoadedProtoVisuals.getNumber();
   for (int i=0; i<numVis; i++)
      handleProtoVisualLoaded(mLoadedProtoVisuals[i]);
   mLoadedProtoVisuals.clear();

   discardDatabaseFiles();

   delete mpGameDataReader;
   delete mpObjectsReader;
   delete mpSquadsReader;
   delete mpTechsReader;
   delete mpLeadersReader;
   delete mpCivsReader;
   delete mpPowersReader;

   mpGameDataReader = NULL;
   mpObjectsReader = NULL;
   mpSquadsReader = NULL;
   mpTechsReader = NULL;
   mpLeadersReader = NULL;
   mpCivsReader = NULL;
   mpPowersReader = NULL;

   return true;
}

//==============================================================================
// BDatabase::shutdown
//==============================================================================
void BDatabase::shutdown()
{
   long count;

#if !defined(BUILD_FINAL) && defined(ENABLE_RELOAD_MANAGER)
   if(mpFileWatcher)
   {
      delete mpFileWatcher;
      mpFileWatcher=NULL;
   }
#endif

   if (mpGameDataReader)
   {
      delete mpGameDataReader;
      mpGameDataReader = NULL;
   }

   if (mpObjectsReader)
   {
      delete mpObjectsReader;
      mpObjectsReader = NULL;
   }

   if (mpSquadsReader)
   {
      delete mpSquadsReader;
      mpSquadsReader = NULL;
   }

   if (mpTechsReader)
   {
      delete mpTechsReader;
      mpTechsReader = NULL;
   }

   if (mpLeadersReader)
   {
      mpLeadersReader = NULL;
      delete mpLeadersReader;
   }

   if (mpCivsReader)
   {
      mpCivsReader = NULL;
      delete mpCivsReader;
   }

   if (mpPowersReader)
   {
      mpPowersReader = NULL;
      delete mpPowersReader;
   }

   gVisualManager.removeProtoVisualHandler(this);

   mShieldBarColor = 0;
   mAmmoBarColor = 0;
   mCloakingDelay = 0;
   mReCloakingDelay = 0;
   mCloakDetectFrequency = 0;
   mShieldRegenDelay = 0;
   mShieldRegenTime = 0;
   mHeightBonusDamage = 0.0f;
   mShieldRegenRate = 0.0f;
   mMaxNumCorpses = 0;
   mInfantryCorpseDecayTime = 0.0f;
   mCorpseSinkingSpacing = 0.0f;
   mMaxCorpseDisposalCount = 0;
   mMaxSquadPathingCallsPerFrame = 10;
   mMaxPlatoonPathingCallsPerFrame = 10;
   mFatalityTransitionScale = 0.0f;
   mFatalityMaxTransitionTime = 0.0f;
   mFatalityPositionOffsetTolerance = 0.0f;
   mFatalityOrientationOffsetTolerance = 0.0f;
   mFatalityExclusionRange = 0.0f;
   mGameOverDelay = 0.0f;
   mSkirmishEmptyBaseObject=-1;
   mRecyleRefundRate=1.0f;
   mBaseRebuildTimer=0.0f;
   mDifficultyEasy=0.0f;
   mDifficultyNormal=0.34f;
   mDifficultyHard=0.67f;
   mDifficultyLegendary=1.0f;
   mDifficultyDefault=0.4f;
   mDifficultySPCAIDefault = 0.5f;
   mPOIDObjectiveArrow = cInvalidProtoObjectID;
   mPOIDObjectiveLocArrow = cInvalidProtoObjectID;
   //mPOIDObjectiveGroundFX = cInvalidProtoObjectID;
   mObjectiveArrowRadialOffset = 0.0f;
   mObjectiveArrowSwitchOffset = 0.0f;
   mObjectiveArrowYOffset = 0.0f;
   mObjectiveArrowMaxIndex = 0;
   mProjectileGravity = 0.0f;
   mSquadLeashLength = 0.0f;
   mSquadAggroLength=0.0f;
   mUnitLeashLength= 0.0f;
   mGarrisonDamageMultiplier = 1.0f;
   mConstructionDamageMultiplier = 1.0f;
   mCaptureDecayRate = 0.0f;
   mDefaultBurningEffectLimit = 1;
   mProjectileTumbleRate = 0.0f;
   mTrackInterceptDistance = 0.0f;
   mStationaryTargetAttackToleranceAngle = 0.0f;
   mMovingTargetAttackToleranceAngle = 0.0f;
   mMovingTargetTrackingAttackToleranceAngle = 0.0f;
   mMovingTargetRangeMultiplier = 1.0f;
   mAirStrikeLoiterTime = 0.0f;

   mOTIDBuilding = cInvalidObjectTypeID;
   mOTIDBuildingSocket = cInvalidObjectTypeID;
   mOTIDTurretSocket = cInvalidObjectTypeID;
   mOTIDTurretBuilding = cInvalidObjectTypeID;   
   mOTIDSettlement = cInvalidObjectTypeID;
   mOTIDBase = cInvalidObjectTypeID;
   mOTIDIcon = cInvalidObjectTypeID;
   mOTIDGatherable = cInvalidObjectTypeID;
   mOTIDInfantry = cInvalidObjectTypeID;
   mOTIDTransporter = cInvalidObjectTypeID;
   mOTIDTransportable = cInvalidObjectTypeID;
   mOTIDFlood = cInvalidObjectTypeID;
   mOTIDCover = cInvalidObjectTypeID;
   mOTIDLOSObstructable = cInvalidObjectTypeID;
   mOTIDObjectiveArrow = cInvalidObjectTypeID;
   mOTIDCampaignHero = cInvalidObjectTypeID;
   mOTIDHero = cInvalidObjectTypeID;
   mOTIDHeroDeath = cInvalidObjectTypeID;
   mOTIDHotDropPickup = cInvalidObjectTypeID;
   mOTIDTeleportDropoff = cInvalidObjectTypeID;
   mOTIDTeleportPickup = cInvalidObjectTypeID;
   mOTIDGroundVehicle = cInvalidObjectTypeID;
   mOTIDGarrison = cInvalidObjectTypeID;
   mOTIDUnscSupplyPad = cInvalidObjectTypeID;
   mOTIDCovSupplyPad = cInvalidObjectTypeID;
   mOTIDWallShield = cInvalidObjectTypeID;
   mOTIDBaseShield = cInvalidObjectTypeID;
   mOTIDLeader = cInvalidObjectTypeID;
   mOTIDCovenant = cInvalidObjectTypeID;
   mOTIDUnsc = cInvalidObjectTypeID;
   mOTIDBlackBox = cInvalidObjectTypeID;
   mOTIDSkull = cInvalidObjectTypeID;
   mOTIDBirthOnTop = cInvalidObjectTypeID;
   mOTIDCanCryo = cInvalidObjectTypeID;
   mOTIDHook = cInvalidObjectTypeID;

   mPPIDRepair = cInvalidProtoPowerID;
   mPPIDRallyPoint = cInvalidProtoPowerID;
   mPPIDHookRepair = cInvalidProtoPowerID;
   mPPIDUnscOdstDrop = cInvalidProtoPowerID;

   mPOIDRevealer = cInvalidProtoObjectID;
   mPOIDBlocker = cInvalidProtoObjectID;
   mPOIDUnitStart = cInvalidProtoObjectID;
   mPOIDRallyStart = cInvalidProtoObjectID;
   mPOIDAIEyeIcon = cInvalidProtoObjectID;
   mPOIDPhysicsThrownObject = cInvalidProtoObjectID;
   mPOIDScn07Scarab = cInvalidProtoObjectID;
   mPSIDSkirmishScarab = cInvalidProtoObjectID;
   mPSIDCobra = cInvalidProtoObjectID;
   mPSIDVampire = cInvalidProtoObjectID;
   mPOIDLeaderPowerCaster = cInvalidProtoObjectID;
   mPSIDLeaderPowerCaster = cInvalidProtoSquadID;
   mPOIDForge = cInvalidProtoObjectID;
   mPOIDForgeWarthog = cInvalidProtoObjectID;
   mPOIDAnders = cInvalidProtoObjectID;
   mPOIDSpartanRocket = cInvalidProtoObjectID;
   mPOIDSpartanMG = cInvalidProtoObjectID;
   mPOIDSpartanSniper = cInvalidProtoObjectID;
   mAIDCommand = cInvalidAbilityID;
   mAIDUngarrison = cInvalidAbilityID;
   mAIDUnhitch = cInvalidAbilityID;
   mAIDUnscRam = cInvalidAbilityID;
   mAIDUnscMarineRockets = cInvalidAbilityID;
   mAIDUnscWolverineBarrage = cInvalidAbilityID;
   mAIDUnscBarrage = cInvalidAbilityID;
   mAIDUnscFlashBang = cInvalidAbilityID;
   mAIDUnscHornetSpecial = cInvalidAbilityID;
   mAIDUnscScorpionSpecial = cInvalidAbilityID;
   mAIDCovGruntGrenade = cInvalidAbilityID;
   mAIDUnscLockdown = cInvalidAbilityID;
   mAIDUnscCyclopsThrow = cInvalidAbilityID;
   mAIDUnscSpartanTakeOver = cInvalidAbilityID;
   mAIDUnscGremlinSpecial = cInvalidAbilityID;
   mAIDCovCloak = cInvalidAbilityID;
   mAIDCovArbCloak = cInvalidAbilityID;
   mAIDCovLeaderGlassing = cInvalidAbilityID;
   mAIDCovGhostRam = cInvalidAbilityID;
   mAIDCovChopperRunOver = cInvalidAbilityID;
   mAIDCovGruntSuicideExplode = cInvalidAbilityID;
   mAIDCovStasis = cInvalidAbilityID;
   mAIDCovLocustOverburn = cInvalidAbilityID;
   mAIDCovJumppack = cInvalidAbilityID;
   mAIDCovWraithSpecial = cInvalidAbilityID;
   mAIDUnscUnload = cInvalidAbilityID;
   mAIDUnscCobraLockdown = cInvalidAbilityID;
   mRIDSupplies = cInvalidResourceID;
   mRIDPower = cInvalidResourceID;
   mRIDLeaderPowerCharge = cInvalidResourceID;
   mRCTRegen = BEntityRef::cTypeNone;
   mDamageTypeLight = cInvalidDamageTypeID;
   mDamageTypeLightInCover = cInvalidDamageTypeID;
   mDamageTypeLightArmored = cInvalidDamageTypeID;
   mDamageTypeLightArmoredInCover = cInvalidDamageTypeID;
   mDamageTypeMedium = cInvalidDamageTypeID;
   mDamageTypeMediumAir = cInvalidDamageTypeID;
   mDamageTypeHeavy = cInvalidDamageTypeID;
   mDamageTypeBuilding = cInvalidDamageTypeID;
   mDamageTypeShielded = cInvalidDamageTypeID;
   mDamageTypeSmallAnimal = cInvalidDamageTypeID;
   mDamageTypeBigAnimal = cInvalidDamageTypeID;

   // Hero stuff
   mHeroDownedLOS = 0.0f;
   mHeroHPRegenTime = 0.0f;
   mHeroRevivalDistance = 0.0f;
   mHeroPercentHPRevivalThreshhold = 0.0f;
   mHeroMaxDeadTransportDist = 0.0f;

   // Transport stuff
   mTransportClearRadiusScale = 1.0f;
   mTransportMaxSearchRadiusScale = 1.0f;
   mTransportMaxSearchLocations = 1;
   mTransportBlockTime = 0;
   mTransportLoadBlockTime = 0;
   mTransportMaxBlockAttempts = 1;

   // Game settings
   if(mBaseGameSettings)
   {
      delete mBaseGameSettings;
      mBaseGameSettings=NULL;
   }
   delete mCurrentGameSettings;
   mCurrentGameSettings=NULL;

   // Tech unit dependencies
   if(mpTechUnitDependecies)
   {
      for(long i=0; i<mNumberTechUnitDependecies; i++)
      {
         if(mpTechUnitDependecies[i])
            delete mpTechUnitDependecies[i];
      }
      delete[] mpTechUnitDependecies;
      mpTechUnitDependecies=NULL;
   }

   // Proto objects
   count = mProtoObjects.size();
   for (long i=0; i<count; i++)
   {
      BProtoObject *pProtoObject = mProtoObjects[i];
      if (pProtoObject)
         delete pProtoObject;
   }
   mProtoObjects.clear();
   if (mpSharedProtoObjectStaticData)
   {
      delete mpSharedProtoObjectStaticData;
      mpSharedProtoObjectStaticData=NULL;
   }

   // Proto squads
   count = mProtoSquads.size();
   for (long i = 0; i < count; i++)
   {
      BProtoSquad *mProtoSquad = mProtoSquads[i];
      if (mProtoSquad)
         delete mProtoSquad;
   }
   mProtoSquads.clear();

   // Civs
   count = mCivs.size();
   for (long i=0; i<count; i++)
   {
      BCiv *pCiv = mCivs[i];
      if (pCiv)
         delete pCiv;
   }
   mCivs.clear();

   // Leaders
   removeAllLeaders();

   // Preload trigger scripts.
   gTriggerManager.shutdown();

   // Proto powers
   count = mProtoPowers.size();
   for (long i=0; i<count; i++)
   {
      BProtoPower *pProtoPower = mProtoPowers[i];
      if (pProtoPower)
         delete pProtoPower;
   }
   mProtoPowers.clear();

   // Abilities
   count = mAbilities.size();
   for (long i=0; i<count; i++)
   {
      BAbility* pAbility = mAbilities[i];
      if (pAbility)
         delete pAbility;
   }
   mAbilities.clear();

   // Damage types
   mDamageTypes.clear();
   mAttackRatingDamageTypes.clear();

   // Proto techs
   count = mProtoTechs.size();
   for (long i=0; i<count; i++)
   {
      BProtoTech* pProtoTech=mProtoTechs[i];
      delete pProtoTech;
   }
   mProtoTechs.clear();

   // Placement rules
   count = mPlacementRules.size();
   for (long i=0; i<count; i++)
   {
      BPlacementRules* pRules=mPlacementRules[i];
      delete pRules;
   }
   mPlacementRules.clear();

   // Weapon types
   count = mWeaponTypes.size();
   for (long i=0; i<count; i++)
   {
      BWeaponType* pWeaponType=mWeaponTypes[i];
      delete pWeaponType;
   }
   mWeaponTypes.clear();

   // Game types
   count = mGameModes.size();
   for (long i=0; i<count; i++)
   {
      BGameMode* pGameMode=mGameModes[i];
      delete pGameMode;
   }
   mGameModes.clear();

   count = mProtoHPBars.size();
   for (long i = 0; i<count; i++)
   {
      delete mProtoHPBars[i];
   }
   mProtoHPBars.clear();

   count = mProtoVeterancyBars.size();
   for (long i = 0; i<count; i++)
   {
      delete mProtoVeterancyBars[i];
   }
   mProtoVeterancyBars.clear();

   count = mProtoPieProgressBars.size();
   for (long i = 0; i<count; i++)
   {
      delete mProtoPieProgressBars[i];
   }
   mProtoPieProgressBars.clear();

   gFatalityManager.reset(true);

   for (uint i=0; i<mPreloadXmlFiles.size(); i++)
   {
      BXMLReader* pReader = mPreloadXmlFiles[i];
      delete pReader;
   }
   mPreloadXmlFiles.clear();
   mPreloadXmlFileTable.clearAll();
}

//==============================================================================
// BDatabase::update
//==============================================================================
void BDatabase::update()
{
#if !defined(BUILD_FINAL) && defined(ENABLE_RELOAD_MANAGER)
   // Handle auto data reloading
   if(mpFileWatcher)
   {
      if(mpFileWatcher->getIsDirty(mPathStringTable))
         setupLocStrings(true);

      if(mpFileWatcher->getIsDirty(mPathRumble))
         setupRumble();
         
      if(mpFileWatcher->getIsDirty(mPlayerColorPathHandle))
         setupPlayerColors();

      if(mpFileWatcher->getIsDirty(mPowersHandle))
         setupProtoPowers();

      mpFileWatcher->getAreAnyDirty(); // Forces a clear of all the dirty flags
   }
#endif
}

//==============================================================================
// BDatabase::setupGameData
//==============================================================================
bool BDatabase::setupGameData()
{
   SCOPEDSAMPLE(BDatabase_setupGameData)
   mpGameDataReader = new BXMLReader();
   if(!mpGameDataReader->load(cDirData, "gamedata.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   BXMLNode rootNode(mpGameDataReader->getRootNode());
   long nodeCount=rootNode.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      BSimString tempStr;
      if(name=="Resources")
      {
         long childCount=node.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode child(node.getChild(j));
            
            BSimString temp;
            mResources.add(child.getTextPtr(temp), j);

            bool deductable=true;
            child.getAttribValueAsBool("Deductable", deductable);
            mResourceDeductable.add(deductable);
         }

         mRIDSupplies = this->getResource("Supplies");
         mRIDPower = this->getResource("Power");
         mRIDLeaderPowerCharge = this->getResource("LeaderPowerCharge");
      }
      if(name=="Rates")
      {
         long childCount=node.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode child(node.getChild(j));

            BSimString temp;
            mRates.add(child.getTextPtr(temp), j);
         }
      }
      else if (name == "GoodAgainstReticle")
      {
         long childCount = node.getNumberChildren();
         for (long j=0; j<childCount; j++)
         {
            BXMLNode childNode = node.getChild(j);
            if (!childNode.getValid())
            {
               BASSERT(false);
               return (false);
            }
            if (childNode.getName() == "NoEffect")
               childNode.getTextAsUInt(mGoodAgainstGrades[cReticleAttackNoEffectAgainst]);
            else if (childNode.getName() == "Weak")
               childNode.getTextAsUInt(mGoodAgainstGrades[cReticleAttackWeakAgainst]);
            else if (childNode.getName() == "Fair")
               childNode.getTextAsUInt(mGoodAgainstGrades[cReticleAttackFairAgainst]);
            else if (childNode.getName() == "Good")
               childNode.getTextAsUInt(mGoodAgainstGrades[cReticleAttackGoodAgainst]);
            else if (childNode.getName() == "Extreme")
               childNode.getTextAsUInt(mGoodAgainstGrades[cReticleAttackExtremeAgainst]);
         }

      }
      else if (name == "DifficultyEasy")
         node.getTextAsFloat(mDifficultyEasy);
      else if (name == "DifficultyNormali")
         node.getTextAsFloat(mDifficultyNormal);
      else if (name == "DifficultyHard")
         node.getTextAsFloat(mDifficultyHard);
      else if (name == "DifficultyLegendary")
         node.getTextAsFloat(mDifficultyLegendary);
      else if (name == "DifficultyDefault")
         node.getTextAsFloat(mDifficultyDefault);
      else if (name == "DifficultySPCAIDefault")
         node.getTextAsFloat(mDifficultySPCAIDefault);
      else if(name=="Pops")
      {
         long childCount=node.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode child(node.getChild(j));
            
            BSimString temp;
            mPops.add(child.getTextPtr(temp), j);
         }
      }
      else if(name=="RefCounts")
      {
         long childCount=node.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode child(node.getChild(j));
            BSimString temp;
            mRefCountTypes.add(child.getTextPtr(temp), j);
         }

         mRCTRegen = this->getRefCountType("Regen");
      }
      else if (name == "PlayerStates")
      {
         int childCount = node.getNumberChildren();
         for (int j = 0; j < childCount; j++)
         {
            BXMLNode child(node.getChild(j));
            BSimString temp;
            mPlayerStates.add(child.getTextPtr(temp), j);
         }
      }
      else if(name=="GarrisonDamageMultiplier")
         node.getTextAsFloat(mGarrisonDamageMultiplier);
      else if(name=="ConstructionDamageMultiplier")
         node.getTextAsFloat(mConstructionDamageMultiplier);
      else if(name=="CaptureDecayRate")
         node.getTextAsFloat(mCaptureDecayRate);
      else if(name=="SquadLeashLength")
         node.getTextAsFloat(mSquadLeashLength);
      else if(name=="SquadAggroLength")
      {
         node.getTextAsFloat(mSquadAggroLength);
         //Ensure that aggro is less than leash.
         if (mSquadAggroLength > (mSquadLeashLength-cFloatCompareEpsilon))
            mSquadAggroLength=mSquadLeashLength-cFloatCompareEpsilon;
      }
      else if(name=="UnitLeashLength")
         node.getTextAsFloat(mUnitLeashLength);
      else if(name=="MaxNumCorpses")
         node.getTextAsLong(mMaxNumCorpses);


      else if (name == "BurningEffectLimits")
      {
         if (!node.getAttribValueAsInt("DefaultLimit", mDefaultBurningEffectLimit))
         {
            BASSERT(false);
            return (false);
         }

         long childCount = node.getNumberChildren();
         for (long j=0; j<childCount; j++)
         {
            BXMLNode burningEffectNode = node.getChild(j);
            if (!burningEffectNode.getValid())
            {
               BASSERT(false);
               return (false);
            }
            if (burningEffectNode.getName() != "BurningEffectLimitEntry")
            {
               BASSERT(false);
               return (false);
            }
            BBurningEffectLimit effectLimit;
            if (!burningEffectNode.getAttribValueAsInt("Limit", effectLimit.mLimit))
            {
               BASSERT(false);
               return (false);
            }
            BSimString tempStr;
            if (!burningEffectNode.getTextAsString(effectLimit.mObjectType))
            {
               BASSERT(false);
               return (false);
            }

            // Set the ID to the index+1 (keep 0 for invalid)
            effectLimit.mID = (mBurningEffectLimits.getSize() + 1);
            mBurningEffectLimits.add(effectLimit);
         }
      }




      else if(name=="FatalityTransitionScale")
         node.getTextAsFloat(mFatalityTransitionScale);
      else if(name=="FatalityMaxTransitionTime")
         node.getTextAsFloat(mFatalityMaxTransitionTime);
      else if(name=="FatalityPositionOffsetTolerance")
         node.getTextAsFloat(mFatalityPositionOffsetTolerance);
      else if(name=="FatalityOrientationOffsetTolerance")
      {
         float tolerance;
         node.getTextAsFloat(tolerance);
         mFatalityOrientationOffsetTolerance = XMConvertToRadians(tolerance);
      }
      else if(name=="FatalityExclusionRange")
         node.getTextAsFloat(mFatalityExclusionRange);
      else if(name=="GameOverDelay")
         node.getTextAsFloat(mGameOverDelay);
      else if(name=="InfantryCorpseDecayTime")
         node.getTextAsFloat(mInfantryCorpseDecayTime);
      else if(name=="CorpseSinkingSpacing")
         node.getTextAsFloat(mCorpseSinkingSpacing);
      else if(name=="MaxCorpseDisposalCount")
         node.getTextAsLong(mMaxCorpseDisposalCount);
      else if(name=="MaxSquadPathsPerFrame")
         node.getTextAsUInt32(mMaxSquadPathingCallsPerFrame);
      else if(name=="MaxPlatoonPathsPerFrame")
         node.getTextAsUInt32(mMaxPlatoonPathingCallsPerFrame);
      else if(name=="ProjectileGravity")
         node.getTextAsFloat(mProjectileGravity);
      else if(name=="ProjectileTumbleRate")
      {
         node.getTextAsFloat(mProjectileTumbleRate);
         mProjectileTumbleRate = XMConvertToRadians(mProjectileTumbleRate);
      }
      else if(name=="TrackInterceptDistance")
         node.getTextAsFloat(mTrackInterceptDistance);
      else if(name=="StationaryTargetAttackToleranceAngle")
         node.getTextAsFloat(mStationaryTargetAttackToleranceAngle);
      else if(name=="MovingTargetAttackToleranceAngle")
         node.getTextAsFloat(mMovingTargetAttackToleranceAngle);
      else if(name=="MovingTargetTrackingAttackToleranceAngle")
         node.getTextAsFloat(mMovingTargetTrackingAttackToleranceAngle);
      else if(name=="MovingTargetRangeMultiplier")
         node.getTextAsFloat(mMovingTargetRangeMultiplier);
      else if(name=="CloakingDelay")
      {
         float floatDelay;
         node.getTextAsFloat(floatDelay);
         floatDelay *= 1000.0f;
         mCloakingDelay = (DWORD) floatDelay;
      }
      else if(name=="ReCloakDelay")
      {
         float floatDelay;
         node.getTextAsFloat(floatDelay);
         floatDelay *=1000.0f;
         mReCloakingDelay = (DWORD) floatDelay;
      }
      else if(name=="CloakDetectFrequency")
      {
         float floatFrequency;
         node.getTextAsFloat(floatFrequency);
         floatFrequency*= 1000.0f;
         mCloakDetectFrequency = (DWORD) floatFrequency;
      }
      else if(name=="ShieldRegenDelay")
      {
         float floatDelay;
         node.getTextAsFloat(floatDelay);
         floatDelay *= 1000.0f;
         mShieldRegenDelay = (DWORD) floatDelay;
      }
      else if(name=="ShieldRegenTime")
      {
         float floatTime;
         node.getTextAsFloat(floatTime);
         mShieldRegenRate = 1.0f / floatTime;
         floatTime *= 1000.0f;
         mShieldRegenTime = (DWORD)floatTime;
      }
      else if (name == "AttackedRevealerLOS")
      {
         float attackedRevealerLOS;
         node.getTextAsFloat(attackedRevealerLOS);
         mAttackedRevealerLOS = attackedRevealerLOS;
      }
      else if (name == "AttackedRevealerLifespan")
      {
         float floatAttackedRevealerLifespan;
         node.getTextAsFloat(floatAttackedRevealerLifespan);
         floatAttackedRevealerLifespan *= 1000.0f;
         mAttackedRevealerLifespan = (DWORD)floatAttackedRevealerLifespan;
      }
      else if (name == "AttackRevealerLOS")
      {
         float attackRevealerLOS;
         node.getTextAsFloat(attackRevealerLOS);
         mAttackRevealerLOS = attackRevealerLOS;
      }
      else if (name == "AttackRevealerLifespan")
      {
         float floatAttackRevealerLifespan;
         node.getTextAsFloat(floatAttackRevealerLifespan);
         floatAttackRevealerLifespan *= 1000.0f;
         mAttackRevealerLifespan = (DWORD)floatAttackRevealerLifespan;
      }
      else if (name == "MinimumRevealerSize")
      {
         float minimumRevealerSize;
         node.getTextAsFloat(minimumRevealerSize);
         mMinimumRevealerSize = minimumRevealerSize;
      }
      else if (name == "AttackRatingMultiplier")
         node.getTextAsFloat(mAttackRatingMultiplier);
      else if (name == "DefenseRatingMultiplier")
         node.getTextAsFloat(mDefenseRatingMultiplier);
      else if (name == "GoodAgainstMinAttackGrade")
         node.getTextAsUInt(mGoodAgainstMinAttackGrade);
      else if(name=="HeightBonusDamage")
      {
         node.getTextAsFloat(mHeightBonusDamage);
      }
      else if(name=="ShieldBarColor")
         convertTokenToDWORDColor(node.getTextPtr(tempStr), mShieldBarColor, 255);
      else if(name=="AmmoBarColor")
         convertTokenToDWORDColor(node.getTextPtr(tempStr), mAmmoBarColor, 255);
      else if(name=="OpportunityDistPriFactor")
         node.getTextAsFloat(mOpportunityDistPriFactor);
      else if(name=="OpportunityBeingAttackedPriBonus")
         node.getTextAsFloat(mOpportunityBeingAttackedPriBonus);                            
      else if(name=="ChanceToRocket")
         node.getTextAsFloat(mChanceToRocket);
      else if(name=="MaxDamageBankPctAdjust")
         node.getTextAsFloat(mMaxDamageBankPctAdjust);      
      else if(name=="DamageBankTimer")
         node.getTextAsFloat(mDamageBankTimer);            
      else if (name == "BuildingSelfDestructTime")
      {
         float time;
         node.getTextAsFloat(time);
         time *= 1000.0f;
         mBuildingSelfDestructTime = (DWORD)time;
      }
      else if (name == "TributeAmount")
         node.getTextAsFloat(mTributeAmount);
      else if (name == "TributeCost")
         node.getTextAsFloat(mTributeCost);
      else if (name == "UnscSupplyPadBonus")
         node.getTextAsFloat(mUnscSupplyPadBonus);
      else if (name == "UnscSupplyPadBreakEvenPoint")
         node.getTextAsFloat(mUnscSupplyPadBreakEvenPoint);
      else if (name == "CovSupplyPadBonus")
         node.getTextAsFloat(mCovSupplyPadBonus);
      else if (name == "CovSupplyPadBreakEvenPoint")
         node.getTextAsFloat(mCovSupplyPadBreakEvenPoint);
      else if (name == "LeaderPowerChargeResource")
         mLeaderPowerChargeResourceID = getResource(node.getTextPtr(tempStr));
      else if (name == "LeaderPowerChargeRate")
         mLeaderPowerChargeRateID = getRate(node.getTextPtr(tempStr));
      else if (name == "DamageReceivedXPFactor")
         node.getTextAsFloat(mDamageReceivedXPFactor);
      else if(name=="AirStrikeLoiterTime")
         node.getTextAsFloat(mAirStrikeLoiterTime);
      else if(name=="RecyleRefundRate")
         node.getTextAsFloat(mRecyleRefundRate);
      else if(name=="BaseRebuildTimer")
         node.getTextAsFloat(mBaseRebuildTimer);
      else if (name == "ObjectiveArrowRadialOffset")
         node.getTextAsFloat(mObjectiveArrowRadialOffset);      
      else if (name == "ObjectiveArrowSwitchOffset")
         node.getTextAsFloat(mObjectiveArrowSwitchOffset);
      else if (name == "ObjectiveArrowYOffset")
         node.getTextAsFloat(mObjectiveArrowYOffset);
      else if (name == "ObjectiveArrowMaxIndex")
         node.getTextAsUInt8(mObjectiveArrowMaxIndex);
      else if(name=="OverrunMinVel")
         node.getTextAsFloat(mOverrunMinVel);
      else if(name=="OverrunJumpForce")
         node.getTextAsFloat(mOverrunJumpForce);
      else if(name=="OverrunDistance")
         node.getTextAsFloat(mOverrunDistance);
      else if (name == "CoopResourceSplitRate")
         node.getTextAsFloat(mCoopResourceSplitRate);
      else if (name == "HeroDownedLOS")
         node.getTextAsFloat(mHeroDownedLOS);
      else if (name == "HeroHPRegenTime")
         node.getTextAsFloat(mHeroHPRegenTime);
      else if (name == "HeroRevivalDistance")
         node.getTextAsFloat(mHeroRevivalDistance);
      else if (name == "HeroPercentHPRevivalThreshhold")
         node.getTextAsFloat(mHeroPercentHPRevivalThreshhold);
      else if (name == "MaxDeadHeroTransportDist")
         node.getTextAsFloat(mHeroMaxDeadTransportDist);
      else if (name == "TransportClearRadiusScale")
         node.getTextAsFloat(mTransportClearRadiusScale);
      else if (name == "TransportMaxSearchRadiusScale")
         node.getTextAsFloat(mTransportMaxSearchRadiusScale);
      else if (name == "TransportMaxSearchLocations")
         node.getTextAsUInt(mTransportMaxSearchLocations);
      else if (name == "TransportBlockTime")
      {
         node.getTextAsDWORD(mTransportBlockTime);
         mTransportBlockTime *= 1000;
      }
      else if (name == "TransportLoadBlockTime")
      {
         node.getTextAsDWORD(mTransportLoadBlockTime);
         mTransportLoadBlockTime *= 1000;
      }
      else if (name == "ALMaxWanderFrequency")
      {
         node.getTextAsDWORD(mALMaxWanderFrequency);
         mALMaxWanderFrequency *= 1000;
      }
      else if (name == "ALPredatorCheckFrequency")
      {
         node.getTextAsDWORD(mALPredatorCheckFrequency);
         mALPredatorCheckFrequency *= 1000;
      }
      else if (name == "ALPreyCheckFrequency")
      {
         node.getTextAsDWORD(mALPreyCheckFrequency);
         mALPreyCheckFrequency *= 1000;
      }
      else if (name == "ALOppCheckRadius")
      {
         node.getTextAsFloat(mALOppCheckRadius);
      }
      else if (name == "ALFleeDistance")
      {
         node.getTextAsFloat(mALFleeDistance);
      }
      else if (name == "ALFleeMovementModifier")
      {
         node.getTextAsFloat(mALFleeMovementModifier);
      }
      else if (name == "ALMinWanderDistance")
      {
         node.getTextAsFloat(mALMinWanderDistance);
      }
      else if (name == "ALMaxWanderDistance")
      {
         node.getTextAsFloat(mALMaxWanderDistance);
      }
      else if (name == "ALSpawnerCheckFrequency")
      {
         node.getTextAsFloat(mALSpawnerCheckFrequency);
         mALSpawnerCheckFrequency *= 1000;
      }
 	  else if (name == "TransportMaxBlockAttempts")
         node.getTextAsUInt(mTransportMaxBlockAttempts);
     else if (name == "TransportIncomingHeight")
        node.getTextAsFloat(mTransportIncomingHeight);
     else if (name == "TransportIncomingOffset")
        node.getTextAsFloat(mTransportIncomingOffset);
     else if (name == "TransportOutgoingHeight")
        node.getTextAsFloat(mTransportOutgoingHeight);
     else if (name == "TransportOutgoingOffset")
        node.getTextAsFloat(mTransportOutgoingOffset);
     else if (name == "TransportPickupHeight")
        node.getTextAsFloat(mTransportPickupHeight);
     else if (name == "TransportDropoffHeight")
        node.getTextAsFloat(mTransportDropoffHeight);
     else if (name == "TransportMax")
        node.getTextAsUInt(mTransportMax);
     else if (name == "HitchOffset")
        node.getTextAsFloat(mHitchOffset);
     else if (name == "TimeFrozenToThaw")
        node.getTextAsFloat(mTimeFrozenToThaw);
     else if (name == "TimeFreezingToThaw")
        node.getTextAsFloat(mTimeFreezingToThaw);
     else if (name == "DefaultCryoPoints")
        node.getTextAsFloat(mDefaultCryoPoints);
     else if (name == "DefaultThawSpeed")
        node.getTextAsFloat(mDefaultThawSpeed);
     else if (name == "FreezingSpeedModifier")
        node.getTextAsFloat(mFreezingSpeedModifier);
     else if (name == "FreezingDamageModifier")
        node.getTextAsFloat(mFreezingDamageModifier);
     else if (name == "FrozenDamageModifier")
        node.getTextAsFloat(mFrozenDamageModifier);
     else if (name == "Dot")
     {
        node.getAttribValueAsFloat("small", mSmallDotSize);
        node.getAttribValueAsFloat("medium", mMediumDotSize);
     }
   }

   // Initialize our cost allocator after resources have been loaded.
   BCost::setNumberResources(getNumberResources());
   BASSERTM(BCost::getNumberResources() <= BCost::cMaxNumResources, "Num resources exceeds max size of array!");

   return true;
}

//==============================================================================
// BDatabase::setupImpactEffects
//==============================================================================
bool BDatabase::setupImpactEffects()
{
   BXMLReader reader;
   if(!reader.load(cDirData, "impacteffects.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   BXMLNode rootNode(reader.getRootNode());
   long nodeCount=rootNode.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if(name=="TerrainEffect")
      {
         long terrainEffectIndex;
         BSimString impactEffectName;
         float lifespan = 3.0f;
         int   limit = 2;
         float radius = 10.0f;
         node.getAttribValueAsString("name", impactEffectName);
         node.getAttribValueAsFloat("lifespan", lifespan);
         node.getAttribValueAsInt("limit", limit);         

         BSimString tempStr;
         terrainEffectIndex = gTerrainEffectManager.getOrCreateTerrainEffect(node.getTextPtr(tempStr), false);
         BTerrainEffect* pTE = gTerrainEffectManager.getTerrainEffect(terrainEffectIndex, false);
         if (!pTE)
            continue;
         pTE->setMeterLength(static_cast<DWORD>(lifespan * 1000.0f));
         pTE->setMeterCount(static_cast<uint>(limit));


         BProtoImpactEffect data;
         data.mName = impactEffectName;
         data.mTerrainEffectIndex = terrainEffectIndex;
         data.mLifespan = lifespan;

         data.mLimit = limit;
         data.mBoundingRadius = radius;

         int newIndex = mProtoImpactEffects.getNumber();
         mProtoImpactEffects.add(data);

         impactEffectName.toLower();
         mImpactEffectHashMap.insert(impactEffectName, newIndex);
      }  
   }

   return true;
}

//=============================================================================
// BDatabase:getImpactSound
//=============================================================================
const BImpactSoundInfo* BDatabase::getImpactSound(int8 soundSetIndex, uint8 surfaceType) const
{
   if ((surfaceType >= getNumberSurfaceTypes()) || (soundSetIndex < 0) || (soundSetIndex >= getNumberImpactSoundSets()))
   {
      return NULL;
   }

   return &mImpactSoundSets[soundSetIndex].mImpactSounds[surfaceType];
}

//==============================================================================
// BDatabase::getImpactSoundSetIndex
//==============================================================================
int8 BDatabase::getImpactSoundSetIndex(const char* pName) const
{
   // Look for the impact sound set by name
   BSimString name(pName);
   for (long i = 0; i < getNumberImpactSoundSets(); i++)
   {
      if (name == mImpactSoundSets[i].mName)
         return (int8) i;
   }

   // Didn't find it
   return -1;
}

//==============================================================================
// BDatabase::resolvePowerTextures
//==============================================================================
void BDatabase::resolveProtoPowerTextures()
{
   for(long i = 0; i < mProtoPowers.getNumber(); i++)
   {
      BProtoPower* pProtoPower = mProtoPowers[i];
      if(pProtoPower)
         pProtoPower->resolveTextures();
   }
}

//==============================================================================
// BDatabase::setupImpactSounds
//==============================================================================
bool BDatabase::setupImpactSounds()
{
   BXMLReader reader;
   if(!reader.load(cDirData, "impactsounds.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   BXMLNode rootNode(reader.getRootNode());

   // Alloc sound sets
   long nodeCount=rootNode.getNumberChildren();
   mImpactSoundSets.setNumber(nodeCount);

   // Iterate over them
   for(long impactSoundSetIndex=0; impactSoundSetIndex<nodeCount; impactSoundSetIndex++)
   {
      BXMLNode node(rootNode.getChild(impactSoundSetIndex));
      
      if(node.getName() != "ImpactSoundSet")
         continue;

      //-- Determine what proto object this is referring to
      BSimString name;
      node.getAttribValueAsString("name", name);
      long protoID = gDatabase.getProtoObject(name);

      BImpactSoundSet& impactSoundSet = mImpactSoundSets[impactSoundSetIndex];

      // Set the name
      impactSoundSet.mName = name;

      // Alloc and init sound cues
      long numSounds = node.getNumberChildren();
      impactSoundSet.mImpactSounds.setNumber(gDatabase.getNumberSurfaceTypes());
      for(long soundCue=0; soundCue < impactSoundSet.mImpactSounds.getNumber(); soundCue++)
         impactSoundSet.mImpactSounds[soundCue].mSoundCue = cInvalidCueIndex;

      //-- Load the sound cues for this impact sound set
      for(long soundCue = 0; soundCue < numSounds; soundCue++)
      {
         BXMLNode sound(node.getChild(soundCue));      
         BSimString surfaceTypeStr;
         sound.getAttribValueAsString("type", surfaceTypeStr);
         byte surfaceType = gDatabase.getSurfaceType(surfaceTypeStr);

         BSimString soundCueStr;
         sound.getText(soundCueStr);

         BCueIndex cueIndex = gSoundManager.getCueIndex(soundCueStr);
         if(cueIndex != cInvalidCueIndex)
         {                  
            BImpactSoundInfo info;

            //-- Load the cue index
            info.mSoundCue = cueIndex;

            //-- Load checkVis or not
            BSimString checkSoundRadius;
            sound.getAttribValueAsString("checkMaxDist", checkSoundRadius);
            if(checkSoundRadius == L"false")
               info.mCheckSoundRadius = false;

            //-- Put it in the array
            impactSoundSet.mImpactSounds[surfaceType] = info;
         }
      }

      BProtoObject *pProto = gDatabase.getGenericProtoObject(protoID);
      if(pProto)
         pProto->setImpactSoundSet((int8) impactSoundSetIndex);
   }

   // mrh 11/29/06 - added return statement to remove warning.
   return(true);
}

//==============================================================================
// BDatabase::setupRumble
//==============================================================================
bool BDatabase::setupRumble()
{
   BXMLReader reader;
   if (!reader.load(cDirData, "rumble.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;

   mRumbleEvents.setNumber(BRumbleEvent::cNumEvents);
   for (uint i=0; i<BRumbleEvent::cNumEvents; i++)
   {
      BRumbleEvent& rumbleEvent=mRumbleEvents[i];
      rumbleEvent.mPattern=-1;
      rumbleEvent.mLeftRumbleType=BGamepad::cRumbleTypeNone;
      rumbleEvent.mRightRumbleType=BGamepad::cRumbleTypeNone;
      rumbleEvent.mLeftStrength=0.0f;
      rumbleEvent.mRightStrength=0.0f;
      rumbleEvent.mDuration=0.0f;
      rumbleEvent.mEnabled=false;
      rumbleEvent.mMultiple=false;
   }

   BXMLNode rootNode(reader.getRootNode());
   int nodeCount=rootNode.getNumberChildren();
   for (int i=0; i<nodeCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if (name == "Pattern")
      {
         BGamepad::loadRumblePattern(node);
      }
      else if (name == "Event")
      {
         BSimString str;
         if (node.getAttribValueAsString("EventType", str))
         {
            int eventType=-1;
            if (str=="Trigger")
               eventType=BRumbleEvent::cTypeTrigger;
            else if (str=="Impact")
               eventType=BRumbleEvent::cTypeImpact;
            else if (str == "Collision")
               eventType=BRumbleEvent::cTypeCollision;
            else if (str=="AnimTag")
               eventType=BRumbleEvent::cTypeAnimTag;
            else if (str=="Flare")
               eventType=BRumbleEvent::cTypeFlare;
            else if (str=="Attack")
               eventType=BRumbleEvent::cTypeAttack;
            else if (str=="TrainComplete")
               eventType=BRumbleEvent::cTypeTrainComplete;
            else if (str=="ResearchComplete")
               eventType=BRumbleEvent::cTypeResearchComplete;
            else if (str=="BuildComplete")
               eventType=BRumbleEvent::cTypeBuildComplete;
            else if (str=="NewObjective")
               eventType=BRumbleEvent::cTypeNewObjective;
            else if (str=="ObjectiveComplete")
               eventType=BRumbleEvent::cTypeObjectiveComplete;
            else if (str=="UnitSelect")
               eventType=BRumbleEvent::cTypeUnitSelect;
            else if (str=="PaintSelect")
               eventType=BRumbleEvent::cTypePaintSelect;
            else if (str=="UnitHover")
               eventType=BRumbleEvent::cTypeUnitHover;
            else if (str=="UnitHovering")
               eventType=BRumbleEvent::cTypeUnitHovering;
            else if (str=="ConfirmCommand")
               eventType=BRumbleEvent::cTypeConfirmCommand;
            else if (str=="CantDoCommand")
               eventType=BRumbleEvent::cTypeCantDoCommand;
            else if (str=="UIRollover")
               eventType=BRumbleEvent::cTypeUIRollover;
            else if (str=="RadialMenuItem")
               eventType=BRumbleEvent::cTypeRadialMenuItem;
            if (eventType!=-1)
            {
               BRumbleEvent& rumbleEvent=mRumbleEvents[eventType];
               if (node.getAttribValueAsString("Pattern", str))
                  rumbleEvent.mPattern=(int8)BGamepad::getRumblePattern(str);
               else
               {
                  if (node.getAttribValueAsString("LeftRumbleType", str))
                     rumbleEvent.mLeftRumbleType=(int8)BGamepad::getRumbleType(str);
                  if (node.getAttribValueAsString("RightRumbleType", str))
                     rumbleEvent.mRightRumbleType=(int8)BGamepad::getRumbleType(str);
                  node.getAttribValueAsHalfFloat("LeftStrength", rumbleEvent.mLeftStrength);
                  node.getAttribValueAsHalfFloat("RightStrength", rumbleEvent.mRightStrength);
                  node.getAttribValueAsHalfFloat("Duration", rumbleEvent.mDuration);
               }
               bool val=false;
               node.getAttribValueAsBool("Enabled", val);
               rumbleEvent.mEnabled=val;
               val=false;
               node.getAttribValueAsBool("Multiple", val);
               rumbleEvent.mMultiple=val;
            }
         }
      }
   }
   return true;
}

//==============================================================================
// bool BDatabase::setupHPBars()
//==============================================================================
bool BDatabase::setupHPBars()
{
   BXMLReader reader;

   if(!reader.load(cDirData, "hpbars.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;

   const BXMLNode rootNode(reader.getRootNode());
   
   // -- make sure we are dealing with the right kind of file
   const BPackedString szRootName(rootNode.getName());
   if (szRootName.compare(("HPBarDefinition")) != 0)
   {
      {setBlogError(4138); blogerror("BHPBar::loadXML: this is not a hpbar definition file.");}
      return (false);
   }

   BXMLNode node;
   int numChildren = rootNode.getNumberChildren();
   for (int i = 0; i < numChildren; ++i)
   {
      node = rootNode.getChild(i);
      const BPackedString szTag(node.getName());
      if (szTag.compare(("HPBar")) == 0)
      {
         BProtoHPBar* pProtoHP = new BProtoHPBar();

         if (!pProtoHP->load(node, &reader))
         {
            delete pProtoHP;
            return false;
         }

         mProtoHPBars.pushBack(pProtoHP);
      }
      else if (szTag.compare("ColorStages") == 0)
      {
         BProtoHPBarColorStages* pProtoColor = new BProtoHPBarColorStages();
         if (!pProtoColor->load(node, &reader))
         {
            delete pProtoColor;
            return false;
         }

         mProtoHPBarColorStages.pushBack(pProtoColor);
      }
      else if (szTag.compare("VeterancyBar") == 0)
      {
         BProtoVeterancyBar* pProto = new BProtoVeterancyBar();
         if (!pProto->load(node, &reader))
         {
            delete pProto;
            return false;
         }

         mProtoVeterancyBars.pushBack(pProto);
      }
      else if (szTag.compare("PieProgress") == 0)
      {
         BProtoPieProgress* pProtoPie = new BProtoPieProgress();
         if (!pProtoPie->load(node, &reader))
         {
            delete pProtoPie;
            return false;
         }

         mProtoPieProgressBars.pushBack(pProtoPie);
      }
      else if (szTag.compare("BobbleHead") == 0)
      {
         BProtoBobbleHead* pProto = new BProtoBobbleHead();
         if (!pProto->load(node, &reader))
         {
            delete pProto;
            return false;
         }

         mProtoBobbleHeads.pushBack(pProto);
      }
      else if (szTag.compare("BuildingStrength") == 0)
      {
         BProtoBuildingStrength* pProto = new BProtoBuildingStrength();
         if (!pProto->load(node, &reader))
         {
            delete pProto;
            return false;
         }

         mProtoBuildingStrength.pushBack(pProto);         
      }
   }
   return true;
}

//==============================================================================
// BDatabase::getProtoHPBarID
//==============================================================================
long BDatabase::getProtoHPBarID(const char* pName) const
{
   long count=mProtoHPBars.getNumber();
   for(long i=0; i<count; i++)
   {
      if(mProtoHPBars[i]->mName==pName)
         return i;
   }
   return -1;   
}

//==============================================================================
// BDatabase::getProtoHPColorStageID
//==============================================================================
long BDatabase::getProtoHPColorStageID(const char* pName) const
{
   long count=mProtoHPBarColorStages.getNumber();
   for(long i=0; i<count; i++)
   {
      if(mProtoHPBarColorStages[i]->mName==pName)
         return i;
   }
   return -1;   
}

//==============================================================================
// BDatabase::getProtoVeterancyBarID
//==============================================================================
long BDatabase::getProtoVeterancyBarID(const char* pName) const
{
   long count=mProtoVeterancyBars.getNumber();
   for(long i=0; i<count; i++)
   {
      if(mProtoVeterancyBars[i]->mName==pName)
         return i;
   }
   return -1;   
}

//==============================================================================
// BDatabase::getProtoPieProgressBarID
//==============================================================================
long BDatabase::getProtoPieProgressBarID(const char* pName) const
{
   long count=mProtoPieProgressBars.getNumber();
   for(long i=0; i<count; i++)
   {
      if(mProtoPieProgressBars[i]->mName==pName)
         return i;
   }
   return -1;   
}

//==============================================================================
// BDatabase::getProtoBobbleHeadID
//==============================================================================
long BDatabase::getProtoBobbleHeadID(const char* pName) const
{
   long count=mProtoBobbleHeads.getNumber();
   for(long i=0; i<count; i++)
   {
      if(mProtoBobbleHeads[i]->mName==pName)
         return i;
   }
   return -1;   
}

//==============================================================================
// BDatabase::getProtoBuildingStrengthID
//==============================================================================
long BDatabase::getProtoBuildingStrengthID(const char* pName) const
{
   long count=mProtoBuildingStrength.getNumber();
   for(long i=0; i<count; i++)
   {
      if(mProtoBuildingStrength[i]->mName==pName)
         return i;
   }
   return -1;   
}

//==============================================================================
// BDatabase::setupLocStrings
//==============================================================================
bool BDatabase::setupLocStrings(bool reload)
{
   BXMLReader reader;
   if(!reader.load(cDirData, "stringtable.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;

   BXMLNode rootNode(reader.getRootNode());
   long nodeCount=rootNode.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      if(node.getName()=="Language")
      {
         BSimString language;
         node.getAttribValueAsString("name", language);
         //AJL FIXME 10/12/06 - Hard-coded to look for English for now
         if(language=="English")
         {
            for(long j=0; j<node.getNumberChildren(); j++)
            {
               BXMLNode childNode(node.getChild(j));
               if(childNode.getName()=="String")
               {
                  long stringID;
                  if(childNode.getAttribValueAsLong("_locID", stringID))
                  {
                     BUString uString;
#ifndef BUILD_FINAL
                     if (gConfig.isDefined(cConfigLocIdInString))
                     {
                        BUString temp;
                        childNode.getText(temp);
                        uString.format(L"(%d) %s", stringID, temp.getPtr());
                     }
                     else
#endif
                        childNode.getText(uString);

                     bool updated=false;
                     if(reload)
                     {
                        long index=getLocStringIndex(stringID);
                        if(index>=0 && index<mLocStrings.getNumber())
                        {
                           mLocStrings[index]=uString;
                           updated=true;
                        }
                     }
                     if(!updated)
                     {
                        long index=mLocStrings.add(uString);
                        if(index!=-1)
                        {
                           // AJL FIXME - String ID's should be integers, not strings
                           BString idName;
                           idName.format("%d", stringID);
                           mLocStringTable.add(idName, index);
                        }
                     }
                  }
               }
            }
         }
      }
   }

   return true;
}

//==============================================================================
// BDatabase::setupDependentData
//==============================================================================
bool BDatabase::setupDependentData()
{
   BXMLReader reader;
   BXMLNode rootNode;
   long count;

   // Surface types
   if (!reader.load(cDirData, "terrainTileTypes.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   rootNode = reader.getRootNode();
   count = rootNode.getNumberChildren();
   mNumberSurfaceTypes = 0;
   for (byte i = 0; i < (byte) count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      BSimString string;
      if(node.getAttribValue("name", &string))
      {
         if (mSurfaceTypes.add(string, i) != BStringTable<byte>::cErrOK)
         {
            blog("BDatabase::setupDependedData - Error - Could not add surface type %s (duplicate?)", string.getPtr());
            BASSERT(0);
            return false;
         }

         mNumberSurfaceTypes++;
      }
   }
   
   if (!gArchiveManager.getArchivesEnabled())
   {
      // Terrain tile type
      mSurfaceTypeImpactEffect = gTerrainEffectManager.getOrCreateTerrainEffect("effects\\terraineffects\\terrainTile", false);
   }

#ifndef BUILD_FINAL
   // Sound Table
   if(!reader.load(cDirData, "soundTable.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   
   BSimString cueName;
   long cueIndex;

   rootNode=reader.getRootNode();
   count=rootNode.getNumberChildren();
   for(long i=0; i<count; i++)
   {
      cueIndex = cInvalidCueIndex;
      cueName.empty();

      BSimString cueName;
      BCueIndex cueIndex = cInvalidCueIndex;
      BXMLNode node(rootNode.getChild(i));
      BSimString string;
      node.getName(string);
      if(string == "Sound")
      {
         long numChildren = node.getNumberChildren();
         for(long j = 0; j < numChildren; j++)
         {
            BXMLNode child(node.getChild(j));
            if(child.getName() == "CueName")
            {
               child.getText(cueName);
            }
            else if(child.getName() == "CueIndex")
            {
               child.getTextAsUInt32(cueIndex);
            }
         }
         
         if(cueIndex != cInvalidCueIndex && !cueName.isEmpty())
         {
            mSoundCueTable.add(cueName, cueIndex);
         }
      }
   }
#endif

   // One object type per proto object
   mpObjectsReader = new BXMLReader();
   if(!mpObjectsReader->load(cDirData, "objects.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   rootNode=mpObjectsReader->getRootNode();
   count=rootNode.getNumberChildren();
   mNumberBaseObjects=0;
   int id = 0;
   for(long i=0; i<count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      BSimString string;
      if(node.getAttribValue("name", &string))
      {
         if(mObjectTypes.add(string, id)!=BStringTableLong::cErrOK)
         {
            blog("BDatabase::setupDependedData - Error - Could not add object %s (duplicate?)", string.getPtr());
            BASSERT(0);
            return false;
         }
         mNumberBaseObjects++;
         id++;
      }
   }

   // Abstract object types
   if(!reader.load(cDirData, "objecttypes.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   rootNode=reader.getRootNode();
   count=rootNode.getNumberChildren();
   for(long i=0; i<count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      BSimString temp;
      mObjectTypes.add(node.getTextPtr(temp), mNumberBaseObjects+i);
   }
   mNumberAbstractObjectTypes = count;
   mNumberObjectTypes = mNumberBaseObjects + mNumberAbstractObjectTypes;

   // Proto squad name table
   mpSquadsReader = new BXMLReader();
   if (!mpSquadsReader->load(cDirData, "squads.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   rootNode = mpSquadsReader->getRootNode();
   count = rootNode.getNumberChildren();
   id = 0;
   for (long i = 0; i < count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if (name == "Squad")
      {
         BSimString string;
         if(node.getAttribValue("name", &string))
         {
            mProtoSquadTable.add(string, id);
            id++;
         }
      }
   }

   // Tech name table
   mpTechsReader = new BXMLReader();
   if(!mpTechsReader->load(cDirData, "techs.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   rootNode=mpTechsReader->getRootNode();
   count=rootNode.getNumberChildren();
   mProtoTechs.reserve(count);
   for(long i=0; i<count; i++)
   {
      BProtoTech* pProtoTech=new BProtoTech();
      if(!pProtoTech)
         return false;
      if(!pProtoTech->init(i))
      {
         delete pProtoTech;
         return false;
      }
      mProtoTechs.add(pProtoTech);
      BXMLNode node(rootNode.getChild(i));
      BSimString string;
      if(node.getAttribValue("name", &string))
         mTechTable.add(string, i);
   }

   // Leaders
   mpLeadersReader = new BXMLReader();
   if (!mpLeadersReader->load(cDirData, "leaders.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return(false);   
   rootNode = mpLeadersReader->getRootNode();
   count = rootNode.getNumberChildren();
   for (long i=0; i < count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      if (node.getName().compare("Leader") != 0)
         continue;
      if (gConfig.isDefined(cConfigAlpha))
      {
         int alpha=-1;
         node.getAttribValueAsInt("Alpha", alpha);
         if (alpha == 0)
            continue;
      }
      BLeader* pLeader = new BLeader();
      if(!pLeader)
         return false;
      if(!pLeader->preload(node))
      {
         delete pLeader;
         return false;
      }
      mLeaders.add(pLeader);
   }

   // Civs
   mpCivsReader = new BXMLReader();
   if(!mpCivsReader->load(cDirData, "civs.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   rootNode=mpCivsReader->getRootNode();
   count=rootNode.getNumberChildren();
   mCivs.reserve(count);
   for(long i=0; i <count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      if(node.getName()!="Civ")
         continue;
      if (gConfig.isDefined(cConfigAlpha))
      {
         int alpha=-1;
         node.getAttribValueAsInt("Alpha", alpha);
         if (alpha == 0)
            continue;
      }
      BCiv* pCiv=new BCiv(mCivs.size());
      if(!pCiv)
         return false;
      if(!pCiv->preload(node))
      {
         delete pCiv;
         return false;
      }
      mCivs.add(pCiv);
   }

   // Powers
   mpPowersReader = new BXMLReader();
   if (!mpPowersReader->load(cDirData, "powers.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   rootNode=mpPowersReader->getRootNode();
   count=rootNode.getNumberChildren();
   mProtoPowers.reserve(count);
   for (long i=0; i<count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      if (node.getName() != "Power")
         continue;
      BProtoPower* pProtoPower = new BProtoPower();
      if (!pProtoPower->preload(node))
      {
         delete pProtoPower;
         return false;
      }
      mProtoPowers.add(pProtoPower);
   }

   // Damage types
   if (!reader.load(cDirData, "damagetypes.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   rootNode = reader.getRootNode();
   count = rootNode.getNumberChildren();
   mDamageTypes.reserve(count);
   for (long i = 0; i < count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      BDamageType damageType;
      bool shielded = false;
      bool useForAttackRating = false;
      if (damageType.load(node, shielded, useForAttackRating))
      {        
         BDamageTypeID damageTypeID = static_cast<BDamageTypeID>(mDamageTypes.getNumber());
         if (useForAttackRating)
            damageType.setAttackRatingIndex(mAttackRatingDamageTypes.add(damageTypeID));
         mDamageTypes.add(damageType);
         if (mDamageTypeShielded == cInvalidDamageTypeID && shielded)
            mDamageTypeShielded = static_cast<BDamageTypeID>(damageTypeID);

         // Get our other damage types please.
         if (node.compareText("Light") == 0)
            mDamageTypeLight = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("LightInCover") == 0)
            mDamageTypeLightInCover = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("LightArmored") == 0)
            mDamageTypeLightArmored = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("LightArmoredInCover") == 0)
            mDamageTypeLightArmoredInCover = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("Medium") == 0)
            mDamageTypeMedium = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("MediumAir") == 0)
            mDamageTypeMediumAir = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("Heavy") == 0)
            mDamageTypeHeavy = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("Building") == 0)
            mDamageTypeBuilding = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("Shielded") == 0)
            mDamageTypeShielded = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("SmallAnimal") == 0)
            mDamageTypeSmallAnimal = static_cast<BDamageTypeID>(damageTypeID);
         if (node.compareText("BigAnimal") == 0)
            mDamageTypeBigAnimal = static_cast<BDamageTypeID>(damageTypeID);
      }
   }

   return true;
}


//==============================================================================
// BDatabase::decomposeObjectType
// Gets a list of proto objects that represent the input object type decomposed
// into only proto object types.
//==============================================================================
uint BDatabase::decomposeObjectType(BObjectTypeID objectTypeID, BProtoObjectIDArray** protoObjectIDs)
{
   bool validObjectType = isValidObjectType(objectTypeID);
   BASSERT(validObjectType);

   uint numObjectTypes = 0;
   if (protoObjectIDs)
      *protoObjectIDs = NULL;

   if (validObjectType)
   {
      numObjectTypes = mDecomposedObjectTypes[objectTypeID].getSize();
      if (protoObjectIDs)
         *protoObjectIDs = &(mDecomposedObjectTypes[objectTypeID]);
   }

   return numObjectTypes;
}


//==============================================================================
// findPlacementFiles
//==============================================================================
static bool CALLBACK findPlacementFiles(const BString& path, void* pParam)
{
   BDynamicSimArray<BSimString>* fileList = (BDynamicSimArray<BSimString>*)pParam;
   if(!fileList)
      return false;
   fileList->add(path);
   return true;
}

//==============================================================================
// BDatabase::setupPlacementRules
//==============================================================================
bool BDatabase::setupPlacementRules()
{
   BDynamicSimArray<BSimString> fileList;
   gFileManager.findFiles(cDirPlacementRules, "*.xml", BFFILE_WANT_FILES|BFFILE_RECURSE_SUBDIRS|BFFILE_TRY_XMB, findPlacementFiles, &fileList, false);
   
   long count=fileList.getNumber();
   for(long i=0; i<count; i++)
   {
      BPlacementRules* pRules = new BPlacementRules;
              
      if(!pRules->loadXML(fileList[i]))
      {
         delete pRules;
         return false;
      }
      mPlacementRules.add(pRules);
   }

   return true;
}


//==============================================================================
// BDatabase::setupPlacementTracking
//==============================================================================
bool BDatabase::setupPlacementTracking()
{
   long numProto = getNumberProtoObjects();
   for (long protoID = 0; protoID < numProto; protoID++)
   {
      bool track = false;
      BProtoObject *pProto = getGenericProtoObject(protoID);
      BDEBUG_ASSERT(pProto);

      long numRules = mPlacementRules.getNumber();
      for (long i = 0; !track && (i < numRules); i++)
      {
         BPlacementRules *pRules = mPlacementRules[i];
         BDEBUG_ASSERT(pRules);
         const BDynamicSimArray<BPlacementRuleUnit> &ruleUnits = pRules->getPlacementRuleUnits();

         long numUnits = ruleUnits.getNumber();
         for (long j = 0; j < numUnits; j++)
         {
            const BPlacementRuleUnit &ruleUnit = ruleUnits[j];

            long unitType = ruleUnit.getUnitType();
            if (pProto->isType(unitType))
            {
               pProto->setFlagTrackPlacement(true);
               track = true;
               break;
            }
         }
      }
   }

   return true;
}

//==============================================================================
// BDatabase::setupCivs
//==============================================================================
bool BDatabase::setupCivs()
{
   BXMLNode rootNode(mpCivsReader->getRootNode());
   long count=rootNode.getNumberChildren();
   long civID = 0;
   for(long i=0; i <count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      if(node.getName()!="Civ")
         continue;
      if (gConfig.isDefined(cConfigAlpha))
      {
         int alpha=-1;
         node.getAttribValueAsInt("Alpha", alpha);
         if (alpha == 0)
            continue;
      }
      BCiv* pCiv=getCiv(civID);
      if(!pCiv)
         return false;
      if(!pCiv->load(node))
         return false;
      civID++;
   }

   return true;
}

//=============================================================================
// BDatabase::setupLeaders
//=============================================================================
bool BDatabase::setupLeaders(void)
{
   BXMLNode rootNode(mpLeadersReader->getRootNode());

   //Load each leader
   long leaderID = 0;
   for (long i=0; i < rootNode.getNumberChildren(); i++)
   {
      BXMLNode leaderNode(rootNode.getChild(i));
      if (leaderNode.getName().compare("Leader") != 0)
         continue;
      if (gConfig.isDefined(cConfigAlpha))
      {
         int alpha=-1;
         leaderNode.getAttribValueAsInt("Alpha", alpha);
         if (alpha == 0)
            continue;
      }
      BLeader* pLeader = getLeader(leaderID);
      if(!pLeader)
         return false;
      pLeader->load(leaderNode);
      leaderID++;
   }

   return(true);
}

//=============================================================================
// BDatabase::removeAllLeaders
//=============================================================================
void BDatabase::removeAllLeaders(void)
{
   for(long i=0; i<mLeaders.getNumber(); i++)
      delete mLeaders[i];
   mLeaders.setNumber(0);
}

//=============================================================================
// BDatabase::getLeader
//=============================================================================
BLeader* BDatabase::getLeader(long index)
{
   if (index >= 0 && index < mLeaders.getNumber())
      return(mLeaders[index]);
   else
      return(NULL);
}

//==============================================================================
// BDatabase::getLeaderID
//==============================================================================
long BDatabase::getLeaderID(const char* pName) const
{
   long count=mLeaders.getNumber();
   for(long i=0; i<count; i++)
   {
      if(mLeaders[i]->mName==pName)
         return i;
   }
   return -1;
}

//==============================================================================
// BDatabase::loadLeaderIcons
//==============================================================================
void BDatabase::loadLeaderIcons()
{
   long count=mLeaders.getNumber();
   for(long i=0; i<count; i++)
      mLeaders[i]->mIconHandle=gUI.loadTexture(mLeaders[i]->mIconName);
}

//==============================================================================
// BDatabase::unloadLeaderIcons
//==============================================================================
void BDatabase::unloadLeaderIcons()
{
   long count=mLeaders.getNumber();
   for(long i=0; i<count; i++)
   {
      gUI.unloadTexture(mLeaders[i]->mIconHandle);
      mLeaders[i]->mIconHandle=cInvalidManagedTextureHandle;
   }
}

//==============================================================================
//==============================================================================
bool BDatabase::getInfectedForm(BProtoObjectID basePOID, BProtoObjectID& infectedPOID, BProtoSquadID& infectedPSID) const
{
   //There shouldn't be much here and it's infrequently called.
   for (uint i=0; i < mInfectionMap.getSize(); i++)
   {
      if (mInfectionMap[i].getPOID1() == basePOID)
      {
         infectedPOID=mInfectionMap[i].getPOID2();
         infectedPSID=mInfectionMap[i].getPSID2();
         return (true);
      }
   }
   return (false);
}


//==============================================================================
// BDatabase::preloadVisFiles
//==============================================================================
bool BDatabase::preloadVisFiles()
{
   // Loads all vis files ever needed when archives are enabled
   //

   if (!gArchiveManager.getArchivesEnabled())
      return false;

   SCOPEDSAMPLE(BDatabase_preloadVisFiles)

#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif   

   BSimString visFileListStr;
   visFileListStr.format("preloadVisFileList.txt");
   const char *pVisFileList = visFileListStr.getPtr();

   BFile visFileList;
   if (!visFileList.open(cDirProduction, pVisFileList, BFILE_OPEN_ENABLE_BUFFERING | BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      gConsoleOutput.error("BDatabase::preloadVisFiles: Unable to open %s", pVisFileList);
      return false;
   }

   BStream* pStream = visFileList.getStream();

   trace("VIS preload %s begin", pVisFileList);
      
   const int numUntilFlush = 8;
   uint currCount =0;
   for ( ; ; currCount++)
   {
      
      BString str;
      if (!pStream->readLine(str))
         break;
                              
      str.standardizePath();
      
      int i = str.findLeft("art\\");
      if (i != -1)
         str.crop(i + 4, str.length() - 1);
      
      if (str.isEmpty())
         continue;
         
      gConsoleOutput.resource("Preloading VIS file: %s, from %s\n", str.getPtr(), pVisFileList);

      
      if (gVisualManager.getOrCreateProtoVisual(str, false) < 0)
      {
         gConsoleOutput.error("Failed preloading VIS file: %s, from %s\n", str.getPtr(), pVisFileList);
      }

      if(currCount >= numUntilFlush)
      {
         gRenderThread.kickCommands();
         currCount=0;
      }
   }      
   
   
   
   visFileList.close();
   
   
   
   trace("VIS preload %s end", pVisFileList);
     
#ifndef BUILD_FINAL   
   double totalTime = timer.getElapsedSeconds();
   trace("BScenario::preloadVisFiles %s: %3.3f seconds", pVisFileList, totalTime);
#endif

   return true;
}

//==============================================================================
// BDatabase::preloadTfxFiles
//==============================================================================
bool BDatabase::preloadTfxFiles()
{
   // Loads all tfx files ever needed when archives are enabled
   //

   if (!gArchiveManager.getArchivesEnabled())
      return false;

   SCOPEDSAMPLE(BDatabase_preloadTfxFiles)

#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif   

   BSimString tfxFileListStr;
   tfxFileListStr.format("preloadTfxFileList.txt");
   const char *pTfxFileList = tfxFileListStr.getPtr();

   BFile tfxFileList;
   if (!tfxFileList.open(cDirProduction, pTfxFileList, BFILE_OPEN_ENABLE_BUFFERING | BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      gConsoleOutput.error("BDatabase::preloadTfxFiles: Unable to open %s", pTfxFileList);
      return false;
   }

   BStream* pStream = tfxFileList.getStream();

   trace("TFX preload %s begin", pTfxFileList);
      
   const int numUntilFlush = 8;
   uint currCount =0;
   for ( ; ; currCount++)
   {
      
      BString str;
      if (!pStream->readLine(str))
         break;
                              
      str.standardizePath();
      
      int i = str.findLeft("art\\");
      if (i != -1)
         str.crop(i + 4, str.length() - 1);
      
      if (str.isEmpty())
         continue;
         
      gConsoleOutput.resource("Preloading TFX file: %s, from %s\n", str.getPtr(), pTfxFileList);

      
      if (gTerrainEffectManager.getOrCreateTerrainEffect(str, false) < 0)
      {
         gConsoleOutput.error("Failed preloading TFX file: %s, from %s\n", str.getPtr(), pTfxFileList);
      }

      if(currCount >= numUntilFlush)
      {
         gRenderThread.kickCommands();
         currCount=0;
      }
   }      
   
   tfxFileList.close();
   
   
   // Terrain tile type
   mSurfaceTypeImpactEffect = gTerrainEffectManager.getOrCreateTerrainEffect("effects\\terraineffects\\terrainTile", false);

   
   trace("TFX preload %s end", pTfxFileList);
     
#ifndef BUILD_FINAL   
   double totalTime = timer.getElapsedSeconds();
   trace("BScenario::preloadTfxFiles %s: %3.3f seconds", pTfxFileList, totalTime);
#endif

   return true;
}

//==============================================================================
// BDatabase::preloadPhysicsFiles
//==============================================================================
bool BDatabase::preloadPhysicsFiles()
{
   // Loads all physics related files (.blueprint, .physics, .shp) ever needed when archives are enabled
   //

   if (!gArchiveManager.getArchivesEnabled())
      return false;

   SCOPEDSAMPLE(BDatabase_preloadPhysicsFiles)

#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif   

   BDynamicArray<BString> files;
   eFileManagerError result = gFileManager.findFiles(cDirPhysics, "*.*", files, false, BFFILE_WANT_FILES, BFileManager::cIgnoreLooseFiles);
   if (result != cFME_SUCCESS)
   {
      gConsoleOutput.error("BDatabase::preloadPhysicsFiles: Unable to find any physics files");
      return false;
   }

   uint count = files.size();

   // Shapes
   for (uint i=0; i<count; i++)
   {
      BString& file = files[i];
      long len = file.length();
      // Check for .shp.xmb extension
      if (len < 8 || strnicmp(file.getPtr() + (len - 8), ".shp", 4) != 0)
         continue;
      file.left(len - 8);
      gPhysics->getShapeManager().getOrCreate(file, false);
   }

   // Blue Prints
   for (uint i=0; i<count; i++)
   {
      BString& file = files[i];
      long len = file.length();
      // Check for .blueprint.xmb extension
      if (len < 14 || strnicmp(file.getPtr() + (len - 14), ".blueprint", 10) != 0)
         continue;
      file.left(len - 14);
      gPhysics->getPhysicsObjectBlueprintManager().getOrCreate(file, true);
   }

   // Physics Infos
   for (uint i=0; i<count; i++)
   {
      BString& file = files[i];
      long len = file.length();
      // Check for .physics.xmb extension
      if (len < 12 || strnicmp(file.getPtr() + (len - 12), ".physics", 8) != 0)
         continue;
      file.left(len - 12);
      gPhysicsInfoManager.getOrCreate(file, true);
   }

#ifndef BUILD_FINAL   
   double totalTime = timer.getElapsedSeconds();
   trace("BScenario::preloadPhysicsFiles: %3.3f seconds", totalTime);
#endif

   return true;
}

//==============================================================================
// BDatabase::postloadProtoObjects
//==============================================================================
void BDatabase::postloadProtoObjects()
{
   // postload
   long count = mProtoObjects.getNumber();
   for (long i = 0; i < count; i++)
   {
      mProtoObjects.get(i)->postload();
   }
}

//==============================================================================
// BDatabase::setupProtoObjects
//==============================================================================
bool BDatabase::setupProtoObjects()
{
   mpSharedProtoObjectStaticData = new BProtoObjectStatic();
   if (!mpSharedProtoObjectStaticData)
      return false;
   mpSharedProtoObjectStaticData->mName = "shared_static_data";

   BXMLNode rootNode(mpObjectsReader->getRootNode());
   long count=rootNode.getNumberChildren();

   mProtoObjects.reserve(count);

   #ifndef BUILD_FINAL
      uint sharedStaticCount = 0;
      uint objectCount = 0;
   #endif

   // load
   for(long i=0; i<count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      BSimString string;
      if(node.getAttribValue("name", &string))
      {
         BProtoObject* pProtoObject=new BProtoObject(i);
         if(!pProtoObject)
            return false;
         if(!pProtoObject->load(node))
         {
            delete pProtoObject;
            return false;
         }
         mProtoObjects.add(pProtoObject);
         #ifndef BUILD_FINAL
            if (!pProtoObject->getFlagOwnStaticData())
               sharedStaticCount++;
            if (pProtoObject->getObjectClass()==cObjectClassObject)
               objectCount++;
         #endif
      }
   }

   #ifndef BUILD_FINAL
      blogtrace("# shared proto object static = %u", sharedStaticCount);
      blogtrace("# objects = %u", objectCount);
      blogtrace("# non-objects = %u", mProtoObjects.size()-objectCount);
      blogtrace("BProtoObject size = %d", sizeof(BProtoObject));
      blogtrace("BProtoObjectStatic size = %d", sizeof(BProtoObjectStatic));
      blogtrace("static savings = %d", sizeof(BProtoObjectStatic) * sharedStaticCount);
      blogtrace("object savings 8 players = %d", sizeof(BProtoObject) * objectCount * 8);
   #endif

#ifdef CONVERT_XML_TO_VIS
   // Read all XML files and convert to VSL
   int numProtoObjects = mProtoObjects.getNumber();
   for(int i = 0; i < numProtoObjects; i++)
   {
      if(mProtoObjects[i]->getProtoVisualIndex() != -1)
      {
         gVisualManager.createVisual(mProtoObjects[i]->getProtoVisualIndex(), false, 0);
      }
   }
#endif

#ifdef DEBUG_SAVE_OBJECTS
   //FIXME AJL TEMP
   BXMLWriter writer;
   if(!writer.create(cDirData, "temp.xml"))
      return true;
   writer.startItem("Objects");
   for(long i=0; i<count; i++)
      mProtoObjects[i]->save(writer);
   writer.endItem();
   writer.close();       
#endif

   return true;
}


//==============================================================================
//==============================================================================
void BDatabase::setupDecomposedObjectTypes()
{
   mDecomposedObjectTypes.setNumber(mNumberObjectTypes);
   for (long objectType=0; objectType<mNumberObjectTypes; objectType++)
   {
      if (objectType < mNumberBaseObjects)
      {
         mDecomposedObjectTypes[objectType].add(objectType);
         continue;
      }
      else
      {
         long numProtoObjects = getNumberProtoObjects();
         for (long j=0; j<numProtoObjects; j++)
         {
            const BProtoObject *pProtoObject = getGenericProtoObject(j);
            if (pProtoObject && pProtoObject->isType(objectType))
               mDecomposedObjectTypes[objectType].add(j);
         }
      }
   }
}


//==============================================================================
// BDatabase::setupProtoSquads
//==============================================================================
bool BDatabase::setupProtoSquads()
{
   BXMLNode rootNode(mpSquadsReader->getRootNode());

   int numChildNodes = rootNode.getNumberChildren();
   mProtoSquads.reserve(numChildNodes);

   for (int i = 0; i < numChildNodes; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());

      // Standard squad specification
      if (name == "Squad")
      {
         uint nextIndex = mProtoSquads.getNumber();
         BProtoSquad *pProtoSquad = new BProtoSquad(nextIndex);
         if (!pProtoSquad)
            return false;

         if (!pProtoSquad->load(node))
         {
            delete pProtoSquad;
            return false;
         }
         mProtoSquads.add(pProtoSquad);
      }
      // Merged squad
      else if (name == "MergedSquads")
      {
         // Get main squad to merge
         BSimString tempText;
         node.getText(tempText);
         BProtoSquad* pToMergeSquad = getGenericProtoSquad(getProtoSquad(tempText.asNative()));
         if (!pToMergeSquad )
            return false;

         for (int j = 0; j < node.getNumberChildren(); j++)
         {
            BXMLNode childNode(node.getChild(j));
            if (childNode.getName() == "MergedSquad")
            {
               // Get squad to merge into
               childNode.getText(tempText);
               BProtoSquad* pBaseSquad = getGenericProtoSquad(getProtoSquad(tempText.asNative()));
               if (!pBaseSquad)
                  return false;

               // Create new protoSquad
               uint nextIndex = mProtoSquads.getNumber();
               BProtoSquad *pMergedProtoSquad = new BProtoSquad(nextIndex, pBaseSquad, pToMergeSquad);
               if (!pMergedProtoSquad)
                  return false;

               // Add
               mProtoSquads.add(pMergedProtoSquad);
               mProtoSquadTable.add(pMergedProtoSquad->getName(), pMergedProtoSquad->getID());

               // Update lists of merged proto squad IDs
               pToMergeSquad->addMergeIntoProtoSquadIDs(pBaseSquad->getID(), pMergedProtoSquad->getID());
               pBaseSquad->addMergeFromProtoSquadIDs(pToMergeSquad->getID(), pMergedProtoSquad->getID());
            }
         }
      }
      else if (name == "ShieldBubbleTypes")
      {
         BSimString shieldText;
         BSimString targetText;
         node.getText(shieldText);
//-- FIXING PREFIX BUG ID 4117
         const BProtoSquad* pDefaultShieldSquad = getGenericProtoSquad(getProtoSquad(shieldText.asNative()));
//--
         if (!pDefaultShieldSquad)
            return false;

         mDefaultShieldID = pDefaultShieldSquad->getID();

         for (int j = 0; j < node.getNumberChildren(); j++)
         {
            BXMLNode childNode(node.getChild(j));
            std::pair<BProtoSquadID, BProtoSquadID> shieldPair;
            if (childNode.getName() == "ShieldBubble")
            {
               childNode.getAttribValue("target", &targetText);
//-- FIXING PREFIX BUG ID 4115
               const BProtoSquad* pTargetSquad = getGenericProtoSquad(getProtoSquad(targetText.asNative()));
//--
               BASSERT(pTargetSquad);

               childNode.getText(shieldText);
//-- FIXING PREFIX BUG ID 4116
               const BProtoSquad* pShieldSquad = getGenericProtoSquad(getProtoSquad(shieldText.asNative()));
//--
               BASSERT(pShieldSquad);

               mProtoShieldIDs.add(pTargetSquad->getID(), pShieldSquad->getID());
            }
         }
      }
   }

   mNumBaseProtoSquads = mProtoSquads.getNumber();

   // Create unit-specific proto-squads for all unit/building proto objects
   int protoObjectCount = mProtoObjects.getNumber();
   for (int i=0; i<protoObjectCount; i++)
   {
      BProtoObject* pProtoObject=mProtoObjects[i];
      int objectClass = pProtoObject->getObjectClass();
      if (objectClass==cObjectClassUnit || objectClass==cObjectClassBuilding || objectClass==cObjectClassSquad)
      {
         // Search for matching protosquad before creating a new one
         int foundProtoSquadID = getProtoSquad(pProtoObject->getName());
//-- FIXING PREFIX BUG ID 4118
         const BProtoSquad* pFoundProtoSquad = getGenericProtoSquad(foundProtoSquadID);
//--
         if (pFoundProtoSquad)
         {
            // Compare contents
            if ((pFoundProtoSquad->getSquadSize() == 1) && 
                (getGenericProtoObject(pFoundProtoSquad->getUnitNode(0).mUnitType) == pProtoObject))
            {
               pProtoObject->setProtoSquadID(foundProtoSquadID);
               continue;
            }
         }

         BProtoSquad *pProtoSquad = new BProtoSquad(i);
         if (!pProtoSquad)
            return false;
         int protoSquadID = mProtoSquads.getNumber();
         if (!pProtoSquad->initFromProtoObject(pProtoObject, protoSquadID))
         {
            delete pProtoSquad;
            return false;
         }
         mProtoSquads.add(pProtoSquad);
         pProtoObject->setProtoSquadID(protoSquadID);

         mProtoSquadTable.add(pProtoSquad->getName(), protoSquadID);
      }
   }

   return true;
}


//==============================================================================
// BDatabase::setupProtoPowers
//==============================================================================
void BDatabase::setupProtoPowers()
{
   BXMLReader reader;
   BXMLNode rootNode;
   if (mpPowersReader)
      rootNode = mpPowersReader->getRootNode();
   else
   {
      // we don't have the file loaded, so load it up here
      if (reader.load(cDirData, "powers.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
         rootNode = reader.getRootNode();
      else
      {
         BASSERTM(false, "powers.xml could not be loaded.");
         return;
      }
   }

   if (rootNode.getName() != "Powers")
   {
      BASSERTM(false, "powers.xml had a root node problem and could not be loaded.");
      return;
   }

   long protoPowerCount = rootNode.getNumberChildren();
   mProtoPowers.reserve(protoPowerCount);
   int protoPowerID=0;
   for (long i=0; i<protoPowerCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      if (node.getName() != "Power")
         continue;

      BProtoPower* pProtoPower = getProtoPowerByID(protoPowerID);
      if (!pProtoPower)
      {
         BASSERTM(false, "missing proto power");
         return;
      }

      if (!pProtoPower->load(node))
      {
#ifndef BUILD_FINAL
         BSimString errorMsg;
         BSimString protoPowerName;
         node.getAttribValueAsString("name", protoPowerName);
         errorMsg.format("Unable to load the proto power %s.  Skipping.", protoPowerName.asNative());
         BASSERTM(false, errorMsg);
#endif
         delete pProtoPower;
         continue;
      }

      protoPowerID++;
   }
}

//==============================================================================
// BDatabase::setupAbilities
//==============================================================================
void BDatabase::setupAbilities()
{
   BXMLReader reader;
   if (!reader.load(cDirData, "abilities.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
   {
      BASSERTM(false, "abilities.xml could not be loaded.");
      return;
   }

   BXMLNode rootNode(reader.getRootNode());
   if (rootNode.getName() != "Abilities")
   {
      BASSERTM(false, "abilities.xml had a root node problem and could not be loaded.");
      return;
   }

   long abilityCount = rootNode.getNumberChildren();
   mAbilities.reserve(abilityCount);
   for (long i=0; i<abilityCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      if (node.getName() != "Ability")
         continue;

      BAbility* pAbility = new BAbility();
      BFATAL_ASSERTM(pAbility, "Unable to allocate a BAbility!  Out of memory?");

      if (!pAbility->load(node))
      {
#ifndef BUILD_FINAL
         BSimString errorMsg;
         BSimString abilityName;
         node.getAttribValueAsString("name", abilityName);
         errorMsg.format("Unable to load the ability %s.  Skipping.", abilityName.asNative());
         BASSERTM(false, errorMsg);
#endif
         delete pAbility;
         continue;
      }

      pAbility->setID(mAbilities.getNumber());

      mAbilities.add(pAbility);
   }

   // Cache off ability ID's here we may use in code.
   mAIDCommand = getAbilityIDFromName("Command");
   mAIDUngarrison = getAbilityIDFromName("Ungarrison");
   mAIDUnhitch = getAbilityIDFromName("Unhitch");
   mAIDUnscRam = getAbilityIDFromName("UnscRam");
   mAIDUnscMarineRockets = getAbilityIDFromName("UnscMarineRockets");
   mAIDUnscWolverineBarrage = getAbilityIDFromName("UnscWolverineBarrage");
   mAIDUnscBarrage = getAbilityIDFromName("UnscBarrage");
   mAIDUnscFlashBang = getAbilityIDFromName("UnscFlashBang");
   mAIDUnscHornetSpecial = getAbilityIDFromName("UnscHornetSpecial");
   mAIDUnscScorpionSpecial = getAbilityIDFromName("UnscScorpionSpecial");
   mAIDCovGruntGrenade = getAbilityIDFromName("CovGruntGrenade");
   mAIDUnscLockdown = getAbilityIDFromName("UnscLockdown");
   mAIDUnscCyclopsThrow = getAbilityIDFromName("UnscCyclopsThrow");
   mAIDUnscSpartanTakeOver = getAbilityIDFromName("UnscSpartanTakeOver");
   mAIDUnscGremlinSpecial = getAbilityIDFromName("UnscGremlinSpecial");
   mAIDCovCloak = getAbilityIDFromName("CovCloak");
   mAIDCovArbCloak = getAbilityIDFromName("CovArbCloak");
   mAIDCovLeaderGlassing = getAbilityIDFromName("CovLeaderGlassing");
   mAIDCovGhostRam = getAbilityIDFromName("CovGhostRam");
   mAIDCovChopperRunOver = getAbilityIDFromName("CovChopperRunOver");
   mAIDCovGruntSuicideExplode = getAbilityIDFromName("CovGruntSuicideExplode");
   mAIDCovStasis = getAbilityIDFromName("CovStasis");
   mAIDCovLocustOverburn = getAbilityIDFromName("CovLocustOverburn");
   mAIDCovJumppack = getAbilityIDFromName("CovJumppack");
   mAIDCovWraithSpecial = getAbilityIDFromName("CovWraithSpecial");
   mAIDUnscUnload = getAbilityIDFromName("UnscUnload");
   mAIDUnscCobraLockdown = getAbilityIDFromName("UnscCobraLockdown");
}

//==============================================================================
// BDatabase::setupProtoTechs
//==============================================================================
bool BDatabase::setupProtoTechs()
{
   mNumberTechUnitDependecies=getNumberProtoObjects();
   mpTechUnitDependecies=new BTechUnitDependencyArray*[mNumberTechUnitDependecies];
   Utils::FastMemSet(mpTechUnitDependecies, 0, sizeof(BTechUnitDependencyArray*)*mNumberTechUnitDependecies);

   BXMLNode rootNode(mpTechsReader->getRootNode());
   long count=rootNode.getNumberChildren();

   for(long i=0; i<count; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      BProtoTech* pProtoTech=mProtoTechs[i];
      if(!pProtoTech->load(node))
         return false;
   }

   // Cache off proto tech IDs
   mPTIDShieldUpgrade = getProtoTech("cov_shield_upgrade");
   mPTIDShieldDowngrade = getProtoTech("cov_shield_downgrade");

   return true;
}

//==============================================================================
// BDatabase::getProtoTechName
//==============================================================================
const char* BDatabase::getProtoTechName(long id) const
{
   const BStringTableLong::BTagsArrayType& tags=mTechTable.getTags();
   long count=tags.getNumber();
   if(id<0 || id>=count)
      return "";
   else
      return tags[id].getPtr();
}

//==============================================================================
// BDatabase::addTechUnitDependecy
//==============================================================================
void BDatabase::addTechUnitDependecy(long techID, long unitID)
{
   if(unitID<0 || unitID>=mNumberTechUnitDependecies)
      return;
   BTechUnitDependencyArray* pArray=mpTechUnitDependecies[unitID];
   if(!pArray)
   {
      pArray=new BTechUnitDependencyArray;
      if(!pArray)
         return;
      mpTechUnitDependecies[unitID]=pArray;
   }
   pArray->add(techID);
}

//==============================================================================
// BDatabase::getTechUnitDependencies
//==============================================================================
BTechUnitDependencyArray* BDatabase::getTechUnitDependencies(long unitID)
{
   if(unitID<0 || unitID>=mNumberTechUnitDependecies)
      return NULL;
   return mpTechUnitDependecies[unitID];
}


//==============================================================================
//==============================================================================
long BDatabase::getTechStatusFromName(const char* pName) const
{
   if (stricmp(pName, "Unobtainable") == 0)
      return (BTechTree::cStatusUnobtainable);
   else if (stricmp(pName, "Obtainable") == 0)
      return (BTechTree::cStatusObtainable);
   else if (stricmp(pName, "Available") == 0)
      return (BTechTree::cStatusAvailable);
   else if (stricmp(pName, "Researching") == 0)
      return (BTechTree::cStatusResearching);
   else if (stricmp(pName, "Active") == 0)
      return (BTechTree::cStatusActive);
   else if (stricmp(pName, "Disabled") == 0)
      return (BTechTree::cStatusDisabled);
   else if (stricmp(pName, "CoopResearching") == 0)
      return (BTechTree::cStatusCoopResearching);
   
   BASSERTM(false, "getTechStatusFromName() Invalid Name.");
   return (BTechTree::cStatusUnobtainable);
}

//==============================================================================
//==============================================================================
long BDatabase::getChatSpeakerFromName(const char* pName) const
{
   if (stricmp(pName, "Serena") == 0)
      return (BChatMessage::cChatSpeakerSerena);
   else if (stricmp(pName, "Forge") == 0)
      return (BChatMessage::cChatSpeakerForge);
   else if (stricmp(pName, "Cutter") == 0)
      return (BChatMessage::cChatSpeakerCutter);
   else if (stricmp(pName, "Voice of God") == 0)
      return (BChatMessage::cChatSpeakerGod);
   else if (stricmp(pName, "Generic Soldiers") == 0)
      return (BChatMessage::cChatSpeakerSoldiers);
   else if (stricmp(pName, "Arcadian Police") == 0)
      return (BChatMessage::cChatSpeakerPolice);
   else if (stricmp(pName, "Civilians") == 0)
      return (BChatMessage::cChatSpeakerCivilians);
   else if (stricmp(pName, "Anders") == 0)
      return (BChatMessage::cChatSpeakerAnders);
   else if (stricmp(pName, "RhinoCommander") == 0)
      return (BChatMessage::cChatSpeakerRhinoCommander);
   else if (stricmp(pName, "Spartan1") == 0)
      return (BChatMessage::cChatSpeakerSpartan1);
   else if (stricmp(pName, "Spartan2") == 0)
      return (BChatMessage::cChatSpeakerSpartan2);
   else if (stricmp(pName, "SpartanSniper") == 0)
      return (BChatMessage::cChatSpeakerSpartanSniper);
   else if (stricmp(pName, "SpartanRockeLauncher") == 0)
      return (BChatMessage::cChatSpeakerSpartanRocketLauncher);
   else if (stricmp(pName, "Covenant") == 0)
      return (BChatMessage::cChatSpeakerAnders);
   else if (stricmp(pName, "Arbiter") == 0)
      return (BChatMessage::cChatSpeakerRhinoCommander);
   else if (stricmp(pName, "ODST") == 0)
      return (BChatMessage::cChatSpeakerODST);

   BASSERTM(false, "getChatSpeakerFromName() - Invalid Name.");
   return (BChatMessage::cChatSpeakerSerena);
}


//==============================================================================
//==============================================================================
long BDatabase::getExposedActionFromName(const char* pName) const
{
   if (stricmp(pName, "ExposedAction0") == 0)
      return (0);
   else if (stricmp(pName, "ExposedAction1") == 0)
      return (1);
   else if (stricmp(pName, "ExposedAction2") == 0)
      return (2);

   BASSERTM(false, "getExposedActionFromName() - Invalid Name.");
   return (0);
}

//==============================================================================
//==============================================================================
uint BDatabase::getDataScalarFromName(const char* pName) const
{
   if (stricmp(pName, "Accuracy") == 0)
      return (BTriggerVarDataScalar::cDataScalarAccuracy);
   else if (stricmp(pName, "WorkRate") == 0)
      return (BTriggerVarDataScalar::cDataScalarWorkRate);
   else if (stricmp(pName, "Damage") == 0)
      return (BTriggerVarDataScalar::cDataScalarDamage);
   else if (stricmp(pName, "LOS") == 0)
      return (BTriggerVarDataScalar::cDataScalarLOS);
   else if (stricmp(pName, "Velocity") == 0)
      return (BTriggerVarDataScalar::cDataScalarVelocity);
   else if (stricmp(pName, "WeaponRange") == 0)
      return (BTriggerVarDataScalar::cDataScalarWeaponRange);
   else if (stricmp(pName, "DamageTaken") == 0)
      return (BTriggerVarDataScalar::cDataScalarDamageTaken);

   BASSERTM(false, "getDataScalarFromName() - Invalid Name.");
   return (BTriggerVarDataScalar::cDataScalarAccuracy);
}


//==============================================================================
//==============================================================================
long BDatabase::getActionStatusFromName(const char* pName) const
{
   if (stricmp(pName, "NotDone") == 0)
      return (BTriggerVarActionStatus::cNotDone);
   else if (stricmp(pName, "DoneSuccess") == 0)
      return (BTriggerVarActionStatus::cDoneSuccess);
   else if (stricmp(pName, "DoneFailure") == 0)
      return (BTriggerVarActionStatus::cDoneFailure);

   BASSERTM(false, "getActionStatusFromName() - Invalid Name.");
   return (BTriggerVarActionStatus::cNotDone);
}


//==============================================================================
//==============================================================================
int BDatabase::getFlareTypeFromName(const char* pName) const
{
   if (stricmp(pName, "Look") == 0)
      return (BUIGame::cFlareLook);
   else if (stricmp(pName, "Help") == 0)
      return (BUIGame::cFlareHelp);
   else if (stricmp(pName, "Meet") == 0)
      return (BUIGame::cFlareMeet);
   else if (stricmp(pName, "Attack") == 0)
      return (BUIGame::cFlareAttack);

   BASSERTM(false, "getFlareTypeFromName() - Invalid Name.");
   return (BUIGame::cFlareLook);
}

//==============================================================================
//==============================================================================
long BDatabase::getAnimTypeFromName(const char* pName) const
{
   for (long i=0; i<cAnimTypeMaxCount; i++)
   {
      if (stricmp(pName, gAnimTypeNames[i]) == 0)
         return (i);
   }

   #ifndef BUILD_FINAL
      BSimString debugStr;
      debugStr.format("getAnimTypeFromName() - %s is not contained in gAnimTypeNames[].", pName);
      BASSERTM(false, debugStr.getPtr());
   #endif
   return (cAnimTypeIdle);
}


//==============================================================================
// BDatabase::getProtoSquadName
//==============================================================================
const char* BDatabase::getProtoSquadName(long id) const
{
   const BStringTableLong::BTagsArrayType& tags=mProtoSquadTable.getTags();
   long count=tags.getNumber();
   if(id<0 || id>=count)
      return "";
   else
      return tags[id].getPtr();
}

//==============================================================================
// BDatabase::getPopName
//==============================================================================
const char* BDatabase::getPopName(long type) const
{
   const BStringTableLong::BTagsArrayType& tags=mPops.getTags();
   long count=tags.getNumber();
   if(type<0 || type>=count)
      return "";
   else
      return tags[type].getPtr();
}

//==============================================================================
// BDatabase::getRefCountTypeName
//==============================================================================
const char* BDatabase::getRefCountTypeName(long type) const
{
   const BStringTableLong::BTagsArrayType& tags=mRefCountTypes.getTags();
   long count=tags.getNumber();
   if(type<0 || type>=count)
      return "";
   else
      return tags[type].getPtr();
}

//==============================================================================
// Get the name of a player state
//==============================================================================
const char* BDatabase::getPlayerStateName(BPlayerState type) const
{
   const BStringTableLong::BTagsArrayType& tags = mPlayerStates.getTags();
   int count = tags.getNumber();
   if ((type < 0) || (type >= count))
   {
      return ("");
   }
   else
   {
      return (tags[type].getPtr());
   }
}

//==============================================================================
// BDatabase::getHUDItem
//==============================================================================
int BDatabase::getHUDItem(const char* pName) const
{   
   // AJL - The list here should match the list of unit flags in gamedata.xml
   if (stricmp(pName, "Minimap")==0)
      return BUser::cHUDItemMinimap;
   else if (stricmp(pName, "Resources")==0)
      return BUser::cHUDItemResources;
   else if (stricmp(pName, "Time")==0)
      return BUser::cHUDItemTime;
   else if (stricmp(pName, "PowerStatus")==0)
      return BUser::cHUDItemPowerStatus;
   else if (stricmp(pName, "Units")==0)
      return BUser::cHUDItemUnits;
   else if (stricmp(pName, "DpadHelp")==0)
      return BUser::cHUDItemDpadHelp;
   else if (stricmp(pName, "ButtonHelp")==0)
      return BUser::cHUDItemButtonHelp;
   else if (stricmp(pName, "Reticle")==0)
      return BUser::cHUDItemReticle;
   else if (stricmp(pName, "Score")==0)
      return BUser::cHUDItemScore;
   else if (stricmp(pName, "UnitStats")==0)
      return BUser::cHUDItemUnitStats;
   else if (stricmp(pName, "CircleMenuExtraInfo")==0)
      return BUser::cHUDItemCircleMenuExtraInfo;
   
   return -1;   
}

//==============================================================================
// BDatabase::getFlashableUIItem
//==============================================================================
int BDatabase::getFlashableUIItem(const char* pName) const
{   
   // AJL - The list here should match the list of unit flags in gamedata.xml

      if (stricmp(pName, "Minimap")==0)
         return BUIGame::cFlashableItemMinimap;
      else if (stricmp(pName, "CircleMenu0")==0)
         return BUIGame::cFlashableItemCircleMenuSlot0;
      else if (stricmp(pName, "CircleMenu1")==0)
         return BUIGame::cFlashableItemCircleMenuSlot1;
      else if (stricmp(pName, "CircleMenu2")==0)
         return BUIGame::cFlashableItemCircleMenuSlot2;
      else if (stricmp(pName, "CircleMenu3")==0)
         return BUIGame::cFlashableItemCircleMenuSlot3;
      else if (stricmp(pName, "CircleMenu4")==0)
         return BUIGame::cFlashableItemCircleMenuSlot4;
      else if (stricmp(pName, "CircleMenu5")==0)
         return BUIGame::cFlashableItemCircleMenuSlot5;
      else if (stricmp(pName, "CircleMenu6")==0)
         return BUIGame::cFlashableItemCircleMenuSlot6;
      else if (stricmp(pName, "CircleMenu7")==0)
         return BUIGame::cFlashableItemCircleMenuSlot7;
      else if (stricmp(pName, "CircleMenuPop")==0)
         return BUIGame::cFlashableItemCircleMenuPop;
      else if (stricmp(pName, "CircleMenuPower")==0)
         return BUIGame::cFlashableItemCircleMenuPower;
      else if (stricmp(pName, "CircleMenuSupply")==0)
         return BUIGame::cFlashableItemCircleMenuSupply;
      else if (stricmp(pName, "Dpad")==0)
         return BUIGame::cFlashableItemDpad;
      else if (stricmp(pName, "DpadUp")==0)
         return BUIGame::cFlashableItemDpadUp;
      else if (stricmp(pName, "DpadDown")==0)
         return BUIGame::cFlashableItemDpadDown;
      else if (stricmp(pName, "DpadLeft")==0)
         return BUIGame::cFlashableItemDpadLeft;
      else if (stricmp(pName, "DpadRight")==0)
         return BUIGame::cFlashableItemDpadRight;
      else if (stricmp(pName, "ResourcePanel")==0)
         return BUIGame::cFlashableItemResourcePanel;
      else if (stricmp(pName, "ResourcePanelPop")==0)
         return BUIGame::cFlashableItemResourcePanelPop;
      else if (stricmp(pName, "ResourcePanelPower")==0)
         return BUIGame::cFlashableItemResourcePanelPower;
      else if (stricmp(pName, "ResourcePanelSupply")==0)
         return BUIGame::cFlashableItemResourcePanelSupply;

   return -1;   
}

//==============================================================================
// BDatabase::getHUDItemName
//==============================================================================
const char* BDatabase::getHUDItemName(int type) const
{
   // AJL - The list here should match the list of unit flags in gamedata.xml
   switch (type)
   {
      case BUser::cHUDItemMinimap: return "Minimap";
      case BUser::cHUDItemResources: return "Resources";
      case BUser::cHUDItemTime: return "Time";
      case BUser::cHUDItemPowerStatus: return "PowerStatus";
      case BUser::cHUDItemUnits: return "Units";
      case BUser::cHUDItemDpadHelp: return "DpadHelp";
      case BUser::cHUDItemButtonHelp: return "ButtonHelp";
      case BUser::cHUDItemReticle: return "Reticle";
      case BUser::cHUDItemScore: return "Score";
      case BUser::cHUDItemUnitStats: return "UnitStats";
      case BUser::cHUDItemCircleMenuExtraInfo: return "CircleMenuExtraInfo";
      default : return "";
   }   
}

//==============================================================================
// BDatabase::getUnitFlag
//==============================================================================
long BDatabase::getUnitFlag(const char* pName) const
{   
   // AJL - The list here should match the list of unit flags in gamedata.xml
   if(stricmp(pName, "AttackBlocked")==0)
      return BUnit::cFlagAttackBlocked;
   return -1;   
}

//==============================================================================
// BDatabase::getUnitFlagName
//==============================================================================
const char* BDatabase::getUnitFlagName(long flag) const
{
   // AJL - The list here should match the list of unit flags in gamedata.xml
   switch(flag)
   {
      case BUnit::cFlagAttackBlocked: return "AttackBlocked";
      default : return "";
   }   
}

//==============================================================================
// BDatabase::getSquadFlag
//==============================================================================
long BDatabase::getSquadFlag(const char* pName) const
{   
   // AJL - The list here should match the list of squad flags in gamedata.xml
   if(stricmp(pName, "AttackBlocked")==0)
      return BSquad::cFlagAttackBlocked;
   return -1;   
}

//==============================================================================
// BDatabase::getSquadFlagName
//==============================================================================
const char* BDatabase::getSquadFlagName(long flag) const
{
   // AJL - The list here should match the list of squad flags in gamedata.xml
   switch(flag)
   {
      case BSquad::cFlagAttackBlocked: return "AttackBlocked";
      default : return "";
   }   
}

//==============================================================================
// BDatabase::getProtoImpactEffectIndex
//==============================================================================
int BDatabase::getProtoImpactEffectIndex(const char* pName)
{
   BSimString tmpStr(pName);
   tmpStr.toLower();
   BImpactEffectHashMap::const_iterator it = mImpactEffectHashMap.find(tmpStr);
   //-- did we find it?
   if (it != mImpactEffectHashMap.end())
   {
      // found it
      return it->second;      
   }
   return -1;   
}

//==============================================================================
// BDatabase::getProtoImpactEffectFromIndex
//==============================================================================
const BProtoImpactEffect* BDatabase::getProtoImpactEffectFromIndex(int index)
{
   if (index < 0 || index >= mProtoImpactEffects.getNumber())
      return NULL;
   else
      return &mProtoImpactEffects[index];
}

//==============================================================================
// BDatabase::getVisualDisplayPriority
//==============================================================================
int BDatabase::getVisualDisplayPriority(const char* pName) const
{
   BSimString tmpStr(pName);
   tmpStr.toLower();

   if (tmpStr.compare("combat")==0)
      return cVisualDisplayPriorityCombat;
   if (tmpStr.compare("high")==0)
      return cVisualDisplayPriorityHigh;
   
   return cVisualDisplayPriorityNormal;   
}

//==============================================================================
// BDatabase::getLocStringIndex
//==============================================================================
long BDatabase::getLocStringIndex(long id) const
{
   //AJL FIXME - Need to use integers instead of a string value for the ID
   BString idName;
   idName.format("%d", id);
   long index; 
   if(mLocStringTable.find(idName, &index)) 
      return index; 
   else 
      return -1; 
}

//==============================================================================
// BDatabase::getLocStringFromIndex
//==============================================================================
const BUString& BDatabase::getLocStringFromIndex(long index) const
{
   if(index<0 || index>=mLocStrings.getNumber())
      return mStringNotFoundString;
   return mLocStrings[index];
}

//==============================================================================
// BDatabase::getLocStringFromID
//==============================================================================
const BUString& BDatabase::getLocStringFromID(long id) const
{
   long index=getLocStringIndex(id);
   return getLocStringFromIndex(index);
}

//==============================================================================
// BDatabase::decodeLocString
//==============================================================================
BUString& BDatabase::decodeLocString( BUString& locString ) const
{
   int idStart = locString.findLeft( L"$$" );
   if( idStart == 0 )
   {
      int idEnd = locString.findLeft( L"$$", idStart + 1 );
      if( idEnd > 0 )
      {
         BUString idString = locString;

         idString.crop( 2, idEnd - 1 );
         int id = idString.asLong();

         int index = getLocStringIndex( id );
         if( index >= 0 )
         {
            locString = getLocStringFromIndex( index );
         }
         else
         {
            locString.right( idEnd + 1 );
            if( locString.length() == 0 )
            {
               locString = L"<<NO LOCALIZED, OR PLACEHOLDER TEXT FOUND>>";
            }
         }
      }
   }
   
   return locString;
}

//==============================================================================
// BDatabase::getAbilityFromID
//==============================================================================
BAbility* BDatabase::getAbilityFromID(long id)
{
   long numAbilities = mAbilities.getNumber();
   if (id < 0 || id >= numAbilities)
      return(NULL);
   else
      return(mAbilities[id]);
}

//==============================================================================
// BDatabase::getAbilityIDFromName
//==============================================================================
long BDatabase::getAbilityIDFromName(const char* pName)
{
   if (!pName)
      return(-1);
   long numAbilities = mAbilities.getNumber();
   for (long i=0; i<numAbilities; i++)
   {
      if (mAbilities[i]->getName() == pName)
         return(i);
   }
   return(-1);
}

//==============================================================================
// BDatabase::getAbilityName
//==============================================================================
const char* BDatabase::getAbilityName(long id)
{
   long numAbilities = mAbilities.getNumber();
   if (id < 0 || id >= numAbilities)
      return("");
   else
      return(mAbilities[id]->getName().getPtr());
}

//==============================================================================
// BDatabase::getSquadAbilityID
//==============================================================================
int BDatabase::getSquadAbilityID(const BSquad* pSquad, int abilityID) const
{
   if (!pSquad)
      return -1;
   if (abilityID == -1)
      return -1;
   if (abilityID != mAIDCommand)
      return abilityID;
   const BProtoObject* pProtoObject = pSquad->getProtoObject();
   if (!pProtoObject)
      return -1;
   return pProtoObject->getAbilityCommand();
}

//==============================================================================
// BDatabase::getAbilityType
//==============================================================================
long BDatabase::getAbilityType(const char* pName)
{
   if(stricmp(pName, "Work")==0)
      return cAbilityWork;
   else if(stricmp(pName, "ChangeMode")==0)
      return cAbilityChangeMode;
   else if(stricmp(pName, "Unload")==0)
      return cAbilityUnload;
   else if(stricmp(pName, "Unpack")==0)
      return cAbilityUnpack;
   else if(stricmp(pName, "CommandMenu")==0)
      return cAbilityCommandMenu;
   else if(stricmp(pName, "Power")==0)
      return cAbilityPower;
   return -1;
}

//==============================================================================
// BDatabase::getAbilityTypeFromID
//==============================================================================
long BDatabase::getAbilityTypeFromID(long id)
{
   if (id != -1)
   {
//-- FIXING PREFIX BUG ID 4119
      const BAbility* pAbility = getAbilityFromID(id);
//--
      if (pAbility)
         return pAbility->getType();
   }
   return -1;
}

//==============================================================================
// BDatabase::getRecoverType
//==============================================================================
int BDatabase::getRecoverType(const char* pName)
{
   if(stricmp(pName, "Move")==0)
      return cRecoverMove;
   else if(stricmp(pName, "Attack")==0)
      return cRecoverAttack;
   else if(stricmp(pName, "Ability")==0)
      return cRecoverAbility;
   return -1;
}

//==============================================================================
// BDatabase::getObjectTypeName
//==============================================================================
const char* BDatabase::getObjectTypeName(long type) const
{
   const BStringTableLong::BTagsArrayType& tags=mObjectTypes.getTags();
   long count=tags.getNumber();
   if(type<0 || type>=count)
      return "";
   else
      return tags[type].getPtr();
}

//==============================================================================
// BDatabase::getSurfaceTypeName
//==============================================================================
const char* BDatabase::getSurfaceTypeName(byte type) const
{
   const BStringTable<byte>::BTagsArrayType& tags = mSurfaceTypes.getTags();
   byte count = (byte) tags.getNumber();
   if (type >= count)
      return "";
   else
      return tags[type].getPtr();
}

//==============================================================================
//==============================================================================
BProtoObject* BDatabase::getGenericProtoObject(BProtoObjectID protoObjectID) const
{
   if (protoObjectID < 0)
      return (NULL);

   if (isValidProtoObject(protoObjectID))
      return (mProtoObjects[protoObjectID]);
   else
   {
      // If this is a player's unique protoObject, get it and
      // return its generic base protoObject
      long playerID = GETUNIQUEPROTOPLAYERID(protoObjectID);
      const BPlayer* pPlayer = gWorld->getPlayer(playerID);
      if (pPlayer)
      {
//-- FIXING PREFIX BUG ID 4120
         const BProtoObject* pPO = pPlayer->getProtoObject(protoObjectID);
//--
         if (pPO)
            return getGenericProtoObject(pPO->getBaseType());
      }
   }

   return NULL;
}

//==============================================================================
//==============================================================================
BProtoSquad* BDatabase::getGenericProtoSquad(long id) const
{
   if (id < 0)
      return NULL;
   if (id < (long) mProtoSquads.getSize())
      return mProtoSquads[id];
   else
   {
      // If this is a player's unique protoObject, get it and
      // return its generic base protoObject
      long playerID = GETUNIQUEPROTOPLAYERID(id);
      const BPlayer* pPlayer = gWorld->getPlayer(playerID);
      if (pPlayer)
      {
//-- FIXING PREFIX BUG ID 4121
         const BProtoSquad* pPS = pPlayer->getProtoSquad(id);
//--
         if (pPS)
            return getGenericProtoSquad(pPS->getBaseType());
      }
   }

   return NULL;
}

//==============================================================================
// Convert tech effect data sub types from tag names to enumerations
//==============================================================================
long BDatabase::getProtoObjectDataType(BSimString subTypeName) const
{
   // Halwes - 12/6/2007 - These tech data sub type tags need to be reflected in techs.xml with TechDataSubType tags for the editor and trigger system
   //                      to work correctly.
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   long dataType = -1;
   if (subTypeName == "Enable")
      dataType = BTechEffect::cDataEnable;
   else if (subTypeName == "Hitpoints")
      dataType = BTechEffect::cDataHitpoints;
   else if (subTypeName == "Shieldpoints")
      dataType = BTechEffect::cDataShieldpoints;
   else if (subTypeName == "AmmoMax")
      dataType = BTechEffect::cDataAmmoMax;
   else if (subTypeName == "LOS")
      dataType = BTechEffect::cDataLOS;
   else if (subTypeName == "MaximumVelocity")
      dataType = BTechEffect::cDataMaximumVelocity;
   else if (subTypeName == "MaximumRange")
      dataType = BTechEffect::cDataMaximumRange;
   else if (subTypeName == "ResearchPoints")
      dataType = BTechEffect::cDataResearchPoints;
   else if (subTypeName == "ResourceTrickleRate")
      dataType = BTechEffect::cDataResourceTrickleRate;
   else if (subTypeName == "MaximumResourceTrickleRate")
      dataType = BTechEffect::cDataMaximumResourceTrickleRate;   
   else if (subTypeName == "RateAmount")
      dataType = BTechEffect::cDataRateAmount;
   else if (subTypeName == "RateMultiplier")
      dataType = BTechEffect::cDataRateMultiplier;
   else if (subTypeName == "Resource")
      dataType = BTechEffect::cDataResource;
   else if (subTypeName == "Projectile")
      dataType = BTechEffect::cDataProjectile;
   else if (subTypeName == "Damage")
      dataType = BTechEffect::cDataDamage;
   else if (subTypeName == "MinRange")
      dataType = BTechEffect::cDataMinRange;
   else if (subTypeName == "AOERadius")
      dataType = BTechEffect::cDataAOERadius;
   else if (subTypeName == "AOEPrimaryTargetFactor")
      dataType = BTechEffect::cDataAOEPrimaryTargetFactor;
   else if (subTypeName == "AOEDistanceFactor")
      dataType = BTechEffect::cDataAOEDistanceFactor;
   else if (subTypeName == "AOEDamageFactor")
      dataType = BTechEffect::cDataAOEDamageFactor;
   else if (subTypeName == "Accuracy")
      dataType = BTechEffect::cDataAccuracy;
   else if (subTypeName == "MovingAccuracy")
      dataType = BTechEffect::cDataMovingAccuracy;
   else if (subTypeName == "MaxDeviation")
      dataType = BTechEffect::cDataMaxDeviation;
   else if (subTypeName == "MovingMaxDeviation")
      dataType = BTechEffect::cDataMovingMaxDeviation;
   else if (subTypeName == "DataAccuracyDistanceFactor")
      dataType = BTechEffect::cDataAccuracyDistanceFactor;
   else if (subTypeName == "AccuracyDeviationFactor")
      dataType = BTechEffect::cDataAccuracyDeviationFactor;
   else if (subTypeName == "MaxVelocityLead")
      dataType = BTechEffect::cDataMaxVelocityLead;
   else if (subTypeName == "WorkRate")
      dataType = BTechEffect::cDataWorkRate;
   else if (subTypeName == "BuildPoints")
      dataType = BTechEffect::cDataBuildPoints;
   else if (subTypeName == "Cost")
      dataType = BTechEffect::cDataCost;
   else if (subTypeName == "ActionEnable")
      dataType = BTechEffect::cDataActionEnable;
   else if (subTypeName == "CommandEnable")
      dataType = BTechEffect::cDataCommandEnable;
   else if (subTypeName == "BountyResource")
      dataType = BTechEffect::cDataBountyResource;
   else if (subTypeName == "AutoCloak")
      dataType = BTechEffect::cDataAutoCloak;
   else if (subTypeName == "MoveWhileCloaked")
      dataType = BTechEffect::cDataCloakMove;
   else if (subTypeName == "AttackWhileCloaked")
      dataType = BTechEffect::cDataCloakAttack;
   else if (subTypeName == "TributeCost")
      dataType = BTechEffect::cDataTributeCost;
   else if (subTypeName == "ShieldRegenRate")
      dataType = BTechEffect::cDataShieldRegenRate;
   else if (subTypeName == "ShieldRegenDelay")
      dataType = BTechEffect::cDataShieldRegenDelay;
   else if (subTypeName == "DamageModifier")
      dataType = BTechEffect::cDataDamageModifier;
   else if (subTypeName == "PopCap")
      dataType = BTechEffect::cDataPopCap;
   else if (subTypeName == "PopMax")
      dataType = BTechEffect::cDataPopMax;
   else if (subTypeName == "UnitTrainLimit")
      dataType = BTechEffect::cDataUnitTrainLimit;
   else if (subTypeName == "SquadTrainLimit")
      dataType = BTechEffect::cDataSquadTrainLimit;
   else if (subTypeName == "RepairCost")
      dataType = BTechEffect::cDataRepairCost;
   else if (subTypeName == "RepairTime")
      dataType = BTechEffect::cDataRepairTime;
   else if (subTypeName == "PowerRechargeTime")
      dataType = BTechEffect::cDataPowerRechargeTime;
   else if (subTypeName == "PowerUseLimit")
      dataType = BTechEffect::cDataPowerUseLimit;
   else if (subTypeName == "Level")
      dataType = BTechEffect::cDataLevel;
   else if (subTypeName == "Bounty")
      dataType = BTechEffect::cDataBounty;
   else if (subTypeName == "MaxContained")
      dataType = BTechEffect::cDataMaxContained;
   else if (subTypeName == "MaxDamagePerRam")
      dataType = BTechEffect::cDataMaxDamagePerRam;
   else if (subTypeName == "ReflectDamageFactor")
      dataType = BTechEffect::cDataReflectDamageFactor;
   else if (subTypeName == "AirBurstSpan")
      dataType = BTechEffect::cDataAirBurstSpan;
   else if (subTypeName == "AbilityDisabled")
      dataType = BTechEffect::cDataAbilityDisabled;
   else if (subTypeName == "DOTrate")
      dataType = BTechEffect::cDataDOTrate;
   else if (subTypeName == "DOTduration")
      dataType = BTechEffect::cDataDOTduration;
   else if (subTypeName == "ImpactEffect")
      dataType = BTechEffect::cDataImpactEffect;
   else if (subTypeName == "AmmoRegenRate")
      dataType = BTechEffect::cDataAmmoRegenRate;
   else if (subTypeName == "CommandSelectable")
      dataType = BTechEffect::cDataCommandSelectable;   
   else if (subTypeName == "DisplayNameID")
      dataType = BTechEffect::cDataDisplayNameID;
   else if (subTypeName == "Icon")
      dataType = BTechEffect::cDataIcon;
   else if (subTypeName == "AltIcon")
      dataType = BTechEffect::cDataAltIcon;
   else if (subTypeName == "Stasis")
      dataType = BTechEffect::cDataStasis;
   else if (subTypeName == "TurretYawRate")
      dataType = BTechEffect::cDataTurretYawRate;
   else if (subTypeName == "TurretPitchRate")
      dataType = BTechEffect::cDataTurretPitchRate;
   else if (subTypeName == "PowerLevel")
      dataType = BTechEffect::cDataPowerLevel;
   else if (subTypeName == "BoardTime")
      dataType = BTechEffect::cDataBoardTime;
   else if (subTypeName == "AbilityRecoverTime")
      dataType = BTechEffect::cDataAbilityRecoverTime;
   else if (subTypeName == "TechLevel")
      dataType = BTechEffect::cDataTechLevel;
   else if (subTypeName == "HPBar")
      dataType = BTechEffect::cDataTechHPBar;
   else if (subTypeName == "WeaponPhysicsMultiplier")
      dataType = BTechEffect::cDataWeaponPhysicsMultiplier;
   else if (subTypeName == "DeathSpawn")
      dataType = BTechEffect::cDataDeathSpawn;

   // Halwes - 12/6/2007 - These tech data sub type tags need to be reflected in techs.xml with TechDataSubType tags for the editor and trigger system
   //                      to work correctly.
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   return (dataType);
}

//==============================================================================
// Convert tech effect data relativity from tag names to enumerations
//==============================================================================
long BDatabase::getProtoObjectDataRelativity(BSimString relativityName) const
{
   // Halwes - 12/6/2007 - These tech data relativity tags need to be reflected in techs.xml with TechDataRelativity tags for the editor and trigger system
   //                      to work correctly.
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   long relativity = -1;
   if (relativityName == "Absolute")
      relativity = BTechEffect::cRelativityAbsolute;
   else if (relativityName == "BasePercent")
      relativity = BTechEffect::cRelativityBasePercent;
   else if (relativityName == "Percent")
      relativity = BTechEffect::cRelativityPercent;
   else if (relativityName == "Assign")
      relativity = BTechEffect::cRelativityAssign;
   else if (relativityName == "BasePercentAssign")
      relativity = BTechEffect::cRelativityBasePercentAssign;
   // Halwes - 12/6/2007 - These tech data relativity tags need to be reflected in techs.xml with TechDataRelativity tags for the editor and trigger system
   //                      to work correctly.
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   return (relativity);
}

//==============================================================================
// BDatabase::getProtoObjectCommandType
//==============================================================================
long BDatabase::getProtoObjectCommandType(const char* pName) const
{
   // Halwes - 12/6/2007 - These tech data command type tags need to be reflected in techs.xml with TechDataCommandType tags for the editor and trigger system
   //                      to work correctly.
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   long commandType = -1;
   if (stricmp(pName, "TrainSquad") == 0)
      commandType = BProtoObjectCommand::cTypeTrainSquad;
   else if (stricmp(pName, "TrainUnit") == 0)
      commandType = BProtoObjectCommand::cTypeTrainUnit;
   else if (stricmp(pName, "Build") == 0)
      commandType = BProtoObjectCommand::cTypeBuild;
   else if (stricmp(pName, "Research") == 0)
      commandType = BProtoObjectCommand::cTypeResearch;
   else if (stricmp(pName, "Unload") == 0)
      commandType = BProtoObjectCommand::cTypeUnloadUnits;
   else if (stricmp(pName, "Reinforce") == 0)
      commandType = BProtoObjectCommand::cTypeReinforce;
   else if (stricmp(pName, "Ability") == 0)
      commandType = BProtoObjectCommand::cTypeAbility;
   else if (stricmp(pName, "ChangeMode") == 0)
      commandType = BProtoObjectCommand::cTypeChangeMode;
   else if (stricmp(pName, "Kill") == 0)
      commandType = BProtoObjectCommand::cTypeKill;
   else if (stricmp(pName, "CancelKill") == 0)
      commandType = BProtoObjectCommand::cTypeCancelKill;
   else if (stricmp(pName, "Tribute") == 0)
      commandType = BProtoObjectCommand::cTypeTribute;
   else if (stricmp(pName, "CustomCommand") == 0)
      commandType = BProtoObjectCommand::cTypeCustomCommand;
   else if (stricmp(pName, "Power") == 0)
      commandType = BProtoObjectCommand::cTypePower;
   else if (stricmp(pName, "BuildOther") == 0)
      commandType = BProtoObjectCommand::cTypeBuildOther;
   else if (stricmp(pName, "TrainLock") == 0)
      commandType = BProtoObjectCommand::cTypeTrainLock;
   else if (stricmp(pName, "TrainUnlock") == 0)
      commandType = BProtoObjectCommand::cTypeTrainUnlock;
   else if (stricmp(pName, "RallyPoint") == 0)
      commandType = BProtoObjectCommand::cTypeRallyPoint;
   else if (stricmp(pName, "ClearRallyPoint") == 0)
      commandType = BProtoObjectCommand::cTypeClearRallyPoint;
   else if (stricmp(pName, "DestroyBase") == 0)
      commandType = BProtoObjectCommand::cTypeDestroyBase;
   else if (stricmp(pName, "CancelDestroyBase") == 0)
      commandType = BProtoObjectCommand::cTypeCancelDestroyBase;
   else if (stricmp(pName, "ReverseHotDrop") == 0)
      commandType = BProtoObjectCommand::cTypeReverseHotDrop;
   // Halwes - 12/6/2007 - These tech data command type tags need to be reflected in techs.xml with TechDataCommandType tags for the editor and trigger system
   //                      to work correctly.
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   return (commandType);
}

//==============================================================================
// BDatabase::getProtoObjectCommandName
//==============================================================================
const char* BDatabase::getProtoObjectCommandName(long type) const
{
   switch(type)
   {
      case BProtoObjectCommand::cTypeResearch   : return "Research";
      case BProtoObjectCommand::cTypeTrainUnit  : return "TrainUnit";
      case BProtoObjectCommand::cTypeBuild      : return "Build";
      case BProtoObjectCommand::cTypeTrainSquad : return "TrainSquad";
      case BProtoObjectCommand::cTypeUnloadUnits: return "Unload";
      case BProtoObjectCommand::cTypeReinforce  : return "Reinforce";
      case BProtoObjectCommand::cTypeChangeMode : return "ChangeMode";
      case BProtoObjectCommand::cTypeAbility    : return "Ability";
      case BProtoObjectCommand::cTypeKill       : return "Kill";
      case BProtoObjectCommand::cTypeCancelKill : return "CancelKill";
      case BProtoObjectCommand::cTypeTribute    : return "Tribute";
      case BProtoObjectCommand::cTypeCustomCommand: return "CustomCommand";
      case BProtoObjectCommand::cTypePower      : return "Power";
      case BProtoObjectCommand::cTypeBuildOther : return "BuildOther";
      case BProtoObjectCommand::cTypeTrainLock  : return "TrainLock";
      case BProtoObjectCommand::cTypeTrainUnlock: return "TrainUnlock";
      case BProtoObjectCommand::cTypeRallyPoint : return "RallyPoint";
      case BProtoObjectCommand::cTypeClearRallyPoint : return "ClearRallyPoint";
      case BProtoObjectCommand::cTypeDestroyBase : return "DestroyBase";
      case BProtoObjectCommand::cTypeCancelDestroyBase : return "CancelDestroyBase";
      case BProtoObjectCommand::cTypeReverseHotDrop : return "ReverseHotDrop";
      default: return "";
   }
}

//==============================================================================
// BDatabase::getProtoShieldType
//==============================================================================
BProtoSquadID BDatabase::getProtoShieldType(BProtoSquadID squadType)
{
   bhandle res = mProtoShieldIDs.find(squadType);
   if (res == NULL)
      return mDefaultShieldID;
   else
      return  mProtoShieldIDs.get(res);
}

//==============================================================================
// BDatabase::getDamageType
//==============================================================================
BDamageTypeID BDatabase::getDamageType(const char* pName) const
{
   if (!pName)
      return (cInvalidDamageTypeID);
   uint count = mDamageTypes.getSize();
   for(uint i = 0; i < count; i++)
   {
      if (mDamageTypes[i].getName() == pName)
         return (static_cast<BDamageTypeID>(i));
   }

   return (cInvalidDamageTypeID);
}

//==============================================================================
// BDatabase::getDamageTypeName
//==============================================================================
const char* BDatabase::getDamageTypeName(BDamageTypeID type) const
{
   if (type < 0 || type >= mDamageTypes.getNumber())
      return ("");
   else
      return (mDamageTypes[type].getName());
}

//==============================================================================
// BDatabase::isBaseDamageType
//==============================================================================
bool BDatabase::isBaseDamageType(BDamageTypeID type) const
{
   if (type < 0 || type >= mDamageTypes.getNumber())
      return false;
   else
      return (mDamageTypes[type].getBaseType());
}


//==============================================================================
// BDatabase::setupDamageTypeExemplars
//==============================================================================
void BDatabase::setupDamageTypeExemplars()
{
   int numDamageTypes = mDamageTypes.getNumber();
   mDamageTypeExemplars.resize(numDamageTypes);
   for(BDamageTypeID i = 0; i < numDamageTypes; i++)
   {
      long numProto = getNumberProtoObjects();
      
      mDamageTypeExemplars[i] = cInvalidProtoObjectID;
      
      for (long protoID = 0; protoID < numProto; protoID++)
      {
         const BProtoObject* pProto = gDatabase.getGenericProtoObject(protoID);
         BDamageTypeID type = pProto->getDamageType();
         
         if (type < 0 || type >= numDamageTypes)
            continue;

         if(type == i)
         {
            mDamageTypeExemplars[i] = protoID;
            break;
         }
      }
   }


   //Fixup for objects that can go in cover:   
   mDamageTypeExemplars[getDamageTypeLightInCover()] = mDamageTypeExemplars[getDamageTypeLight()];   
   mDamageTypeExemplars[getDamageTypeLightArmoredInCover()] = mDamageTypeExemplars[getDamageTypeLightArmored()];

   //fixup medium 
   mDamageTypeExemplars[getDamageTypeMedium()] = gDatabase.getProtoObject("unsc_veh_warthog_01");

}


//==============================================================================
// BDatabase::setupTacticAttackRatings
//==============================================================================
void BDatabase::setupTacticAttackRatings()
{
   long count = mProtoObjects.size();
   for (long i=0; i<count; i++)
   {
      BProtoObject *pProtoObject = mProtoObjects[i];
      if (pProtoObject)
      {
         //Debug code: This is a good place to step through a specific tactic getting setup
         //if(pProtoObject->getName().compare("unsc_inf_marine_01") == 0)
         //{
         //   i = i;
         //}
         BTactic* pTactic = pProtoObject->getTactic();
         if(pTactic)
         {
            pTactic->setupTacticAttackRatings();
         }
      }
   }
}

//==============================================================================
// BDatabase::getDamageTypeExemplar
//==============================================================================
BProtoObjectID BDatabase::getDamageTypeExemplar(BDamageTypeID type)
{
   if (type < 0 || type >= mDamageTypeExemplars.getNumber())
      return cInvalidProtoObjectID;

   return mDamageTypeExemplars[type];
}


//==============================================================================
// BDatabase::getAttackRatingIndex
//==============================================================================
bool BDatabase::getAttackRatingIndex(BDamageTypeID damageType, uint& indexOut) const
{
   if (damageType < 0 || damageType >= mDamageTypes.getNumber())
      return false;
   else
   {
      int index = mDamageTypes[damageType].getAttackRatingIndex();
      if (index != -1)
      {
         indexOut = (uint)index;
         return true;
      }
      else
         return false;
   }
}

//==============================================================================
// BDatabase::getAttackRating
//==============================================================================
float BDatabase::getAttackRating(float dps)
{
   return dps * mAttackRatingMultiplier;
}

//==============================================================================
// BDatabase::getDefenseRating
//==============================================================================
float BDatabase::getDefenseRating(float hp)
{
   return hp * mDefenseRatingMultiplier;
}

//==============================================================================
// BDatabase::getGoodAgainstAttackGrade
//==============================================================================
uint BDatabase::getGoodAgainstAttackGrade(long type)
{
   if ((type >=0) && (type<cNumReticleAttackGrades))
      return mGoodAgainstGrades[type];

   return 0;
}

//==============================================================================
// BDatabase::getAttackGrade
//==============================================================================
uint BDatabase::getAttackGrade(float baseDPS, float attackDPS)
{
   if (baseDPS==0.0f || attackDPS==0.0f)
      return 0;
   float ratio = attackDPS / baseDPS;
   // AJL FIXME 8/23/07 - Move hard-coded ratios to gamedata.xml
   if (ratio <= 0.2f)
      return 0;
   if (ratio <= 0.8f)
      return 1;
   else if (ratio <= 1.4f)
      return 2;
   else if (ratio <= 2.0f)
      return 3;
   else if (ratio <= 2.6f)
      return 4;
   else
      return 5;
}

//==============================================================================
// BDatabase::getAttackGradeRatio
//==============================================================================
float BDatabase::getAttackGradeRatio(float baseDPS, float attackDPS)
{
   if (baseDPS==0.0f || attackDPS==0.0f)
      return 0.0f;
   else
      return (attackDPS / baseDPS);
}

//==============================================================================
// BDatabase::getAutoLockDownType
//==============================================================================
long BDatabase::getAutoLockDownType(const char* pType) const
{
   if(stricmp(pType, "LockAndUnlock")==0)
      return cAutoLockAndUnlock;
   else if(stricmp(pType, "LockOnly")==0)
      return cAutoLockOnly;
   else if(stricmp(pType, "UnlockOnly")==0)
      return cAutoUnlockOnly;
   return cAutoLockNone;
}

//==============================================================================
// BDatabase::getAutoLockDownTypeName
//==============================================================================
const char* BDatabase::getAutoLockDownTypeName(long type) const
{
   switch(type)
   {
   case cAutoLockNone         : return "None";
   case cAutoLockAndUnlock    : return "LockAndUnlock";
   case cAutoLockOnly         : return "LockOnly";
   case cAutoUnlockOnly       : return "UnlockOnly";
   default : return "";
   }
}

//==============================================================================
// BDatabase::getDamageDirection
//==============================================================================
int BDatabase::getDamageDirection(const char* pName) const
{
   if (stricmp(pName, "Full") == 0)
      return (cDamageDirectionFull);
   else if (stricmp(pName, "FrontHalf") == 0)
      return (cDamageDirectionFrontHalf);
   else if (stricmp(pName, "BackHalf") == 0)
      return (cDamageDirectionBackHalf);
   else if (stricmp(pName, "Front") == 0)
      return (cDamageDirectionFront);
   else if (stricmp(pName, "Back") == 0)
      return (cDamageDirectionBack);
   else if (stricmp(pName, "Left") == 0)
      return (cDamageDirectionLeft);
   else if (stricmp(pName, "Right") == 0)
      return (cDamageDirectionRight);
   else
      return (-1);
}

//==============================================================================
// BDatabase::getDamageDirectionName
//==============================================================================
const char* BDatabase::getDamageDirectionName(int type) const
{
   switch (type)
   {
      case cDamageDirectionFull:       return ("Full");
      case cDamageDirectionFrontHalf:  return ("FrontHalf");
      case cDamageDirectionBackHalf:   return ("BackHalf");
      case cDamageDirectionFront:      return ("Front");
      case cDamageDirectionBack:       return ("Back");
      case cDamageDirectionLeft:       return ("Left");
      case cDamageDirectionRight:      return ("Right");
      default:                         return ("");
   }
}

//==============================================================================
// BDatabase::getObjectClass
//==============================================================================
long BDatabase::getObjectClass(const char* pName) const
{
   if(stricmp(pName, "Squad")==0)
      return cObjectClassSquad;
   else if(stricmp(pName, "Building")==0)
      return cObjectClassBuilding;
   else if(stricmp(pName, "Unit")==0)
      return cObjectClassUnit;
   else if(stricmp(pName, "Projectile")==0)
      return cObjectClassProjectile;
   return cObjectClassObject;
}

//==============================================================================
// BDatabase::getObjectClassName
//==============================================================================
const char* BDatabase::getObjectClassName(long type) const
{
   switch(type)
   {
      case cObjectClassSquad:       return "Squad";
      case cObjectClassBuilding:    return "Building";
      case cObjectClassUnit:        return "Unit";
      case cObjectClassProjectile:  return "Projectile";
      case cObjectClassObject:      return "Object";
      default : return "";
   }
}

//==============================================================================
// BDatabase::getMovementType
//==============================================================================
long BDatabase::getMovementType(const char* pName) const
{
   if(stricmp(pName, "Land")==0)
      return cMovementTypeLand;
   else if(stricmp(pName, "Air")==0)
      return cMovementTypeAir;
   else if(stricmp(pName, "Flood")==0)
   {
      if (gConfig.isDefined(cConfigAlpha))
         return cMovementTypeLand;
      else
         return cMovementTypeFlood;
   }
   else if(stricmp(pName, "Scarab")==0)
   {
      if (gConfig.isDefined(cConfigAlpha))
         return cMovementTypeLand;
      else
         return cMovementTypeScarab;
   }
   else if(stricmp(pName, "Hover")==0)
   {
      // ajl 11/14/06 - temp remove support for hover units to decrease size of lrp trees
      //return cMovementTypeHover;
      return cMovementTypeLand;
   }
   return cMovementTypeNone;
}

//==============================================================================
// BDatabase::getMovementTypeName
//==============================================================================
const char* BDatabase::getMovementTypeName(long type) const
{
   switch(type)
   {
      case cMovementTypeLand   : return "Land";
      case cMovementTypeAir    : return "Air";
      case cMovementTypeFlood  : return "Flood";
      case cMovementTypeScarab : return "Scarab";
      case cMovementTypeHover  : return "Hover";
      default: return "";
   }
}

//==============================================================================
// BDatabase::getPickPriority
//==============================================================================
long BDatabase::getPickPriority(const char* pName) const
{
   if(stricmp(pName, "Building")==0)
      return cPickPriorityBuilding;
   else if(stricmp(pName, "Resource")==0)
      return cPickPriorityResource;
   else if(stricmp(pName, "Unit")==0)
      return cPickPriorityUnit;
   else if(stricmp(pName, "Rally")==0)
      return cPickPriorityRally;
   return cPickPriorityNone;
}

//==============================================================================
// BDatabase::getPickPriorityName
//==============================================================================
const char* BDatabase::getPickPriorityName(long type) const
{
   switch(type)
   {
      case cPickPriorityBuilding : return "Building";
      case cPickPriorityResource : return "Resource";
      case cPickPriorityUnit     : return "Unit";
      case cPickPriorityRally    : return "Rally";
      default : return "";
   }
}

//==============================================================================
// BDatabase::getSelectType
//==============================================================================
long BDatabase::getSelectType(const char* pName) const
{
   if(stricmp(pName, "Unit")==0)
      return cSelectTypeUnit;
   else if(stricmp(pName, "Command")==0)
      return cSelectTypeCommand;
   else if(stricmp(pName, "Target")==0)
      return cSelectTypeTarget;
   else if(stricmp(pName, "SingleUnit")==0)
      return cSelectTypeSingleUnit;
   else if(stricmp(pName, "SingleType")==0)
      return cSelectTypeSingleType;
   return cSelectTypeNone;
}

//==============================================================================
// BDatabase::getSelectTypeName
//==============================================================================
const char* BDatabase::getSelectTypeName(long type) const
{
   switch(type)
   {
      case cSelectTypeUnit    : return "Unit";
      case cSelectTypeCommand : return "Command";
      case cSelectTypeTarget  : return "Target";
      case cSelectTypeSingleUnit : return "SingleUnit";
      case cSelectTypeSingleType : return "SingleType";
      default : return "";
   }
}


//==============================================================================
//==============================================================================
BAIMissionType BDatabase::getMissionType(const char* pName) const
{
   if (stricmp(pName, "Invalid") == 0)
      return (MissionType::cInvalid);
   else if (stricmp(pName, "Attack") == 0)
      return (MissionType::cAttack);
   else if (stricmp(pName, "Defend") == 0)
      return (MissionType::cDefend);
   else if (stricmp(pName, "Scout") == 0)
      return (MissionType::cScout);
   else if (stricmp(pName, "Claim") == 0)
      return (MissionType::cClaim);
   else if (stricmp(pName, "Power") == 0)
      return (MissionType::cPower);
   else
   {
      BASSERTM(false, "Unknown BAIMissionType.");
      return (MissionType::cInvalid);
   }
}


//==============================================================================
//==============================================================================
const char* BDatabase::getMissionTypeName(BAIMissionType missionType) const
{
   switch (missionType)
   {
      case MissionType::cInvalid:
         return ("Invalid");
      case MissionType::cAttack:
         return ("Attack");
      case MissionType::cDefend:
         return ("Defend");
      case MissionType::cScout:
         return ("Scout");
      case MissionType::cClaim:
         return ("Claim");
      case MissionType::cPower:
         return ("Power");
      default:
      {
         BASSERTM(false, "Unknown BAIMissionType.");
         return ("Invalid");
      }
   }
}


//==============================================================================
//==============================================================================
BAIGroupTaskType BDatabase::getAIGroupTaskType(const char* pName) const
{
   if (stricmp(pName, "Wait") == 0)
      return (AIGroupTaskType::cWait);
   else if (stricmp(pName, "Move") == 0)
      return (AIGroupTaskType::cMove);
   else if (stricmp(pName, "Attack") == 0)
      return (AIGroupTaskType::cAttack);
   else if (stricmp(pName, "Hijack") == 0)
      return (AIGroupTaskType::cHijack);
   else if (stricmp(pName, "Cloak") == 0)
      return (AIGroupTaskType::cCloak);
   else if (stricmp(pName, "Gather") == 0)
      return (AIGroupTaskType::cGather);
   else if (stricmp(pName, "Garrison") == 0)
      return (AIGroupTaskType::cGarrison);
   else if (stricmp(pName, "RepairOther") == 0)
      return (AIGroupTaskType::cRepairOther);
   else if (stricmp(pName, "Detonate") == 0)
      return (AIGroupTaskType::cDetonate);
   else
   {
      BASSERTM(false, "Unknown AIGroupTaskType");
      return (AIGroupTaskType::cInvalid);
   }
}


//==============================================================================
//==============================================================================
const char* BDatabase::getAIGroupTaskTypeName(BAIGroupTaskType aiGroupTaskType) const
{
   switch (aiGroupTaskType)
   {
      case AIGroupTaskType::cInvalid:
         return ("Invalid");
      case AIGroupTaskType::cWait:
         return ("Wait");
      case AIGroupTaskType::cMove:
         return ("Move");
      case AIGroupTaskType::cAttack:
         return ("Attack");
      case AIGroupTaskType::cHijack:
         return ("Hijack");
      case AIGroupTaskType::cCloak:
         return ("Cloak");
      case AIGroupTaskType::cGather:
         return ("Gather");
      case AIGroupTaskType::cGarrison:
         return ("Garrison");
      case AIGroupTaskType::cRepairOther:
         return ("RepairOther");
      case AIGroupTaskType::cDetonate:
         return ("Detonate");
      default:
      {
         BASSERTM(false, "Unknown BAIGroupTaskType.");
         return ("Invalid");
      }
   }
}


//==============================================================================
//==============================================================================
BAIGroupState BDatabase::getAIGroupState(const char* pName) const
{
   if (stricmp(pName, "Invalid") == 0)
      return (AIGroupState::cInvalid);
   else if (stricmp(pName, "Moving") == 0)
      return (AIGroupState::cMoving);
   else if (stricmp(pName, "Waiting") == 0)
      return (AIGroupState::cWaiting);
   else if (stricmp(pName, "Withdraw") == 0)
      return (AIGroupState::cWithdraw);
   else if (stricmp(pName, "Engaged") == 0)
      return (AIGroupState::cEngaged);
   else
      {
         BASSERTM(false, "Unknown AIGroupState");
         return (AIGroupState::cInvalid);
      }
}


//==============================================================================
//==============================================================================
const char* BDatabase::getAIGroupStateName(BAIGroupState aiGroupState) const
{
   switch (aiGroupState)
   {
      case AIGroupState::cInvalid:
         return ("Invalid");
      case AIGroupState::cMoving:
         return ("Moving");
      case AIGroupState::cWaiting:
         return ("Waiting");
      case AIGroupState::cWithdraw:
         return ("Withdraw");
      case AIGroupState::cEngaged:
         return ("Engaged");
      default:
         return ("Invalid");
   }
}


//==============================================================================
//==============================================================================
BAIMissionState BDatabase::getMissionState(const char* pName) const
{
   if (stricmp(pName, "Invalid") == 0)
      return (MissionState::cInvalid);
   else if (stricmp(pName, "Success") == 0)
      return (MissionState::cSuccess);
   else if (stricmp(pName, "Failure") == 0)
      return (MissionState::cFailure);
   else if (stricmp(pName, "Create") == 0)
      return (MissionState::cCreate);
   else if (stricmp(pName, "Working") == 0)
      return (MissionState::cWorking);
   else if (stricmp(pName, "Withdraw") == 0)
      return (MissionState::cWithdraw);
   else if (stricmp(pName, "Retreat") == 0)
      return (MissionState::cRetreat);
   else
   {
      BASSERTM(false, "Unknown BAIMissionState.");
      return (MissionState::cInvalid);
   }
}


//==============================================================================
//==============================================================================
const char* BDatabase::getMissionStateName(BAIMissionState missionState) const
{
   switch (missionState)
   {
   case MissionState::cInvalid:
      return ("Invalid");
   case MissionState::cSuccess:
      return ("Success");
   case MissionState::cFailure:
      return ("Failure");
   case MissionState::cCreate:
      return ("Create");
   case MissionState::cWorking:
      return ("Working");
   case MissionState::cWithdraw:
      return ("Withdraw");
   case MissionState::cRetreat:
      return ("Retreat");
   default:
      {
         BASSERTM(false, "Unknown BAIMissionState.");
         return ("Invalid");
      }
   }
}


//==============================================================================
//==============================================================================
BAIMissionTargetType BDatabase::getMissionTargetType(const char* pName) const
{
   if (stricmp(pName, "Invalid") == 0)
      return (MissionTargetType::cInvalid);
   else if (stricmp(pName, "Area") == 0)
      return (MissionTargetType::cArea);
   else if (stricmp(pName, "KBBase") == 0)
      return (MissionTargetType::cKBBase);
   else if (stricmp(pName, "CaptureNode") == 0)
      return (MissionTargetType::cCaptureNode);
   else
   {
      BASSERTM(false, "Unknown BAIMissionTargetType.");
      return (MissionTargetType::cInvalid);
   }
}


//==============================================================================
//==============================================================================
const char* BDatabase::getMissionTargetTypeName(BAIMissionTargetType missionTargetType) const
{
   switch (missionTargetType)
   {
   case MissionTargetType::cInvalid:
      return ("Invalid");
   case MissionTargetType::cArea:
      return ("Area");
   case MissionTargetType::cKBBase:
      return ("KBBase");
   case MissionTargetType::cCaptureNode:
      return ("CaptureNode");
   default:
      {
         BASSERTM(false, "Unknown BAIMissionTargetType.");
         return ("Invalid");
      }
   }
}

//==============================================================================
//==============================================================================
BBidType BDatabase::getBidType(const char* pName) const
{
   if (stricmp(pName, "Invalid") == 0)
      return (BidType::cInvalid);
   else if (stricmp(pName, "None") == 0)
      return (BidType::cNone);
   else if (stricmp(pName, "Squad") == 0)
      return (BidType::cSquad);
   else if (stricmp(pName, "Tech") == 0)
      return (BidType::cTech);
   else if (stricmp(pName, "Building") == 0)
      return (BidType::cBuilding);
   else if (stricmp(pName, "Power") == 0)
      return (BidType::cPower);
   else
   {
      BASSERTM(false, "Unknown BBidType.");
      return (BidType::cInvalid);
   }
}

//==============================================================================
//==============================================================================
const char* BDatabase::getBidTypeName(BBidType bidType) const
{
   switch (bidType)
   {
      case BidType::cInvalid:
         return ("Invalid");
      case BidType::cNone:
         return ("None");
      case BidType::cSquad:
         return ("Squad");
      case BidType::cTech:
         return ("Tech");
      case BidType::cBuilding:
         return ("Building");
      case BidType::cPower:
         return ("Power");
      default:
      {
         BASSERTM(false, "Unknown BBidType.");
         return ("Invalid");
      }
   }
}

//==============================================================================
//==============================================================================
BBidState BDatabase::getBidState(const char* pName) const
{
   if (stricmp(pName, "Invalid") == 0)
      return (BidState::cInvalid);
   else if (stricmp(pName, "Inactive") == 0)
      return (BidState::cInactive);
   else if (stricmp(pName, "Waiting") == 0)
      return (BidState::cWaiting);
   else if (stricmp(pName, "Approved") == 0)
      return (BidState::cApproved);
   else
   {
      BASSERTM(false, "Unknown BBidState.");
      return (BidState::cInvalid);
   }
}

//==============================================================================
//==============================================================================
const char* BDatabase::getBidStateName(BBidState bidState) const
{
   switch (bidState)
   {
      case BidState::cInvalid:
         return ("Invalid");
      case BidState::cInactive:
         return ("Inactive");
      case BidState::cWaiting:
         return ("Waiting");
      case BidState::cApproved:
         return ("Approved");
      default:
      {
         BASSERTM(false, "Unknown BBidState.");
         return ("Invalid");
      }
   }
}

//==============================================================================
//==============================================================================
BMissionConditionType BDatabase::getMissionConditionType(const char* pName) const
{
   if (stricmp(pName, "Invalid") == 0)
      return (MissionConditionType::cInvalid);
   else if (stricmp(pName, "SquadCount") == 0)
      return (MissionConditionType::cSquadCount);
   else if (stricmp(pName, "AreaSecured") == 0)
      return (MissionConditionType::cAreaSecured);
   else
   {
      BASSERTM(false, "Unknown MissionConditionType.");
      return (MissionConditionType::cInvalid);
   }
}


//==============================================================================
//==============================================================================
const char* BDatabase::getMissionConditionTypeName(BMissionConditionType missionConditionType) const
{
   switch (missionConditionType)
   {
      case MissionConditionType::cInvalid:
         return ("Invalid");
      case MissionConditionType::cSquadCount:
         return ("SquadCount");
      case MissionConditionType::cAreaSecured:
         return ("AreaSecured");
      default:
         BASSERTM(false, "Invalid mission condition type.");
         return ("Invalid");
   }
}


//==============================================================================
//==============================================================================
BAISquadAnalysisComponent BDatabase::getAISquadAnalysisComponent(const char* pName) const
{
   if (stricmp(pName, "Invalid") == 0)
      return (AISquadAnalysisComponent::cInvalid);
   else if (stricmp(pName, "CVLight") == 0)
      return (AISquadAnalysisComponent::cCVLight);
   else if (stricmp(pName, "CVLightArmored") == 0)
      return (AISquadAnalysisComponent::cCVLightArmored);
   else if (stricmp(pName, "CVMedium") == 0)
      return (AISquadAnalysisComponent::cCVMedium);
   else if (stricmp(pName, "CVMediumAir") == 0)
      return (AISquadAnalysisComponent::cCVMediumAir);
   else if (stricmp(pName, "CVHeavy") == 0)
      return (AISquadAnalysisComponent::cCVHeavy);
   else if (stricmp(pName, "CVBuilding") == 0)
      return (AISquadAnalysisComponent::cCVBuilding);
   else if (stricmp(pName, "CVTotal") == 0)
      return (AISquadAnalysisComponent::cCVTotal);
   else if (stricmp(pName, "HPLight") == 0)
      return (AISquadAnalysisComponent::cHPLight);
   else if (stricmp(pName, "HPLightArmored") == 0)
      return (AISquadAnalysisComponent::cHPLightArmored);
   else if (stricmp(pName, "HPMedium") == 0)
      return (AISquadAnalysisComponent::cHPMedium);
   else if (stricmp(pName, "HPMediumAir") == 0)
      return (AISquadAnalysisComponent::cHPMediumAir);
   else if (stricmp(pName, "HPHeavy") == 0)
      return (AISquadAnalysisComponent::cHPHeavy);
   else if (stricmp(pName, "HPBuilding") == 0)
      return (AISquadAnalysisComponent::cHPBuilding);
   else if (stricmp(pName, "HPTotal") == 0)
      return (AISquadAnalysisComponent::cHPTotal);
   else if (stricmp(pName, "SPLight") == 0)
      return (AISquadAnalysisComponent::cSPLight);
   else if (stricmp(pName, "SPLightArmored") == 0)
      return (AISquadAnalysisComponent::cSPLightArmored);
   else if (stricmp(pName, "SPMedium") == 0)
      return (AISquadAnalysisComponent::cSPMedium);
   else if (stricmp(pName, "SPMediumAir") == 0)
      return (AISquadAnalysisComponent::cSPMediumAir);
   else if (stricmp(pName, "SPHeavy") == 0)
      return (AISquadAnalysisComponent::cSPHeavy);
   else if (stricmp(pName, "SPBuilding") == 0)
      return (AISquadAnalysisComponent::cSPBuilding);
   else if (stricmp(pName, "SPTotal") == 0)
      return (AISquadAnalysisComponent::cSPTotal);
   else if (stricmp(pName, "DPSLight") == 0)
      return (AISquadAnalysisComponent::cDPSLight);
   else if (stricmp(pName, "DPSLightArmored") == 0)
      return (AISquadAnalysisComponent::cDPSLightArmored);
   else if (stricmp(pName, "DPSMedium") == 0)
      return (AISquadAnalysisComponent::cDPSMedium);
   else if (stricmp(pName, "DPSMediumAir") == 0)
      return (AISquadAnalysisComponent::cDPSMediumAir);
   else if (stricmp(pName, "DPSHeavy") == 0)
      return (AISquadAnalysisComponent::cDPSHeavy);
   else if (stricmp(pName, "DPSBuilding") == 0)
      return (AISquadAnalysisComponent::cDPSBuilding);
   else if (stricmp(pName, "DPSTotal") == 0)
      return (AISquadAnalysisComponent::cDPSTotal);
   else if (stricmp(pName, "CVPercentLight") == 0)
      return (AISquadAnalysisComponent::cCVPercentLight);
   else if (stricmp(pName, "CVPercentLightArmored") == 0)
      return (AISquadAnalysisComponent::cCVPercentLightArmored);
   else if (stricmp(pName, "CVPercentMedium") == 0)
      return (AISquadAnalysisComponent::cCVPercentMedium);
   else if (stricmp(pName, "CVPercentMediumAir") == 0)
      return (AISquadAnalysisComponent::cCVPercentMediumAir);
   else if (stricmp(pName, "CVPercentHeavy") == 0)
      return (AISquadAnalysisComponent::cCVPercentHeavy);
   else if (stricmp(pName, "CVPercentBuilding") == 0)
      return (AISquadAnalysisComponent::cCVPercentBuilding);
   else if (stricmp(pName, "CVStarsLight") == 0)
      return (AISquadAnalysisComponent::cCVStarsLight);
   else if (stricmp(pName, "CVStarsLightArmored") == 0)
      return (AISquadAnalysisComponent::cCVStarsLightArmored);
   else if (stricmp(pName, "CVStarsMedium") == 0)
      return (AISquadAnalysisComponent::cCVStarsMedium);
   else if (stricmp(pName, "CVStarsMediumAir") == 0)
      return (AISquadAnalysisComponent::cCVStarsMediumAir);
   else if (stricmp(pName, "CVStarsHeavy") == 0)
      return (AISquadAnalysisComponent::cCVStarsHeavy);
   else if (stricmp(pName, "CVStarsBuilding") == 0)
      return (AISquadAnalysisComponent::cCVStarsBuilding);
   else if (stricmp(pName, "CVStarsTotal") == 0)
      return (AISquadAnalysisComponent::cCVStarsTotal);
   else
   {
      BASSERTM(false, "Unknown AISquadAnalysisComponent.");
      return (AISquadAnalysisComponent::cInvalid);
   }
}


//==============================================================================
//==============================================================================
const char* BDatabase::getAISquadAnalysisComponentName(BAISquadAnalysisComponent aiSquadAnalysisComponent) const
{
   switch (aiSquadAnalysisComponent)
   {
   case AISquadAnalysisComponent::cInvalid:
      return ("Invalid");
   case AISquadAnalysisComponent::cCVLight:
      return ("CVLight");
   case AISquadAnalysisComponent::cCVMedium:
      return ("CVMedium");
   case AISquadAnalysisComponent::cCVMediumAir:
      return ("CVMediumAir");
   case AISquadAnalysisComponent::cCVHeavy:
      return ("CVHeavy");
   case AISquadAnalysisComponent::cCVBuilding:
      return ("CVBuilding");
   case AISquadAnalysisComponent::cCVTotal:
      return ("CVTotal");
   case AISquadAnalysisComponent::cHPLight:
      return ("HPLight");
   case AISquadAnalysisComponent::cHPMedium:
      return ("HPMedium");
   case AISquadAnalysisComponent::cHPMediumAir:
      return ("HPMediumAir");
   case AISquadAnalysisComponent::cHPHeavy:
      return ("HPHeavy");
   case AISquadAnalysisComponent::cHPBuilding:
      return ("HPBuilding");
   case AISquadAnalysisComponent::cHPTotal:
      return ("HPTotal");
   case AISquadAnalysisComponent::cSPLight:
      return ("SPLight");
   case AISquadAnalysisComponent::cSPMedium:
      return ("SPMedium");
   case AISquadAnalysisComponent::cSPMediumAir:
      return ("SPMediumAir");
   case AISquadAnalysisComponent::cSPHeavy:
      return ("SPHeavy");
   case AISquadAnalysisComponent::cSPBuilding:
      return ("SPBuilding");
   case AISquadAnalysisComponent::cSPTotal:
      return ("SPTotal");
   case AISquadAnalysisComponent::cDPSLight:
      return ("DPSLight");
   case AISquadAnalysisComponent::cDPSMedium:
      return ("DPSMedium");
   case AISquadAnalysisComponent::cDPSMediumAir:
      return ("DPSMediumAir");
   case AISquadAnalysisComponent::cDPSHeavy:
      return ("DPSHeavy");
   case AISquadAnalysisComponent::cDPSBuilding:
      return ("DPSBuilding");
   case AISquadAnalysisComponent::cDPSTotal:
      return ("DPSTotal");
   case AISquadAnalysisComponent::cCVPercentLight:
      return ("CVPercentLight");
   case AISquadAnalysisComponent::cCVPercentMedium:
      return ("CVPercentMedium");
   case AISquadAnalysisComponent::cCVPercentMediumAir:
      return ("CVPercentMediumAir");
   case AISquadAnalysisComponent::cCVPercentHeavy:
      return ("CVPercentHeavy");
   case AISquadAnalysisComponent::cCVPercentBuilding:
      return ("CVPercentBuilding");
   case AISquadAnalysisComponent::cCVStarsLight:
      return ("CVStarsLight");
   case AISquadAnalysisComponent::cCVStarsMedium:
      return ("CVStarsMedium");
   case AISquadAnalysisComponent::cCVStarsMediumAir:
      return ("CVStarsMediumAir");
   case AISquadAnalysisComponent::cCVStarsHeavy:
      return ("CVStarsHeavy");
   case AISquadAnalysisComponent::cCVStarsBuilding:
      return ("CVStarsBuilding");
   case AISquadAnalysisComponent::cCVStarsTotal:
      return ("CVStarsTotal");
   default:
      BASSERTM(false, "Invalid AISquadAnalysisComponent.");
      return ("Invalid");
   }
}


//==============================================================================
//==============================================================================
BPowerType BDatabase::getPowerType(const char* pName) const
{
   if (stricmp(pName, "Cleansing") == 0)
      return (PowerType::cCleansing);
   else if (stricmp(pName, "Orbital") == 0)
      return (PowerType::cOrbital);
   else if (stricmp(pName, "CarpetBombing") == 0)
      return (PowerType::cCarpetBombing);
   else if (stricmp(pName, "Cryo") == 0)
      return (PowerType::cCryo);
   else if (stricmp(pName, "Wave") == 0)
      return (PowerType::cWave);
   else if (stricmp(pName, "Rage") == 0)
      return (PowerType::cRage);
   else if (stricmp(pName, "Disruption") == 0)
      return (PowerType::cDisruption);
   else if (stricmp(pName, "Repair") == 0)
      return (PowerType::cRepair);
   else if (stricmp(pName, "Transport") == 0)
      return (PowerType::cTransport);
   else if (stricmp(pName, "ODST") == 0)
      return (PowerType::cODST);
   else
      return (PowerType::cInvalid);
}


//==============================================================================
//==============================================================================
const char* BDatabase::getPowerTypeName(BPowerType powerType) const
{
   if (powerType == PowerType::cCleansing)
      return ("Cleansing");
   else if (powerType == PowerType::cOrbital)
      return ("Orbital");
   else if (powerType == PowerType::cCarpetBombing)
      return ("CarpetBombing");
   else if (powerType == PowerType::cCryo)
      return ("Cryo");
   else if (powerType == PowerType::cRage)
      return ("Rage");
   else if (powerType == PowerType::cWave)
      return ("Wave");
   else if (powerType == PowerType::cDisruption)
      return ("Disruption");
   else if (powerType == PowerType::cRepair)
      return ("Repair");
   else if (powerType == PowerType::cTransport)
      return ("Transport");
   else if (powerType == PowerType::cODST)
      return ("ODST");
   else
      return ("Invalid");
}

//==============================================================================
// BDatabase::getGotoType
//==============================================================================
long BDatabase::getGotoType(const char* pName) const
{
   if(stricmp(pName, "Base")==0)
      return cGotoTypeBase;
   else if(stricmp(pName, "Military")==0)
      return cGotoTypeMilitary;
   else if(stricmp(pName, "Infantry")==0)
      return cGotoTypeInfantry;
   else if(stricmp(pName, "Vehicle")==0)
      return cGotoTypeVehicle;
   else if(stricmp(pName, "Air")==0)
      return cGotoTypeAir;
   else if(stricmp(pName, "Civilian")==0)
      return cGotoTypeCivilian;
   else if(stricmp(pName, "Scout")==0)
      return cGotoTypeScout;
   else if(stricmp(pName, "Node")==0)
      return cGotoTypeNode;
   else if(stricmp(pName, "Hero")==0)
      return cGotoTypeHero;
   return cGotoTypeNone;
}

//==============================================================================
// BDatabase::getGotoTypeName
//==============================================================================
const char* BDatabase::getGotoTypeName(long type) const
{
   switch(type)
   {
      case cGotoTypeBase     : return "Base";
      case cGotoTypeMilitary : return "Military";
      case cGotoTypeInfantry : return "Infantry";
      case cGotoTypeVehicle  : return "Vehicle";
      case cGotoTypeAir      : return "Air";
      case cGotoTypeCivilian : return "Civilian";
      case cGotoTypeScout    : return "Scout";
      case cGotoTypeNode     : return "Node";
      case cGotoTypeHero     : return "Hero";
      default : return "";
   }
}

//==============================================================================
// BDatabase::getRelationType
//==============================================================================
BRelationType BDatabase::getRelationType(const char* pName) const
{
   if(stricmp(pName, "Self")==0)
      return cRelationTypeSelf;
   else if(stricmp(pName, "Ally")==0)
      return cRelationTypeAlly;
   else if(stricmp(pName, "Any")==0)
      return cRelationTypeAny;
   else if(stricmp(pName, "Enemy")==0)
      return cRelationTypeEnemy;
   else if(stricmp(pName, "Neutral")==0)
      return cRelationTypeNeutral;
   return cRelationTypeAny;
}

//==============================================================================
// BDatabase::getRelationTypeName
//==============================================================================
const char* BDatabase::getRelationTypeName(BRelationType type) const
{
   switch(type)
   {
      case cRelationTypeSelf     : return "Self";
      case cRelationTypeAlly     : return "Ally";
      case cRelationTypeAny      : return "Any";
      case cRelationTypeEnemy    : return "Enemy";
      case cRelationTypeNeutral  : return "Neutral";
      default : return "Any";
   }
}

//==============================================================================
// Get the enumerated squad mode
//==============================================================================
int BDatabase::getSquadMode(const char* pName) const
{
   if (stricmp(pName, "Normal") == 0)
      return (BSquadAI::cModeNormal);
   else if (stricmp(pName, "StandGround") == 0)
      return (BSquadAI::cModeStandGround);
   else if (stricmp(pName, "Lockdown") == 0)
      return (BSquadAI::cModeLockdown);
   else if (stricmp(pName, "Sniper") == 0)
      return (BSquadAI::cModeSniper);
   else if (stricmp(pName, "HitAndRun") == 0)
      return (BSquadAI::cModeHitAndRun);
   else if (stricmp(pName, "Passive") == 0)
      return (BSquadAI::cModePassive);
   else if (stricmp(pName, "Cover") == 0)
      return (BSquadAI::cModeCover);
   else if (stricmp(pName, "Ability") == 0)
      return (BSquadAI::cModeAbility);
   else if (stricmp(pName, "CarryingObject") == 0)
      return (BSquadAI::cModeCarryingObject);
   else if (stricmp(pName, "Power") == 0)
      return (BSquadAI::cModePower);
   // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
   else if (stricmp(pName, "ScarabScan") == 0)
      return (BSquadAI::cModeScarabScan);
   else if (stricmp(pName, "ScarabTarget") == 0)
      return (BSquadAI::cModeScarabTarget);
   else if (stricmp(pName, "ScarabKill") == 0)
      return (BSquadAI::cModeScarabKill);
   return (-1);
}

//==============================================================================
// Get the name of the enumerated squad mode
//==============================================================================
const char* BDatabase::getSquadModeName(int type) const
{
   switch (type)
   {
      case BSquadAI::cModeNormal         : return ("Normal");
      case BSquadAI::cModeStandGround    : return ("StandGround");
      case BSquadAI::cModeLockdown       : return ("Lockdown");
      case BSquadAI::cModeSniper         : return ("Sniper");
      case BSquadAI::cModeHitAndRun      : return ("HitAndRun");
      case BSquadAI::cModePassive        : return ("Passive");
      case BSquadAI::cModeCover          : return ("Cover");
      case BSquadAI::cModeAbility        : return ("Ability");
      case BSquadAI::cModeCarryingObject : return ("CarryingObject");
      case BSquadAI::cModePower          : return ("Power");
      // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
      case BSquadAI::cModeScarabScan     : return ("ScarabScan");
      case BSquadAI::cModeScarabTarget   : return ("ScarabTarget");
      case BSquadAI::cModeScarabKill     : return ("ScarabKill");
      default : return ("Unknown");
   }
}

//==============================================================================
// BDatabase::getImpactEffectSize
//==============================================================================
long BDatabase::getImpactEffectSize(const char* pName) const
{
   if(stricmp(pName, "Small")==0)
      return cImpactEffectSizeSmall;
   else if(stricmp(pName, "Medium")==0)
      return cImpactEffectSizeMedium;
   else if(stricmp(pName, "Large")==0)
      return cImpactEffectSizeLarge;
   return -1;
}

//==============================================================================
// BDatabase::getImpactEffectSizeName
//==============================================================================
const char* BDatabase::getImpactEffectSizeName(long type) const
{
   switch(type)
   {
   case cImpactEffectSizeSmall      : return "Small";
   case cImpactEffectSizeMedium     : return "Medium";
   case cImpactEffectSizeLarge      : return "Large";
   default : return "";
   }
}

//==============================================================================
// BDatabase::getCivID
//==============================================================================
long BDatabase::getCivID(const char* pName) const
{
   long count=mCivs.getNumber();
   for(long i=0; i<count; i++)
   {
//-- FIXING PREFIX BUG ID 4125
      const BCiv* pCiv=mCivs[i];
//--
      if(pCiv->getCivName()==pName)
         return i;
   }
   return -1;
}


//==============================================================================
//==============================================================================
uint BDatabase::getNumberBurningEffectLimits() const
{
   return (mBurningEffectLimits.getSize());
}

//==============================================================================
//==============================================================================
int BDatabase::getBurningEffectLimitByProtoObject(const BProtoObject* proto) const
{
   uint numBurningEffectLimits = mBurningEffectLimits.getSize();
   for (uint i=0; i<numBurningEffectLimits; i++)
   {
      if (proto->isType(gDatabase.getObjectType(mBurningEffectLimits[i].mObjectType)))
         return (mBurningEffectLimits[i].mLimit);
   }
   return (mDefaultBurningEffectLimit);   
}


//==============================================================================
// BDatabase::resetGameSettings
//==============================================================================
void BDatabase::resetGameSettings()
{
   *mCurrentGameSettings=*mBaseGameSettings;
   mCurrentGameSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
}

//==============================================================================
// BDatabase::resetPlayer
//==============================================================================
void BDatabase::resetPlayer( long playerID )
{
   mCurrentGameSettings->resetPlayer( playerID );
}

//==============================================================================
// BDatabase::getGameSettingsNetworkType
//==============================================================================
long BDatabase::getGameSettingsNetworkType() const
{
   long networkType = BGameSettings::cNetworkTypeLocal;
   mCurrentGameSettings->getLong( BGameSettings::cNetworkType, networkType );

   return networkType;
}

//==============================================================================
// BDatabase::getProtoPowerByID
//==============================================================================
BProtoPower* BDatabase::getProtoPowerByID(long id)
{
   long numProtoPowers = mProtoPowers.getSize();
   if (id >= 0 && id < numProtoPowers)
      return(mProtoPowers[id]);
   else
      return(NULL);
}

//==============================================================================
// BDatabase::getProtoPowerIDByName
//==============================================================================
long BDatabase::getProtoPowerIDByName(const char*  pProtoPowerName)
{
   long numProtoPowers = mProtoPowers.getSize();
   for (long i=0; i<numProtoPowers; i++)
   {
      if (mProtoPowers[i]->getName() == pProtoPowerName)
         return(i);
   }
   return(-1);
}

//==============================================================================
// BDatabase::getResourceName
//==============================================================================
const char* BDatabase::getResourceName(long resourceID) const
{
   if(resourceID<0 || resourceID>=mResources.getTags().getNumber())
      return "";
   return mResources.getTags().get(resourceID).getPtr();
}

//==============================================================================
// BDatabase::getResourceDeductable
//==============================================================================
bool BDatabase::getResourceDeductable(long resourceID) const
{
   if(resourceID<0 || resourceID>=mResourceDeductable.getNumber())
      return true;
   return mResourceDeductable[resourceID];
}

//==============================================================================
// BDatabase::getRateName
//==============================================================================
const char* BDatabase::getRateName(int rateID)
{
   if(rateID<0 || rateID>=mRates.getTags().getNumber())
      return "";
   return mRates.getTags().get(rateID).getPtr();
}

//==============================================================================
// BDatabase::getActionTypeByName
//==============================================================================
BActionType BDatabase::getActionTypeByName(const char *pName, bool* pMelee) const
{
   // Hacky way to get if it is a melee attack (since ranged/melee currently use the same actiontype)
   if (pMelee)
      *pMelee = false;

   if(stricmp(pName, "RangedAttack")==0)
      return BAction::cActionTypeUnitRangedAttack;
   else if(stricmp(pName, "HandAttack")==0)
   {
      if (pMelee)
         *pMelee = true;
      return BAction::cActionTypeUnitRangedAttack;
   }
   else if(stricmp(pName, "Garrison")==0)
      return BAction::cActionTypeUnitGarrison;
   else if (stricmp(pName, "Ungarrison") == 0)
      return (BAction::cActionTypeUnitUngarrison);
   else if(stricmp(pName, "SpawnSquad")==0)
      return BAction::cActionTypeUnitSpawnSquad;
   else if(stricmp(pName, "AmbientLifeSpawner")==0)
      return BAction::cActionTypeUnitAmbientLifeSpawner;
   else if(stricmp(pName, "Honk")==0)
      return BAction::cActionTypeUnitHonk;
   else if(stricmp(pName, "Capture")==0)
      return BAction::cActionTypeUnitCapture;
   else if(stricmp(pName, "Join")==0)
      return BAction::cActionTypeUnitJoin;
   else if(stricmp(pName, "ChangeOwner")==0)
      return BAction::cActionTypeUnitChangeOwner;
   else if(stricmp(pName, "Physics")==0)
      return BAction::cActionTypeUnitPhysics;
   else if(stricmp(pName, "Cloak")==0)
      return BAction::cActionTypeSquadCloak;
   else if (stricmp(pName, "SpiritBond")==0)
      return BAction::cActionTypeSquadSpiritBond;
   else if(stricmp(pName, "CloakDetector")==0)
   {
      // mrh 7/5/08 - Temporarily removed squad action cloak detect for E3.
      //return BAction::cActionTypeSquadCloakDetect;
      return (BAction::cActionTypeInvalid);
   }
   else if (stricmp(pName, "ReflectDamage")==0)
      return BAction::cActionTypeSquadReflectDamage;
   else if(stricmp(pName, "Gather")==0)
      return BAction::cActionTypeUnitGather;
   else if(stricmp(pName, "Mines")==0)
      return BAction::cActionTypeUnitMines;
   else if(stricmp(pName, "Detonate")==0)
      return BAction::cActionTypeUnitDetonate;
   else if(stricmp(pName, "CollisionAttack")==0)
      return BAction::cActionTypeUnitCollisionAttack;
   else if(stricmp(pName, "AreaAttack")==0)
      return BAction::cActionTypeUnitAreaAttack;
   else if(stricmp(pName, "UnderAttack")==0)
      return BAction::cActionTypeUnitUnderAttack;
   else if(stricmp(pName, "MoveAir")==0)
      return BAction::cActionTypeUnitMoveAir;
   else if(stricmp(pName, "SecondaryTurretAttack")==0)
      return BAction::cActionTypeUnitSecondaryTurretAttack;
   else if(stricmp(pName, "RevealToTeam")==0)
      return BAction::cActionTypeUnitRevealToTeam;
   else if(stricmp(pName, "GaggleMove")==0)
      return BAction::cActionTypeUnitMove;
   else if(stricmp(pName, "AirTrafficControl")==0)
      return BAction::cActionTypeUnitAirTrafficControl;
   else if(stricmp(pName, "AvoidCollisionAir")==0)
      return BAction::cActionTypeUnitAvoidCollisionAir;
   else if(stricmp(pName, "CarpetBomb")==0)
      return BAction::cActionTypeSquadCarpetBomb;
   else if(stricmp(pName, "AirStrike")==0)
      return BAction::cActionTypeSquadAirStrike;
   else if(stricmp(pName, "Hitch")==0)
      return BAction::cActionTypeUnitHitch;
   else if (stricmp(pName, "Unhitch") == 0)
      return (BAction::cActionTypeUnitUnhitch);
   else if (stricmp(pName, "SlaveTurretAttack") == 0)
      return (BAction::cActionTypeUnitSlaveTurretAttack);
   else if (stricmp(pName, "Thrown") == 0)
      return (BAction::cActionTypeUnitThrown);
   else if (stricmp(pName, "Dodge") == 0)
      return (BAction::cActionTypeUnitDodge);
   else if (stricmp(pName, "Deflect") == 0)
      return (BAction::cActionTypeUnitDeflect);
   else if (stricmp(pName, "Move") == 0)
      return (BAction::cActionTypeUnitMove);
   else if (stricmp(pName, "Transport") == 0)
      return (BAction::cActionTypeSquadTransport);
   else if (stricmp(pName, "Heal") == 0)
      return (BAction::cActionTypeUnitHeal);
   else if (stricmp(pName, "Revive") == 0)
      return (BAction::cActionTypeUnitRevive);
   else if (stricmp(pName, "Buff") == 0)
      return (BAction::cActionTypeUnitBuff);
   else if (stricmp(pName, "RepairOther") == 0)
      return (BAction::cActionTypeSquadRepairOther);
   else if (stricmp(pName, "ChangeMode") == 0)
      return (BAction::cActionTypeSquadChangeMode);
   else if (stricmp(pName, "Infect") == 0)
      return (BAction::cActionTypeUnitInfect);
   else if (stricmp(pName, "Wander") == 0)
      return (BAction::cActionTypeSquadWander);
   else if (stricmp(pName, "HotDrop") == 0)
      return (BAction::cActionTypeUnitHotDrop);
   else if (stricmp(pName, "TentacleDormant") == 0)
      return (BAction::cActionTypeUnitTentacleDormant);
   else if (stricmp(pName, "Stasis") == 0)
      return (BAction::cActionTypeUnitStasis);
   else if (stricmp(pName, "Daze") == 0)
      return (BAction::cActionTypeSquadDaze);
   else if (stricmp(pName, "Cryo") == 0)
      return (BAction::cActionTypeSquadCryo);
   else if (stricmp(pName, "BubbleShield") == 0)
      return (BAction::cActionTypeUnitBubbleShield);
   else if (stricmp(pName, "Bomb") == 0)
      return (BAction::cActionTypeUnitBomb);
   else if (stricmp(pName, "PlasmaShieldGen") == 0)
      return (BAction::cActionTypeUnitPlasmaShieldGen);
   else if (stricmp(pName, "Jump") == 0)
      return (BAction::cActionTypeUnitJump);
   else if (stricmp(pName, "JumpGather") == 0)
      return (BAction::cActionTypeUnitJumpGather);
   else if (stricmp(pName, "JumpGarrison") == 0)
      return (BAction::cActionTypeUnitJumpGarrison);
   else if (stricmp(pName, "JumpAttack") == 0)
      return (BAction::cActionTypeUnitJumpAttack);
   else if (stricmp(pName, "AmbientLife") == 0)
      return (BAction::cActionTypeSquadAmbientLife);
   else if (stricmp(pName, "PointBlankAttack") == 0)
      return (BAction::cActionTypeUnitPointBlankAttack);
   else if (stricmp(pName, "Roar") == 0)
      return (BAction::cActionTypeUnitRoar);
   else if (stricmp(pName, "EnergyShield") == 0)
      return (BAction::cActionTypeUnitEnergyShield);
   else if (stricmp(pName, "Charge") == 0)
      return (BAction::cActionTypeUnitChargedRangedAttack);
   else if (stricmp(pName, "TowerWall") == 0)
      return (BAction::cActionTypeUnitTowerWall);
   else if (stricmp(pName, "AoeHeal") == 0)
      return BAction::cActionTypeUnitAoeHeal;
   else if (stricmp(pName, "CoreSlide") == 0)
      return BAction::cActionTypeUnitCoreSlide;
   else if (stricmp(pName, "InfantryEnergyShield") == 0)
      return (BAction::cActionTypeUnitInfantryEnergyShield);
   else if (stricmp(pName, "Dome") == 0)
      return BAction::cActionTypeUnitDome;
   else if (stricmp(pName, "Rage") == 0)
      return BAction::cActionTypeUnitRage;

   BASSERTM(false, "BDatabase::getActionTypeByName() - invalid name.");
   return(BAction::cActionTypeInvalid);
}

//=============================================================================
// BDatabase:setupPlayerColors
//=============================================================================
bool BDatabase::setupPlayerColors()
{
   BXMLReader reader;
   if(!reader.load(cDirData, "playercolors.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;

   const BXMLNode rootNode(reader.getRootNode());

   long num=reader.getRootNode().getNumberChildren();
   for (long i=0; i<num; i++)
   {
      const BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if ((name=="spc") || (name=="skirmish"))
      {
         const BPlayerColorCategory category = (name=="skirmish") ? cPlayerColorCategorySkirmish : cPlayerColorCategorySPC;
         
         for (long j = 0; j < node.getNumberChildren(); j++)
         {
            const BXMLNode childNode(node.getChild(j));
            const BPackedString childName(childNode.getName());
            
            if(childName=="color")
            {
               BSimString val;
               if(!childNode.getAttribValue("num", &val))
                  continue;
               long colorNum=val.asLong();      
               if(colorNum<0 || colorNum>=cMaxPlayerColors)
                  continue;
               mPlayerColors[category][colorNum].load(childNode);
            }
            else if(childName=="friendOrFoeSelf")
               mFOFSelfColor[category].load(childNode);
            else if(childName=="friendOrFoeAlly")
               mFOFAllyColor[category].load(childNode);
            else if(childName=="friendOrFoeNeutral")
               mFOFNeutralColor[category].load(childNode);
            else if(childName=="friendOrFoeEnemy")
               mFOFEnemyColor[category].load(childNode);
         }
      }
      else if(name=="Civ")
      {
         BSimString nodeText;
         int civ=getCivID(node.getTextPtr(nodeText));
         if (civ>=0 && civ<cMaximumSupportedCivs)
         {
            int numChildren=node.getNumberChildren();
            for (int j=0; j<numChildren; j++)
            {
               const BXMLNode childNode(node.getChild(j));
               const BPackedString childName(childNode.getName());
               if (childName=="player")
               {
                  BSimString val;
                  if (!childNode.getAttribValue("num", &val))
                     continue;
                  long playerNum=val.asLong();      
                  if (playerNum<0 || playerNum>=cMaximumSupportedPlayers)
                     continue;
                  if (!childNode.getAttribValue("color", &val))
                     continue;
                  long colorNum=val.asLong();      
                  if (colorNum<0 || colorNum>=cMaxPlayerColors)
                     continue;
                  mCivPlayerColors[civ][playerNum]=colorNum;
               }
            }
         }
      }
   }
   
#ifndef BUILD_FINAL
   mPlayerColorLoadCount++;
#endif   

   gConsoleOutput.resource("BDatabase: Loaded playerColors.xml");

   return true;
}


//============================================================================
//============================================================================
bool BDatabase::setupDBIDs()
{
   BXMLNode rootNode(mpGameDataReader->getRootNode());
   BXMLNode childNode;
   BSimString nodeText;
   BSimString name;

   // Cache off proto object IDs that are referenced by code.
   childNode = rootNode.getChildNode("CodeProtoObjects");
   if (childNode.getValid())
   {
      int childCount = childNode.getNumberChildren();
      for(int j=0; j < childCount; j++)
      {
         BXMLNode childNode2(childNode.getChild(j));
         if (childNode2.getAttribValueAsString("Type", name))
         {
            childNode2.getText(nodeText);
            int protoID = getProtoObject(nodeText);
            if (protoID == -1)
               gConsoleOutput.error("BDatabase::setupDBIDs: Missing %s proto object named %s", name.getPtr(), nodeText.getPtr());
            else
            {
               if (name == "Revealer")
                  mPOIDRevealer = protoID;
               else if (name == "Blocker")
                  mPOIDBlocker = protoID;
               else if (name == "UnitStart")
                  mPOIDUnitStart = protoID;
               else if (name == "RallyStart")
                  mPOIDRallyStart = protoID;
               else if (name == "AIEyeIcon")
                  mPOIDAIEyeIcon = protoID;
               else if (name == "Obstruction")
                  mPOIDObstruction = protoID;
               else if (name == "ObjectiveArrow")
                  mPOIDObjectiveArrow = protoID;
               else if (name == "ObjectiveLocArrow")
                  mPOIDObjectiveLocArrow = protoID;
               //else if (name == "ObjectiveGroundFX")
               //   mPOIDObjectiveGroundFX = protoID;
               else if (name == "PhysicsThrownObject")
                  mPOIDPhysicsThrownObject = protoID;
               else if (name == "SkirmishEmptyBaseObject")
                  mSkirmishEmptyBaseObject = protoID;
               else if (name == "Scn07Scarab")
                  mPOIDScn07Scarab = protoID;
               else if (name == "SkirmishScarab")
                  mPSIDSkirmishScarab = getProtoSquad(nodeText);
               else if (name == "Cobra")
                  mPSIDCobra = getProtoSquad(nodeText);
               else if (name == "Vampire")
                  mPSIDVampire = getProtoSquad(nodeText);
               else if (name == "LeaderPowerCaster")
               {
                  mPOIDLeaderPowerCaster = protoID;
                  mPSIDLeaderPowerCaster = getProtoSquad(nodeText);
               }
               else if (name == "SPCHeroForge")
                  mPOIDForge = protoID;
               else if (name == "SPCHeroForgeWarthog")
                  mPOIDForgeWarthog = protoID;
               else if (name == "SPCHeroAnders")
                  mPOIDAnders = protoID;
               else if (name == "SPCHeroSpartanRocket")
                  mPOIDSpartanRocket = protoID;
               else if (name == "SPCHeroSpartanMG")
                  mPOIDSpartanMG = protoID;
               else if (name == "SPCHeroSpartanSniper")
                  mPOIDSpartanSniper = protoID;
               else if (name == "SmallSnowMound")
                  mSmallSnowMoundPOID = protoID;
               else if (name == "MediumSnowMound")
                  mMediumSnowMoundPOID = protoID;
               else if (name == "LargeSnowMound")
                  mLargeSnowMoundPOID = protoID;
               else if (name == "HotdropPadBeam")
                  mHotdropPadBeamPOID = protoID;
               else if (name == "Stun")
                  mOTIDStun = protoID;
               else if (name == "EMP")
                  mOTIDEmp = protoID;
            }
         }
      }
   }

   // Cache off object types that are referenced by code.
   childNode = rootNode.getChildNode("CodeObjectTypes");
   if (childNode.getValid())
   {
      int childCount = childNode.getNumberChildren();
      for(int j=0; j < childCount; j++)
      {
         BXMLNode childNode2(childNode.getChild(j));
         if (childNode2.getAttribValueAsString("Type", name))
         {
            childNode2.getText(nodeText);
            int objectType = getObjectType(nodeText);
            if (objectType == -1)
               gConsoleOutput.error("BDatabase::setupDBIDs: Missing %s object type named %s", name.getPtr(), nodeText.getPtr());
            else
            {
               if (name == "Building")
                  mOTIDBuilding = objectType;
               else if (name == "BuildingSocket")
                  mOTIDBuildingSocket = objectType;
               else if (name == "TurretSocket")
                  mOTIDTurretSocket = objectType;
               else if (name == "TurretBuilding")
                  mOTIDTurretBuilding = objectType;               
               else if (name == "Settlement")
                  mOTIDSettlement = objectType;
               else if (name == "Base")
                  mOTIDBase = objectType;
               else if (name == "Icon")
                  mOTIDIcon = objectType;
               else if (name == "Gatherable")
                  mOTIDGatherable = objectType;
               else if (name == "Infantry")
                  mOTIDInfantry = objectType;
               else if (name == "Transporter")
                  mOTIDTransporter = objectType;
               else if (name == "Transportable")
                  mOTIDTransportable = objectType;
               else if (name == "Flood")
                  mOTIDFlood = objectType;
               else if (name == "Cover")
                  mOTIDCover = objectType;
               else if (name == "LOSObstructable")
                  mOTIDLOSObstructable = objectType;
               else if (name == "ObjectiveArrow")
                  mOTIDObjectiveArrow = objectType;
               else if (name == "CampaignHero")
                  mOTIDCampaignHero = objectType;
               else if (name == "Hero")
                  mOTIDHero = objectType;
               else if (name == "HeroDeath")
                  mOTIDHeroDeath = objectType;
               else if (name == "HotDropPickup")
                  mOTIDHotDropPickup = objectType;
               else if (name == "TeleportDropoff")
                  mOTIDTeleportDropoff = objectType;
               else if (name == "TeleportPickup")
                  mOTIDTeleportPickup = objectType;
               else if (name == "GroundVehicle")
                  mOTIDGroundVehicle = objectType;
               else if (name == "Garrison")
                  mOTIDGarrison = objectType;
               else if (name == "UnscSupplyPad")
                  mOTIDUnscSupplyPad = objectType;
               else if (name == "CovSupplyPad")
                  mOTIDCovSupplyPad = objectType;
               else if (name == "WallShield")
                  mOTIDWallShield = objectType;
               else if (name == "BaseShield")
                  mOTIDBaseShield = objectType;
               else if (name == "Leader")
                  mOTIDLeader = objectType;
               else if (name == "Covenant")
                  mOTIDCovenant = objectType;
               else if (name == "Unsc")
                  mOTIDUnsc = objectType;
               else if (name == "BlackBox")
                  mOTIDBlackBox = objectType;
               else if (name == "Skull")
                  mOTIDSkull = objectType;
               else if (name == "BirthOnTop")
                  mOTIDBirthOnTop = objectType;
               else if (name == "CanCryo")
                  mOTIDCanCryo = objectType;
               else if (name == "Hook")
                  mOTIDHook = objectType;
            }
         }
      }
   }

   // Cache off some proto power ids
   mPPIDRepair = getProtoPowerIDByName("_Repair");
   mPPIDRallyPoint = getProtoPowerIDByName("_RallyPoint");
   mPPIDHookRepair = getProtoPowerIDByName("HookRepair");
   mPPIDUnscOdstDrop = getProtoPowerIDByName("UnscOdstDrop");

   //Infection Map.
   childNode=rootNode.getChildNode("InfectionMap");
   if (childNode.getValid())
   {
      long childCount=childNode.getNumberChildren();
      for(long j=0; j < childCount; j++)
      {
         BXMLNode childNode2(childNode.getChild(j));
         BSimString baseName;
         BSimString infectedName;
         BSimString infectedSquadName;
         if (childNode2.getAttribValueAsString("base", baseName) && childNode2.getAttribValueAsString("infected", infectedName) && childNode2.getAttribValueAsString("infectedSquad", infectedSquadName))
         {
            BProtoObjectID basePOID=getProtoObject(baseName);
            BProtoObjectID infectedPOID=getProtoObject(infectedName);
            BProtoSquadID infectedPSID=getProtoSquad(infectedSquadName);
            if ((basePOID != cInvalidProtoObjectID) && (infectedPOID != cInvalidProtoObjectID) && (infectedPSID != cInvalidProtoSquadID))
            {
               uint newIndex=mInfectionMap.getSize();
               mInfectionMap.setNumber(newIndex+1);
               mInfectionMap[newIndex].setPOID1(basePOID);
               mInfectionMap[newIndex].setPOID2(infectedPOID);
               mInfectionMap[newIndex].setPSID2(infectedPSID);
            }
         }
      }
   }

   // Made it.
   return (true);
}

//=============================================================================
// BDatabase::getPlayerColor
//=============================================================================
const BPlayerColor& BDatabase::getPlayerColor(BPlayerColorCategory category, long index)
{
   BDEBUG_ASSERT(category < cMaxPlayerColorCategories); 
   BDEBUG_ASSERT((index >= 0) && (index < cMaxPlayerColors));
      
   return mPlayerColors[category][index];
}

//=============================================================================
// BDatabase::getCivPlayerColor
//=============================================================================
int BDatabase::getCivPlayerColor(int civ, int player)
{
   if (civ<0 || civ>=cMaximumSupportedCivs)
      return(-1);
   return(mCivPlayerColors[civ][player]);
}

//=============================================================================
// BDatabase::getPlacementRules
//=============================================================================
long BDatabase::getPlacementRules(const char* pName)
{
   if(!pName || !pName[0])
      return -1;
   long count=mPlacementRules.getNumber();
   for(long i=0; i<count; i++)
   {
//-- FIXING PREFIX BUG ID 4114
      const BPlacementRules* pRules=mPlacementRules[i];
//--
      if(pRules->getFilename()==pName)
         return i;
   }
   return -1;
}

//=============================================================================
// BDatabase::getPlacementRules
//=============================================================================
BPlacementRules* BDatabase::getPlacementRules(long index)
{
   if(index<0 || index>=mPlacementRules.getNumber())
      return NULL;
   else
      return mPlacementRules[index];
}


//=============================================================================
// BDatabase:setupWeaponTypes
//=============================================================================
bool BDatabase::setupWeaponTypes()
{
   BXMLReader reader;
   if(!reader.load(cDirData, "weapontypes.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;

   const BXMLNode rootNode(reader.getRootNode());

   long num=reader.getRootNode().getNumberChildren();
   for (long i=0; i<num; i++)
   {
      const BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if(name!="WeaponType")
         continue;

      BWeaponType *pWeaponType = new BWeaponType();
      bool result = pWeaponType->loadWeaponType(node);
      if(result)
      {
         mWeaponTypes.add(pWeaponType);
      }
      else
      {
         delete pWeaponType;
      }
   }  

   return true;
}

//=============================================================================
// BDatabase:getWeaponTypeIDByName
//=============================================================================
long BDatabase::getWeaponTypeIDByName(const char*  pWeaponTypeName)
{
   long numWeaponTypes = mWeaponTypes.getSize();
   for (long i=0; i < numWeaponTypes; i++)
   {
      if (mWeaponTypes[i]->getName() == pWeaponTypeName)
         return(i);
   }
   return(-1);   
}

//=============================================================================
// BDatabase:getWeaponTypeByID
//=============================================================================
BWeaponType* BDatabase::getWeaponTypeByID(long id) const
{
   if(id < 0 || id >= mWeaponTypes.getNumber())
      return NULL;
   return mWeaponTypes[id];
}

//==============================================================================
// BDatabase::getDamageModifier
// Lookup the correct modifier amount for a given target type
//==============================================================================
float BDatabase::getDamageModifier(long weaponTypeID, BEntityID targetID, BVector damageDirection, float &shieldedDamageModifier)
{
   BWeaponType *pWeaponType = getWeaponTypeByID(weaponTypeID);
   if(pWeaponType)
   {
      shieldedDamageModifier = pWeaponType->getShieldedModifier();
      return pWeaponType->getDamagePercentage(targetID, damageDirection);
   }
   else
   {
      shieldedDamageModifier = 1.0f;
      return 1.0f;
   }
}

//=============================================================================
// BDatabase:getVisualLogicValue
//=============================================================================
bool BDatabase::getVisualLogicValue(long logicType, const char* pName, DWORD& valDword, float& valFloat) const
{
   switch(logicType)
   {
      case cVisualLogicVariation:
         valDword=0;
         valFloat=0.0F;
         return true;

      case cVisualLogicTech:
         valDword=(DWORD)getProtoTech(pName);
         valFloat=0.0F;
         return true;

      case cVisualLogicBuildingCompletion:
         valDword=0;
         if(pName[0]!=NULL)
            valFloat=(float)(atof(pName+1)*0.01f);
         else
            valFloat=0.0f;
         return true;

      case cVisualLogicSquadMode:
         valDword=(DWORD)getSquadMode(pName);
         valFloat=0.0F;
         break;

      case cVisualLogicImpactSize:
         valDword=(DWORD)getImpactEffectSize(pName);
         valFloat=0.0F;
         return true;

      case cVisualLogicDestruction:
         valDword=0;
         if(pName[0]!=NULL)
            valFloat=(float)(atof(pName+1)*0.01f);
         else
            valFloat=0.0f;
         return true;
   }
   return false;
}

//=============================================================================
// BDatabase::handleProtoVisualLoaded
//=============================================================================
void BDatabase::handleProtoVisualLoaded(BProtoVisual* pProtoVisual)
{
   // If this proto visual has tech logic, link units to the techs in the logic
   if (pProtoVisual->getFlag(BProtoVisual::cFlagHasTechLogic))
   {
      if (mPostProcessProtoVisualLoads)
      {
         mLoadedProtoVisuals.add(pProtoVisual);
         return;
      }

      BSmallDynamicSimArray<long> techIDs;
      pProtoVisual->getTechLogicIDs(techIDs);
      if (techIDs.getSize()>0)
      {
         for (int i=0; i<mProtoObjects.getNumber(); i++)
         {
//-- FIXING PREFIX BUG ID 4131
            const BProtoObject* pProtoObject=mProtoObjects[i];
//--
            if (gVisualManager.getProtoVisual(pProtoObject->getProtoVisualIndex(), false) == pProtoVisual)
            {
               for (uint j=0; j<techIDs.getSize(); j++)
               {
                  BProtoTech* pProtoTech=getProtoTech(techIDs[j]);
                  pProtoTech->addVisualLogicProtoUnit(i);
               }
            }
         }
      }
   }
}

//=============================================================================
// BDatabase::getPlayerScope
//=============================================================================
int BDatabase::getPlayerScope(const char* pName) const
{
   if (stricmp(pName, "any")==0)
      return cPlayerScopeAny;
   else if (stricmp(pName, "player")==0)
      return cPlayerScopePlayer;
   else if (stricmp(pName, "team")==0)
      return cPlayerScopeTeam;
   else if (stricmp(pName, "enemy")==0)
      return cPlayerScopeEnemy;
   else if (stricmp(pName, "gaia")==0)
      return cPlayerScopeGaia;
   else
      return -1;
}

//=============================================================================
// BDatabase::getLifeScope
//=============================================================================
int BDatabase::getLifeScope(const char* pName) const
{
   if (stricmp(pName, "any")==0)
      return cLifeScopeAny;
   else if (stricmp(pName, "alive")==0)
      return cLifeScopeAlive;
   else if (stricmp(pName, "dead")==0)
      return cLifeScopeDead;
   else
      return -1;
}

//=============================================================================
// BDatabase::checkPlayerScope
//=============================================================================
bool BDatabase::checkPlayerScope(const BUnit* pUnit, BPlayerID playerID, int playerScope) const
{
   switch (playerScope) 
   {
      case cPlayerScopeAny:
         return true;
      case cPlayerScopePlayer:
         // only look at units for this player
         return (pUnit->getPlayerID() == playerID);
      case cPlayerScopeTeam:
         // only look at buildings for this team
         return (pUnit->getPlayer()->isAlly(playerID));
      case cPlayerScopeEnemy:
         return (pUnit->getPlayer()->isEnemy(playerID) && !pUnit->getProtoObject()->getFlagNeutral());  // only look at buildings of the enemy
      case cPlayerScopeGaia:
         // only look at units owned by gaia
         return (pUnit->getPlayerID()==0);
      default:
         return true;
   }
}

//=============================================================================
// BDatabase::checkLifeScope
//=============================================================================
bool BDatabase::checkLifeScope(const BUnit* pUnit, int lifeScope) const
{
   switch (lifeScope) 
   {
      case cLifeScopeAny:
         return true;
      case cLifeScopeAlive:
         return(pUnit->isAlive());
      case cLifeScopeDead:
         return(!pUnit->isAlive());
      default:
         return true;
   }
}

#if !defined(BUILD_FINAL) && defined(ENABLE_RELOAD_MANAGER)
//=============================================================================
// BDatabase:setupDataReloading
//=============================================================================
void BDatabase::setupDataReloading()
{
   mpFileWatcher = new BFileWatcher();
   if (mpFileWatcher)
   {
      mPathStringTable = mpFileWatcher->add(cDirData, "stringtable.xml", 0);
      mPathTrigerScripts = mpFileWatcher->add(cDirTriggerScripts, "*.triggerscript", 0);
      mPathRumble = mpFileWatcher->add(cDirData, "rumble.xml", 0);
      mPlayerColorPathHandle = mpFileWatcher->add(cDirData, "playercolors.xml");
      mPowersHandle = mpFileWatcher->add(cDirData, "powers.xml");
   }
}
#endif

//=============================================================================
// BDatabase::getRumbleEvent
//=============================================================================
const BRumbleEvent* BDatabase::getRumbleEvent(int eventType) const
{
   if (eventType >= 0 && eventType < mRumbleEvents.getNumber() && mRumbleEvents[eventType].mEnabled)
      return &(mRumbleEvents[eventType]);
   else
      return NULL;
}

//=============================================================================
// BDatabase::isRumbleEventEnabled
//=============================================================================
bool BDatabase::isRumbleEventEnabled(int eventType) const
{
   if (eventType >= 0 && eventType < mRumbleEvents.getNumber() && mRumbleEvents[eventType].mEnabled)
      return true;
   else
      return false;
}

//=============================================================================
//=============================================================================
int BDatabase::getRumbleEventID(int eventType)
{
   if (eventType < 0 || eventType >= BRumbleEvent::cNumEvents)
      return -1;
   return mRumbleEventIDs[eventType];
}

//=============================================================================
//=============================================================================
void BDatabase::setRumbleEventID(int eventType, int rumbleID)
{
   if (eventType < 0 || eventType >= BRumbleEvent::cNumEvents)
      return;
   mRumbleEventIDs[eventType] = rumbleID;
}

#ifndef BUILD_FINAL
//=============================================================================
// BDatabase::getSoundCueIndex
//=============================================================================
BCueIndex BDatabase::getSoundCueIndex(const char* pName)
{
   BCueIndex index;
   bool returnVal = mSoundCueTable.find(pName, &index);

   if(returnVal)
      return index;
   else
      return cInvalidCueIndex;
}
#endif


//=============================================================================
// BDatabase:setupGameModes
//=============================================================================
bool BDatabase::setupGameModes()
{
   BXMLReader reader;
   if(!reader.load(cDirData, "gamemodes.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;

   const BXMLNode rootNode(reader.getRootNode());

   long num=reader.getRootNode().getNumberChildren();
   for (long i=0; i<num; i++)
   {
      const BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if(name!="GameMode")
         continue;

      BGameMode* pGameMode = new BGameMode();
      bool result = pGameMode->load(node);
      if(result)
         mGameModes.add(pGameMode);
      else
         delete pGameMode;
   }  

   return true;
}

//=============================================================================
// BDatabase:getGameModeIDByName
//=============================================================================
long BDatabase::getGameModeIDByName(const char*  pGameModeName)
{
   int numGameModes = mGameModes.getSize();
   for (int i=0; i < numGameModes; i++)
   {
      if (mGameModes[i]->getName() == pGameModeName)
         return(i);
   }
   return(-1);   
}

//=============================================================================
// BDatabase:getGameModeByID
//=============================================================================
BGameMode* BDatabase::getGameModeByID(int id) const
{
   if(id < 0 || id >= mGameModes.getNumber())
      return NULL;
   return mGameModes[id];
}

//=============================================================================
// BDatabase:getWorldID
//=============================================================================
uint8 BDatabase::getWorldID(BSimString worldName) const
{
   worldName.toLower();   
   
   if(worldName == "harvest")
      return cWorldHarvest;
   if(worldName == "arcadia")
      return cWorldArcadia;
   if(worldName == "swi")
      return cWorldSWI;
   if(worldName == "swe")
      return cWorldSWE;

   return cWorldNone;
}


//=============================================================================
//=============================================================================
float BDatabase::calculateUnscSupplyPadModifier(float numSupplyPads) const
{
   // S = numSupplyPads
   // P = mUnscSupplyPadBreakEvenPoint (point at which the modifier goes from positive to negative.)
   // B = mUnscSupplyPadBonus (1.0 == 100%)
   //
   //       B + 1
   // ------------------
   //        SB
   //      ------ + 1
   //        P

   float s = numSupplyPads;
   float b = mUnscSupplyPadBonus;
   float p = mUnscSupplyPadBreakEvenPoint;

   BASSERT(b > 0.0f);
   BASSERT(p > 0.0f);

   float num = (b + 1.0f);

   float denom;
   if (p > 0.0f)
      denom = ((s * b) / p) + 1.0f;
   else
      denom = 1.0f;

   return (num / denom);
}


//=============================================================================
//=============================================================================
float BDatabase::calculateCovSupplyPadModifier(float numSupplyPads) const
{
   // S = numSupplyPads
   // P = mCovSupplyPadBreakEvenPoint (point at which the modifier goes from positive to negative.)
   // B = mCovSupplyPadBonus (1.0 == 100%)
   //
   //       B + 1
   // ------------------
   //        SB
   //      ------ + 1
   //        P

   float s = numSupplyPads;
   float b = mCovSupplyPadBonus;
   float p = mCovSupplyPadBreakEvenPoint;

   BASSERT(b > 0.0f);
   BASSERT(p > 0.0f);

   float num = (b + 1.0f);

   float denom;
   if (p > 0.0f)
      denom = ((s * b) / p) + 1.0f;
   else
      denom = 1.0f;

   return (num / denom);
}

//=============================================================================
//=============================================================================
const BUString& BDatabase::getDifficultyStringByType(int difficultyType)
{
   switch (difficultyType)
   {
      case DifficultyType::cEasy:
         return getLocStringFromID(24573);
      case DifficultyType::cNormal:
         return getLocStringFromID(24574);
      case DifficultyType::cHard:
         return getLocStringFromID(24575);
      case DifficultyType::cLegendary:
         return getLocStringFromID(24576);
      case DifficultyType::cAutomatic:
         return getLocStringFromID(24577);
      case DifficultyType::cCustom:
         return getLocStringFromID(24819);
   }

   return getLocStringFromID(0);    // empty string
}


//=============================================================================
//=============================================================================
float BDatabase::getDifficulty(int type)
{
   switch (type)
   {
      case DifficultyType::cEasy:
         return mDifficultyEasy;
      case DifficultyType::cNormal:
         return mDifficultyNormal;
      case DifficultyType::cHard:
         return mDifficultyHard;
      case DifficultyType::cLegendary:
         return mDifficultyLegendary;
   }

   return mDifficultyDefault;
}

//=============================================================================
//=============================================================================
void BDatabase::setupPreloadXmlFiles()
{
   // Pre-load all XML files that are needed by the game during gameplay such as external triggers scripts that can be launched from 
   // another trigger script and other misc XML files.
   BXMLReader reader;
   if (reader.load(cDirData, "preloadxmlfiles.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
   {
      BXMLNode rootNode(reader.getRootNode());
      long nodeCount=rootNode.getNumberChildren();
      for(long i=0; i<nodeCount; i++)
      {
         BXMLNode node(rootNode.getChild(i));
         const BPackedString name(node.getName());
         if (name=="File")
         {
            BSimString fileName;
            node.getText(fileName);

            BSimString dirName;
            node.getAttribValueAsString("Dir", dirName);
            int dirID = 0;
            if (dirName == "data")
               dirID = cDirData; 
            else if (dirName == "art")
               dirID = cDirArt; 
            else if (dirName == "talkingheads")
               dirID = cDirTalkingHead; 
            else if (dirName == "Scenario")
               dirID = cDirScenario; 
            else if (dirName == "sound")
               dirID = cDirSound; 
            else if (dirName == "Fonts")
               dirID = cDirFonts; 
            else if (dirName == "terrain")
               dirID = cDirTerrain; 
            else if (dirName == "physics")
               dirID = cDirPhysics; 
            else if (dirName == "shaders")
               dirID = cDirRenderEffects; 
            else if (dirName == "tactics")
               dirID = cDirTactics; 
            else if (dirName == "placementrules")
               dirID = cDirPlacementRules; 
            else if (dirName == "triggerscripts")
               dirID = cDirTriggerScripts; 
            else if (dirName == "recordgame")
               dirID = cDirRecordGame; 
            else if (dirName == "savegame")
               dirID = cDirSaveGame; 
            else if (dirName == "campaign")
               dirID = cDirCampaign; 
            else if (dirName == "aidata")
               dirID = cDirAIData; 

            BXMLReader* pReader = new BXMLReader();
            if (!pReader)
               break;
            if (!pReader->load(dirID, fileName))
            {
               delete pReader;
#ifndef BUILD_FINAL
               BSimString errorMsg;
               errorMsg.format("Unable to load the file %s!", fileName.getPtr());
               trace(errorMsg);
#endif
               continue;
            }

            uint index = mPreloadXmlFiles.add(pReader);
            mPreloadXmlFileTable.add(fileName, index);
         }
      }
   }
}

//=============================================================================
//=============================================================================
const BXMLReader* BDatabase::getPreloadXmlFile(long dirID, const char* pFilename) const
{
   uint index = UINT_MAX;
   if (mPreloadXmlFileTable.find(pFilename, &index) && index != UINT_MAX)
      return mPreloadXmlFiles[index];
   else
      return NULL;
}
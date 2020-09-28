//==============================================================================
// triggercondition.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xsystem
#include "xmlreader.h"

// xgame
#include "triggervar.h"

// forward declarations
class BTrigger;
class BTriggerScript;
class BTriggerVar;

typedef long BTriggerConditionID;
__declspec(selectany) extern const BTriggerConditionID cInvalidTriggerConditionID = -1;

typedef BUInt8<BTriggerVarSigID, BTriggerVar::cVarSigIDInvalid, BTriggerVar::cVarSigIDMax> BTriggerVarSigIDSmall;


//==============================================================================
// class BTriggerCondition
//==============================================================================
class BTriggerCondition : public IPoolable
{
public:

   //===========================================================================
   // Trigger Condition Type Enumerations
   // Each trigger condition type gets its own enumeration that never changes.
   // The name of the condition can change but it is tracked by this enum.
   // Even if you obsolete a condition, keep the enumeration around, and DO NOT
   // reorder them.
   //===========================================================================
   enum 
   { 
      // OBSOLETE                            = 1,
      cTCCanGetUnits                         = 2,
      // OBSOLETE                            = 3,
      cTCCanGetSquads                        = 4,
      cTCTechStatus                          = 5,
      // OBSOLETE                            = 6,
      cTCTriggerActiveTime                   = 7,
      cTCUnitUnitDistance                    = 8,
      cTCUnitLocationDistance                = 9,
      // OBSOLETE                            = 10,
      // OBSOLETE                            = 11,
      // OBSOLETE                            = 12,
      // OBSOLETE                            = 13,
      cTCCompareCount                        = 14,
      cTCCanPayCost                          = 15,
      cTCUILocationOK                        = 16,
      cTCUILocationCancel                    = 17,
      // OBSOLETE                            = 18,
      // OBSOLETE                            = 19,
      cTCPlayerSelectingUnit                 = 20,
      cTCPlayerLookingAtUnit                 = 21,
      cTCPlayerLookingAtLocation             = 22,
      // OBSOLETE                            = 23,
      // OBSOLETE                            = 24,
      // OBSOLETE                            = 25,
      // OBSOLETE                            = 26,
      cTCCanUsePower                         = 27,
      cTCIsUnitPower                         = 28,
      cTCIsOwnedBy                           = 29,
      cTCIsProtoObject                       = 30,
      cTCIsAlive                             = 80,
      cTCIsDead                              = 81,
      cTCCheckPlacement                      = 82,
      // OBSOLETE                            = 109,
      cTCCompareBool                         = 114,
      // OBSOLETE                            = 115,
      cTCNextPlayer                          = 118,
      cTCNextTeam                            = 119,
      cTCUnitListLocationDistance            = 128,
      cTCPlayerInState                       = 131,
      cTCContainsGarrisoned                  = 136,
      cTCNextUnit                            = 140,
      cTCNextSquad                           = 141,
      cTCUIUnitOK                            = 155,
      cTCUIUnitCancel                        = 156,
      cTCUISquadOK                           = 157,
      cTCUISquadCancel                       = 158,
      cTCCompareTime                         = 170,
      cTCCompareString                       = 171,
      cTCComparePercent                      = 175,
      cTCCompareHitpoints                    = 182,
      cTCIsMultiplayerActive                 = 191,
      cTCCanRetrieveExternals                = 192,
      cTCSquadLocationDistance               = 238,
      cTCCompareCiv                          = 240,
      // OBSOLETE                            = 242,
      cTCCompareAmmoPercent                  = 256,
      cTCIsHitZoneActive                     = 260,
      cTCIsIdle                              = 264,
      cTCGameTime                            = 266,
      cTCGameTimeReached                     = 268,
      cTCTriggerActiveTimeReached            = 269,
      cTCPlayerSelectingSquad                = 280,
      cTCNextObject                          = 282,
      cTCNextLocation                        = 292,
      cTCRandomListLocation                  = 293,
      cTCRefCountUnit                        = 314,
      cTCRefCountSquad                       = 315,
      cTCUnitFlag                            = 320,
      // OBSOLETE                            = 322,
      // OBSOLETE                            = 323,
      // OBSOLETE                            = 326,
      cTCUILocationUILockError               = 327,
      cTCUIUnitUILockError                   = 328,
      cTCUISquadUILockError                  = 329,
      cTCUILocationWaiting                   = 331,
      cTCUIUnitWaiting                       = 332,
      cTCUISquadWaiting                      = 333,
      cTCCompareCost                         = 339,
      cTCCheckResourceTotals                 = 340,
      cTCComparePopulation                   = 352,
      cTCCanGetOneUnit                       = 356,
      cTCCanGetOneSquad                      = 357,
      cTCCheckDiplomacy                      = 362,
      // OBSOLETE                            = 366,
      // OBSOLETE                            = 367,
      // OBSOLETE                            = 368,
      // OBSOLETE                            = 380,
      cTCCheckModeChange                     = 386,
      cTCIsBuilt                             = 414,
      cTCIsMoving                            = 416,
      // OBSOLETE                            = 421,
      cTCPlayerIsHuman                       = 426,
      cTCPlayerIsGaia                        = 427,
      cTCComparePlayers                      = 428,
      cTCCompareTeams                        = 429,
      cTCCanGetOnePlayer                     = 434,
      cTCCanGetOneTeam                       = 435,
      cTCComparePlayerUnitCount              = 436,
      cTCIsSquadAtMaxSize                    = 438,
      cTCCanRetrieveExternalLocation         = 440,
      cTCCanRetrieveExternalLocationList     = 441,
      cTCNextKBBase                          = 444,
      cTCCompareFloat                        = 453,
      cTCIsUnderAttack                       = 461,
      // OBSOLETE                            = 462,
      cTCCompareLeader                       = 476,
      cTCPlayerUsingLeader                   = 477,
      cTCCanGetOneProtoObject                = 485,
      cTCCanGetOneProtoSquad                 = 486,
      cTCCanGetOneObjectType                 = 487,
      cTCCanGetOneTech                       = 488,
      cTCNextProtoObject                     = 494,
      cTCNextProtoSquad                      = 495,
      cTCNextObjectType                      = 496,
      cTCNextTech                            = 497,
      cTCHasAttached                         = 508,
      cTCIsMobile                            = 509,
      cTCCompareProtoSquad                   = 513,
      cTCHasCinematicTagFired                = 518,
      cTCCanGetBuilder                       = 524,
      cTCUIButtonPressed                     = 527,
      cTCUIButtonWaiting                     = 528,
      // OBSOLETE                            = 540,
      cTCCompareAIMissionType                = 546,
      cTCCompareAIMissionState               = 547,
      cTCCompareAIMissionTargetType          = 548,
      cTCCanGetOneInteger                    = 552,
      cTCCanGetUnitsAlongRay                 = 558,
      cTCCanRemoveOneInteger                 = 563,
      cTCCanGetOneLocation                   = 565,
      cTCCanRemoveOneLocation                = 566,
      cTCCompareProtoObject                  = 575,
      cTCCompareTech                         = 576,
      cTCProtoObjectListContains             = 577,
      cTCProtoSquadListContains              = 578,
      cTCTechListContains                    = 579,
      cTCCanRemoveOneProtoObject             = 583,
      cTCCanRemoveOneProtoSquad              = 584,
      cTCCanRemoveOneTech                    = 585,
      cTCBidState                            = 586,
      cTCBuildingCommandDone                 = 596,
      cTCAITopicIsActive                     = 600,
      cTCHasGarrisoned                       = 602,
      cTCIsGarrisoned                        = 603,
      cTCIsAttached                          = 604,
      cTCComparePlayerSquadCount             = 606,
      cTCIsPassable                          = 611,
      cTCCanGetCentroid                      = 621,
      cTCPlayerIsPrimaryUser                 = 624,
      cTCIsCoop                              = 627,
      cTCIsConfigDefined                     = 629,
      cTCCustomCommandCheck                  = 635,
      cTCUISquadListOK                       = 640,
      cTCUISquadListCancel                   = 641,
      cTCUISquadListUILockError              = 642,
      cTCUISquadListWaiting                  = 643,
      cTCIsObjectType                        = 649,
      cTCIsTimerDone                         = 661,
      cTCNextKBSquad                         = 662,
      cTCIsAttacking                         = 709,
      cTCCanGetUnitLaunchLocation            = 714,
      cTCIsGathering                         = 722,
      cTCIsCapturing                         = 723,
      cTCPlayerIsComputerAI                  = 740,
      cTCCanGetGreatestThreat                = 743,
      cTCCanGetTargetedSquad                 = 744,
      cTCIsObjectiveComplete                 = 746,
      cTCCompareVector                       = 752,
      cTCGetTableRow                         = 762,
      cTCCheckPop                            = 763,
      // OBSOLETE                            = 782,
      // OBSOLETE                            = 785,
      // OBSOLETE                            = 788,
      cTCIsUserModeNormal                    = 789,
      cTCIsHitched                           = 800,
      cTCHasHitched                          = 801,
      cTCSquadSquadDistance                  = 807,
      cTCEventTriggered                      = 813,
      cTCIsInQueue                           = 814,
      cTCCanGetObjects                       = 819,
      cTCCompareLocStringID                  = 825,
      cTCCanGetOneObject                     = 835,
      cTCCanGetCorpseUnits                   = 844,
      // OBSOLETE                            = 846,
      cTCCanGetOneTime                       = 860,
      // OBSOLETE                            = 861,
      cTCCanGetOneDesignLine                 = 865,
      cTCCompareDesignLine                   = 866,
      cTCCanRetrieveExternalFlag             = 877,
      cTCAICanGetTopicFocus                  = 883,
      cTCCanGetOneFloat                      = 885,
      cTCUILocationMinigameWaiting           = 893,
      cTCUILocationMinigameOK                = 894,
      cTCUILocationMinigameCancel            = 895,
      cTCUILocationMinigameUILockError       = 896,
      cTCIsSelectable                        = 899,
      cTCChatCompleted                       = 905,
      cTCCinematicCompleted                  = 906,
      cTCSquadFlag                           = 911,
      cTCCanGetSocketUnits                   = 908,
      cTCCanGetOneSocketUnit                 = 909,
      cTCFadeCompleted                       = 913,
      cTCIsAutoAttackable                    = 916,
      //OBSOLETE                             = 917,
      cTCIsBeingGatheredFrom                 = 918,
      cTCAICanGetDifficultySetting           = 920,
      cTCConceptGetParent                    = 929,
      cTCConceptGetCommand                   = 930,
      cTCConceptGetStateChange               = 931,
      cTCConceptCompareState                 = 932,
      cTCIsEmptySocketUnit                   = 942,
      cTCIsForbidden                         = 944,
      cTCCanGetSocketParentBuilding          = 954,
      cTCCanGetRandomLocation                = 956,
      cTCCheckDifficulty                     = 972,
      cTCCanGetDesignSpheres                 = 974,
      cTCCanRemoveOneFloat                   = 975,
      cTCCanRetrieveExternalFloat            = 977,
      cTCMarkerSquadsInArea                  = 980,
      cTCCanGetSocketPlugUnit                = 986,
      cTCCanGetHoverPoint                    = 990,
      cTCCanGetCoopPlayer                    = 991,
      cTCCompareUnit                         = 1028,
      cTCASYNCUnitsOnScreenSelected          = 1053,
      cTCCheckAndSetFalse                    = 1056,
      cTCAITopicGetTickets                   = 1059,

      // NEWTRIGGERCONDITION
      // Add your enumeration for your new trigger condition here.
      // Follow the naming convention as shown.
      // The enumeration value is set by the triggerDestription tool, do not make one up.
      // Do not change the enumeration, evar.
      // Any questions?  Ask Marc.
   };

   enum
   {
      cTCVersionInvalid = 0,
   };

   bool evaluate();

   virtual void onAcquire();
   virtual void onRelease();
   DECLARE_FREELIST(BTriggerCondition, 6);

   BTriggerCondition(){}
   ~BTriggerCondition(){}

   long getID() const { return (mID); }
   void setParentTriggerScript(BTriggerScript *pParentScript) { mpParentTriggerScript = pParentScript; }
   BTriggerScript* getParentTriggerScript() { return (mpParentTriggerScript); }
   void setParentTrigger(BTrigger *pParentTrigger) { mpParentTrigger = pParentTrigger; }

   bool loadFromXML(BXMLNode  node, BTrigger *pParentTrigger);
   uint getVersion() const { return (mVersion); }
   bool getAsyncCondition() const { return (mbAsyncCondition); }
   void setConsensusState(bool v) { mbConsensusState = v; }

   void initAsyncSettings();
   void updateAsyncCondition();

   bool getLocalAsyncMachine() const { return mbLocalAsyncMachine; }
   bool getLocalMachineState() const { return mbLocalMachineState; }
   bool getLastBroadcastState() const { return mbLastBroadcastState; }
   bool getConsensusState() const { return mbConsensusState; }

   static bool compareValues(int val1, BCompareType op, int val2);
   static bool compareValues(float val1, BCompareType op, float val2);
   static bool compareValues(DWORD val1, BCompareType op, DWORD val2);
   static bool compareValues(bool val1, BCompareType op, bool val2);
   static bool compareValues(BSimUString val1, BCompareType op, BSimUString val2);
   static bool compareValues(uint val1, BCompareType op, uint val2);
   static bool compareRelationType(BTeamID teamID1, BRelationType relationType, BTeamID teamID2);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BTriggerVar* getVar(BTriggerVarSigID sigID) { BASSERT((sigID-1) < mConditionVars.getSize()); return (mConditionVars[sigID-1]); }
   void broadcastLocalMachineState();

   // Trigger condition evaluation functions.
   bool tcCanGetUnits();
   bool tcCanGetUnitsV5();
   bool tcCanGetUnitsV6();
   bool tcCanGetSquads();
   bool tcCanGetSquadsV8();
   bool tcCanGetSquadsV9();
   bool tcTechStatus();
   bool tcTechStatusV1();
   bool tcTechStatusV2();
   bool tcTriggerActiveTime();
   bool tcUnitUnitDistance();
   bool tcUnitUnitDistanceV2();
   bool tcUnitLocationDistance();
   bool tcUnitLocationDistanceV2();
   bool tcUILocationOK();
   bool tcUILocationCancel();
   bool tcCompareCount();
   bool tcCanPayCost();
   bool tcCheckPlacement();
   bool tcCheckPlacementV5();
   bool tcCheckPlacementV6();
   bool tcCanUsePower();
   bool tcIsUnitPower();
   bool tcIsOwnedBy();
   bool tcIsOwnedByV3();
   bool tcIsOwnedByV4();
   bool tcIsProtoObject();
   bool tcIsProtoObjectV2();
   bool tcIsAlive();
   bool tcIsAliveV3();
   bool tcIsDead();
   bool tcIsDeadV3();
   bool tcCompareBool();
   bool tcNextPlayer();
   bool tcNextTeam();
   bool tcUnitListLocationDistance();
   bool tcUnitListLocationDistanceV2();
   bool tcPlayerInState();
   bool tcPlayerInStateV2();
   bool tcContainsGarrisoned();
   bool tcContainsGarrisonedV3();
   bool tcNextUnit();
   bool tcNextSquad();
   bool tcUIUnitOK();
   bool tcUIUnitCancel();
   bool tcUISquadOK();
   bool tcUISquadCancel();
   bool tcCompareTime();
   bool tcCompareString();
   bool tcComparePercent();
   bool tcCompareHitpoints();
   bool tcIsMultiplayerActive();
   bool tcCanRetrieveExternals();  
   bool tcCanRetrieveExternalsV2();
   bool tcCanRetrieveExternalsV3();
   bool tcSquadLocationDistance();
   bool tcCompareCiv();
   bool tcCompareAmmoPercent();
   bool tcCompareAmmoPercentV2();
   bool tcIsHitZoneActive();
   bool tcIsIdle();
   bool tcGameTime();
   bool tcGameTimeReached();
   bool tcTriggerActiveTimeReached();
   bool tcPlayerSelectingSquad();
   bool tcNextObject();
   bool tcNextLocation();
   bool tcRandomListLocation();
   bool tcRefCountUnit();
   bool tcRefCountSquad();
   bool tcUnitFlag();
   bool tcUILocationUILockError();
   bool tcUIUnitUILockError();
   bool tcUISquadUILockError();
   bool tcUILocationWaiting();
   bool tcUIUnitWaiting();
   bool tcUISquadWaiting();
   bool tcCompareCost();
   bool tcCheckResourceTotals();
   bool tcComparePopulation();
   bool tcCanGetOneUnit();
   bool tcCanGetOneSquad();
   bool tcCheckDiplomacy();
   bool tcCheckModeChange();
   bool tcIsBuilt();
   bool tcIsMoving();
   bool tcPlayerIsHuman();
   bool tcPlayerIsGaia();
   bool tcComparePlayers();
   bool tcCompareTeams();
   bool tcCanGetOnePlayer();
   bool tcCanGetOneTeam();
   bool tcComparePlayerUnitCount();
   bool tcComparePlayerUnitCountV1();
   bool tcComparePlayerUnitCountV2();
   bool tcIsSquadAtMaxSize();
   bool tcCanRetrieveExternalLocation();
   bool tcCanRetrieveExternalLocationList();
   bool tcNextKBBase();
   bool tcCompareFloat();
   bool tcIsUnderAttack();
   bool tcIsUnderAttackV1();
   bool tcIsUnderAttackV2();
   bool tcCompareLeader();
   bool tcPlayerUsingLeader();
   bool tcCanGetOneProtoObject();
   bool tcCanGetOneProtoSquad();
   bool tcCanGetOneObjectType();
   bool tcCanGetOneTech();
   bool tcNextProtoObject();
   bool tcNextProtoSquad();
   bool tcNextObjectType();
   bool tcNextTech();
   bool tcHasAttached();
   bool tcIsMobile();
   bool tcCompareProtoSquad();
   bool tcHasCinematicTagFired();
   bool tcCanGetBuilder();
   bool tcUIButtonPressed();
   bool tcUIButtonWaiting();
   bool tcCompareAIMissionType();
   bool tcCompareAIMissionState();
   bool tcCompareAIMissionTargetType();
   bool tcCanGetOneInteger();
   bool tcCanGetUnitsAlongRay();
   bool tcCanGetUnitsAlongRayV1();
   bool tcCanGetUnitsAlongRayV2();
   bool tcCanRemoveOneInteger();
   bool tcCanGetOneLocation();
   bool tcCanRemoveOneLocation();
   bool tcCompareProtoObject();
   bool tcCompareTech();
   bool tcProtoObjectListContains();
   bool tcProtoSquadListContains();
   bool tcTechListContains();
   bool tcCanRemoveOneProtoObject();
   bool tcCanRemoveOneProtoSquad();
   bool tcCanRemoveOneTech();
   bool tcBidState();
   bool tcBuildingCommandDone();
   bool tcAITopicIsActive();
   bool tcComparePlayerSquadCount();
   bool tcHasGarrisoned();
   bool tcHasGarrisonedV1();
   bool tcHasGarrisonedV2();
   bool tcIsGarrisoned();
   bool tcIsGarrisonedV1();
   bool tcIsGarrisonedV2();
   bool tcIsAttached();
   bool tcIsPassable();
   bool tcCanGetCentroid();
   bool tcPlayerIsPrimaryUser();
   bool tcIsConfigDefined();
   bool tcIsCoop();
   bool tcCustomCommandCheck();
   bool tcIsObjectType();
   bool tcIsObjectTypeV1();
   bool tcIsObjectTypeV2();
   bool tcUISquadListOK();
   bool tcUISquadListCancel();
   bool tcUISquadListUILockError();
   bool tcUISquadListWaiting();
   bool tcIsTimerDone();
   bool tcNextKBSquad();
   bool tcIsAttacking();
   bool tcIsAttackingV2();
   bool tcIsAttackingV3();
   bool tcCanGetUnitLaunchLocation();
   bool tcIsGathering();
   bool tcIsCapturing();
   bool tcPlayerIsComputerAI();
   bool tcCanGetGreatestThreat();
   bool tcCanGetTargetedSquad();
   bool tcIsObjectiveComplete();
   bool tcCompareVector();
   bool tcGetTableRow();
   bool tcCheckPop();
   bool tcIsUserModeNormal();
   bool tcCanGetObjects();
   bool tcCanGetObjectsV1();
   bool tcCanGetObjectsV2();
   bool tcIsHitched();
   bool tcHasHitched();
   bool tcSquadSquadDistance();
   bool tcEventTriggered();
   bool tcIsInQueue();
   bool tcCompareLocStringID();
   bool tcCanGetOneObject();
   bool tcCanGetCorpseUnits();
   bool tcCanGetOneTime();
   bool tcCanGetOneTimeV1();
   bool tcCanGetOneTimeV2();
   bool tcCanGetOneDesignLine();
   bool tcCompareDesignLine();
   bool tcCanRetrieveExternalFlag();
   bool tcAICanGetTopicFocus();
   bool tcCanGetOneFloat();
   bool tcUILocationMinigameWaiting();
   bool tcUILocationMinigameOK();
   bool tcUILocationMinigameCancel();
   bool tcUILocationMinigameUILockError();
   bool tcIsSelectable();
   bool tcChatCompleted();
   bool tcChatCompletedV1();
   bool tcChatCompletedV2();
   bool tcCinematicCompleted();
   bool tcSquadFlag();
   bool tcCanGetSocketUnits();
   bool tcCanGetSocketUnitsV1();
   bool tcCanGetSocketUnitsV2();
   bool tcCanGetOneSocketUnit();
   bool tcFadeCompleted();
   bool tcIsAutoAttackable();
   bool tcIsBeingGatheredFrom();
   bool tcAICanGetDifficultySetting();
   bool tcConceptGetParent();
   bool tcConceptGetCommand();
   bool tcConceptGetStateChange();
   bool tcConceptCompareState();
   bool tcIsEmptySocketUnit();
   bool tcIsForbidden();
   bool tcCanGetSocketParentBuilding();
   bool tcCanGetRandomLocation();
   bool tcCheckDifficulty();
   bool tcCheckDifficultyV1();
   bool tcCheckDifficultyV2();
   bool tcCanGetDesignSpheres();
   bool tcCanRemoveOneFloat();
   bool tcCanRetrieveExternalFloat();
   bool tcMarkerSquadsInArea();
   bool tcCanGetSocketPlugUnit();
   bool tcCanGetHoverPoint();
   bool tcCanGetCoopPlayer();
   bool tcCompareUnit();
   bool tcASYNCUnitsOnScreenSelected();
   bool tcCheckAndSetFalse();
   bool tcAITopicGetTickets();


   // NEWTRIGGERCONDITION
   // Add your trigger condition function header here.
   // Format is bool tcYOURCONDITIONNAME();
   // If you add new versions of a condition, the original function tcYOURCONDITIONNAME() will be the switch statement,
   // which calls a separate function for each version that is supported, named tcYOURCONDITIONNAMEV<n>(); where <n> is the version number.
   // Also, make sure to keep them in order and together in this list for easier bookkeeping.
   // For example:
   // tcYOURCONDITIONNAME();
   // tcYOURCONDITIONNAMEV1();
   // tcYOURCONDITIONNAMEV2();
   // tcANOTHERONE();
   // tcANOTHERONEV1();
   // tcANOTHERONEV2();
   // AND so on...
   // Any questions?  Ask Marc.


   // Handles all async evaluations of consensus state.
   bool evaluateAsyncConditionConsensusState() const { BASSERT(mbAsyncCondition); return (mbConsensusState); }

   // Updates the async consensus state by sending notification commands out to other players.
   bool tcPlayerSelectingUnit();
   bool tcPlayerSelectingUnitV1();
   bool tcPlayerSelectingUnitV2();
   bool tcPlayerLookingAtUnit();
   bool tcPlayerLookingAtUnitV1();
   bool tcPlayerLookingAtUnitV2();

   bool tcPlayerLookingAtLocation();

   // Trigger condition helper function
   static void refCountHelper(BEntityID entityID, short refCountType, bool doCompare, long operatorType, long compareCount, long& maxCount, bool& retval, bool& anyCompared);
   static void unitFlagHelper(BEntityID unitID, long flagType, bool doCompare, bool compareValue, bool& retval, bool& allOn);
   static void squadFlagHelper(BEntityID squadID, long flagType, bool doCompare, bool compareValue, bool& retval, bool& allOn);
                                                         // 27 bytes total
   BSmallDynamicSimArray<BTriggerVar*> mConditionVars;   // 8 bytes
   BTriggerScript* mpParentTriggerScript;                // 4 bytes
   BTrigger* mpParentTrigger;                            // 4 bytes
   long mDBID;                                           // 4 bytes
   long mID;                                             // 4 bytes
   BUInt8<uint, UINT8_MIN, UINT8_MAX> mVersion;          // 1 byte
   BTriggerVarSigIDSmall mAsyncKeyVarID;                 // 1 byte

   bool mbInvert              : 1;                       // 1 byte   Is the condition inverted?  (Checking for !true instead of true)
   bool mbAsyncCondition      : 1;                       //          Is this condition an async condition that needs to be handled specially.
   bool mbLocalAsyncMachine   : 1;                       //          Is this the player that should perform the logic of the condition.
   bool mbLocalMachineState   : 1;                       //          What is the state of the condition result on the local machine.
   bool mbLastBroadcastState  : 1;                       //          What is the last state of the condition result the local machine has broadcast to everyone else.
   bool mbConsensusState      : 1;                       //          What is the consensus state of the condition result on all the machines (received by the broadcast.)
};

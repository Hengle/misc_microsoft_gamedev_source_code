//==============================================================================
// triggereffect.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xsystem
#include "cost.h"
#include "xmlreader.h"

// forward declarations
class BTriggerScript;
class BTriggerVar;

//==============================================================================
// class BTriggerEffect
//==============================================================================
class BTriggerEffect : public IPoolable
{
public:

   //===========================================================================
   // Trigger Effect Type Enumerations
   // Each trigger effect type gets its own enumeration that never changes.tri
   // The name of the effect can change but it is tracked by this enum.
   // Even if you obsolete an effect, keep the enumeration around, and DO NOT
   // reorder them.
   //===========================================================================
   enum
   {
      cTETriggerActivate                     = 31,
      cTETriggerDeactivate                   = 32,
      cTEPlaySound                           = 33,
      cTEPlayRelationSound                   = 34,
      cTECreateObject                        = 35,
      cTECreateSquad                         = 36,
      cTEKill                                = 37,
      cTEDestroy                             = 38,
      // OBSOLETE                            = 39,
      // OBSOLETE                            = 40,
      // OBSOLETE                            = 41,
      // OBSOLETE                            = 42,
      // OBSOLETE                            = 43,
      // OBSOLETE                            = 44,
      // OBSOLETE                            = 45,
      // OBSOLETE                            = 46,
      // OBSOLETE                            = 47,
      // OBSOLETE                            = 48,
      // OBSOLETE                            = 49,
      cTEPayCost                             = 50,
      cTERefundCost                          = 51,
      cTECountIncrement                      = 52,
      cTECountDecrement                      = 53,
      // OBSOLETE                            = 54,
      cTERandomLocation                      = 55,
      cTEShutdown                            = 56,
      cTELaunchProjectile                    = 57,
      cTELocationTieToGround                 = 58,
      cTELocationAdjust                      = 59,
      cTETechActivate                        = 60,
      cTETechDeactivate                      = 61,
      // OBSOLETE                            = 62,
      // OBSOLETE                            = 63,
      // OBSOLETE                            = 64,
      cTEUnload                              = 65,
      cTEMove                                = 66,
      // OBSOLETE                            = 67,
      // OBSOLETE                            = 68,
	   cTETransportSquads                     = 69,
      // OBSOLETE                            = 70,
      cTECarpetBomb                          = 71,
      // OBSOLETE                            = 72,
      // OBSOLETE                            = 73,
      cTEAttachmentAddType                   = 74,
      cTEAttachmentRemoveAll                 = 75,
      cTEOBSOLETEUsePower                    = 76,
      cTEUsePower                            = 77,
      cTEGetClosestPowerSquad                = 78,
      cTESetUIPowerRadius                    = 79,
      cTEAttachmentRemoveType                = 83,
      cTEInputUILocation                     = 84,
      cTECopyTech                            = 85,
      cTECopyTechStatus                      = 86,
      cTECopyOperator                        = 87,
      cTECopyProtoObject                     = 88,
      cTECopyObjectType                      = 89,
      cTECopyProtoSquad                      = 90,
      cTECopySound                           = 91,
      cTECopyEntity                          = 92,
      cTECopyEntityList                      = 93,
      cTECopyCost                            = 94,
      cTECopyDistance                        = 95,
      cTECopyTime                            = 96,
      cTECopyPlayer                          = 97,
      cTECopyCount                           = 98,
      cTECopyLocation                        = 99,
      cTECopyPercent                         = 100,
      cTECopyHitpoints                       = 101,
      cTECopyBool                            = 102,
      cTECopyFloat                           = 103,
      // OBSOLETE                            = 104,
      // OBSOLETE                            = 105,
      // OBSOLETE                            = 106,
      // OBSOLETE                            = 107,
      // OBSOLETE                            = 108,
      cTEGetDistanceUnitUnit                 = 110,
      cTEGetDistanceUnitLocation             = 111,
      cTEGetUnits                            = 112,
      cTEGetSquads                           = 113,
      // OBSOLETE                            = 116,
      cTEWork                                = 117,
      cTEIteratorPlayerList                  = 120,
      cTEIteratorTeamList                    = 121,
      cTEGetTeams                            = 122,
      cTEGetTeamPlayers                      = 123,
      cTEPlayerListAdd                       = 124,
      cTEPlayerListRemove                    = 125,
      cTETeamListAdd                         = 126,
      cTETeamListRemove                      = 127,
      // OBSOLETE                            = 129,
      cTESetPlayerState                      = 130,
      cTEFlareMinimapSpoof                   = 132,
      cTEObjectiveComplete                   = 133,
      // OBSOLETE                            = 134,
      cTEFlareMinimapNormal                  = 135,
      cTEChangeOwner                         = 137,
      cTEMKTest                              = 139,
      cTECopyUnit                            = 142,
      cTECopyUnitList                        = 143,
      cTECopySquad                           = 144,
      cTECopySquadList                       = 145,
      cTEUnitListGetSize                     = 146,
      cTESquadListGetSize                    = 147,
      cTEUnitListAdd                         = 148,
      cTESquadListAdd                        = 149,
      cTEUnitListRemove                      = 150,
      cTESquadListRemove                     = 151,
      cTEIteratorUnitList                    = 152,
      cTEIteratorSquadList                   = 153,
      cTECreateUnit                          = 154,
      cTEPlayAnimationUnit                   = 159,
      cTEPlayAnimationSquad                  = 160,
      cTEInputUIUnit                         = 161,
      cTEInputUISquad                        = 162,
      cTEObjectiveUserMessage                = 163,
      cTEUserMessage                         = 164,
      // OBSOLETE                            = 165,
      // OBSOLETE                            = 166,
      cTETimeUserMessage                     = 167,
      cTECopyColor                           = 168,
      cTECopyString                          = 169,
      cTETransform                           = 172,
      cTEGetPlayers                          = 173,
      cTEGetHealth                           = 174,
      //OBSOLETE                             = 176,
      //OBSOLETE                             = 177,
      cTERandomCount                         = 178,
      cTEMathCount                           = 179,
      cTEMathHitpoints                       = 180,
      cTEAsString                            = 181,
      cTEObjectiveDisplay                    = 183,
      cTECalculatePercentCount               = 184,
      cTECalculatePercentHitpoints           = 185,
      cTECalculatePercentTime                = 186,
      cTELerpCount                           = 187,
      cTELerpColor                           = 188,
      cTEGetLocation                         = 189,
      cTEMathPercent                         = 190,
      cTEGetOwner                            = 193,
      //OBSOLETE                             = 194,
      //OBSOLETE                             = 195,
      //OBSOLETE                             = 196,
      cTEDebugVarTech                        = 197,
      cTEDebugVarTechStatus                  = 198,
      cTEDebugVarOperator                    = 199,
      cTEDebugVarProtoObject                 = 200,
      cTEDebugVarObjectType                  = 201,
      cTEDebugVarProtoSquad                  = 202,
      cTEDebugVarSound                       = 203,
      //OBSOLETE                             = 204,
      //OBSOLETE                             = 205,
      cTEDebugVarDistance                    = 206,
      cTEDebugVarTime                        = 207,
      cTEDebugVarPlayer                      = 208,
      cTEDebugVarCount                       = 209,
      cTEDebugVarLocation                    = 210,
      cTEDebugVarUILocation                  = 211,
      cTEDebugVarCost                        = 212,
      cTEDebugVarAnimType                    = 213,
      cTEDebugVarPercent                     = 214,
      cTEDebugVarHitpoints                   = 215,
      cTEDebugVarPower                       = 216,
      cTEDebugVarBool                        = 217,
      cTEDebugVarFloat                       = 218,
      cTEDebugVarIterator                    = 219,
      cTEDebugVarTeam                        = 220,
      cTEDebugVarPlayerList                  = 221,
      cTEDebugVarTeamList                    = 222,
      cTEDebugVarPlayerState                 = 223,
      cTEDebugVarObjective                   = 224,
      cTEDebugVarUnit                        = 225,
      cTEDebugVarUnitList                    = 226,
      cTEDebugVarSquad                       = 227,
      cTEDebugVarUIUnit                      = 228,
      cTEDebugVarUISquad                     = 229,
      cTEDebugVarString                      = 230,
      cTEDebugVarColor                       = 231,
      cTEDebugVarProtoObjectList             = 232,
      cTEDebugVarObjectTypeList              = 233,
      cTEDebugVarProtoSquadList              = 234,
      cTEDebugVarTechList                    = 235,
      cTEDebugVarMathOperator                = 236,
      cTEModifyProtoData                     = 237,
      cTEGetPlayerCiv                        = 239,
      // OBSOLETE                            = 241,
      cTEGetIdleDuration                     = 243,
      // OBSOLETE                            = 244,
      // OBSOLETE                            = 245,
      // OBSOLETE                            = 246,
      // OBSOLETE                            = 247,
      // OBSOLETE                            = 248,
      // OBSOLETE                            = 249,
      // OBSOLETE                            = 250,
      // OBSOLETE                            = 251,
      // OBSOLETE                            = 252,
      // OBSOLETE                            = 253,
      // OBSOLETE                            = 254,
      cTEEnableAttackNotifications           = 255,
      cTEGetHitZoneHealth                    = 257,
      cTESetHitZoneHealth                    = 258,
      cTESetHitZoneActive                    = 259,
      cTECopyMessageIndex                    = 261,
      cTELerpPercent                         = 262,
      cTEMathTime                            = 263,
      cTEGetGameTime                         = 265,
      cTEGetGameTimeRemaining                = 267,
      // OBSOLETE                            = 270,
      // OBSOLETE                            = 271,
      // OBSOLETE                            = 272,
      // OBSOLETE                            = 273,
      // OBSOLETE                            = 274,
      // OBSOLETE                            = 275,
      // OBSOLETE                            = 276,
      cTESetResources                        = 277,
      cTEGetResources                        = 278,
      cTEMathResources                       = 279,
      cTEIteratorObjectList                  = 281,
      cTEForbid                              = 283,
      cTEInvertBool                          = 284,
      cTERevealer                            = 285,
      cTEMathDistance                        = 286,
      cTEGroupDeactivate                     = 287,
      cTEIteratorLocationList                = 288,
      cTELocationListAdd                     = 289,
      cTELocationListRemove                  = 290,
      cTELocationListGetSize                 = 291,
      cTELerpLocation                        = 294,
      cTEMathLocation                        = 295,
      cTESquadListPartition                  = 296,
      cTESquadListShuffle                    = 297,
      cTELocationListShuffle                 = 298,
      cTEEntityListShuffle                   = 299,
      cTEPlayerListShuffle                   = 300,
      cTETeamListShuffle                     = 301,
      cTEUnitListShuffle                     = 302,
      cTEProtoObjectListShuffle              = 303,
      cTEObjectTypeListShuffle               = 304,
      cTEProtoSquadListShuffle               = 305,
      cTETechListShuffle                     = 306,
      //OBSOLETE                             = 307,
      cTEUnitListPartition                   = 308,
      cTELocationListPartition               = 309,
      // OBSOLETE                            = 310,
      cTERefCountUnitAdd                     = 311,
      cTERefCountUnitRemove                  = 312,
      cTERefCountSquadAdd                    = 316,
      cTERefCountSquadRemove                 = 317,
      cTERepair                              = 318,
      cTEUnitFlagSet                         = 319,
      cTEGetChildUnits                       = 324,
      cTEDamage                              = 325,
      cTEUIUnlock                            = 330,
      cTESquadListDiff                       = 334,
      cTEUnitListDiff                        = 335,
      cTECombatDamage                        = 336,
      cTESetResourcesTotals                  = 337,
      cTEGetResourcesTotals                  = 338,
      cTEEntityFilterClear                   = 341,
      cTEEntityFilterAddIsAlive              = 342,
      cTEEntityFilterAddInList               = 343,
      cTEEntityFilterAddPlayers              = 344,
      cTEEntityFilterAddTeams                = 345,
      cTEEntityFilterAddProtoObjects         = 346,
      cTEEntityFilterAddProtoSquads          = 347,
      cTEEntityFilterAddObjectTypes          = 348,
      cTEUnitListFilter                      = 349,
      cTESquadListFilter                     = 350,
      cTEEntityFilterAddRefCount             = 351,
      cTEMathFloat                           = 353,
      cTETeleport                            = 354,
      cTEEntityFilterAddIsIdle               = 355,
      cTEAsFloat                             = 358,
      cTESettle                              = 359,
      cTECopyObjective                       = 360,
      cTEAttachmentRemoveObject              = 361,
      cTEAttachmentAddObject                 = 363,
      cTEAttachmentAddUnit                   = 364,
      cTEAttachmentRemoveUnit                = 365,
      cTEBidCreateBlank                      = 369,
      cTEBidCreateBuilding                   = 370,
      cTEBidCreateTech                       = 371,
      cTEBidCreateSquad                      = 372,
      cTEBidDelete                           = 373,
      cTEBidSetBuilding                      = 374,
      cTEBidSetTech                          = 375,
      cTEBidSetSquad                         = 376,
      cTEBidClear                            = 377,
      cTEBidSetPriority                      = 378,
      cTEEntityFilterAddDiplomacy            = 379,
      //OBSOLETE                             = 381,
      cTESetIgnoreUserInput                  = 382,
      //OBSOLETE                             = 383,
      //OBSOLETE                             = 384,
      cTEEnableFogOfWar                      = 385,
      // OBSOLETE                            = 387,
      cTEChangeSquadMode                     = 388,
      cTEGetAmmo                             = 389,
      cTESetAmmo                             = 390,
      cTEBidPurchase                         = 391,
      cTELaunchScript                        = 392,
      cTEGetPlayerTeam                       = 393,
      // OBSOLETE                            = 394,
      // OBSOLETE                            = 395,
      // OBSOLETE                            = 396,
      // OBSOLETE                            = 407,
      // OBSOLETE                            = 408,
      // OBSOLETE                            = 409,
      // OBSOLETE                            = 410,
      // OBSOLETE                            = 411,
      // OBSOLETE                            = 412,
      cTEModifyDataScalar                    = 413,
      // OBSOLETE                            = 415,
      cTECloak                               = 417,
      cTEBlocker                             = 418,
      cTESensorLock                          = 419,
      // OBSOLETE                            = 420,
      // OBSOLETE                            = 422,
      // OBSOLETE                            = 423,
      // OBSOLETE                            = 424,
      cTEDesignFindSphere                    = 425,
      cTEReinforceSquad                      = 430,
      cTEGetPlayers2                         = 431,
      cTEPlayersToTeams                      = 432,
      cTETeamsToPlayers                      = 433,
      cTEMovePath                            = 439,
      cTEIteratorKBBaseList                  = 443,
      // OBSOLETE                            = 445,
      cTEKBBaseGetDistance                   = 446,
      cTEKBBaseGetMass                       = 447,
      cTEKBBQReset                           = 448,
      cTEKBBQExecute                         = 449,
      cTEKBBQPointRadius                     = 450,
      cTEKBBQPlayerRelation                  = 451,
      // OBSOLETE                            = 452,
      // OBSOLETE                            = 454,
      cTECopyKBBase                          = 455,
      cTEPowerGrant                          = 456,
      cTEPowerRevoke                         = 457,
      cTEGetSquadTrainerType                 = 458,
      cTEGetTechResearcherType               = 459,
      cTEDesignLineGetPoints                 = 460,
      cTEEnableShield                        = 463,
      // OBSOLETE                            = 464,
      // OBSOLETE                            = 465,
      // OBSOLETE                            = 466,
      // OBSOLETE                            = 467,
      // OBSOLETE                            = 468,
      // OBSOLETE                            = 469,
      // OBSOLETE                            = 470,
      cTEKBBQMinStaleness                    = 471,
      cTEKBBQMaxStaleness                    = 472,
      // OBSOLETE                            = 473,
      // OBSOLETE                            = 474,
      cTEGetPlayerLeader                     = 475,
      // OBSOLETE                            = 478,
      cTELaunchCinematic                     = 480,
      cTECopyProtoObjectList                 = 481,
      cTECopyProtoSquadList                  = 482,
      cTECopyObjectTypeList                  = 483,
      cTECopyTechList                        = 484,
      cTESetDirection                        = 489,            
      cTEIteratorProtoObjectList             = 490,
      cTEIteratorProtoSquadList              = 491,
      cTEIteratorObjectTypeList              = 492,
      cTEIteratorTechList                    = 493,      
      cTEAsCount                             = 498,
      cTEGetDirection                        = 499,
      cTEGetDirectionFromLocations           = 500,
      cTECopyDirection                       = 501,      
      cTEDebugDirection                      = 502,
      // OBSOLETE                            = 503,
      cTEKBBQExecuteClosest                  = 504,
      cTEGetLegalSquads                      = 505,
      cTEGetLegalTechs                       = 506,
      cTEGetLegalBuildings                   = 507,
      cTESetMobile                           = 510,
      cTEGetKBBaseLocation                   = 511,
      cTEGetDistanceLocationLocation         = 512,
      cTEPlayWorldSoundAtPosition            = 514,
      cTEPlayWorldSoundOnObject              = 515,
      cTERandomTime                          = 516,
      cTEGetParentSquad                      = 519,
      cTECreateIconObject                    = 520,
      cTESetFollowCam                        = 521,
      cTEEnableFollowCam                     = 522,
      cTEGetGarrisonedUnits                  = 523,
      cTEGetDifficulty                       = 525,
      cTEHUDToggle                           = 526,
      cTEInputUIButton                       = 529,
      cTEDebugVarUIButton                    = 530,
      cTESetRenderTerrainSkirt               = 532,
      cTELightsetAnimate                     = 533,
      cTEAttack                              = 534,
      cTESetBlockAttack                      = 535,
      cTESetOccluded                         = 536,
      cTESetPosition                         = 537,
      cTESetFlagNearLayer                    = 541,
      cTEAIGenerateMissionTargets            = 543,
      cTEAIScoreMissionTargets               = 544,
      cTEAISortMissionTargets                = 545,
      cTEAIMissionLaunch                     = 549,
      cTEAIMissionCancel                     = 550,
      cTEAIGetMissionTargets                 = 551,
      cTELocationAdjustDir                   = 554,
      cTEBuildingCommand                     = 559,
      cTEAIMissionAddSquads                  = 555,
      cTEAIMissionRemoveSquads               = 556,
      cTEAIMissionGetSquads                  = 557,
      cTECloakDetected                       = 560,
      cTEAIMissionTargetGetScores            = 561,
      cTECopyIntegerList                     = 562,
      cTEAISetScoringParms                   = 564,
      cTECountToInt                          = 567,
      cTEIntToCount                          = 568,
      cTEProtoObjectListAdd                  = 569,
      cTEProtoObjectListRemove               = 570,
      cTEProtoSquadListAdd                   = 571,
      cTEProtoSquadListRemove                = 572,
      cTETechListAdd                         = 573,
      cTETechListRemove                      = 574,
      cTEIntegerListAdd                      = 580,
      cTEIntegerListRemove                   = 581,
      cTEIntegerListGetSize                  = 582,
      cTEBidGetData                          = 587,
      cTEPlayAnimationObject                 = 588,
      cTEAITopicCreate                       = 589,
      cTEAITopicDestroy                      = 590,
      cTEAITopicModifyTickets                = 591,
      // OBSOLETE                            = 592,
      cTEAIMissionModifyTickets              = 593,
      // OBSOLETE                            = 594,
      cTECopyObject                          = 595,
      cTEDebugVarBuildingCommandState        = 597,
      cTEAIScnMissionAttackArea              = 598,
      cTEAIScnMissionDefendArea              = 599,
      cTEAIGetMissions                       = 605,
      cTEGetDeadUnitCount                    = 607,
      cTEGetDeadSquadCount                   = 608,
      cTECopyLocationList                    = 609,
      cTEAIMissionGetLaunchScores            = 610,
      cTEAIRemoveFromMissions                = 612,
      cTEBreakpoint                          = 614,
      cTEGetPrimaryUser                      = 615,
      cTEGetBidsMatching                     = 618,
      cTEAIMissionGetTarget                  = 619,
      // OBSOLETE                            = 620,
      cTEBidSetTargetLocation                = 622,
      cTEBidSetBuilder                       = 623,
      cTEAITopicLotto                        = 625,
      cTEEnableUserMessage                   = 626,
      // OBSOLETE                            = 628,
      cTEGetProtoSquad                       = 630,
      cTEGetProtoObject                      = 631,
      cTECameraShake                         = 632,
      cTECustomCommandAdd                    = 633,
      cTECustomCommandRemove                 = 634,
      cTEConnectHitpointBar                  = 638,
      cTEInputUISquadList                    = 639,
      cTEAirStrike                           = 644,
      cTEGetPlayerPop                        = 646,
      cTEGetPlayerEconomy                    = 647,
      cTEGetPlayerScore                      = 648,
      // OBSOLETE                            = 650,
      // OBSOLETE                            = 651,
      // OBSOLETE                            = 652,
      // OBSOLETE                            = 653,
      // OBSOLETE                            = 654,
      // OBSOLETE                            = 655,
      // OBSOLETE                            = 656,
      cTEEntityFilterAddMaxObjectType        = 657,
      cTECreateTimer                         = 658,
      cTEDestroyTimer                        = 659,
      cTEKBSQInit                            = 663,
      cTEKBSQReset                           = 664,
      cTEKBSQExecute                         = 665,
      cTEAICalculations                      = 666, 
      cTEAICalculations2                     = 667,
      cTEAIAnalyzeSquadList                  = 668,
      cTEAIAnalyzeOffenseAToB                = 669,
      cTEGetMeanLocation                     = 670,
      cTECopyObjectList                      = 671,
      cTEAISAGetComponent                    = 672,
      cTEAIAnalyzeKBSquadList                = 673,
      cTEAIAnalyzeProtoSquadList             = 674,
      cTEShowGarrisonedCount                 = 675,
      cTEShowCitizensSaved                   = 676,
      cTESetCitizensSaved                    = 677,
      cTECostToFloat                         = 678,
      cTEGetCost                             = 679,
      cTEAIUnopposedTimeToKill               = 681,
      cTEGetPrimaryHealthComponent           = 682,
      cTEGetLOS                              = 683,
      cTEKBSquadFilterClear                  = 684,
      cTEKBSFAddCurrentlyVisible             = 685,
      cTEKBSFAddObjectTypes                  = 686,
      cTEShowObjectivePointer                = 687,
      cTEKBSFAddPlayers                      = 688,
      cTEKBSFAddInList                       = 689,
      cTEKBSFAddPlayerRelation               = 690,
      cTEKBSquadListFilter                   = 691,
      cTEKBSFAddMinStaleness                 = 692,
      cTEKBSFAddMaxStaleness                 = 693,
      cTEKBSquadListGetSize                  = 694,
      cTEKBSquadGetOwner                     = 695,
      cTEKBSquadGetLocation                  = 696,
      cTEConvertKBSquadsToSquads             = 697,
      cTEKBSquadGetProtoSquad                = 698,
      cTEKBBaseGetKBSquads                   = 699,
      cTECopyKBSquad                         = 700,
      cTEKBSQExecuteClosest                  = 701,
      cTEKBSQPointRadius                     = 702,
      cTEKBSQPlayerRelation                  = 703,
      cTEKBSQObjectType                      = 704,
      cTEKBSQBase                            = 705,
      cTEKBSQMinStaleness                    = 706,
      cTEKBSQMaxStaleness                    = 707,
      cTEKBSQCurrentlyVisible                = 708,
      cTESetUnitAttackTarget                 = 710,
      cTEAISetFocus                          = 711,
      cTESetPlayableBounds                   = 712,
      cTEResetBlackMap                       = 713,
      cTEAsTime                              = 716,
      cTERallyPointSet                       = 717,
      cTERallyPointClear                     = 718,
      cTERallyPointGet                       = 719,
      cTEAIBindLog                           = 720,
      cTEUITogglePowerOverlay                = 726,
      cTEPlayChat                            = 729,
      cTEShowProgressBar                     = 730,
      cTEUpdateProgressBar                   = 731,
      cTEShowObjectCounter                   = 732,
      cTEUpdateObjectCounter                 = 733,
      cTEDebugVarSquadList                   = 734,
      cTERoundFloat                          = 735,
      cTEGetPop                              = 736,
      cTEPowerClear                          = 737,
      cTEMegaTurretAttack                    = 738,
      cTEPowerInvoke                         = 739,
      cTESetPlayerPop                        = 741,
      cTECopyIconType                        = 742,
      cTEGetPlayerMilitaryStats              = 745,
      cTEGetObjectiveStats                   = 747,
      cTELerpTime                            = 748,
      cTECombineString                       = 750,
      cTETimeToFloat                         = 751,
      cTEBlockMinimap                        = 753,
      cTEAIChat                              = 754,
      cTECopyChatSpeaker                     = 755,
      cTEBlockLeaderPowers                   = 756,
      cTEAIMissionTargetGetLocation          = 757,
      cTEAIGetFlareAlerts                    = 758,
      cTEAIGetAttackAlerts                   = 759,
      cTEAIGetLastFlareAlert                 = 760,
      cTEAIGetAlertData                      = 761,
      cTEAIGetLastAttackAlert                = 764,
		cTEEventFilterGameState						= 765,
      cTEBidSetBlockedBuilders               = 766,
      cTEGetGarrisonedSquads                 = 767,
      cTETransferGarrisonedSquad             = 768,
      cTETransferGarrisoned                  = 769,
      cTEAISetBiases                         = 770,
      cTEInputUIPlaceSquads                  = 771,
      cTEGetNumTransports                    = 772,
      cTERumbleStart                         = 773,
      cTERumbleStop                          = 774,
      cTESetOverrideTint                     = 775,
      cTEEnableOverrideTint                  = 776,
      cTESetTransportPickUpLocations         = 777,
      cTEBidQuery                            = 778,
      cTETimerSet                            = 779,
      cTETimerGet                            = 780,
      cTETimerSetPaused                      = 781,
      cTEAITopicPriorityRequest              = 783,
      cTEPlayerSelectSquads                  = 784,
      // OBSOLETE                            = 786,
      cTESetResourceHandicap                 = 787,
      cTEParkingLotSet                       = 790,
      cTEUnpack                              = 791,
      // OBSOLETE                            = 793,
      // OBSOLETE                            = 794,
      cTETableLoad                           = 796,
      // OBSOLETE                            = 802,
      // OBSOLETE                            = 803,
      // OBSOLETE                            = 804,
      cTEHintMessageShow                     = 805,
      cTEHintGlowToggle                      = 808,
      cTEHintCalloutCreate                   = 809,
      cTEHintCalloutDestroy                  = 810,
      cTEEventSubscribe                      = 811,
      cTEEventSetFilter                      = 812,
      cTEHintMessageDestroy                  = 815,
      cTEGetPlayerColor                      = 816,
      cTECopyControlType                     = 817,
      cTECopyLocStringID                     = 818,
      cTESetLevel                            = 820,
      cTEGetLevel                            = 822,
      cTEKBSquadListDiff                     = 826,
      cTEEventFilterType                     = 827,
      cTEEventFilterNumeric                  = 828,
      cTEModifyProtoSquadData                = 829,
      cTESetScenarioScore                    = 832,
      cTECreateObstructionUnit               = 833,
      cTEPlayVideo                           = 834,
      cTEObjectListRemove                    = 836,
      cTEEventReset                          = 837,
      cTEEventFilterCamera                   = 838,
      cTEEventFilterEntity                   = 839,
      cTEEventFilterEntityList               = 840,
      cTEEnableChats                         = 841,
      cTEObjectListAdd                       = 842,
      cTEObjectListGetSize                   = 843,
      cTEClearCorpseUnits                    = 845,
      cTEConceptGetParameters                = 849,
      cTEConceptSetttings                    = 850,
      cTEAddXP                               = 852,
      cTEGetClosestSquad                     = 853,
      //cTEMoveToFace                          = 854,
      cTEGetBuildingTrainQueue               = 854,
      cTEAIRegisterHook                      = 855,
      cTECopyTimeList                        = 856,
      cTETimeListAdd                         = 857,
      cTETimeListRemove                      = 858,
      cTETimeListGetSize                     = 859,
      cTEClearBlackMap                       = 863,
      cTECopyDesignLineList                  = 867,
      cTEDesignLineListAdd                   = 868,
      cTEDesignLineListRemove                = 869,
      cTEDesignLineListGetSize               = 870,
      cTEBidSetQueueLimits                   = 871,
      cTECopyDesignLine                      = 872,
      cTEProtoSquadListGetSize               = 873,
      cTEGetObstructionRadius                = 874,
      cTECreateSquads                        = 875,
      cTECopyLOSType                         = 876,
      cTEAISetAssetMultipliers               = 878,
      cTEAIGetOpportunityRequests            = 879,
      cTEAIGetTerminalMissions               = 880,
      cTEAIClearOpportunityRequests          = 881,
      cTEAITopicSetFocus                     = 882,
      cTESetCamera                           = 884,
      cTECopyFloatList                       = 886,
      cTEFloatListAdd                        = 887,
      cTEFloatListRemove                     = 888,
      cTEFloatListGetSize                    = 889,
      cTEGetUTechBldings                     = 890,
      cTERepairByCombatValue                 = 891,
      cTEInputUILocationMinigame             = 892,
      cTEAIQueryMissionTargets               = 897,
      cTEGetClosestPath                      = 898,
      cTESetSelectable                       = 900,
      cTESetTrickleRate                      = 901,
      cTEGetTrickleRate                      = 902,
      cTEChatDestroy                         = 903,
      cTEShowMessage                         = 904,
      cTESquadFlagSet                        = 910,
      cTEFadeToColor                         = 912,      
      cTEAICalculateOffenseRatioAToB         = 914,
      cTESetAutoAttackable                   = 915,
      // REMOVED                             = 919,
      cTECopyAISquadAnalysis                 = 921,
      cTEFadeTransition                      = 922,
      cTEFlashUIElement                      = 923,
      cTEConceptPermission                   = 924,
      cTEConceptSetState                     = 925,
      cTEConceptSetPrecondition              = 926,
      cTEConceptStartSub                     = 927,
      cTEConceptClearSub                     = 928,
      cTEConceptSetParameters                = 934,
      cTEObjectiveIncrementCounter           = 935,
      cTEObjectiveDecrementCounter           = 936,
      cTEObjectiveGetCurrentCounter          = 937,
      cTEObjectiveGetFinalCounter            = 938,
      cTEGetClosestUnit                      = 939,
      cTEClearBuildingCommandState           = 940,
      cTETeamSetDiplomacy                    = 941,
      cTEGetNPCPlayersByName                 = 943,
      cTEActivateTentacle                    = 953,
      cTERecycleBuilding                     = 955,
      cTEAIGetMemory                         = 957,
      cTEBidCreatePower                      = 958,
      cTEBidSetPower                         = 959,
      cTEBidAddToMissions                    = 960,
      cTEBidRemoveFromMissions               = 961,
      cTEPowerUsed                           = 964,
      cTESetTeleporterDestination            = 967,
      cTESetGarrisonedCount                  = 968,
      //OBSOLETE                             = 969,
      cTEPlayerListGetSize                   = 970,
      cTEConceptResetCooldown                = 971,
      cTEGetPowerRadius                      = 976,
      cTETransportSquads2                    = 978,
      cTELocationListGetByIndex              = 979,
      cTELocationListGetClosest              = 981,
      cTEAIMissionSetMoveAttack              = 982,
      cTEInfect                              = 983,
      cTESetScenarioScoreInfo                = 984,
      cTEKBAddSquadsToKB                     = 985,
      cTEBidSetPadSupplies                   = 987,
      cTEEventDelete                         = 988,
      cTEEventClearFilters                   = 989,
      cTEAIFactoidSubmit                     = 993,
      // OBSOLETE                            = 995,      
      cTEFlashEntity                         = 1000,
      cTESetTowerWallDestination             = 1001,
      cTEMissionResult                       = 1002,
      // OBSOLETE                            = 1003,
      cTEAICreateAreaTarget                  = 1004,
      cTEAIMissionCreate                     = 1005,
      cTEEnableMusicManager                  = 1006,
      cTEResetDopple                         = 1007,
      cTEAICreateWrapper                     = 1008,
      cTEAIReorderWrapper                    = 1009,
      cTEAIDestroyWrapper                    = 1010,
      cTEAIWrapperModifyRadius               = 1011,
      cTEAIWrapperModifyFlags                = 1012,
      cTEAIWrapperModifyParms                = 1013,
      cTESaveGame                            = 1014,
      cTELoadGame                            = 1015,
      cTEReverseHotDrop                      = 1016,
      // OBSOLETE                            = 1017,
      cTEEventSubscribeUseCount              = 1018,
      cTEObjectTypeToProtoObjects            = 1019,
      cTEDebugVarDesignLine                  = 1020,
      cTEShowInfoDialog                      = 1021,
      cTEPowerUserShutdown                   = 1022,
      cTEPowerToInt                          = 1025,
      cTEIntToPower                          = 1026,
      cTEChangeControlledPlayer              = 1027,
      cTEGrantAchievement                    = 1030,
      cTEAIAddTeleporterZone                 = 1031,
      cTEResetAbilityTimer                   = 1032,
      cTECustomCommandExecute                = 1033,
      cTEIgnoreDpad                          = 1034,
      cTEAISetPlayerAssetModifier            = 1035,
      cTEEnableLetterBox                     = 1036,
      cTEEnableScreenBlur                    = 1037,
      cTEAISetPlayerDamageModifiers          = 1038,
      cTESetPowerAvailableTime               = 1043,
      cTEPowerMenuEnable                     = 1044,
      cTESetMinimapNorthPointerRotation      = 1045,      
      cTEHideCircleMenu                      = 1048,      
      cTEChatForceSubtitles                  = 1049,
      cTEAISetPlayerMultipliers              = 1050,
      cTEAIMissionSetFlags                   = 1051,
      cTEEntityFilterAddIsSelected           = 1052,
      cTESetMinimapSkirtMirroring            = 1054,
      cTEAISetWinRange                       = 1055,
      cTEGetPopularSquadType                 = 1058,
      cTETeleportUnitsOffObstruction         = 1060,
      cTELockPlayerUser                      = 1061,
      cTEGetGameMode                         = 1065,
      cTEAISetPlayerBuildSpeedModifiers      = 1066,
      cTEEntityFilterAddCanChangeOwner       = 1067,
      cTEPatherObstructionUpdates            = 1068,
      cTEPatherObstructionRebuild            = 1069,
      cTEEntityFilterAddJacking              = 1070,

      // NEWTRIGGEREFFECT
      // Add your enumeration for your new trigger effect here.
      // Follow the naming convention as shown.
      // The enumeration value is set by the triggerDestription tool, do not make one up.
      // Do not change the enumeration, evar.
      // Any questions, ask Marc.      
      cNumTriggerEffectTypes
   };

   enum
   {
      cTEVersionInvalid = 0,
   };

   BTriggerEffect(){}
   ~BTriggerEffect(){}
   virtual void onAcquire(){ mpParentTriggerScript = NULL; mDBID = -1; mVersion = cTEVersionInvalid; }
   virtual void onRelease(){ mEffectVars.clear(); }
   DECLARE_FREELIST(BTriggerEffect, 6);

   bool loadFromXML(BXMLNode cArrowLenpNode);
   void fire();
   uint getVersion() const { return (mVersion); }
   void setParentTriggerScript(BTriggerScript *pParentScript) { mpParentTriggerScript = pParentScript; }
   BTriggerScript* getParentTriggerScript() { return (mpParentTriggerScript); }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BTriggerVar* getVar(BTriggerVarSigID sigID) { BASSERT((sigID-1) < mEffectVars.getSize()); return (mEffectVars[sigID-1]); }

   void teTriggerActivate();
   void teTriggerDeactivate();
   void tePlaySound();
   void tePlaySoundV1();
   void tePlaySoundV2();
   void tePlayRelationSound();
   void tePlayRelationSoundV1();
   void tePlayRelationSoundV2();
   void teCreateObject();
   void teCreateObjectV5();
   void teCreateObjectV6();
   void teCreateSquad();  
   void teCreateSquadV6();
   void teCreateSquadV7();   
   void teKill();
   void teKillV3();
   void teKillV4();
   void teDestroy();
   void teDestroyV3();
   void teDestroyV4();
   void teInputUILocation();
   void teInputUILocationV3();
   void teInputUILocationV4();
   void teInputUILocationV5();
   void tePayCost();
   void teRefundCost();
   void teCountIncrement();
   void teCountDecrement();
   void teRandomLocation();   
   void teRandomLocationV3();
   void teRandomLocationV4();
   void teShutdown();
   void teLaunchProjectile();
   void teLaunchProjectileV1();
   void teLaunchProjectileV2();
   void teLaunchProjectileV3();
   void teLocationTieToGround();
   void teLocationAdjust();
   void teLocationAdjustV2();
   void teLocationAdjustV3();
   void teTechActivate();
   void teTechDeactivate();
   void teUnload();
   //void teUnloadV2();
   void teUnloadV3();
   void teUnloadV4();
   void teMove();
   void teMoveV6();
   void teTransportSquads();
   void teTransportSquadsV4();
   void teTransportSquadsV5();
   void teCarpetBomb();
   void teCarpetBombV3();
   void teCarpetBombV4();
   void teAttachmentAddType();
   void teAttachmentAddTypeV3();
   void teAttachmentAddTypeV5();
   void teAttachmentAddObject();
   void teAttachmentAddUnit();
   void teAttachmentRemoveAll();
   void teAttachmentRemoveAllV2();
   void teAttachmentRemoveAllV3();
   void teAttachmentRemoveObject();
   void teAttachmentRemoveUnit();
   void teUsePower();
   void teUsePowerV4();
   void teUsePowerV5();
   void teGetClosestPowerSquad();
   void teGetClosestPowerSquadV3();
   void teSetUIPowerRadius();
   void teAttachmentRemoveType();
   void teAttachmentRemoveTypeV2();
   void teAttachmentRemoveTypeV3();
   void teCopyTech();
   void teCopyTechStatus();
   void teCopyOperator();
   void teCopyProtoObject();
   void teCopyObjectType();
   void teCopyProtoSquad();
   void teCopySound();
   void teCopyEntity();
   void teCopyEntityList();
   void teCopyCost();
   void teCopyDistance();
   void teCopyTime();
   void teCopyPlayer();
   void teCopyCount();
   void teCopyLocation();
   void teCopyPercent();
   void teCopyHitpoints();
   void teCopyBool();
   void teCopyFloat();
   void teGetDistanceUnitUnit();
   void teGetDistanceUnitUnitV2();
   void teGetDistanceUnitLocation();
   void teGetDistanceUnitLocationV2();
   void teGetUnits();
   void teGetUnitsV3();
   void teGetSquads();
   void teGetSquadsV4();
   void teWork();
   void teWorkV3();
   void teWorkV4();
   void teIteratorPlayerList();
   void teIteratorTeamList();
   void teGetTeams();
   void teGetTeamPlayers();
   void tePlayerListAdd();
   void tePlayerListAddV2();
   void tePlayerListRemove();
   void tePlayerListRemoveV2();
   void teTeamListAdd();
   void teTeamListAddV2();
   void teTeamListRemove();
   void teTeamListRemoveV2();
   void teSetPlayerState();
   void teSetPlayerStateV2();
   void teFlareMinimapSpoof();
   void teFlareMinimapSpoofV2();
   void teFlareMinimapSpoofV3();
   void teObjectiveComplete();
   void teObjectiveCompleteV2();
   void teObjectiveUserMessage();   
   void teFlareMinimapNormal();
   void teFlareMinimapNormalV1();
   void teFlareMinimapNormalV2();
   void teChangeOwner();
   void teChangeOwnerV2();
   void teChangeOwnerV3();
   void teMKTest();
   void triggerCreateMilitaryPlan();   // Takes the bulk of code out of triggereffect.cpp
   void teCopyUnit();
   void teCopyUnitList();
   void teCopySquad();
   void teCopySquadList();
   void teUnitListGetSize();
   void teSquadListGetSize();
   void teUnitListAdd();
   void teSquadListAdd();
   void teUnitListRemove();
   void teSquadListRemove();
   void teIteratorUnitList();
   void teIteratorSquadList();
   void teCreateUnit();
   void teCreateUnitV1();
   void teCreateUnitV2();
   void tePlayAnimationUnit();
   void tePlayAnimationUnitV2();
   void tePlayAnimationSquad();
   void tePlayAnimationSquadV2();
   void teInputUIUnit();
   void teInputUIUnitV1();
   void teInputUIUnitV2();
   void teInputUISquad();
   void teInputUISquadV2();
   void teInputUISquadV3();
   void teTimeIncrement();
   void teTimeDecrement();
   void teTimeUserMessage();   
   void teTimeUserMessageV4();
   void teCopyColor();
   void teCopyString();
   void teTransform();
   void teGetPlayers();   
   void teGetPlayersV2();   
   void teUserMessage();
   void teUserMessageV3();
   void teGetHealth();
   void teGetHealthV2();
   void teGetHealthV3();
   void teRandomCount();
   void teMathCount();
   void teMathHitpoints();
   void teAsString();
   void teAsStringV1();
   void teAsStringV2();
   void teObjectiveDisplay();
   void teObjectiveDisplayV1();
   void teObjectiveDisplayV2();
   void teCalculatePercentCount();
   void teCalculatePercentHitpoints();
   void teCalculatePercentTime();
   void teCalculatePercentTimeV1();
   void teCalculatePercentTimeV2();
   void teLerpCount();
   void teLerpColor();
   void teGetLocation();
   void teGetLocationV1();
   void teGetLocationV2();
   void teMathPercent();
   void teGetOwner();
   void teDebugVarTech();
   void teDebugVarTechStatus();
   void teDebugVarOperator();
   void teDebugVarProtoObject();
   void teDebugVarObjectType();
   void teDebugVarProtoSquad();
   void teDebugVarSound();
   void teDebugVarDistance();
   void teDebugVarTime();
   void teDebugVarPlayer();
   void teDebugVarCount();
   void teDebugVarLocation();
   void teDebugVarUILocation();
   void teDebugVarCost();
   void teDebugVarAnimType();
   void teDebugVarPercent();
   void teDebugVarHitpoints();
   void teDebugVarPower();
   void teDebugVarBool();
   void teDebugVarFloat();
   void teDebugVarIterator();
   void teDebugVarTeam();
   void teDebugVarPlayerList();
   void teDebugVarTeamList();
   void teDebugVarPlayerState();
   void teDebugVarObjective();
   void teDebugVarUnit();
   void teDebugVarUnitList();
   void teDebugVarSquad();
   void teDebugVarUIUnit();
   void teDebugVarUISquad();
   void teDebugVarString();
   void teDebugVarColor();
   void teDebugVarProtoObjectList();
   void teDebugVarObjectTypeList();
   void teDebugVarProtoSquadList();
   void teDebugVarTechList();
   void teDebugVarMathOperator();
   void teModifyProtoData();
   void teModifyProtoDataV4();
   void teModifyProtoDataV5();
   void teGetPlayerCiv();
   void teGetIdleDuration();
   void teEnableAttackNotifications();
   void teLerpPercent();
   void teMathTime();
   void teGetGameTime();
   void teGetGameTimeRemaining();
   void teGetHitZoneHealth();
   void teSetHitZoneHealth();
   void teSetHitZoneActive();
   void teCopyMessageIndex();
   void teSetResources();
   void teGetResources();
   void teGetResourcesTotals();
   void teSetResourcesTotals();
   void teMathResources();
   void teIteratorObjectList();
   void teForbid();
   void teInvertBool();
   void teRevealer();
   void teRevealerV2();
   void teRevealerV3();
   void teMathDistance();
   void teGroupDeactivate();
   void teIteratorLocationList();
   void teLocationListAdd();
   void teLocationListRemove();
   void teLocationListGetSize();
   void teLerpLocation();
   void teMathLocation();
   void teSquadListPartition();
   void teSquadListShuffle();
   void teLocationListShuffle();
   void teEntityListShuffle();
   void tePlayerListShuffle();
   void teTeamListShuffle();
   void teUnitListShuffle();
   void teProtoObjectListShuffle();
   void teObjectTypeListShuffle();
   void teProtoSquadListShuffle();
   void teTechListShuffle();
   void teUnitListPartition();
   void teLocationListPartition();
   void teRefCountUnitAdd();
   void teRefCountUnitRemove();
   void teRefCountSquadAdd();
   void teRefCountSquadRemove();
   void teRepair();
   void teUnitFlagSet();
   void teGetChildUnits();
   void teDamage();
   void teUIUnlock();
   void teSquadListDiff();
   void teUnitListDiff();
   void teCombatDamage();
   void teCombatDamageV1();
   void teCombatDamageV2();
   void teEntityFilterClear();
   void teEntityFilterAddIsAlive();
   void teEntityFilterAddInList();
   void teEntityFilterAddPlayers();
   void teEntityFilterAddTeams();
   void teEntityFilterAddProtoObjects();
   void teEntityFilterAddProtoSquads();
   void teEntityFilterAddObjectTypes();
   void teEntityFilterAddDiplomacy();
   void teEntityFilterAddIsSelected();
   void teEntityFilterAddCanChangeOwner();
   void teEntityFilterAddJacking();
   void tePatherObstructionUpdates();
   void tePatherObstructionRebuild();
   void teUnitListFilter();
   void teSquadListFilter();
   void teEntityFilterAddRefCount();
   void teMathFloat();
   void teTeleport();
   void teTeleportV2();
   void teTeleportV3();
   void teEntityFilterAddIsIdle();
   void teAsFloat();
   void teAsFloatV2();
   void teAsFloatV3();
   void teSettle();
   void teCopyObjective();
   void teSetIgnoreUserInput();
   void teSetIgnoreUserInputV1();
   void teSetIgnoreUserInputV2();
   void teEnableFogOfWar();
   void teBidCreateBlank();
   void teBidCreateBlankV2();
   void teBidCreateBuilding();
   void teBidCreateBuildingV3();
   void teBidCreateTech();
   void teBidCreateTechV3();
   void teBidCreateSquad();
   void teBidCreateSquadV3();
   void teBidDelete();
   void teBidDeleteV2();
   void teBidSetBuilding();
   void teBidSetBuildingV2();
   void teBidSetTech();
   void teBidSetTechV2();
   void teBidSetSquad();
   void teBidSetSquadV2();
   void teBidClear();
   void teBidClearV2();
   void teBidSetPriority();
   void teBidSetPriorityV2();
   void teChangeSquadMode();
   void teChangeSquadModeV1();
   void teChangeSquadModeV2();
   void teGetAmmo();
   void teGetAmmoV1();
   void teGetAmmoV2();
   void teSetAmmo();
   void teSetAmmoV1();
   void teSetAmmoV2();
   void teBidPurchase();
   void teBidPurchaseV3();
   void teBidPurchaseV4();
   void teLaunchScript();
   void teLaunchScriptV4();
   void teGetPlayerTeam();
   void teModifyDataScalar();
   void teModifyDataScalarV1();
   void teModifyDataScalarV2();
   void teCloak();
   void teBlocker();
   void teSensorLock();
   void teDesignFindSphere();
   void teGetPlayers2();
   void teGetPlayers2V2();
   void tePlayersToTeams();
   void teTeamsToPlayers();
   void teReinforceSquad();
   void teReinforceSquadV1();
   void teReinforceSquadV2();
   void teMovePath();
   void teMovePathV3();
   void teIteratorKBBaseList();
   void teKBBaseGetDistance();
   void teKBBaseGetMass();
   void teKBBQReset();
   void teKBBQExecute();
   void teKBBQPointRadius();
   void teKBBQPlayerRelation();
   void teKBBQPlayerRelationV1();
   void teKBBQPlayerRelationV2();
   void teCopyKBBase();
   void tePowerGrant();
   void tePowerGrantV2();
   void tePowerGrantV3();
   void tePowerRevoke();
   void tePowerRevokeV2();
   void tePowerRevokeV3();
   void teGetSquadTrainerType();
   void teGetTechResearcherType();
   void teDesignLineGetPoints();
   void teEnableShield();
   void teKBBQMinStaleness();
   void teKBBQMaxStaleness();
   void teGetPlayerLeader();
   void teLaunchCinematic();
   void teLaunchCinematicV3();
   void teLaunchCinematicV4();
   void teCopyProtoObjectList();
   void teCopyProtoSquadList();
   void teCopyObjectTypeList();
   void teCopyTechList();
   void teIteratorProtoObjectList();
   void teIteratorProtoSquadList();
   void teIteratorObjectTypeList();
   void teIteratorTechList();   
   void teAsCount();
   void teGetDirection();
   void teGetDirectionV1();
   void teGetDirectionV2();
   void teGetDirectionFromLocations();
   void teCopyDirection();
   void teDebugDirection();
   void teSetDirection();          
   void teSetDirectionV1();          
   void teSetDirectionV2();          
   void teKBBQExecuteClosest();
   void teGetLegalSquads();
   void teGetLegalTechs();
   void teGetLegalBuildings();
   void teSetMobile();
   void teSetMobileV1();
   void teSetMobileV2();
   void teGetKBBaseLocation();
   void teGetDistanceLocationLocation();
   void tePlayWorldSoundAtPosition();
   void tePlayWorldSoundOnObject();
   void tePlayWorldSoundOnObjectV1();
   void tePlayWorldSoundOnObjectV2();
   void teRandomTime();
   void teGetParentSquad();
   void teCreateIconObject();
   void teCreateIconObjectV2();
   void teCreateIconObjectV3();
   void teSetFollowCam();
   void teEnableFollowCam();
   void teGetGarrisonedUnits();
   void teGetDifficulty();
   void teGetDifficultyV1();
   void teGetDifficultyV2();
   void teHUDToggle();
   void teInputUIButton();
   void teDebugVarUIButton();
   void teSetRenderTerrainSkirt();
   void teLightsetAnimate();
   void teSetOccluded();
   void teSetPosition();
   void teSetPositionV2();
   void teSetPositionV3();
   //XXXHalwes - 7/17/2007 - Added during E3, but not presently used.  May be used in the future.  Marked as dev only.
   void teSetFlagNearLayer();
   void teAIGenerateMissionTargets();
   void teAIScoreMissionTargets();
   void teAISortMissionTargets();
   void teAIMissionLaunch();
   void teAIMissionLaunchV1();
   void teAIMissionLaunchV2();
   void teAIMissionCancel();
   void teAIGetMissionTargets();
   void teAIGetMissionTargetsV1();
   void teAIGetMissionTargetsV2();
   void teAIGetMissionTargetsV3();
   void teAIGetMissionTargetsV4();
   void teLocationAdjustDir();
   void teBuildingCommand();
   void teBuildingCommandV3();
   void teBuildingCommandV4();
   void teAIMissionAddSquads();
   void teAIMissionRemoveSquads();
   void teAIMissionGetSquads();
   void teAIMissionGetSquadsV1();
   void teAIMissionGetSquadsV2();
   void teCloakDetected();
   void teAIMissionTargetGetScores();
   void teCopyIntegerList();
   void teAISetScoringParms();
   void teCountToInt();
   void teIntToCount();
   void teProtoObjectListAdd();
   void teProtoObjectListRemove();
   void teProtoSquadListAdd();
   void teProtoSquadListRemove();
   void teTechListAdd();
   void teTechListRemove();
   void teIntegerListAdd();
   void teIntegerListRemove();
   void teIntegerListGetSize();
   void teBidGetData();
   void teBidGetDataV1();
   void teBidGetDataV2();
   void teAITopicCreate();
   void teAITopicDestroy();
   void teAITopicModifyTickets();
   void teAIMissionModifyTickets();
   void tePlayAnimationObject();
   void tePlayAnimationObjectV2();
   void tePlayAnimationObjectV3();
   void teCopyObject();
   void teDebugVarBuildingCommandState();
   void teAIScnMissionAttackArea();
   void teAIScnMissionDefendArea();
   void teAIGetMissions();
   void teGetDeadUnitCount();
   void teGetDeadSquadCount();
   void teCopyLocationList();
   void teAIMissionGetLaunchScores();
   void teAIRemoveFromMissions();
   void teBreakpoint();
   void teGetPrimaryUser();
   void teGetBidsMatching();
   void teGetBidsMatchingV1();
   void teGetBidsMatchingV2();
   void teAIMissionGetTarget();
   void teBidSetTargetLocation();
   void teBidSetBuilder();
   void teAITopicLotto();
   void teEnableUserMessage();
   void teGetProtoSquad();
   void teGetProtoObject();
   void teGetProtoObjectV1();
   void teGetProtoObjectV2();
   void teCameraShake();
   void teCameraShakeV1();
   void teCameraShakeV2();
   void teCustomCommandAdd();
   void teCustomCommandAddV1();
   void teCustomCommandAddV2();
   void teCustomCommandRemove();
   void teConnectHitpointBar();
   void teAirStrike();
   void teGetPlayerPop();
   void teGetPlayerEconomy();
   void teGetPlayerScore();
   void teInputUISquadList();
   void teEntityFilterAddMaxObjectType();
   void teCreateTimer();
   void teCreateTimerV4();
   void teCreateTimerV5();
   void teDestroyTimer();
   void teKBSQInit();
   void teKBSQReset();
   void teKBSQExecute();
   void teAICalculations();
   void teAICalculations2();
   void teAIAnalyzeSquadList();
   void teAIAnalyzeOffenseAToB();
   void teGetMeanLocation();
   void teGetMeanLocationV1();
   void teGetMeanLocationV2();
   void teCopyObjectList();
   void teAISAGetComponent();
   void teAIAnalyzeKBSquadList();
   void teAIAnalyzeProtoSquadList();
   void teShowGarrisonedCount();
   void teShowCitizensSaved();
   void teSetCitizensSaved();
   void teSetCitizensSavedV1();
   void teSetCitizensSavedV2();
   void teCostToFloat();
   void teGetCost();
   void teAIUnopposedTimeToKill();
   void teGetPrimaryHealthComponent();
   void teShowObjectivePointer();
   void teShowObjectivePointerV4();
   void teShowObjectivePointerV5();

   void teObjectiveIncrementCounter();
   void teObjectiveDecrementCounter();
   void teObjectiveGetCurrentCounter();
   void teObjectiveGetFinalCounter();

   void teKBSquadFilterClear();
   void teKBSFAddCurrentlyVisible();
   void teKBSFAddObjectTypes();
   void teKBSFAddPlayers();
   void teKBSFAddInList();
   void teKBSFAddPlayerRelation();
   void teKBSFAddPlayerRelationV1();
   void teKBSFAddPlayerRelationV2();
   void teKBSquadListFilter();

   void teKBSFAddMinStaleness();
   void teKBSFAddMaxStaleness();
   void teKBSquadListGetSize();

   void teIteratorKBSquadList();
   void teKBSquadGetOwner();
   void teKBSquadGetLocation();
   void teConvertKBSquadsToSquads();

   void teKBSquadGetProtoSquad();

   void teKBBaseGetKBSquads();
   void teCopyKBSquad();

   void teKBSQExecuteClosest();

   void teKBSQPointRadius();
   void teKBSQPlayerRelation();
   void teKBSQPlayerRelationV1();
   void teKBSQPlayerRelationV2();
   void teKBSQObjectType();
   void teKBSQBase();
   void teKBSQMinStaleness();
   void teKBSQMaxStaleness();
   void teKBSQCurrentlyVisible();

   void teAISetFocus();
   void teAISetFocusV2();

   void teSetPlayableBounds();

   void teResetBlackMap();
   void teGetLOS();
   void teSetUnitAttackTarget();
   void teAsTime();
   void teRallyPointSet();
   void teRallyPointSetV1();
   void teRallyPointSetV2();
   void teRallyPointClear();   
   void teRallyPointClearV1();
   void teRallyPointClearV2();
   void teRallyPointGet();
   void teAIBindLog();
   void teAIBindLogV1();
   void teAIBindLogV2();
   void teAIBindLogV3();
   void tePlayChat();
   void tePlayChatV3();
   void tePlayChatV4();
   void teUITogglePowerOverlay();
   void teShowProgressBar();
   void teUpdateProgressBar();
   void teShowObjectCounter();
   void teUpdateObjectCounter();
   void teDebugVarSquadList();
   void teMegaTurretAttack();
   void teGetPop();
   void teGetPopV1();
   void teGetPopV2();
   void teRoundFloat();
   void tePowerClear();
   void tePowerInvoke();
   void tePowerInvokeV6();
   void tePowerInvokeV7();
   void teSetPlayerPop();
   void teCopyIconType();
   void teGetPlayerMilitaryStats();
   void teGetObjectiveStats();
   void teLerpTime();
   void teCombineString();
   void teTimeToFloat();
   void teBlockMinimap();
   void teAIChat();
   void teCopyChatSpeaker();
   void teBlockLeaderPowers();
   void teAIMissionTargetGetLocation();
   void teAIGetFlareAlerts();
   void teAIGetAttackAlerts();
   void teAIGetLastFlareAlert();
   void teAIGetLastAttackAlert();
   void teAIGetAlertData();
   void teAIGetAlertDataV1();
   void teAIGetAlertDataV2();
   void teGetGarrisonedSquads();
   void teTransferGarrisonedSquad();
   void teTransferGarrisoned();
   void teAISetBiases();
   void teRumbleStart();
   void teRumbleStartV1();
   void teRumbleStartV2();
   void teRumbleStartV3();
   void teRumbleStartV4();
   void teRumbleStop();
   void teBidQuery();
   void teBidQueryV1();
   void teBidQueryV2();
   void teInputUIPlaceSquads();
   void teGetNumTransports();
   void teGetNumTransportsV1();
   void teGetNumTransportsV2();
   void teSetOverrideTint();
   void teEnableOverrideTint();
   void teSetTransportPickUpLocations();
   void teTimerSet();
   void teTimerGet();
   void teTimerSetPaused();
   void teAITopicPriorityRequest();
   void teSetResourceHandicap();
   void tePlayerSelectSquads();
   void teParkingLotSet();
   void teUnpack();
   void teTableLoad();
   void teHintMessageShow();
   void teHintGlowToggle();
   void teHintGlowToggleV2();
   void teHintGlowToggleV3();
   void teHintCalloutCreate();
   void teHintCalloutDestroy();
   void teEventSetFilter();
   void teEventSubscribe();
   void teEventSubscribeV1();
   void teEventSubscribeV2();
   void teHintMessageDestroy();
   void teHintMessageDestroyV1();
   void teHintMessageDestroyV2();
   void teGetPlayerColor();
   void teCopyControlType();
   void teCopyLocStringID();
   void teSetLevel();
   void teGetLevel();
   void teKBSquadListDiff();
   void teModifyProtoSquadData();
   void teSetScenarioScore();
   void teSetScenarioScore1();
   void teSetScenarioScore2();
   void teSetScenarioScoreInfo();
   void teCreateObstructionUnit();
   void tePlayVideo();
   void teObjectListRemove();
   void teEventReset();
   void teEventFilterCamera();
   void teEventFilterCameraV1();
   void teEventFilterCameraV2();
   void teEventFilterEntity();
   void teEventFilterEntityList();
   void teEnableChats();
   void teObjectListAdd();
   void teObjectListGetSize();
   void teClearCorpseUnits();
   void teAddXP();
   void teEventFilterGameState();
   void teEventFilterType();
   void teEventFilterNumeric();
   void teBidSetBlockedBuilders();
   void teGetBuildingTrainQueue();
   void teAIRegisterHook();
   void teGetClosestSquad();
   //void teMoveToFace();
   void teCopyTimeList();
   void teTimeListAdd();
   void teTimeListRemove();
   void teTimeListRemoveV2();
   void teTimeListGetSize();
   void teClearBlackMap();
   void teCopyDesignLineList();
   void teDesignLineListAdd();
   void teDesignLineListRemove();
   void teDesignLineListGetSize();
   void teBidSetQueueLimits();
   void teCopyDesignLine();
   void teProtoSquadListGetSize();
   void teGetObstructionRadius();
   void teGetObstructionRadiusV2();
   void teGetObstructionRadiusV3();
   void teCreateSquads();
   void teCopyLOSType();
   void teAISetAssetMultipliers();
   void teAIGetOpportunityRequests();
   void teAIGetTerminalMissions();
   void teAIClearOpportunityRequests();
   void teAITopicSetFocus();
   void teSetCamera();
   void teSetCameraV2();
   void teSetCameraV3();
   void teSetCameraV4();
   void teCopyFloatList();
   void teFloatListAdd();
   void teFloatListRemove();
   void teFloatListGetSize();
   void teGetUTechBldings();
   void teRepairByCombatValue();
   void teRepairByCombatValueV1();
   void teRepairByCombatValueV2();
   void teInputUILocationMinigame();
   void teAIQueryMissionTargets();
   void teGetClosestPath();
   void teSetSelectable();
   void teSetTrickleRate();
   void teGetTrickleRate();
   void teChatDestroy();
   void teShowMessage();
   void teFadeToColor();
   void teSquadFlagSet();
   void teAICalculateOffenseRatioAToB();
   void teSetAutoAttackable();
   void teCopyAISquadAnalysis();
   void teFadeTransition();
   void teFlashUIElement();
   void teFlashUIElementV1();
   void teFlashUIElementV2();
   void teTeamSetDiplomacy();
   void teConceptGetParameters();
   void teConceptSetttings();
   void teConceptPermission();
   void teConceptSetState();
   void teConceptSetPrecondition();
   void teConceptStartSub();
   void teConceptClearSub();
   void teConceptSetParameters();
   void teClearBuildingCommandState();
   void teGetNPCPlayersByName();
   void teGetClosestUnit();
   void teActivateTentacle();
   void teRecycleBuilding();
   void teAIGetMemory();
   void teBidCreatePower();
   void teBidSetPower();
   void teBidAddToMissions();
   void teBidRemoveFromMissions();
   void tePowerUsed();
   void teSetTeleporterDestination();
   void teSetGarrisonedCount();
   void tePlayerListGetSize();
   void teConceptResetCooldown();
   void teGetPowerRadius();
   void teTransportSquads2();
   void teLocationListGetByIndex();
   void teLocationListGetClosest();
   void teAIMissionSetMoveAttack();
   void teInfect();
   void teKBAddSquadsToKB();
   void teBidSetPadSupplies();
   void teEventDelete();
   void teEventClearFilters();
   void teAIFactoidSubmit();
   void teFlashEntity();
   void teSetTowerWallDestination();
   void teMissionResult();
   void teAICreateAreaTarget();
   void teAIMissionCreate();
   void teEnableMusicManager();
   void teResetDopple();
   void teAICreateWrapper();
   void teAIReorderWrapper();
   void teAIDestroyWrapper();
   void teAIWrapperModifyRadius();
   void teAIWrapperModifyFlags();
   void teAIWrapperModifyParms();
   void teSaveGame();
   void teLoadGame();
   void teReverseHotDrop();
   void teEventSubscribeUseCount();
   void teObjectTypeToProtoObjects();
   void teDebugVarDesignLine();
   void teShowInfoDialog();
   void tePowerUserShutdown();
   void tePowerToInt();
   void teIntToPower();
   void teChangeControlledPlayer();
   void teGrantAchievement();
   void teAIAddTeleporterZone();
   void teResetAbilityTimer();
   void teCustomCommandExecute();
   void teIgnoreDpad();
   void teAISetPlayerAssetModifier();
   void teEnableLetterBox();
   void teEnableScreenBlur();
   void teAISetPlayerDamageModifiers();
   void teSetPowerAvailableTime();
   void tePowerMenuEnable();
   void teSetMinimapNorthPointerRotation();
   void teHideCircleMenu();
   void teChatForceSubtitles();
   void teAISetPlayerMultipliers();
   void teAIMissionSetFlags();
   void teSetMinimapSkirtMirroring();
   void teAISetWinRange();
   void teGetPopularSquadType();
   void teTeleportUnitsOffObstruction();
   void teLockPlayerUser();
   void teGetGameMode();
   void teAISetPlayerBuildSpeedModifiers();

   // NEWTRIGGEREFFECT
   // Add your trigger effect function header here.
   // Format is void teYOUREFFECTNAME();
   // If you add new versions of an effect, the original function teYOUREFFECTNAME() will be the switch statement,
   // which calls a separate function for each version that is supported, named tcYOUREFFECTNAMEV<n>(); where <n> is the version number.
   // Also, make sure to keep them in order and together in this list for easier bookkeeping.
   // For example:
   // tcYOUREFFECTNAME();
   // tcYOUREFFECTNAMEV1();
   // tcYOUREFFECTNAMEV2();
   // tcANOTHERONE();
   // tcANOTHERONEV1();
   // tcANOTHERONEV2();
   // AND so on...
   // Any questions?  Ask Marc.

   static float calculateValuesFloat(float val1, long op, float val2);
   static long  calculateValuesLong(long val1, long op, long val2);
   static DWORD calculateValuesDWORD(DWORD val1, long op, DWORD val2);
   static BCost calculateValuesCost(BCost &cost1, long op, BCost &cost2);
   static BVector calculateValuesVector(BVector val1, long op, BVector val2);


   // Trigger effect helper functions
   void refCountAddHelper(BEntityID entityID, short refCountType, short& maxCount);
   void refCountRemoveHelper(BEntityID entityID, short refCountType, short& maxCount);
   void unitFlagSetHelper(BEntityID unitID, long flagType, bool flagValue);
   void squadFlagSetHelper(BEntityID squadID, long flagType, bool flagValue);
                                                      // 17 bytes total
   BSmallDynamicSimArray<BTriggerVar*> mEffectVars;   // 8 bytes
   BTriggerScript *mpParentTriggerScript;             // 4 bytes
   long mDBID;                                        // 4 bytes
   BUInt8<uint, UINT8_MIN, UINT8_MAX> mVersion;       // 1 byte
};

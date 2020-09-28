//==============================================================================
// triggereffectcopy.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "simtypes.h"
#include "triggereffect.h"
#include "triggermanager.h"


//==============================================================================
// BTriggerEffect::teCopyTech()
//==============================================================================
void BTriggerEffect::teCopyTech()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long techID = getVar(cInputSource)->asTech()->readVar();
   getVar(cOutputCopy)->asTech()->writeVar(techID);
}


//==============================================================================
// BTriggerEffect::teCopyTechStatus()
//==============================================================================
void BTriggerEffect::teCopyTechStatus()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long techStatus = getVar(cInputSource)->asTechStatus()->readVar();
   getVar(cOutputCopy)->asTechStatus()->writeVar(techStatus);
}


//==============================================================================
// BTriggerEffect::teCopyOperator()
//==============================================================================
void BTriggerEffect::teCopyOperator()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long op = getVar(cInputSource)->asOperator()->readVar();
   getVar(cOutputCopy)->asOperator()->writeVar(op);
}


//==============================================================================
// BTriggerEffect::teCopyProtoObject()
//==============================================================================
void BTriggerEffect::teCopyProtoObject()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long protoObjectID = getVar(cInputSource)->asProtoObject()->readVar();
   getVar(cOutputCopy)->asProtoObject()->writeVar(protoObjectID);
}


//==============================================================================
// BTriggerEffect::teCopyObjectType()
//==============================================================================
void BTriggerEffect::teCopyObjectType()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long objectType = getVar(cInputSource)->asObjectType()->readVar();
   getVar(cOutputCopy)->asObjectType()->writeVar(objectType);
}


//==============================================================================
// BTriggerEffect::teCopyProtoSquad()
//==============================================================================
void BTriggerEffect::teCopyProtoSquad()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long protoSquadID = getVar(cInputSource)->asProtoSquad()->readVar();
   getVar(cOutputCopy)->asProtoSquad()->writeVar(protoSquadID);
}


//==============================================================================
// BTriggerEffect::teCopySound()
//==============================================================================
void BTriggerEffect::teCopySound()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   const BSimString &sound = getVar(cInputSource)->asSound()->readVar();
   getVar(cOutputCopy)->asSound()->writeVar(sound);
}


//==============================================================================
// BTriggerEffect::teCopyEntity()
//==============================================================================
void BTriggerEffect::teCopyEntity()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   BEntityID entityID = getVar(cInputSource)->asEntity()->readVar();
   getVar(cOutputCopy)->asEntity()->writeVar(entityID);
}


//==============================================================================
// BTriggerEffect::teCopyEntityList()
//==============================================================================
void BTriggerEffect::teCopyEntityList()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   const BEntityIDArray& entityList = getVar(cInputSource)->asEntityList()->readVar();
   getVar(cOutputCopy)->asEntityList()->writeVar(entityList);
}


//==============================================================================
// BTriggerEffect::teCopyCost()
//==============================================================================
void BTriggerEffect::teCopyCost()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   BCost cost = getVar(cInputSource)->asCost()->readVar();
   getVar(cOutputCopy)->asCost()->writeVar(cost);
}


//==============================================================================
// BTriggerEffect::teCopyDistance()
//==============================================================================
void BTriggerEffect::teCopyDistance()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   float distance = getVar(cInputSource)->asFloat()->readVar();
   getVar(cOutputCopy)->asFloat()->writeVar(distance);
}


//==============================================================================
// BTriggerEffect::teCopyTime()
//==============================================================================
void BTriggerEffect::teCopyTime()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   DWORD time = getVar(cInputSource)->asTime()->readVar();
   getVar(cOutputCopy)->asTime()->writeVar(time);
}


//==============================================================================
// BTriggerEffect::teCopyPlayer()
//==============================================================================
void BTriggerEffect::teCopyPlayer()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long playerID = getVar(cInputSource)->asPlayer()->readVar();
   getVar(cOutputCopy)->asPlayer()->writeVar(playerID);
}


//==============================================================================
// BTriggerEffect::teCopyCount()
//==============================================================================
void BTriggerEffect::teCopyCount()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long count = getVar(cInputSource)->asInteger()->readVar();
   getVar(cOutputCopy)->asInteger()->writeVar(count);
}


//==============================================================================
// BTriggerEffect::teCopyLocation()
//==============================================================================
void BTriggerEffect::teCopyLocation()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   BVector location = getVar(cInputSource)->asVector()->readVar();
   getVar(cOutputCopy)->asVector()->writeVar(location);
}


//==============================================================================
// BTriggerEffect::teCopyPercent()
//==============================================================================
void BTriggerEffect::teCopyPercent()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   float percent = getVar(cInputSource)->asFloat()->readVar();
   getVar(cOutputCopy)->asFloat()->writeVar(percent);
}


//==============================================================================
// BTriggerEffect::teCopyHitpoints()
//==============================================================================
void BTriggerEffect::teCopyHitpoints()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   float hitpoints = getVar(cInputSource)->asFloat()->readVar();
   getVar(cOutputCopy)->asFloat()->writeVar(hitpoints);
}


//==============================================================================
// BTriggerEffect::teCopyBool()
//==============================================================================
void BTriggerEffect::teCopyBool()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   bool bVal = getVar(cInputSource)->asBool()->readVar();
   getVar(cOutputCopy)->asBool()->writeVar(bVal);
}


//==============================================================================
// BTriggerEffect::teCopyFloat()
//==============================================================================
void BTriggerEffect::teCopyFloat()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   float fVal = getVar(cInputSource)->asFloat()->readVar();
   getVar(cOutputCopy)->asFloat()->writeVar(fVal);
}


//==============================================================================
// BTriggerEffect::teCopyColor()
//==============================================================================
void BTriggerEffect::teCopyColor()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));

   BColor input = getVar(cInputSource)->asColor()->readVar();
   getVar(cOutputCopy)->asColor()->writeVar(input);
}


//==============================================================================
// BTriggerEffect::teCopyString()
//==============================================================================
void BTriggerEffect::teCopyString()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));

   BSimUString input = getVar(cInputSource)->asString()->readVar();
   getVar(cOutputCopy)->asString()->writeVar(input);
}


//==============================================================================
// BTriggerEffect::teCopyUnit()
//==============================================================================
void BTriggerEffect::teCopyUnit()
{
   enum { cUnitSource = 1, cUnitCopy = 2, };
   BTRIGGER_ASSERT(getVar(cUnitSource) != getVar(cUnitCopy));
   BEntityID unitID = getVar(cUnitSource)->asUnit()->readVar();
   getVar(cUnitCopy)->asUnit()->writeVar(unitID);
}


//==============================================================================
// BTriggerEffect::teCopyUnitList()
//==============================================================================
void BTriggerEffect::teCopyUnitList()
{
   enum { cUnitListSource = 1, cUnitListCopy = 2, };
   BTRIGGER_ASSERT(getVar(cUnitListSource) != getVar(cUnitListCopy));
   const BEntityIDArray& unitList = getVar(cUnitListSource)->asUnitList()->readVar();
   getVar(cUnitListCopy)->asUnitList()->writeVar(unitList);
}


//==============================================================================
// BTriggerEffect::teCopySquad()
//==============================================================================
void BTriggerEffect::teCopySquad()
{
   enum { cSquadSource = 1, cSquadCopy = 2, };
   BTRIGGER_ASSERT(getVar(cSquadSource) != getVar(cSquadCopy));
   BEntityID squadID = getVar(cSquadSource)->asSquad()->readVar();
   getVar(cSquadCopy)->asSquad()->writeVar(squadID);
}


//==============================================================================
// BTriggerEffect::teCopySquadList()
//==============================================================================
void BTriggerEffect::teCopySquadList()
{
   enum { cSquadListSource = 1, cSquadListCopy = 2, };
   BTRIGGER_ASSERT(getVar(cSquadListSource) != getVar(cSquadListCopy));
   const BEntityIDArray& squadList = getVar(cSquadListSource)->asSquadList()->readVar();
   getVar(cSquadListCopy)->asSquadList()->writeVar(squadList);
}


//XXXHalwes - 7/17/2007 - Invalid?
//=============================================================================
// BTriggerEffect::teSetHitZoneActive
//=============================================================================
void BTriggerEffect::teCopyMessageIndex()
{
   enum
   {
      cInput  = 1,
      cOutput = 2
   };
   BTRIGGER_ASSERT(getVar(cInput) != getVar(cOutput));

   long input = getVar(cInput)->asMessageIndex()->readVar();
   getVar(cOutput)->asMessageIndex()->writeVar(input);
}


//==============================================================================
// BTriggerEffect::teCopyObjective()
//==============================================================================
void BTriggerEffect::teCopyObjective()
{
   enum 
   { 
      cInputSource = 1, 
      cOutputCopy  = 2, 
   };

   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));

   long val = getVar(cInputSource)->asObjective()->readVar();
   getVar(cOutputCopy)->asObjective()->writeVar(val);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyKBBase()
{
   enum { cInputKBBase = 1, cOutputKBBase = 2, };
   BTRIGGER_ASSERT(getVar(cInputKBBase) != getVar(cOutputKBBase));
   BKBBaseID kbBaseID = getVar(cInputKBBase)->asKBBase()->readVar();
   getVar(cOutputKBBase)->asKBBase()->writeVar(kbBaseID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyProtoObjectList()
{
   enum { cSource = 1, cCopy = 2, };
   BTriggerVarProtoObjectList* pSrc = getVar(cSource)->asProtoObjectList();
   BTriggerVarProtoObjectList* pCopy = getVar(cCopy)->asProtoObjectList();
   BTRIGGER_ASSERT(pSrc != pCopy);
   if (pSrc != pCopy)
      pCopy->writeVar(pSrc->readVar());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyProtoSquadList()
{
   enum { cSource = 1, cCopy = 2, };
   BTriggerVarProtoSquadList* pSrc = getVar(cSource)->asProtoSquadList();
   BTriggerVarProtoSquadList* pCopy = getVar(cCopy)->asProtoSquadList();
   BTRIGGER_ASSERT(pSrc != pCopy);
   if (pSrc != pCopy)
      pCopy->writeVar(pSrc->readVar());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyObjectTypeList()
{
   enum { cSource = 1, cCopy = 2, };
//-- FIXING PREFIX BUG ID 2807
   const BTriggerVarObjectTypeList* pSrc = getVar(cSource)->asObjectTypeList();
//--
   BTriggerVarObjectTypeList* pCopy = getVar(cCopy)->asObjectTypeList();
   BTRIGGER_ASSERT(pSrc != pCopy);
   if (pSrc != pCopy)
      pCopy->writeVar(pSrc->readVar());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyTechList()
{
   enum { cSource = 1, cCopy = 2, };
   BTriggerVarTechList* pSrc = getVar(cSource)->asTechList();
   BTriggerVarTechList* pCopy = getVar(cCopy)->asTechList();
   BTRIGGER_ASSERT(pSrc != pCopy);
   if (pSrc != pCopy)
      pCopy->writeVar(pSrc->readVar());
}


//==============================================================================
// Copies direction value between variables
//==============================================================================
void BTriggerEffect::teCopyDirection()
{
   enum 
   { 
      cInputSource = 1, 
      cOutputCopy = 2, 
   };

   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   BVector dir = getVar(cInputSource)->asVector()->readVar();
   dir.normalize();
   getVar(cOutputCopy)->asVector()->writeVar(dir);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyIntegerList()
{
   enum { cSrcList = 1, cDestList = 2, };
   BTRIGGER_ASSERT(getVar(cSrcList) != getVar(cDestList));
   const BInt32Array& srcList = getVar(cSrcList)->asIntegerList()->readVar();
   getVar(cDestList)->asIntegerList()->writeVar(srcList);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyObject()
{
   enum 
   { 
      cObjectSource = 1, 
      cObjectCopy = 2, 
   };

   BTRIGGER_ASSERT(getVar(cObjectSource) != getVar(cObjectCopy));
   BEntityID objectID = getVar(cObjectSource)->asObject()->readVar();
   getVar(cObjectCopy)->asObject()->writeVar(objectID);
}


//==============================================================================
// Copy list of locations to a destination list of locations
//==============================================================================
void BTriggerEffect::teCopyLocationList()
{
   enum 
   { 
      cInputSource = 1, 
      cOutputCopy = 2, 
   };

   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
//-- FIXING PREFIX BUG ID 2808
   const BVectorArray& locationList = getVar(cInputSource)->asVectorList()->readVar();
//--
   getVar(cOutputCopy)->asVectorList()->writeVar(locationList);   
}


//==============================================================================
// Copy a list of objects
//==============================================================================
void BTriggerEffect::teCopyObjectList()
{
   enum 
   { 
      cObjectListSource = 1, 
      cObjectListCopy = 2, 
   };

   BTRIGGER_ASSERT(getVar(cObjectListSource) != getVar(cObjectListCopy));
   const BEntityIDArray& objectList = getVar(cObjectListSource)->asObjectList()->readVar();
   getVar(cObjectListCopy)->asObjectList()->writeVar(objectList);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyKBSquad()
{
   enum { cInputKBSquad = 1, cOutputKBSquad = 2, };
   BTRIGGER_ASSERT(getVar(cInputKBSquad) != getVar(cOutputKBSquad));
   BKBSquadID KBSquadID = getVar(cInputKBSquad)->asKBSquad()->readVar();
   getVar(cOutputKBSquad)->asKBSquad()->writeVar(KBSquadID);
}

//==============================================================================
// Copy the value of the icon type
//==============================================================================
void BTriggerEffect::teCopyIconType()
{
   enum 
   { 
      cInputSource = 1, 
      cOutputCopy = 3, 
   };

   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   int iconType = getVar(cInputSource)->asIconType()->readVar();
   getVar(cOutputCopy)->asIconType()->writeVar(iconType);
}


//==============================================================================
// BTriggerEffect::teCopyChatSpeaker()
//==============================================================================
void BTriggerEffect::teCopyChatSpeaker()
{
   enum { cInputSource = 1, cOutputCopy = 2, };
   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long val = getVar(cInputSource)->asChatSpeaker()->readVar();
   getVar(cOutputCopy)->asChatSpeaker()->writeVar(val);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyControlType()
{
   enum 
   { 
      cInputSource = 1, 
      cOutputCopy = 2, 
   };

   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   int controlid = getVar(cInputSource)->asControlType()->readVar();
   getVar(cOutputCopy)->asControlType()->writeVar(controlid);   
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCopyLocStringID()
{
   enum 
   { 
      cInputSource = 1, 
      cOutputCopy = 2, 
   };

   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   uint32 stringid = getVar(cInputSource)->asLocStringID()->readVar();
   getVar(cOutputCopy)->asLocStringID()->writeVar(stringid);   
}


//==============================================================================
// Copy a time list
//==============================================================================
void BTriggerEffect::teCopyTimeList()
{
   enum 
   { 
      cSrcList = 1, 
      cDestList = 2, 
   };

   BTRIGGER_ASSERT(getVar(cSrcList) != getVar(cDestList));
   const BDWORDArray& srcList = getVar(cSrcList)->asTimeList()->readVar();
   getVar(cDestList)->asTimeList()->writeVar(srcList);
}

//==============================================================================
// Copy the values of a design line list to another one
//==============================================================================
void BTriggerEffect::teCopyDesignLineList()
{
   enum 
   { 
      cSrcList = 1, 
      cDestList = 2, 
   };

   BTRIGGER_ASSERT(getVar(cSrcList) != getVar(cDestList));
   const BDesignLineIDArray& srcList = getVar(cSrcList)->asDesignLineList()->readVar();
   getVar(cDestList)->asDesignLineList()->writeVar(srcList);
}


//==============================================================================
// Copy design line value to another design line variable
//==============================================================================
void BTriggerEffect::teCopyDesignLine()
{
   enum 
   { 
      cSrcLine = 1, 
      cDestLine = 2, 
   };

   BTRIGGER_ASSERT(getVar(cSrcLine) != getVar(cDestLine));
   BDesignLineID srcLine = getVar(cSrcLine)->asDesignLine()->readVar();
   getVar(cDestLine)->asDesignLine()->writeVar(srcLine);
}


//==============================================================================
// Copy a LOS type value from one variable to another
//==============================================================================
void BTriggerEffect::teCopyLOSType()
{
   enum 
   { 
      cInputSource = 1, 
      cOutputCopy = 2, 
   };

   BTRIGGER_ASSERT(getVar(cInputSource) != getVar(cOutputCopy));
   long losType = getVar(cInputSource)->asLOSType()->readVar();
   getVar(cOutputCopy)->asLOSType()->writeVar(losType);
}


//==============================================================================
// Copy the values of one float list to another
//==============================================================================
void BTriggerEffect::teCopyFloatList()
{
   enum 
   { 
      cSrcList = 1, 
      cDestList = 2, 
   };

   BTRIGGER_ASSERT(getVar(cSrcList) != getVar(cDestList));
   const BFloatArray& srcList = getVar(cSrcList)->asFloatList()->readVar();
   getVar(cDestList)->asFloatList()->writeVar(srcList);
}


//==============================================================================
// Copy squad analysis
//==============================================================================
void BTriggerEffect::teCopyAISquadAnalysis()
{
   enum 
   { 
      cAISquadAnalysisSource = 1, 
      cAISquadAnalysisCopy = 2, 
   };
   BTRIGGER_ASSERT(getVar(cAISquadAnalysisSource) != getVar(cAISquadAnalysisCopy));
   const BAISquadAnalysis& squadAnalysis = getVar(cAISquadAnalysisSource)->asAISquadAnalysis()->readVar();   
   getVar(cAISquadAnalysisCopy)->asAISquadAnalysis()->writeVar(squadAnalysis);
}
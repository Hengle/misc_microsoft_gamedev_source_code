//==============================================================================
// techeffect.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "techeffect.h"
#include "database.h"
#include "player.h"
#include "protoobject.h"
#include "protopower.h"
#include "protosquad.h"
#include "prototech.h"
#include "tactic.h"
#include "techtree.h"
#include "unit.h"
#include "visiblemap.h"
#include "world.h"
#include "xmlreader.h"
#include "syncmacros.h"
#include "entityactionidle.h"
#include "weapontype.h"
#include "usermanager.h"
#include "user.h"
#include "selectionmanager.h"
#include "game.h"
#include "uimanager.h"
#include "team.h"
#include "ability.h"
#include "SimOrderManager.h"
#include "SquadActionWork.h"
#include "actionmanager.h"
#include "configsgame.h"
#include "simhelper.h"
#include "squadactionplayblockinganimation.h"

//==============================================================================
// BTechEffect::BTechEffect
//==============================================================================
BTechEffect::BTechEffect() :
   mEffectType(-1),
   mEffectValue(-1),
   mAllActions(false),
   mActionName(),
   mDataType(-1),
   mResource(-1),
   mObjectType(-1),
   mAmount(0.0f),
   mRelativity(-1),
   mTargets()
{
}

//==============================================================================
// BTechEffect::~BTechEffect
//==============================================================================
BTechEffect::~BTechEffect()
{
}

//==============================================================================
// BTechEffect::load
//==============================================================================
bool BTechEffect::load(BXMLNode root)
{
   BSimString effectTypeName;
   if (!root.getAttribValue("Type", &effectTypeName))
      return (false);

   BSimString tempStr;
   BSimString tempHardpointStr;
   if (effectTypeName == "Data")
   {
      mEffectType = cEffectData;

      mAllActions = (root.getAttribute("AllActions").getValid());
      root.getAttribValueAsString("Action", mActionName);

      BSimString  subTypeName;
      if (root.getAttribValue("SubType", &subTypeName))
      {
         mDataType = gDatabase.getProtoObjectDataType(subTypeName);
      }

      root.getAttribValueAsFloat("Amount", mAmount);

      BSimString relativityName;
      if (root.getAttribValue("Relativity", &relativityName))
      {
         mRelativity = gDatabase.getProtoObjectDataRelativity(relativityName);
      }

      if ((mDataType == cDataCommandEnable) || (mDataType == cDataCommandSelectable))
      {
         BSimString commandType;
         if (root.getAttribValue("CommandType", &commandType))
            mResource = gDatabase.getProtoObjectCommandType(commandType);

         BSimString commandData;
         if (root.getAttribValue("CommandData", &commandData))
         {
            switch (mResource)
            {
               case BProtoObjectCommand::cTypeTrainSquad : mObjectType = gDatabase.getProtoSquad(commandData); break;
               case BProtoObjectCommand::cTypeTrainUnit  : mObjectType = gDatabase.getProtoObject(commandData); break;
               case BProtoObjectCommand::cTypeBuild      : mObjectType = gDatabase.getProtoObject(commandData); break;
               case BProtoObjectCommand::cTypeResearch   : mObjectType = gDatabase.getProtoTech(commandData); break;
               case BProtoObjectCommand::cTypeAbility    : mObjectType = gDatabase.getAbilityIDFromName(commandData); break;
               case BProtoObjectCommand::cTypeChangeMode : mObjectType = gDatabase.getSquadMode(commandData); break;
               case BProtoObjectCommand::cTypePower      : mObjectType = gDatabase.getProtoPowerIDByName(commandData); break;
               case BProtoObjectCommand::cTypeBuildOther : mObjectType = gDatabase.getProtoObject(commandData); break;
            }
         }
      }
      else if (mDataType == cDataRateAmount || mDataType == cDataRateMultiplier)
      {
         BSimString rateName;
         if (root.getAttribValue("Rate", &rateName))
            mResource = gDatabase.getRate(rateName);
      }
      else if (mDataType == cDataDamageModifier)
      {
         BSimString weaponTypeName;
         if (root.getAttribValue("WeaponType", &weaponTypeName))
            mResource = gDatabase.getWeaponTypeIDByName(weaponTypeName);

         BSimString damageTypeName;
         if (root.getAttribValue("DamageType", &damageTypeName))
            mObjectType = gDatabase.getDamageType(damageTypeName);
      }
      else if (mDataType == cDataPopCap || mDataType == cDataPopMax)
      {
         BSimString popTypeName;
         if (root.getAttribValue("PopType", &popTypeName))
            mResource = gDatabase.getPop(popTypeName);
      }
      else if (mDataType == cDataUnitTrainLimit)
      {
         BSimString unitTypeName;
         if (root.getAttribValue("UnitType", &unitTypeName))
            mObjectType = gDatabase.getProtoObject(unitTypeName);
      }
      else if (mDataType == cDataSquadTrainLimit)
      {
         BSimString squadTypeName;
         if (root.getAttribValue("SquadType", &squadTypeName))
            mObjectType = gDatabase.getProtoSquad(squadTypeName);
      }
      else if (mDataType == cDataPowerRechargeTime)
      {
         BSimString powerName;
         if (root.getAttribValue("Power", &powerName))
            mEffectValue = gDatabase.getProtoPowerIDByName(powerName);
      }
      else if (mDataType == cDataPowerUseLimit)
      {
         BSimString powerName;
         if (root.getAttribValue("Power", &powerName))
            mEffectValue = gDatabase.getProtoPowerIDByName(powerName);
      }
      else if (mDataType == cDataPowerLevel)
      {
         BSimString powerName;
         if (root.getAttribValue("Power", &powerName))
            mEffectValue = gDatabase.getProtoPowerIDByName(powerName);
      }
      else if (mDataType == cDataImpactEffect)
      {
         BSimString effectName;
         if (root.getAttribValue("ImpactEffect", &effectName))
            mObjectType = gDatabase.getProtoImpactEffectIndex(effectName);
      }
      else if (mDataType == cDataDisplayNameID)
      {
         long stringID=-1;
         if (root.getAttribValueAsLong("StringID", stringID))
            mObjectType = gDatabase.getLocStringIndex(stringID);
      }
      else if (mDataType == cDataIcon)
      {
         BSimString iconType;
         if (root.getAttribValue("IconType", &iconType))
         {
            if (iconType == "unit")
               mObjectType = 0;
            else if (iconType == "building")
               mObjectType = 1;
            else if (iconType == "misc")
               mObjectType = 2;
            else if (iconType == "tech")
               mObjectType = 3;
         }
         root.getAttribValueAsString("IconName", mActionName);
      }
      else if (mDataType == cDataTurretYawRate || mDataType == cDataTurretPitchRate)
      {
         //-- Load the hardpoint ID
         BSimString hardpointName;
         if(root.getAttribValue("Hardpoint", &hardpointName))         
            tempHardpointStr = hardpointName;                     

         //-- Modify the amount to radians
         mAmount *= cRadiansPerDegree;         
      }
      else if (mDataType == cDataAbilityRecoverTime)
      {
         BSimString abilityName;
         if (root.getAttribValue("Ability", &abilityName))
            mResource = gDatabase.getAbilityIDFromName(abilityName);
      }
      else if (mDataType == cDataTechHPBar)
      {
         BSimString hpbarName;

         if (root.getAttribValue("hpbar", &hpbarName))
            mObjectType = gDatabase.getProtoHPBarID(hpbarName);
      }
      else if (mDataType == cDataDeathSpawn)
      {
         BSimString squadName;
         if (root.getAttribValue("squadName", &squadName))
            mObjectType = gDatabase.getProtoSquad(squadName);
      }
      else
      {
         BSimString resourceName;
         if (root.getAttribValue("Resource", &resourceName))
            mResource = gDatabase.getResource(resourceName);

         BSimString unitTypeName;
         if (root.getAttribValue("UnitType", &unitTypeName))
            mObjectType = gDatabase.getObjectType(unitTypeName);
      }

      getTargets(root);

      //-- Lookup the hardpointID now that we know our target
      if (mDataType == cDataTurretYawRate || mDataType == cDataTurretPitchRate)
      {
         BASSERT(mTargets.getSize() == 1);
         BASSERT(mTargets[0].mTargetType == cTargetProtoUnit);         

         BProtoObjectID protoID = mTargets[0].mTargetID;;
//-- FIXING PREFIX BUG ID 2333
         const BProtoObject* pProto = gDatabase.getGenericProtoObject(protoID);            
//--
         mEffectValue = pProto->findHardpoint(tempHardpointStr);
         BASSERT(mEffectValue != -1);
      }
   }
   else if (effectTypeName == "TransformUnit")
   {
      mEffectType = cEffectTransformUnit;

      mEffectValue = gDatabase.getProtoObject(root.getTextPtr(tempStr));
   }
   else if (effectTypeName == "TransformProtoUnit")
   {
      mEffectType = cEffectTransformProtoUnit;

      BSimString typeName;
      if (root.getAttribValue("FromType", &typeName))
         mObjectType = gDatabase.getProtoObject(typeName);

      if (root.getAttribValue("ToType", &typeName))
         mEffectValue = gDatabase.getProtoObject(typeName);
   }
   else if (effectTypeName == "TransformProtoSquad")
   {
      mEffectType = cEffectTransformProtoSquad;

      BSimString typeName;
      if (root.getAttribValue("FromType", &typeName))
         mObjectType = gDatabase.getProtoSquad(typeName);

      if (root.getAttribValue("ToType", &typeName))
         mEffectValue = gDatabase.getProtoSquad(typeName);
   }
   else if (effectTypeName == "Build")
   {
      mEffectType = cEffectBuild;
      mEffectValue = gDatabase.getProtoObject(root.getTextPtr(tempStr));
   }
   else if (effectTypeName == "SetAge")
   {
      mEffectType = cEffectAge;
      BSimString ageName;
      root.getText(ageName);
      if (ageName == "Age2")
         mEffectValue = 2;
      else if (ageName == "Age3")
         mEffectValue = 3;
      else if (ageName == "Age4")
         mEffectValue = 4;
   }
   else if (effectTypeName == "GodPower")
   {
      mEffectType = cEffectPower;      
      mEffectValue = gDatabase.getProtoPowerIDByName(root.getTextPtr(tempStr));
      root.getAttribValueAsFloat("Amount", mAmount);
   }
   else if (effectTypeName == "TechStatus")
   {
      mEffectType = cEffectTech;
            
      mEffectValue = gDatabase.getProtoTech(root.getTextPtr(tempStr));
   }
   else if (effectTypeName == "Ability")
   {
      mEffectType = cEffectAbility;            
      mEffectValue = gDatabase.getAbilityIDFromName(root.getTextPtr(tempStr));
   }
   else if (effectTypeName == "SharedLOS")
   {
      mEffectType = cEffectSharedLOS;            
   }
   else if (effectTypeName == "AttachSquad")
   {
      BSimString squadTypeName;
      if (root.getAttribValue("squadType", &squadTypeName))
      {
         mEffectValue = gDatabase.getProtoObject(squadTypeName);
      }

      mEffectType = cEffectAttachSquad;

      getTargets(root);
   }

   return (true);
}

//==============================================================================
// BTechEffect::apply
//==============================================================================
void BTechEffect::apply(BTechTree* pTechTree, long unitID, bool unapply, bool noCost)
{
#ifdef SYNC_Tech
   syncTechData("BTechEffect::apply playerID", pTechTree->getPlayer()->getID());
   syncTechData("BTechEffect::apply unitID", unitID);
   syncTechData("BTechEffect::apply unapply", unapply);
   syncTechData("BTechEffect::apply noCost", noCost);
   syncTechData("BTechEffect::apply mEffectType", mEffectType);
   //syncTechData("BTechEffect::apply mAmount", mAmount);
#endif

   switch(mEffectType)
   {
      case cEffectData:
      {
         long count=mTargets.getNumber();
         for(long i=0; i<count; i++)
         {
//-- FIXING PREFIX BUG ID 2343
            const BTechEffectTarget* pTarget=&(mTargets[i]);
//--
            switch(pTarget->mTargetType)
            {
               case cTargetProtoUnit: //FIXME - SHOULD BE cTargetObjectType
               {
                  BPlayer* pPlayer=pTechTree->getPlayer();
                  uint numProtoObjectTypes = static_cast<uint>(gDatabase.getNumberProtoObjects());
                  if (static_cast<uint>(pTarget->mTargetID) < numProtoObjectTypes)
                  {
                     applyProtoObjectEffect(pPlayer, pPlayer->getProtoObject(pTarget->mTargetID), gDatabase.getGenericProtoObject(pTarget->mTargetID), unapply);
                  }
                  else
                  {
                     BProtoObjectIDArray *decomposedProtoObjectIDs = NULL;
                     uint numDecomposedProtoObjectIDs = gDatabase.decomposeObjectType(pTarget->mTargetID, &decomposedProtoObjectIDs);
                     if (decomposedProtoObjectIDs)
                     {
                        for (uint i=0; i<numDecomposedProtoObjectIDs; i++)
                        {
                           BProtoObject* pBaseProtoObject = gDatabase.getGenericProtoObject(decomposedProtoObjectIDs->get(i));
                           BASSERT(pBaseProtoObject->isType(pTarget->mTargetID));
                           applyProtoObjectEffect(pPlayer, pPlayer->getProtoObject(decomposedProtoObjectIDs->get(i)), pBaseProtoObject, unapply);
                        }
                     }
                  }
                  break;
               }

               case cTargetProtoSquad:
               {
                  BPlayer* pPlayer = pTechTree->getPlayer();
                  BProtoSquad* pCurrentProtoSquad = pPlayer->getProtoSquad(pTarget->mTargetID);
                  BProtoSquad* pBaseProtoSquad = gDatabase.getGenericProtoSquad(pCurrentProtoSquad->getBaseType());
                  applyProtoSquadEffect(pPlayer, pCurrentProtoSquad, pBaseProtoSquad, unapply);
                  break;
               }

               case cTargetUnit:
               {
                  if(unitID==-1)
                     return;
                  switch (mDataType)
                  {
                     case cDataLevel:
                     {
                        BUnit* pUnit=gWorld->getUnit(unitID);
                        if(pUnit)
                        {
                           BSquad* pSquad = pUnit->getParentSquad();
                           if (pSquad)
                              pSquad->upgradeLevel((int)mAmount, true);
                        }
                        break;
                     }
                  }
                  break;
               }

               case cTargetTech:
               {
                  BPlayer* pPlayer=pTechTree->getPlayer();
                  BProtoTech* pCurrentProtoTech=pPlayer->getProtoTech(pTarget->mTargetID);
//-- FIXING PREFIX BUG ID 2335
                  const BProtoTech* pBaseProtoTech=gDatabase.getProtoTech(pTarget->mTargetID);
//--
                  switch(mDataType)
                  {
                     case cDataResearchPoints:
                        pCurrentProtoTech->setResearchPoints(calcAmount(pBaseProtoTech->getResearchPoints(), pCurrentProtoTech->getResearchPoints(), unapply));
                        break;
                     case cDataCost:
                     {
                        BCost baseCost=*(pBaseProtoTech->getCost());
                        BCost currentCost=*(pCurrentProtoTech->getCost());
                        if(mResource==-1)
                        {
                           for(long i=0; i<currentCost.getNumberResources(); i++)
                           {
                              float baseVal=baseCost.get(i);
                              float currentVal=baseCost.get(i);
                              currentCost.set(i, calcAmount(baseVal, currentVal, unapply));
                           }
                           pCurrentProtoTech->setCost(&currentCost);
                        }
                        else
                        {
                           float baseVal=baseCost.get(mResource);
                           float currentVal=baseCost.get(mResource);
                           currentCost.set(mResource, calcAmount(baseVal, currentVal, unapply));
                           pCurrentProtoTech->setCost(&currentCost);
                        }
                        break;
                     }
                  }
                  break;
               }

               case cTargetTechAll:
               {
                  BPlayer* pPlayer=pTechTree->getPlayer();
                  long techCount=gDatabase.getNumberProtoTechs();
                  for(long j=0; j<techCount; j++)
                  {
                     BProtoTech* pCurrentProtoTech=pPlayer->getProtoTech(j);
//-- FIXING PREFIX BUG ID 2336
                     const BProtoTech* pBaseProtoTech=gDatabase.getProtoTech(j);
//--
                     switch(mDataType)
                     {
                        case cDataResearchPoints:
                           pCurrentProtoTech->setResearchPoints(calcAmount(pBaseProtoTech->getResearchPoints(), pCurrentProtoTech->getResearchPoints(), unapply));
                           break;
                        case cDataCost:
                        {
                           BCost baseCost=*(pBaseProtoTech->getCost());
                           BCost currentCost=*(pCurrentProtoTech->getCost());
                           if(mResource==-1)
                           {
                              for(long i=0; i<currentCost.getNumberResources(); i++)
                              {
                                 float baseVal=baseCost.get(i);
                                 float currentVal=baseCost.get(i);
                                 currentCost.set(i, calcAmount(baseVal, currentVal, unapply));
                              }
                              pCurrentProtoTech->setCost(&currentCost);
                           }
                           else
                           {
                              float baseVal=baseCost.get(mResource);
                              float currentVal=baseCost.get(mResource);
                              currentCost.set(mResource, calcAmount(baseVal, currentVal, unapply));
                              pCurrentProtoTech->setCost(&currentCost);
                           }
                           break;
                        }
                     }
                  }
                  break;
               }

               case cTargetPlayer:
               {
                  BPlayer* pPlayer=pTechTree->getPlayer();
                  switch(mDataType)
                  {
                     case cDataResourceTrickleRate:
#ifdef SYNC_Player
                        syncPlayerData("BTechEffect::apply getResourceTrickleRate", pPlayer->getResourceTrickleRate(mResource));
                        syncPlayerData("BTechEffect::apply mAmount", mAmount);
                        syncPlayerData("BTechEffect::apply mRelativity", mRelativity);
#endif
                        pPlayer->setResourceTrickleRate(mResource, calcAmount(pPlayer->getResourceTrickleRate(mResource), pPlayer->getResourceTrickleRate(mResource), unapply));
                        break;
                     case cDataResource:
                        pPlayer->setResource(mResource, calcAmount(pPlayer->getResource(mResource), pPlayer->getResource(mResource), unapply));
                        break;
                     case cDataRateAmount:
                        pPlayer->setRateAmount(mResource, calcAmount(pPlayer->getRateAmount(mResource), pPlayer->getRateAmount(mResource), unapply));
                        break;
                     case cDataRateMultiplier:
                        pPlayer->setRateMultiplier(mResource, calcAmount(pPlayer->getRateMultiplier(mResource), pPlayer->getRateMultiplier(mResource), unapply));
                        break;
                     case cDataBountyResource:
                        if(unapply)
                        {
                           pPlayer->setFlagBountyResource((mAmount==0.0f));
                        }
                        else
                        {
                           if(mAmount==0.0f)
                              pPlayer->setFlagBountyResource(false);
                           else
                           {
                              pPlayer->setFlagBountyResource(true);
                              pPlayer->setBountyResource(mResource);
                           }
                        }
                        break;
                     case cDataShieldRegenRate:
                        pPlayer->setShieldRegenRate(calcAmount(gDatabase.getShieldRegenRate(), pPlayer->getShieldRegenRate(), unapply));
                        break;
                     case cDataShieldRegenDelay:
                     {
                        DWORD baseVal=gDatabase.getShieldRegenDelay();
                        DWORD curVal=pPlayer->getShieldRegenDelay();
                        float baseValFloat=baseVal*0.001f;
                        float curValFloat=curVal*0.001f;
                        float newValFloat=calcAmount(baseValFloat, curValFloat, unapply);
                        DWORD newVal=(DWORD)(newValFloat*1000.0f);
                        pPlayer->setShieldRegenDelay(newVal);
                        break;
                     }
                     case cDataTributeCost:
                        pPlayer->setTributeCost(calcAmount(gDatabase.getTributeCost(), pPlayer->getTributeCost(), unapply));
                        break;
                     case cDataDamageModifier:
                     {
                        int weaponType=mResource;
                        int damageType=mObjectType;
                        BWeaponType* pCurrentWeaponType=pPlayer->getWeaponTypeByID(weaponType);
//-- FIXING PREFIX BUG ID 2337
                        const BWeaponType* pBaseWeaponType=gDatabase.getWeaponTypeByID(weaponType);
//--
                        if (pCurrentWeaponType && pBaseWeaponType)
                        {
                           pCurrentWeaponType->setDamageModifier(damageType, calcAmount(pBaseWeaponType->getDamageModifier(damageType), pCurrentWeaponType->getDamageModifier(damageType), unapply));

                           // AJL FIXME 8/14/07 - Might be slow updating all proto objects and squads
                           long objectCount=pPlayer->getNumberProtoObjects();
                           for(long i=0; i<objectCount; i++)
                           {
//-- FIXING PREFIX BUG ID 2349
                              const BProtoObject* pProtoObject=pPlayer->getProtoObject(i);
//--
                              BTactic* pTactic=pProtoObject->getTactic();
                              if (pTactic)
                                 pTactic->updateAttackRatings(pPlayer);
                           }

                           long squadCount=pPlayer->getNumberProtoSquads();
                           for(long i=0; i<squadCount; i++)
                           {
                              BProtoSquad* pProtoSquad=pPlayer->getProtoSquad(i);
                              // Skip auto-created unit specific proto squads
                              if (pProtoSquad->getProtoObjectID()!=-1)
                                 continue;
                              pProtoSquad->updateAttackRatings();
                           }
                        }
                        break;
                     }
                     case cDataPopCap:
                     {
                        int popType=mResource;
                        float baseVal=0.0f;

                        // this is one effect we don't want to duplicate for each player since it double adjusts the pop.
                        if (pPlayer->getCoopID()!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
                           break;

//-- FIXING PREFIX BUG ID 2338
                        const BLeader* pLeader=pPlayer->getLeader();
//--
                        const BLeaderPopArray& leaderPops=pLeader->getPops();
                        for (uint i=0; i<leaderPops.getSize(); i++)
                        {
                           const BLeaderPop& leaderPop=leaderPops[i];
                           if (leaderPop.mID == popType)
                           {
                              baseVal=leaderPop.mCap;
                              break;
                           }
                        }
                        pPlayer->setPopCap(popType, calcAmount(baseVal, pPlayer->getPopCap(popType), unapply));
                        break;
                     }
                     case cDataPopMax:
                     {
                        int popType=mResource;
                        float baseVal=0.0f;
//-- FIXING PREFIX BUG ID 2339
                        const BLeader* pLeader=pPlayer->getLeader();
//--
                        const BLeaderPopArray& leaderPops=pLeader->getPops();
                        for (uint i=0; i<leaderPops.getSize(); i++)
                        {
                           const BLeaderPop& leaderPop=leaderPops[i];
                           if (leaderPop.mID == popType)
                           {
                              baseVal=leaderPop.mMax;
                              break;
                           }
                        }
                        pPlayer->setPopMax(popType, calcAmount(baseVal, pPlayer->getPopMax(popType), unapply));
                        break;
                     }
                     case cDataRepairCost:
                        pPlayer->setRepairCost(mResource, calcAmount(pPlayer->getLeader()->getRepairCost(mResource), pPlayer->getRepairCost(mResource), unapply));
                        break;
                     case cDataRepairTime:
                        pPlayer->setRepairTime(calcAmount(pPlayer->getLeader()->getRepairTime(), pPlayer->getRepairTime(), unapply));
                        break;
                     case cDataPowerRechargeTime:
                     {
//-- FIXING PREFIX BUG ID 2340
                        const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mEffectValue);
//--
                        if (pProtoPower)
                        {
                           float baseTime=(float)pProtoPower->getAutoRechargeTime();
                           float oldTime=(float)pPlayer->getPowerRechargeTime(mEffectValue);
                           float newTime=calcAmount(baseTime, oldTime, unapply);
                           pPlayer->setPowerRechargeTime(mEffectValue, (DWORD)(newTime+0.5f));
                        }
                        break;
                     }
                     case cDataPowerUseLimit:
                     {
//-- FIXING PREFIX BUG ID 2341
                        const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mEffectValue);
//--
                        if (pProtoPower)
                        {
                           float baseLimit=(float)pProtoPower->getUseLimit();
                           float oldLimit=(float)pPlayer->getPowerUseLimit(mEffectValue);
                           float newLimit=calcAmount(baseLimit, oldLimit, unapply);
                           pPlayer->setPowerUseLimit(mEffectValue, (int)(newLimit+0.5f));
                        }
                        break;
                     }
                     case cDataPowerLevel:
                     {
//-- FIXING PREFIX BUG ID 2342
                        const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mEffectValue);
//--
                        if (pProtoPower)
                        {
                           float baseLevel = 0.0f;
                           float oldLevel = static_cast<float>(pPlayer->getPowerLevel(mEffectValue));
                           float newLevel = calcAmount(baseLevel, oldLevel, unapply);
                           pPlayer->setPowerLevel(mEffectValue, static_cast<BPowerLevel>(newLevel + 0.5f));
                        }
                        break;
                     }
                     case cDataAbilityRecoverTime:
                     {
                        int abilityID = mResource;
                        const BAbility* pBaseAbility = gDatabase.getAbilityFromID(abilityID);
                        if (pBaseAbility)
                           pPlayer->setAbilityRecoverTime(abilityID, calcAmount(pBaseAbility->getRecoverTime(), pPlayer->getAbilityRecoverTime(abilityID), unapply));
                        break;
                     }
                     case cDataWeaponPhysicsMultiplier:
                     {
                        pPlayer->setWeaponPhysicsMultiplier(calcAmount(pPlayer->getWeaponPhysicsMultiplier(), pPlayer->getWeaponPhysicsMultiplier(), unapply));
                        break;
                     }
                  }
                  break;
               }
            }
         }
         break;
      }

      case cEffectTransformUnit:
         if(!unapply) //FIXME - Do we need to handle unapplying here?
         {
            if(unitID!=-1)
            {
               BUnit* pUnit=gWorld->getUnit(unitID);
               if(pUnit)
               {
                  const BProtoObject* pOldProtoObject=pUnit->getProtoObject();
                  const BProtoObject* pNewProtoObject=pUnit->getPlayer()->getProtoObject(mEffectValue);
                  if(pOldProtoObject && pNewProtoObject)
                     pUnit->transform(pOldProtoObject, pNewProtoObject);
               }
            }
         }
         break;

      case cEffectTransformProtoUnit:
         if(!unapply) //FIXME - Do we need to handle unapplying here?
         {
            BProtoObjectID fromProtoID = mObjectType;
            BProtoObjectID toProtoID = mEffectValue;

            BPlayer* pPlayer=pTechTree->getPlayer();
            BProtoObject* pFromProtoObject=pPlayer->getProtoObject(fromProtoID);
            const BProtoObject* pToProtoObject=pPlayer->getProtoObject(toProtoID);
            if(pFromProtoObject && pToProtoObject)
            {
               BEntityIDArray units;
               static BEntityIDArray trainingSquads;
               trainingSquads.setNumber(0);
               pPlayer->getUnitsOfType(fromProtoID, units);
               uint unitCount = units.getSize();

               BSmallDynamicSimArray<BUnitTransformData> transformDataList;

               // Pre-transform units that use the proto object
               if(unitCount > 0)
               {
                  transformDataList.reserve(unitCount);
                  if (transformDataList.getCapacity() == unitCount)
                  {
                     for (uint i=0; i<unitCount; i++)
                     {
                        BUnitTransformData transformData;
                        BUnit* pUnit=gWorld->getUnit(units[i]);
                        // Don't transform units in this list with unique protoObjects
                        if (pUnit && (!pUnit->getProtoObject() || !pUnit->getProtoObject()->getFlagUniqueInstance()))
                        {
                           // Check whether squad running birth anim
                           BSquad* pSquad = pUnit->getParentSquad();
                           if (pSquad)
                           {
                              const BSquadActionPlayBlockingAnimation* pAnimAction = reinterpret_cast<const BSquadActionPlayBlockingAnimation*>(pSquad->getActionByTypeConst(BAction::cActionTypeSquadPlayBlockingAnimation));
                              if (pAnimAction && pAnimAction->getFlagBirthAnim())
                                 trainingSquads.uniqueAdd(pSquad->getID());
                           }

                           pUnit->preTransform(pFromProtoObject, pToProtoObject, true, transformData);
                        }
                        transformDataList.add(transformData);
                     }
                  }
               }

               // Transform the proto object
               pFromProtoObject->transform(pToProtoObject);

               // Post-transform units that use the proto object
               if(unitCount > 0)
               {
                  if (transformDataList.getCapacity() == unitCount)
                  {
                     for (uint i=0; i<unitCount; i++)
                     {
                        BUnit* pUnit=gWorld->getUnit(units[i]);
                        // Don't transform units in this list with unique protoObjects
                        if (pUnit && (!pUnit->getProtoObject() || !pUnit->getProtoObject()->getFlagUniqueInstance()))
                           pUnit->postTransform(pFromProtoObject, true, transformDataList[i]);
                     }
                  }
               }


               // For squads in the middle of birth anims, pop them out to a valid position
               for (uint i = 0; i < trainingSquads.getSize(); i++)
               {
                  BSquad* pSquad = gWorld->getSquad(trainingSquads[i]);
                  if (pSquad)
                     BSimHelper::putSquadAtUnobstructedPosition(pSquad);
               }

               pPlayer->recalculateMaxForProtoSquads();
            }
         }
         break;

      case cEffectTransformProtoSquad:
         if(!unapply) //FIXME - Do we need to handle unapplying here?
         {
            BProtoSquadID fromProtoID = mObjectType;
            BProtoSquadID toProtoID = mEffectValue;

            BPlayer* pPlayer=pTechTree->getPlayer();

            // Before transforming, get a list of all combinations
            // of merged squads that should be transformed as well
            BProtoSquad* pFromProtoSquad=pPlayer->getProtoSquad(fromProtoID);
            const BProtoSquad* pToProtoSquad=pPlayer->getProtoSquad(toProtoID);
            static BProtoSquadIDPairArray results;
            results.setNumber(0);
            pFromProtoSquad->getCommonMergeProtoSquadIDs(pToProtoSquad, results);

            // Do transformations
            doProtoSquadTransform(pPlayer, fromProtoID, toProtoID);
            for (int i = 0; i < results.getNumber(); i++)
            {
               doProtoSquadTransform(pPlayer, results[i].mBaseID, results[i].mMergedID);
               pPlayer->recalculateMaxForProtoSquad(results[i].mMergedID);
            }
         }
         break;

      case cEffectBuild:
         if(!unapply) //FIXME - Do we need to handle unapplying here?
         {
            if(unitID!=-1)
            {
               BUnit* pUnit=gWorld->getUnit(unitID);
               if(pUnit)
               {
                  const BProtoObject* pBuilderProtoObject=pUnit->getProtoObject();
                  const BProtoObject* pNewProtoObject=pUnit->getPlayer()->getProtoObject(mEffectValue);
                  if(pBuilderProtoObject && pNewProtoObject)
                  {
                     // AJL 8/1/07 - Only handling building on sockets for now
                     if (pBuilderProtoObject->isType(pNewProtoObject->getSocketID()))
                     {
//-- FIXING PREFIX BUG ID 2344
                        const BUnit* pBuilderUnit=gWorld->getUnit(pUnit->getBaseBuilding());
//--
                        if (!pBuilderUnit)
                           pBuilderUnit=pUnit;
                        gWorld->createEntity(pNewProtoObject->getID(), false, pUnit->getPlayerID(), pUnit->getPosition(), cZAxisVector, cXAxisVector, noCost, false, false, cInvalidObjectID, pUnit->getPlayerID(), pBuilderUnit->getID(), pUnit->getID(), true);
                     }
                  }
               }
            }
         }
         break;

      case cEffectAge:
         break;

      case cEffectPower:
         if(!unapply) //FIXME - Do we need to handle unapplying here?
         {
            BProtoPower *pPP = gDatabase.getProtoPowerByID(mEffectValue);
            if (pPP)
            {
               if(mAmount<0.0f)
               {
                  int count=(int)mAmount;
                  for (int i=count; i<0; i++)
                     pTechTree->getPlayer()->removePowerEntry(mEffectValue, cInvalidObjectID);
               }
               else
                  pTechTree->getPlayer()->addPowerEntry(mEffectValue, cInvalidObjectID, (long)mAmount, -1);
            }
         }
         break;

      case cEffectTech:
         if(!unapply) //FIXME - Do we need to handle unapplying here?
            pTechTree->makeObtainable(mEffectValue, unitID);
         break;

      case cEffectAbility:
         if (!unapply) // FIXME - Do we need to handle unapplying here.
            pTechTree->getPlayer()->addAbility(mEffectValue);
         break;

      case cEffectSharedLOS:
         if(!unapply) //FIXME - Do we need to handle unapplying here?
         {
            BTeamID teamID = pTechTree->getPlayer()->getTeamID();
            BTeam::setSpiesForTeam(teamID);
         }
         break;

      case cEffectAttachSquad:
         if(!unapply) //FIXME - Do we need to handle unapplying here?
         {
            long count=mTargets.getNumber();
            for(long i=0; i<count; i++)
            {
//-- FIXING PREFIX BUG ID 2348
               const BTechEffectTarget* pTarget=&(mTargets[i]);
//--

               BProtoObjectID protoID = pTarget->mTargetID;
               BProtoObject* pProto = gDatabase.getGenericProtoObject(protoID);            

               switch(pTarget->mTargetType)
               {
                  case cTargetProtoUnit:
                  {
                     BPlayer* pPlayer=pTechTree->getPlayer();
                     BEntityIDArray units;
                     pPlayer->getUnitsOfType(pProto->getID(), units);
                     uint unitCount=units.getSize();
                     for (uint i=0; i<unitCount; i++)
                     {
                        BUnit* pUnit=gWorld->getUnit(units[i]);
                        if (pUnit)
                        {
                           BSquad* pSquad=pUnit->getParentSquad();
                           if (pSquad)
                           {
                              BEntityID id;
                              id = gWorld->createEntity(mEffectValue, false, pSquad->getPlayerID(), pSquad->getPosition(), cZAxisVector, cXAxisVector, true);
                              BSquad* pNewSquad = gWorld->getSquad(id);
                              if (pNewSquad)
                              {
                                 BSimOrder* pOrder = gSimOrderManager.createOrder();
                                 BASSERT(pOrder);
                                 if (pOrder)  
                                 {
                                    BSimTarget target;
                                    target.setID(pSquad->getID());
                                    target.setAbilityID(0);

                                    pOrder->setPriority(BSimOrder::cPrioritySim);
                                    pOrder->setTarget(target);

                                    /*BUnitOpp opp;
                                    opp.init();
                                    opp.setTarget(target);
                                    opp.setType(BUnitOpp::cTypeJoin);
                                    opp.setSource(pSquad->getID());
                                    opp.setPriority(BUnitOpp::cPriorityCritical);
                                    opp.setUserData(1);
                                    pNewSquad->addOppToChildren(opp);*/

                                    BSquadActionWork* pAction=reinterpret_cast<BSquadActionWork*>(gActionManager.createAction(BAction::cActionTypeSquadWork));

                                    if (pAction)
                                    {
                                       pAction->setUserData(1);
                                       pAction->setTarget(target);
                                       pAction->setUnitOppType(BUnitOpp::cTypeJoin);
                                       pAction->setFlagDoneOnOppComplete(true);
                                       pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

                                       pNewSquad->addAction(pAction, pOrder);
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }
                  break;

                  default:
                     BASSERT(0);
               }
            }
         }
         break;
   }
}

//==============================================================================
// Exposed function for trigger system apply changes to proto data
//==============================================================================
void BTechEffect::applyProtoObjectEffect(BPlayer* pPlayer, BProtoObject* pCurrentProtoObject, BProtoObject* pBaseProtoObject, float amount, long dataSubType, long relativity, bool allActions, BSimString actionName, long resource, long objectType, bool unapply /*= false*/)
{
   mAmount = amount;
   mDataType = dataSubType;
   mRelativity = relativity;
   mAllActions = allActions;
   mActionName = actionName;
   mResource = resource;
   mObjectType = objectType;

   applyProtoObjectEffect(pPlayer, pCurrentProtoObject, pBaseProtoObject, unapply);
}

//==============================================================================
// BTechEffect::applyProtoObjectEffect
//==============================================================================
void BTechEffect::applyProtoObjectEffect(BPlayer* pPlayer, BProtoObject* pCurrentProtoObject, BProtoObject* pBaseProtoObject, bool unapply)
{
   if(!pCurrentProtoObject || !pBaseProtoObject)
      return;
   switch(mDataType)
   {
      case cDataEnable:
         if(unapply)
            pCurrentProtoObject->setFlagAvailable(!(mAmount>0.0f));
         else
            pCurrentProtoObject->setFlagAvailable((mAmount>0.0f));
         break;
      case cDataShieldpoints:
      {
         float oldVal=pCurrentProtoObject->getShieldpoints();
         float newVal=calcAmount(pBaseProtoObject->getShieldpoints(), oldVal, unapply);
         pCurrentProtoObject->setShieldpoints(newVal);
         pPlayer->recalculateMaxSPForProtoSquads();
         if (newVal > oldVal)
         {
            BEntityIDArray units;
            pPlayer->getUnitsOfType(pCurrentProtoObject->getID(), units);
            uint unitCount=units.getSize();
            for (uint i=0; i<unitCount; i++)
            {
//-- FIXING PREFIX BUG ID 2353
               const BUnit* pUnit=gWorld->getUnit(units[i]);
//--
               if (pUnit)
               {
                  BSquad* pSquad=pUnit->getParentSquad();
                  if (pSquad)
                     pSquad->setFlagShieldDamaged(true);
               }
            }
         }
         break;
      }
      case cDataHitpoints:
      case cDataAmmoMax:
      {
         float oldVal=0.f;
         float newVal=0.0f;
         switch(mDataType)
         {
            case cDataHitpoints:
               oldVal=pCurrentProtoObject->getHitpoints();
               newVal=calcAmount(pBaseProtoObject->getHitpoints(), oldVal, unapply);
               pCurrentProtoObject->setHitpoints(newVal);
               pPlayer->recalculateMaxHPForProtoSquads();
               break;
            case cDataAmmoMax:
               oldVal=pCurrentProtoObject->getMaxAmmo();
               newVal=calcAmount(pBaseProtoObject->getMaxAmmo(), oldVal, unapply);
               pCurrentProtoObject->setMaxAmmo(newVal);
               pPlayer->recalculateMaxAmmoForProtoSquads();
               break;
         }
         if(newVal!=oldVal && oldVal!=0.0f)
         {
            long playerID=pPlayer->getID();
            BProtoObjectID protoID=pCurrentProtoObject->getID();
            if(pPlayer->getNumUnitsOfType(protoID) > 0)
            {
               float ratio=newVal/oldVal;
               //FIXME AJL 7/12/06 - Need to keep a list of units per player and per proto object type so we don't
               // have to traverse the full list
               BEntityHandle handle=cInvalidObjectID;
               for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
               {
                  //FIXME AJL 7/12/06 - Sucks that the player ID and proto ID in the handle is invalid here
                  // which causes us to have to deref the pUnit to get that info
                  //if(handle.getPlayerID()==playerID && handle.getProtoID()==protoID)
                  if(pUnit->getPlayerID()==playerID && pUnit->getProtoID()==protoID)
                  {
#ifdef SYNC_Tech
                     syncTechData("BTechEffect::applyProtoObjectEffect unitID", pUnit->getID().asLong());
#endif
                     switch(mDataType)
                     {
                        case cDataHitpoints:
                        {
                           float val=pUnit->getHitpoints()*ratio;
#ifdef SYNC_Tech
                           syncTechData("BTechEffect::applyProtoObjectEffect setHitpoints", val);
#endif
                           pUnit->setHitpoints(val);
                           break;
                        }
                        case cDataAmmoMax:
                        {
                           float val=pUnit->getAmmunition()*ratio;
#ifdef SYNC_Tech
                           syncTechData("BTechEffect::applyProtoObjectEffect setAmmunition", val);
#endif
                           pUnit->setAmmunition(val);
                           break;
                        }
                     }
                  }
               }
            }
         }
         break;
      }
      case cDataLOS:
      {
         float oldVal=pCurrentProtoObject->getProtoLOS();
         float newVal=calcAmount(pBaseProtoObject->getProtoLOS(), oldVal, unapply);
         long newSimVal = (long) (newVal * gTerrainSimRep.getReciprocalDataTileScale());
         pCurrentProtoObject->setProtoLOS(newVal);
         pCurrentProtoObject->setProtoSimLOS(newSimVal);
         if(newVal!=oldVal && oldVal!=0.0f)
         {
            long playerID=pPlayer->getID();
            BProtoObjectID protoID=pCurrentProtoObject->getID();
            if(pPlayer->getNumUnitsOfType(protoID) > 0)
            {
               //FIXME AJL 7/12/06 - Need to keep a list of units per player and per proto object type so we don't
               // have to traverse the full list
               BEntityHandle handle=cInvalidObjectID;
               for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
               {
                  //FIXME AJL 7/12/06 - Sucks that the player ID and proto ID in the handle is invalid here
                  // which causes us to have to deref the pUnit to get that info
                  //if(handle.getPlayerID()==playerID && handle.getProtoID()==protoID)
                  if(pUnit->getPlayerID()==playerID && pUnit->getProtoID()==protoID)
                  {
                     pUnit->setFlagLOSDirty(true);
                  }
               }
            }
         }
         break;
      }
      case cDataMaximumVelocity:
         pCurrentProtoObject->setVelocity(calcAmount(pBaseProtoObject->getDesiredVelocity(), pCurrentProtoObject->getDesiredVelocity(), unapply));
         break;
      case cDataMaximumRange:
      case cDataDamage:      
      case cDataMinRange:
      case cDataAOERadius:
      case cDataAOEPrimaryTargetFactor:
      case cDataAOEDistanceFactor:
      case cDataAOEDamageFactor:   
      case cDataAccuracy:
      case cDataMovingAccuracy:
      case cDataMaxDeviation:
      case cDataMovingMaxDeviation:
      case cDataAccuracyDistanceFactor:
      case cDataAccuracyDeviationFactor:
      case cDataMaxVelocityLead:
      case cDataMaxDamagePerRam:
      case cDataReflectDamageFactor:
      case cDataAirBurstSpan:
      case cDataDOTrate:
      case cDataDOTduration:
      case cDataStasis:
      {
         BTactic* pCurrentTactic=pCurrentProtoObject->getTactic();
         BTactic* pBaseTactic=pBaseProtoObject->getTactic();
         if(pCurrentTactic && pBaseTactic)
         {
            if(mAllActions)
            {
               for(long i=0; i<pCurrentTactic->getNumberWeapons(); i++)
                  applyWeaponEffect(pPlayer, pCurrentProtoObject->getID(), pCurrentTactic, pBaseTactic, i, unapply);
            }
            else
            {
               long weaponID=pCurrentTactic->getWeaponID(mActionName);
               if(weaponID!=-1)
                  applyWeaponEffect(pPlayer, pCurrentProtoObject->getID(), pCurrentTactic, pBaseTactic, weaponID, unapply);
            }
         }
         break;
      }
      case cDataProjectile:
      {
         if (mObjectType != -1)
         {
            BTactic* pCurrentTactic=pCurrentProtoObject->getTactic();
            if (pCurrentTactic)
            {
               if(mAllActions)
               {
                  for(long i=0; i<pCurrentTactic->getNumberWeapons(); i++)
                  {
                     BWeapon* pCurrentWeapon=const_cast<BWeapon*>(pCurrentTactic->getWeapon(i));
                     if (pCurrentWeapon)
                        pCurrentWeapon->mProjectileObjectID = mObjectType;
                  }
               }
               else
               {
                  long weaponID=pCurrentTactic->getWeaponID(mActionName);
                  if(weaponID!=-1)
                  {
                     BWeapon* pCurrentWeapon=const_cast<BWeapon*>(pCurrentTactic->getWeapon(weaponID));
                     if (pCurrentWeapon)
                        pCurrentWeapon->mProjectileObjectID = mObjectType;
                  }
               }
            }
         }
         break;
      }
      case cDataImpactEffect:
      {
         if (mObjectType != -1)
         {
            BTactic* pCurrentTactic=pCurrentProtoObject->getTactic();
            if (pCurrentTactic)
            {
               if(mAllActions)
               {
                  for(long i=0; i<pCurrentTactic->getNumberWeapons(); i++)
                  {
                     BWeapon* pCurrentWeapon=const_cast<BWeapon*>(pCurrentTactic->getWeapon(i));
                     if (pCurrentWeapon)
                        pCurrentWeapon->mImpactEffectProtoID = mObjectType;
                  }
               }
               else
               {
                  long weaponID=pCurrentTactic->getWeaponID(mActionName);
                  if(weaponID!=-1)
                  {
                     BWeapon* pCurrentWeapon=const_cast<BWeapon*>(pCurrentTactic->getWeapon(weaponID));
                     if (pCurrentWeapon)
                        pCurrentWeapon->mImpactEffectProtoID = mObjectType;
                  }
               }
            }
         }
         break;
      }
      case cDataWorkRate:
      case cDataActionEnable:
      case cDataTurretPitchRate:
      case cDataTurretYawRate:
      case cDataBoardTime:
      {
         BTactic* pCurrentTactic=pCurrentProtoObject->getTactic();
         BTactic* pBaseTactic=pBaseProtoObject->getTactic();
         if(pCurrentTactic && pBaseTactic)
         {
            if(mAllActions)
            {
               for(long i=0; i<pCurrentTactic->getNumberProtoActions(); i++)
                  applyProtoActionEffect(pCurrentTactic, pBaseTactic, i, unapply);
            }
            else
            {
               long actionID=pCurrentTactic->getProtoActionID(mActionName);
               if(actionID!=-1)
                  applyProtoActionEffect(pCurrentTactic, pBaseTactic, actionID , unapply);
            }
         }

         if(mDataType == cDataTurretYawRate)
         {         
            //mEffectValue == hardpointID
            if(mEffectValue != -1)
            {
               BHardpoint* pCurrentHP = pCurrentProtoObject->getHardpointMutable(mEffectValue);
//-- FIXING PREFIX BUG ID 2351
               const BHardpoint* pBaseHP = pBaseProtoObject->getHardpointMutable(mEffectValue);
//--
               if(pCurrentHP && pBaseHP)
               {
                  float oldVal = pCurrentHP->getYawRotationRate();
                  float baseVal = pBaseHP->getYawRotationRate();
                  float newVal = calcAmount(baseVal, oldVal, unapply);                  
                  pCurrentHP->setYawRotationRate(newVal);
               }
            }
            break;
         }         
         if(mDataType == cDataTurretPitchRate)
         {         
            if(mEffectValue != -1)
            {
               BHardpoint* pCurrentHP = pCurrentProtoObject->getHardpointMutable(mEffectValue);
//-- FIXING PREFIX BUG ID 2352
               const BHardpoint* pBaseHP = pBaseProtoObject->getHardpointMutable(mEffectValue);
//--
               if(pCurrentHP && pBaseHP)
               {
                  float oldVal = pCurrentHP->getPitchRotationRate();
                  float baseVal = pBaseHP->getPitchRotationRate();
                  float newVal = calcAmount(baseVal, oldVal, unapply);                  
                  pCurrentHP->setPitchRotationRate(newVal);
               }
            }            
         }

         break;
      }
      case cDataBuildPoints:
         pCurrentProtoObject->setBuildPoints(calcAmount(pBaseProtoObject->getBuildPoints(), pCurrentProtoObject->getBuildPoints(), unapply));
         break;
      case cDataCost:
         {
            BCost baseCost=*(pBaseProtoObject->getCost());
            BCost currentCost=*(pCurrentProtoObject->getCost());
            if(mResource==-1)
            {
               for(long i=0; i<currentCost.getNumberResources(); i++)
               {
                  float baseVal=baseCost.get(i);
                  float currentVal=baseCost.get(i);
                  currentCost.set(i, calcAmount(baseVal, currentVal, unapply));
               }
               pCurrentProtoObject->setCost(&currentCost);
            }
            else
            {
               float baseVal=baseCost.get(mResource);
               float currentVal=baseCost.get(mResource);
               currentCost.set(mResource, calcAmount(baseVal, currentVal, unapply));
               pCurrentProtoObject->setCost(&currentCost);
            }
            break;
         }
      case cDataCommandEnable:
      {
         uint numCommands = pCurrentProtoObject->getNumberCommands();
         for(uint i=0; i<numCommands; i++)
         {
            BProtoObjectCommand command=pCurrentProtoObject->getCommand(i);
            if(command.getType()==mResource && command.getID()==mObjectType)
            {
               bool oldVal=pCurrentProtoObject->getCommandAvailable(i);

               bool newVal;
               if(unapply)
                  newVal=!(mAmount>0.0f);
               else
                  newVal=(mAmount>0.0f);

               if(newVal!=oldVal)
                  pCurrentProtoObject->setCommandAvailable(i, newVal);
               break;
            }
         }
         break;
      }
      case cDataCommandSelectable:
         {
            uint numCommands = pCurrentProtoObject->getNumberCommands();
            for (uint i = 0; i < numCommands; i++)
            {
               BProtoObjectCommand command = pCurrentProtoObject->getCommand(i);
               if ((command.getType() == mResource) && (command.getID() == mObjectType))
               {
                  bool oldVal = pCurrentProtoObject->getCommandSelectable(i);

                  bool newVal;
                  if (unapply)
                     newVal = !(mAmount > 0.0f);
                  else
                     newVal = (mAmount > 0.0f);

                  if (newVal != oldVal)
                     pCurrentProtoObject->setCommandSelectable(i, newVal);
                  break;
               }
            }
            break;
         }
      case cDataAutoCloak:
      {
         bool oldVal=pCurrentProtoObject->getFlagAutoCloak();

         bool newVal;
         if(unapply)
            newVal=!(mAmount>0.0f);
         else
            newVal=(mAmount>0.0f);

         if(newVal!=oldVal)
         {
            pCurrentProtoObject->setFlagAutoCloak(newVal);

            long playerID=pPlayer->getID();
            BProtoObjectID protoID=pCurrentProtoObject->getID();
            if(pPlayer->getNumUnitsOfType(protoID) > 0)
            {
               //FIXME AJL 7/12/06 - Need to keep a list of units per player and per proto object type so we don't
               // have to traverse the full list
               BEntityHandle handle=cInvalidObjectID;
               for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
               {
                  //FIXME AJL 7/12/06 - Sucks that the player ID and proto ID in the handle is invalid here
                  // which causes us to have to deref the pUnit to get that info
                  //if(handle.getPlayerID()==playerID && handle.getProtoID()==protoID)
                  if(pUnit->getPlayerID()==playerID && pUnit->getProtoID()==protoID)
                  {
/*
                     // Cloaking is done by the unit's idle action, so find that action and update it
                     BEntityActionIdle* pIdleAction=(BEntityActionIdle*)pUnit->getActionByType(BAction::cActionTypeEntityIdle);
                     if(pIdleAction)
                        pIdleAction->setCloak(newVal);
*/
                  }
               }
            }
         }
         break;
      }

      case cDataCloakMove:
      {
         bool oldVal=pCurrentProtoObject->getFlagCloakMove();

         bool newVal;
         if(unapply)
            newVal=!(mAmount>0.0f);
         else
            newVal=(mAmount>0.0f);

         if(newVal!=oldVal)
         {
            pCurrentProtoObject->setFlagCloakMove(newVal);

            BSmallDynamicRenderArray<long> squadIDs;

            long playerID=pPlayer->getID();
            BProtoObjectID protoID=pCurrentProtoObject->getID();
            if(pPlayer->getNumUnitsOfType(protoID) > 0)
            {
               //FIXME AJL 7/12/06 - Need to keep a list of units per player and per proto object type so we don't
               // have to traverse the full list
               BEntityHandle handle=cInvalidObjectID;
               for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
               {
                  //FIXME AJL 7/12/06 - Sucks that the player ID and proto ID in the handle is invalid here
                  // which causes us to have to deref the pUnit to get that info
                  //if(handle.getPlayerID()==playerID && handle.getProtoID()==protoID)
                  if(pUnit->getPlayerID()==playerID && pUnit->getProtoID()==protoID)
                  {
                     BAction* pAction=pUnit->getActionByType(BAction::cActionTypeUnitMove);
                     if(pAction)
                        pAction->setFlagConflictsWithIdle(!newVal);
                  }
               }
            }
         }
         break;
      }

      case cDataCloakAttack:
         {
            // DMG 4/28/08:  This may all be legacy code...
            bool oldVal=pCurrentProtoObject->getFlagCloakAttack();

            bool newVal;
            if(unapply)
               newVal=!(mAmount>0.0f);
            else
               newVal=(mAmount>0.0f);

            if(newVal!=oldVal)
            {
               pCurrentProtoObject->setFlagCloakAttack(newVal);
               
            }
            break;
         }

      case cDataRateAmount:
      {
         float oldVal=pCurrentProtoObject->getRateAmount();
         float newVal=calcAmount(pBaseProtoObject->getRateAmount(), oldVal, unapply);
         pCurrentProtoObject->setRateAmount(newVal);
         uint count = pPlayer->getNumUnitsOfType(pCurrentProtoObject->getID());
         if (count > 0)
         {
            float diffVal=newVal-oldVal;
            pPlayer->adjustRateAmount(pCurrentProtoObject->getRateID(), diffVal * (float)count);
         }
         break;
      }

      case cDataUnitTrainLimit:
      case cDataSquadTrainLimit:
      {
         bool squad=(mDataType==cDataSquadTrainLimit);
         int oldVal=pCurrentProtoObject->getTrainLimit(mObjectType, squad, NULL);
         int baseVal=pBaseProtoObject->getTrainLimit(mObjectType, squad, NULL);
         int newVal=(int)(calcAmount((float)baseVal, (float)oldVal, unapply) + 0.5f);
         pCurrentProtoObject->setTrainLimit(mObjectType, squad, newVal);
         break;
      }

      case cDataBounty:
      {
         float oldVal=pCurrentProtoObject->getBounty();
         float baseVal=pBaseProtoObject->getBounty();
         float newVal=calcAmount(baseVal, oldVal, unapply);
         pCurrentProtoObject->setBounty(newVal);
         break;
      }

      case cDataMaxContained:
         pCurrentProtoObject->setMaxContained((int)calcAmount((float)pBaseProtoObject->getMaxContained(), (float)pCurrentProtoObject->getMaxContained(), unapply));
         break;

      case cDataAbilityDisabled:
      {
         if(unapply)
            pCurrentProtoObject->setFlagAbilityDisabled(!(mAmount>0.0f));
         else
            pCurrentProtoObject->setFlagAbilityDisabled((mAmount>0.0f));

         for (int j=0; j<2; j++)
         {
            BUser* pUser;
            if (j == 0)
               pUser = gUserManager.getPrimaryUser();
            else if (gGame.isSplitScreen())
               pUser = gUserManager.getSecondaryUser();
            else
               break;
            BSelectionManager* pSelectionManager = pUser->getSelectionManager();
            for (int i=0; i<pSelectionManager->getNumberSelectedUnits(); i++)
            {
               BUnit* pUnit = gWorld->getUnit(pSelectionManager->getSelected(i));
               if (pUnit && pUnit->getProtoID() == pCurrentProtoObject->getID())
               {
                  pUser->updateSelectionChangeForHover();
                  break;
               }
            }
         }
         break;
      }
      case cDataAmmoRegenRate:
         pCurrentProtoObject->setAmmoRegenRate(calcAmount(pBaseProtoObject->getAmmoRegenRate(), pCurrentProtoObject->getAmmoRegenRate(), unapply));
         break;
      case cDataDisplayNameID:
         if (!unapply)
            pCurrentProtoObject->setDisplayNameIndex(mObjectType);
         break;
      case cDataIcon:
         if (!unapply)
         {
            if (pPlayer && pPlayer->getUser())
            {
               int id = -1;
               BUIContext* pUIContext = pPlayer->getUser()->getUIContext();
               BDEBUG_ASSERT(pUIContext);
               if (pUIContext)
               {               
                  if (mObjectType == 0 || mObjectType == -1)
                     id = pUIContext->lookupIconID("uniticon", mActionName);
                  else if (mObjectType == 1)
                     id = pUIContext->lookupIconID("buildingicon", mActionName);
                  else if (mObjectType == 2)
                     id = pUIContext->lookupIconID("miscicon", mActionName);
                  else if (mObjectType == 3)
                     id = pUIContext->lookupIconID("techicon", mActionName);
               }

               if (id != -1)
                  pCurrentProtoObject->setCircleMenuIconID(id);
            }
         }
         break;
         case cDataShieldRegenRate:
            {
               long playerID=pPlayer->getID();
               BProtoObjectID protoID=pCurrentProtoObject->getID();

               if(pPlayer->getNumUnitsOfType(protoID) > 0)
               {
                  //FIXME AJL 7/12/06 - Need to keep a list of units per player and per proto object type so we don't
                  // have to traverse the full list
                  BEntityHandle handle=cInvalidObjectID;
                  for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
                  {
                     //FIXME AJL 7/12/06 - Sucks that the player ID and proto ID in the handle is invalid here
                     // which causes us to have to deref the pUnit to get that info
                     //if(handle.getPlayerID()==playerID && handle.getProtoID()==protoID)
                     if(pUnit->getPlayerID()==playerID && pUnit->getProtoID()==protoID)
                     {
   #ifdef SYNC_Tech
                        syncTechData("BTechEffect::applyProtoObjectEffect setShieldRegenRate", mAmount);
   #endif
                        pUnit->setShieldRegenRate(mAmount);
                     }
                  }
               }
            }
            break;
         case cDataShieldRegenDelay:
            {
               long playerID=pPlayer->getID();
               BProtoObjectID protoID=pCurrentProtoObject->getID();

               if(pPlayer->getNumUnitsOfType(protoID) > 0)
               {
                  //FIXME AJL 7/12/06 - Need to keep a list of units per player and per proto object type so we don't
                  // have to traverse the full list
                  BEntityHandle handle=cInvalidObjectID;
                  for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
                  {
                     //FIXME AJL 7/12/06 - Sucks that the player ID and proto ID in the handle is invalid here
                     // which causes us to have to deref the pUnit to get that info
                     //if(handle.getPlayerID()==playerID && handle.getProtoID()==protoID)
                     if(pUnit->getPlayerID()==playerID && pUnit->getProtoID()==protoID)
                     {
   #ifdef SYNC_Tech
                        syncTechData("BTechEffect::applyProtoObjectEffect setShieldRegenDelay", mAmount);
   #endif
                        pUnit->setShieldRegenDelay(mAmount);
                     }
                  }
               }
            }
            break;

            
      case cDataDeathSpawn:
         if (mObjectType != -1)
            pCurrentProtoObject->setDeathSpawnSquad(mObjectType);
         break;
   }


   //Update dps info:
   if(pCurrentProtoObject != NULL)
   {
      BTactic* pCurrentTactic = pCurrentProtoObject->getTactic();
      if(pCurrentTactic != NULL)
      {
         pCurrentTactic->updateAttackRatings(pPlayer);
         

         long squadCount=pPlayer->getNumberProtoSquads();
         for(long i=0; i<squadCount; i++)
         {
            BProtoSquad* pProtoSquad=pPlayer->getProtoSquad(i);
            // Skip auto-created unit specific proto squads
            if (pProtoSquad->getProtoObjectID()!=-1)
               continue;
            long numUnitNodes = pProtoSquad->getNumberUnitNodes();
            for (long i=0; i<numUnitNodes; i++)
            {
               const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
               const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
               if (!pProtoObject)
                  continue;
               if(pProtoObject->getID() == pCurrentProtoObject->getID())  
               {
                  pProtoSquad->updateAttackRatings();  
                  break;
               }
            }
         
         }


      }
   }

}

//==============================================================================
// Exposed function for trigger system apply changes to proto data
//==============================================================================
void BTechEffect::applyProtoSquadEffect(BPlayer* pPlayer, BProtoSquad* pCurrentProtoSquad, BProtoSquad* pBaseProtoSquad, float amount, long dataSubType, long relativity, bool allActions, BSimString actionName, long resource, long objectType, bool unapply /*= false*/)
{
   mAmount = amount;
   mDataType = dataSubType;
   mRelativity = relativity;
   mAllActions = allActions;
   mActionName = actionName;
   mResource = resource;
   mObjectType = objectType;

   applyProtoSquadEffect(pPlayer, pCurrentProtoSquad, pBaseProtoSquad, unapply);
}

//==============================================================================
// Apply a tech effect to a ProtoSquad
//==============================================================================
void BTechEffect::applyProtoSquadEffect(BPlayer* pPlayer, BProtoSquad* pCurrentProtoSquad, BProtoSquad* pBaseProtoSquad, bool unapply)
{   
   // Halwes - 12/12/2007 - These tech data sub type tags need to be reflected in techs.xml with TechDataSubTypeProtoSquad tags for the editor and trigger system
   //                       to work correctly.
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   switch (mDataType)
   {
      case cDataEnable:
         if (unapply)
            pCurrentProtoSquad->setFlagAvailable(!(mAmount > 0.0f));
         else
            pCurrentProtoSquad->setFlagAvailable((mAmount > 0.0f));
         break;

      case cDataBuildPoints:
         pCurrentProtoSquad->setBuildPoints(calcAmount(pBaseProtoSquad->getBuildPoints(), pCurrentProtoSquad->getBuildPoints(), unapply));
         break;

      case cDataCost:
         {
            BCost baseCost = *(pBaseProtoSquad->getCost());
            BCost currentCost = *(pCurrentProtoSquad->getCost());
            if (mResource == -1)
            {
               for (long i = 0; i < currentCost.getNumberResources(); i++)
               {
                  float baseVal = baseCost.get(i);
                  float currentVal = currentCost.get(i);
                  currentCost.set(i, calcAmount(baseVal, currentVal, unapply));
               }
               pCurrentProtoSquad->setCost(&currentCost);
            }
            else
            {
               float baseVal = baseCost.get(mResource);
               float currentVal = currentCost.get(mResource);
               currentCost.set(mResource, calcAmount(baseVal, currentVal, unapply));
               pCurrentProtoSquad->setCost(&currentCost);
            }
            break;
         }

      case cDataLevel:
         {
            int oldLevel = pCurrentProtoSquad->getLevel();
            int newLevel = (int)calcAmount((float)pBaseProtoSquad->getLevel(), (float)pCurrentProtoSquad->getLevel(), unapply);
            pCurrentProtoSquad->setLevel(newLevel);
            if (mAllActions && (newLevel > oldLevel))
            {
               BEntityIDArray squads;
               pPlayer->getSquadsOfType(pCurrentProtoSquad->getID(), squads);
               uint squadCount = squads.getSize();   
               for (uint j=0; j<squadCount; j++)
               {
                  BSquad* pSquad = gWorld->getSquad(squads[j]);
                  if (pSquad)
                     pSquad->upgradeLevel(newLevel, true);
               }
            }
            break;
         }

      case cDataTechLevel:
         {
            int newLevel = (int)calcAmount((float)pBaseProtoSquad->getTechLevel(), (float)pCurrentProtoSquad->getTechLevel(), unapply);
            pCurrentProtoSquad->setTechLevel(newLevel);
            break;
         }

      case cDataTechHPBar:
         {
            if (mObjectType != -1)
               pCurrentProtoSquad->setHPBarID(mObjectType);
            break;
         }

      case cDataDisplayNameID:
         if (!unapply)
            pCurrentProtoSquad->setDisplayNameIndex(mObjectType);
         break;

      case cDataIcon:
      case cDataAltIcon:
         if (!unapply)
         {                        
            int id = -1;
            if (pPlayer && pPlayer->getUser())
            {
               BUIContext* pUIContext = pPlayer->getUser()->getUIContext();
               if (pUIContext)
               {
                  if (mObjectType == 0 || mObjectType == -1)
                     id = pUIContext->lookupIconID("uniticon", mActionName);
                  else if (mObjectType == 1)
                     id = pUIContext->lookupIconID("buildingicon", mActionName);
                  else if (mObjectType == 2)
                     id = pUIContext->lookupIconID("miscicon", mActionName);
                  else if (mObjectType == 3)
                     id = pUIContext->lookupIconID("techicon", mActionName);
               }
            }

            if (id != -1)
            {
               if (mDataType == cDataIcon)
                  pCurrentProtoSquad->setCircleMenuIconID(id);
               else
                  pCurrentProtoSquad->setAltCircleMenuIconID(id);
            }
         }
         break;
   }
   // Halwes - 12/12/2007 - These tech data sub type enums need to be reflected in techs.xml with TechDataSubTypeProtoSquad tags for the editor and trigger system
   //                       to work correctly.
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

//==============================================================================
// BTechEffect::applyProtoActionEffect
//==============================================================================
void BTechEffect::applyProtoActionEffect(BTactic* pCurrentTactic, BTactic* pBaseTactic, long actionID, bool unapply)
{
   BProtoAction* pCurrentAction=pCurrentTactic->getProtoAction(actionID);
   if(!pCurrentAction)
      return;
//-- FIXING PREFIX BUG ID 2354
   const BProtoAction* pBaseAction=pBaseTactic->getProtoAction(actionID);
//--
   if(!pBaseAction)
      return;
   switch(mDataType)
   {
      case cDataWorkRate: 
         pCurrentAction->setWorkRate(calcAmount(pBaseAction->getWorkRate(), pCurrentAction->getWorkRate(), unapply));
         break;
      case cDataActionEnable:
      {
         bool val=(mAmount == 0.0f);
         if(unapply)
            val=!val;
         pCurrentAction->setFlagDisabled(val);

         break;
      }
      case cDataTurretYawRate:
      {
         //mEffectValue == hardpointID
         if(mEffectValue!= -1 && pCurrentAction->getHardpointID() == mEffectValue)
         {
            float oldVal = pCurrentAction->getStrafingTrackingSpeed();
            float baseVal = pBaseAction->getStrafingTrackingSpeed();
            float newVal = calcAmount(baseVal, oldVal, unapply);
            pCurrentAction->setStrafingTurnRate(newVal);
         }
         break;
      }
      case cDataBoardTime:
         pCurrentAction->setJoinBoardTime(calcAmount(pBaseAction->getJoinBoardTime(), pCurrentAction->getJoinBoardTime(), unapply));
         break;
   }
}

//==============================================================================
// BTechEffect::applyWeaponEffect
//==============================================================================
void BTechEffect::applyWeaponEffect(BPlayer* pPlayer, int protoObjectID, BTactic* pCurrentTactic, BTactic* pBaseTactic, long weaponID, bool unapply)
{
   BWeapon* pCurrentWeapon=const_cast<BWeapon*>(pCurrentTactic->getWeapon(weaponID));
   if(!pCurrentWeapon)
      return;
   const BWeapon* pBaseWeapon=pBaseTactic->getWeapon(weaponID);
   if(!pBaseWeapon)
      return;
   switch(mDataType)
   {
      case cDataMaximumRange:
         pCurrentWeapon->mMaxRange=calcAmount(pBaseWeapon->mMaxRange, pCurrentWeapon->mMaxRange, unapply);
         pCurrentTactic->updateOverallRanges();
         break;
      case cDataDamage:
      {
         float dps=pCurrentWeapon->mDamagePerSecond;
         pCurrentWeapon->mDamagePerSecond=calcAmount(pBaseWeapon->mDamagePerSecond, pCurrentWeapon->mDamagePerSecond, unapply);
         
         //-- Find all the Proto Actions using this weapon and adjust their DPA
         BProtoAction *pCurrentProtoAction = NULL;
         long numProtoActions = pCurrentTactic->getNumberProtoActions();
         for(long i=0; i < numProtoActions; i++)
         {
            pCurrentProtoAction = pCurrentTactic->getProtoAction(i);
            if(!pCurrentProtoAction)
               continue;

            if(pCurrentProtoAction->getWeaponID() == weaponID)
            {  
               float pct=pCurrentProtoAction->getDamagePerAttack()/dps;
               pCurrentProtoAction->setDamagePerAttack(pCurrentWeapon->mDamagePerSecond*pct);               
            }
         }
        
         // Update attack ratings for the tactic and any proto squads that refer to this tactic
         pCurrentTactic->updateAttackRatings(pPlayer);
         // AJL FIXME 5/2/07 - At database load time, we should create a list on the proto object of all proto squads 
         //   that refer to the proto object so that we don't have to manually go through all the proto squads.
         long squadCount=pPlayer->getNumberProtoSquads();
         for(long i=0; i<squadCount; i++)
         {
            BProtoSquad* pProtoSquad=pPlayer->getProtoSquad(i);
            // Skip auto-created unit specific proto squads
            if (pProtoSquad->getProtoObjectID()!=-1)
               continue;
            long unitCount=pProtoSquad->getNumberUnitNodes();
            for(long j=0; j<unitCount; j++)
            {
               const BProtoSquadUnitNode unitNode=pProtoSquad->getUnitNode(j);
               if (unitNode.mUnitType == protoObjectID)
               {
                  pProtoSquad->updateAttackRatings();
                  break;
               }
            }
         }
         break;
      }      
      case cDataMinRange:
         pCurrentWeapon->mMinRange=calcAmount(pBaseWeapon->mMinRange, pCurrentWeapon->mMinRange, unapply);
         break;
      case cDataAOERadius:
         pCurrentWeapon->mAOERadius=calcAmount(pBaseWeapon->mAOERadius, pCurrentWeapon->mAOERadius, unapply);
         break;
      case cDataAOEPrimaryTargetFactor:
         pCurrentWeapon->mAOEPrimaryTargetFactor=calcAmount(pBaseWeapon->mAOEPrimaryTargetFactor, pCurrentWeapon->mAOEPrimaryTargetFactor, unapply);
         break;
      case cDataAOEDistanceFactor:
         pCurrentWeapon->mAOEDistanceFactor=calcAmount(pBaseWeapon->mAOEDistanceFactor, pCurrentWeapon->mAOEDistanceFactor, unapply);
         break;
      case cDataAOEDamageFactor:   
         pCurrentWeapon->mAOEDamageFactor=calcAmount(pBaseWeapon->mAOEDamageFactor, pCurrentWeapon->mAOEDamageFactor, unapply);
         break;
      case cDataAccuracy:
         pCurrentWeapon->mAccuracy=calcAmount(pBaseWeapon->mAccuracy, pCurrentWeapon->mAccuracy, unapply);
         break;
      case cDataMovingAccuracy:
         pCurrentWeapon->mMovingAccuracy=calcAmount(pBaseWeapon->mMovingAccuracy, pCurrentWeapon->mMovingAccuracy, unapply);
         break;
      case cDataMaxDeviation:
         pCurrentWeapon->mMaxDeviation=calcAmount(pBaseWeapon->mMaxDeviation, pCurrentWeapon->mMaxDeviation, unapply);
         break;
      case cDataMovingMaxDeviation:
         pCurrentWeapon->mMovingMaxDeviation=calcAmount(pBaseWeapon->mMovingMaxDeviation, pCurrentWeapon->mMovingMaxDeviation, unapply);
         break;
      case cDataAccuracyDistanceFactor:
         pCurrentWeapon->mAccuracyDistanceFactor=calcAmount(pBaseWeapon->mAccuracyDistanceFactor, pCurrentWeapon->mAccuracyDistanceFactor, unapply);
         break;
      case cDataAccuracyDeviationFactor:
         pCurrentWeapon->mAccuracyDeviationFactor=calcAmount(pBaseWeapon->mAccuracyDeviationFactor, pCurrentWeapon->mAccuracyDeviationFactor, unapply);
         break;
      case cDataMaxVelocityLead:
         pCurrentWeapon->mMaxVelocityLead=calcAmount(pBaseWeapon->mMaxVelocityLead, pCurrentWeapon->mMaxVelocityLead, unapply);
         break;
      case cDataMaxDamagePerRam:
         pCurrentWeapon->mMaxDamagePerRam=calcAmount(pBaseWeapon->mMaxDamagePerRam, pCurrentWeapon->mMaxDamagePerRam, unapply);
         break;
      case cDataReflectDamageFactor:
         pCurrentWeapon->mReflectDamageFactor=calcAmount(pBaseWeapon->mReflectDamageFactor, pCurrentWeapon->mReflectDamageFactor, unapply);
         break;
      case cDataAirBurstSpan:
         pCurrentWeapon->mAirBurstSpan=calcAmount(pBaseWeapon->mAirBurstSpan, pCurrentWeapon->mAirBurstSpan, unapply);
         break;
      case cDataDOTrate:
         pCurrentWeapon->mDOTrate=calcAmount(pBaseWeapon->mDOTrate, pCurrentWeapon->mDOTrate, unapply);
         break;
      case cDataDOTduration:
         pCurrentWeapon->mDOTduration=calcAmount(pBaseWeapon->mDOTduration, pCurrentWeapon->mDOTduration, unapply);
         break;
   }
}

//==============================================================================
// BTechEffect::calcAmount
//==============================================================================
float BTechEffect::calcAmount(float baseVal, float currentVal, bool invert)
{
   switch(mRelativity)
   {
      case cRelativityAbsolute:
         if(invert)
            return currentVal-mAmount;
         else
            return currentVal+mAmount;
      case cRelativityBasePercent:
         if( invert )
         {
            return( currentVal - ( ( baseVal * mAmount ) - baseVal ) );
         }
         else
         {
            return( currentVal + ( ( baseVal * mAmount ) - baseVal ) );
         }
      case cRelativityPercent:
         if(invert)
            return currentVal/mAmount;
         else
            return currentVal*mAmount;
      case cRelativityAssign:
         return mAmount;
      case cRelativityBasePercentAssign:
         if( invert )
         {
            return( baseVal / mAmount );
         }
         else
         {
            return( baseVal * mAmount );
         }
      default:
         return currentVal;
   }
}

//==============================================================================
//==============================================================================
void BTechEffect::doProtoSquadTransform(BPlayer* pPlayer, BProtoSquadID fromProtoID, BProtoSquadID toProtoID)
{
   BProtoSquad* pFromProtoSquad=pPlayer->getProtoSquad(fromProtoID);
   const BProtoSquad* pToProtoSquad=pPlayer->getProtoSquad(toProtoID);

   if(pFromProtoSquad && pToProtoSquad)
   {
      BEntityIDArray squads;
      pPlayer->getSquadsOfType(fromProtoID, squads);
      uint count=squads.getSize();

      // Pre-transform squads - Add missing squad members to existing squads that the proto squad
      if(count > 0)
      {
         for (uint i=0; i<count; i++)
         {
            BSquad* pSquad=gWorld->getSquad(squads[i]);
            // Don't transform squads in this list with unique protoSquads
            if (pSquad && (!pSquad->getProtoSquad() || !pSquad->getProtoSquad()->getFlagUniqueInstance()))
               pSquad->preTransform(pFromProtoSquad, pToProtoSquad, true);
         }
      }

      // Transform the proto squad
      pFromProtoSquad->transform(pToProtoSquad);

      // Post-transform squads
      if(count > 0)
      {
         for (uint i=0; i<count; i++)
         {
            BSquad* pSquad=gWorld->getSquad(squads[i]);
            // Don't transform squads in this list with unique protoSquads
            if (pSquad && (!pSquad->getProtoSquad() || !pSquad->getProtoSquad()->getFlagUniqueInstance()))
               pSquad->postTransform(pFromProtoSquad, pToProtoSquad, true);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BTechEffect::getTargets(BXMLNode &root)
{
   BSimString tempStr;

   long nodeCount = root.getNumberChildren();
   for (long i = 0; i < nodeCount; i++)
   {
      BXMLNode  node(root.getChild(i));
      const BPackedString name(node.getName());
      
      if (name == "Target")
      {
         BSimString  targetTypeName;
         if (!node.getAttribValue("Type", &targetTypeName))
            continue;
         BTechEffectTarget target;            
         
         if (targetTypeName == "ProtoUnit")
         {
            target.mTargetType = cTargetProtoUnit;
            target.mTargetID = gDatabase.getObjectType(node.getTextPtr(tempStr));              

            if (target.mTargetID != -1)
               mTargets.add(target);              
         }
         else if (targetTypeName == "ProtoSquad")
         {
            target.mTargetType = cTargetProtoSquad;
            target.mTargetID = gDatabase.getProtoSquad(node.getTextPtr(tempStr));
            if (target.mTargetID != -1)
               mTargets.add(target);
         }
         else if (targetTypeName == "Unit")
         {
            target.mTargetType = cTargetUnit;
            target.mTargetID = -1;
            mTargets.add(target);
         }
         else if (targetTypeName == "Tech")
         {
            target.mTargetType = cTargetTech;
            target.mTargetID = gDatabase.getProtoTech(node.getTextPtr(tempStr));
            if (target.mTargetID != -1)
               mTargets.add(target);
         }
         else if (targetTypeName == "TechAll")
         {
            target.mTargetType = cTargetTechAll;
            target.mTargetID = -1;
            mTargets.add(target);
         }
         else if (targetTypeName == "Player")
         {
            target.mTargetType = cTargetPlayer;
            target.mTargetID = -1;
            mTargets.add(target);
         }              
      }
   }
}

//==============================================================================
// ability.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "ability.h"
#include "database.h"
#include "xmlreader.h"

//==============================================================================
// BAbility::BAbility
//==============================================================================
BAbility::BAbility() :
   mID(-1),
   mType(-1),
   mAmmoCost(0.0f),
   mSquadMode(-1),
   mRecoverStart(-1),
   mRecoverType(-1),
   mRecoverTime(0.0f),
   mMovementSpeedModifier(0.0f),
   mMovementModifierType(cMovementModifierMode),
   mCircleMenuIconID(-1),
   mDisplayNameIndex(-1),
   mDisplayName2Index(-1),
   mRolloverTextIndex(-1),
   mTargetType(cTargetNone),
   mRecoverAnimAttachment(-1),
   mRecoverStartAnim(-1),
   mRecoverEndAnim(-1),
   mSprinting(false),
   mDontInterruptAttack(false),
   mKeepSquadMode(false),
   mAttackSquadMode(false),
   mDuration(0.0f),
   mDamageTakenModifier(0.0f),
   mAccuracyModifier(0.0f),
   mDodgeModifier(0.0f),
   mSmartTargetRange(15.0f),
   mCanHeteroCommand(true),
   mNoAbilityReticle(false)
{
}

//==============================================================================
// BAbility::load
//==============================================================================
bool BAbility::load(BXMLNode root)
{
   if (!root.getAttribValueAsString("Name", mName))
      return(false);

   BSimString tempStr;

   long nodeCount = root.getNumberChildren();
   for (long i=0; i<nodeCount; i++)
   {
      BXMLNode node = root.getChild(i);
      const BPackedString nodeName(node.getName());
      
      if (nodeName == "Type")
         mType = gDatabase.getAbilityType(node.getTextPtr(tempStr));
      else if (nodeName == "AmmoCost")
         node.getTextAsFloat(mAmmoCost);
      else if (nodeName == "Object")
      {
         long objectID = gDatabase.getProtoObject(node.getTextPtr(tempStr));
         if(objectID!=-1)
            mObjects.add(objectID);
      }
      else if (nodeName == "SquadMode")
         mSquadMode = gDatabase.getSquadMode(node.getTextPtr(tempStr));
      else if (nodeName == "RecoverStart")
         mRecoverStart = gDatabase.getRecoverType(node.getTextPtr(tempStr));
      else if (nodeName == "RecoverType")
         mRecoverType = gDatabase.getRecoverType(node.getTextPtr(tempStr));
      else if (nodeName == "RecoverTime")
         node.getTextAsFloat(mRecoverTime);
      else if (nodeName == "MovementSpeedModifier")
         node.getTextAsFloat(mMovementSpeedModifier);
      else if (nodeName == "MovementModifierType")
      {
         node.getText(tempStr);
         if (tempStr == "Ability")
            mMovementModifierType = cMovementModifierAbility;
         else if (tempStr == "Mode")
            mMovementModifierType = cMovementModifierMode;
      }
      else if (nodeName == "DamageTakenModifier")
         node.getTextAsFloat(mDamageTakenModifier);
      else if (nodeName == "DodgeModifier")
         node.getTextAsFloat(mDodgeModifier);
      else if (nodeName == "Icon")
         node.getText(mIconTextureName);
      else if (nodeName == "DisplayNameID")
      {
         long id;
         if(node.getTextAsLong(id))
            mDisplayNameIndex=gDatabase.getLocStringIndex(id);
      }
      else if (nodeName == "DisplayName2ID")
      {
         long id;
         if(node.getTextAsLong(id))
            mDisplayName2Index=gDatabase.getLocStringIndex(id);
      }
      else if (nodeName == "RolloverTextID")
      {
         long id;
         if(node.getTextAsLong(id))
           mRolloverTextIndex=gDatabase.getLocStringIndex(id);
      }
      else if (nodeName == "TargetType")
      {
         node.getText(tempStr);
         if (tempStr == "Location")
            mTargetType = cTargetLocation;
         else if (tempStr == "Unit")
            mTargetType = cTargetUnit;
         else if (tempStr == "UnitOrLocation")
            mTargetType = cTargetUnitOrLocation;
         else
            mTargetType = cTargetNone;
      }
      else if (nodeName == "RecoverAnimAttachment")
      {
         node.getText(tempStr);
         mRecoverAnimAttachment = gVisualManager.getAttachmentType(tempStr);
      }
      else if (nodeName == "RecoverStartAnim")
      {
         node.getText(tempStr);
         mRecoverStartAnim = gVisualManager.getAnimType(tempStr);
      }
      else if (nodeName == "RecoverEndAnim")
      {
         node.getText(tempStr);
         mRecoverEndAnim = gVisualManager.getAnimType(tempStr);
      }
      else if (nodeName == "Sprinting")
      {
         bool sprinting = false;
         node.getTextAsBool(sprinting);   
         mSprinting = sprinting;
      }
      else if (nodeName == "DontInterruptAttack")
      {
         bool dontInterruptAttack = false;
         node.getTextAsBool(dontInterruptAttack);   
         mDontInterruptAttack = dontInterruptAttack;
      }
      else if (nodeName == "KeepSquadMode")
      {
         bool keepSquadMode = false;
         node.getTextAsBool(keepSquadMode);   
         mKeepSquadMode = keepSquadMode;
      }
      else if (nodeName == "AttackSquadMode")
      {
         bool val = false;
         node.getTextAsBool(val);
         mAttackSquadMode = val;
      }
      else if (nodeName == "Duration")
      {
         node.getTextAsFloat(mDuration);
	   }
      else if (nodeName == "SmartTargetRange")
      {
         node.getTextAsFloat(mSmartTargetRange);
      }
      else if (nodeName == "CanHeteroCommand")
      {
         bool val = true;
         node.getTextAsBool(val);
         mCanHeteroCommand = val;
      }
      else if (nodeName == "NoAbilityReticle")
      {
         bool val=false;
         node.getTextAsBool(val);
         mNoAbilityReticle=val;
      }
   }

   return(true);
}

//==============================================================================
// BAbility::getPostAmmoCost
//==============================================================================
bool BAbility::getPostAmmoCost()
{
   return false;
}

//==============================================================================
// BAbility::getDisplayName
//==============================================================================
void BAbility::getDisplayName(BUString& string) const
{
   if(mDisplayNameIndex==-1)
      string=mName.getPtr();
   else
      string=gDatabase.getLocStringFromIndex(mDisplayNameIndex);
}

//==============================================================================
// BAbility::getDisplayName2
//==============================================================================
void BAbility::getDisplayName2(BUString& string) const
{
   if(mDisplayName2Index==-1)
      getDisplayName(string);
   else
      string=gDatabase.getLocStringFromIndex(mDisplayName2Index);
}

//==============================================================================
// BAbility::getRolloverText
//==============================================================================
void BAbility::getRolloverText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mRolloverTextIndex);
}

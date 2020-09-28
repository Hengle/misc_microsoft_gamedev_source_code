//==============================================================================
// weapontype.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "common.h"
#include "weapontype.h"
#include "database.h"
#include "xmlreader.h"
#include "protoobject.h"
#include "unit.h"
#include "world.h"


//==============================================================================
// BWeapon::BWeapon
//==============================================================================
BWeaponType::BWeaponType()
{
   mShieldedModifier = 1.0f;
}

//==============================================================================
// BWeaponType::init
//==============================================================================
void BWeaponType::init(BWeaponType* pBase)
{
   mName=pBase->mName;
   mDeathAnimation = pBase->mDeathAnimation;
   mModifiers=pBase->mModifiers;
   mShieldedModifier=pBase->mShieldedModifier;
}

//==============================================================================
// BWeaponType::loadWeaponType
//==============================================================================
bool BWeaponType::loadWeaponType(BXMLNode root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {                 
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      if(name == "Name")
         node.getText(mName);
      else if (name == "DeathAnimation")
         node.getText(mDeathAnimation);
      else if(name == "DamageModifier")
      {
         BWeaponModifier& modifier = mModifiers.grow();
         //-- Load the damage Type
         BSimString damageType;
         node.getAttribValue("type", &damageType);
         modifier.mTargetDamageType = gDatabase.getDamageType(damageType);

         //-- Load the damage rating
         node.getAttribValueAsFloat("rating", modifier.mDamageRating);
         
         //-- Load Damage Percentage
         node.getTextAsFloat(modifier.mDamagePercentage);

         if (modifier.mTargetDamageType == gDatabase.getDamageTypeShielded())
            mShieldedModifier = modifier.mDamagePercentage;

         // Reflection damage percentage
         node.getAttribValueAsFloat("reflectDamageFactor", modifier.mReflectDamageFactor);

         // Load bowlable flag
         bool bowlable = false;
         if (node.getAttribValueAsBool("bowlable", bowlable))
            modifier.mBowlable = bowlable;

         // Load rammable flag
         bool rammable = false;
         if (node.getAttribValueAsBool("rammable", rammable))
            modifier.mRammable = rammable;

         //-- If we failed to find the object type, then discard the modifier.
         if(modifier.mTargetDamageType == -1)
         {
            blogtrace("Error loading weaponType. Unable to find damageType %s", damageType.getPtr());
            mModifiers.popBack();
         }
      }
   }
   return true;
}

//==============================================================================
// BWeaponType::getDamagePercentage
//==============================================================================
float BWeaponType::getDamagePercentage(BEntityID targetID, BVector damageDirection)
{
   const BObject* pTargetObject = gWorld->getObject(targetID);
   if (!pTargetObject)
      return 1.0f;

   const BUnit* pTargetUnit = pTargetObject->getUnit();
   int mode = (pTargetUnit ? pTargetUnit->getDamageTypeMode() : 0);

   BDamageTypeID damageType = pTargetObject->getProtoObject()->getDamageType(damageDirection, pTargetObject->getForward(), pTargetObject->getRight(), false, true, mode);

   return getDamagePercentage(damageType);
}

//==============================================================================
// BWeaponType::getDamagePercentage
//==============================================================================
float BWeaponType::getDamagePercentage(long damageType) const
{
   for (long i=0; i<mModifiers.getNumber(); i++)
   {
      if (damageType == mModifiers[i].mTargetDamageType)
         return mModifiers[i].mDamagePercentage;
   }
   return 1.0f;
}

//==============================================================================
// BWeaponType::getDamageRating
//==============================================================================
float BWeaponType::getDamageRating(long damageType) const
{
   for (long i=0; i<mModifiers.getNumber(); i++)
   {
      if (damageType == mModifiers[i].mTargetDamageType)
         return mModifiers[i].mDamageRating;
   }
   return 1.0f;
}

//==============================================================================
// BWeaponType::getDamageModifier
//==============================================================================
float BWeaponType::getDamageModifier(int damageType) const
{
   for (long i=0; i<mModifiers.getNumber(); i++)
   {
      if (damageType == mModifiers[i].mTargetDamageType)
         return mModifiers[i].mDamagePercentage;
   }
   return 1.0f;
}

//==============================================================================
// BWeaponType::setDamageModifier
//==============================================================================
void BWeaponType::setDamageModifier(int damageType, float val)
{
   for (long i=0; i<mModifiers.getNumber(); i++)
   {
      if (damageType == mModifiers[i].mTargetDamageType)
      {
         mModifiers[i].mDamagePercentage=val;
         break;
      }
   }
}

//==============================================================================
//==============================================================================
float BWeaponType::getReflectDamageFactor(long damageType) const
{
   for (long i=0; i<mModifiers.getNumber(); i++)
   {
      if (damageType == mModifiers[i].mTargetDamageType)
         return mModifiers[i].mReflectDamageFactor;
   }
   return 0.0f;
}

//==============================================================================
//==============================================================================
bool BWeaponType::getBowlable(int damageType) const
{
   for (long i=0; i<mModifiers.getNumber(); i++)
   {
      if (damageType == mModifiers[i].mTargetDamageType)
         return mModifiers[i].mBowlable;
   }
   return false;
}

//==============================================================================
//==============================================================================
bool BWeaponType::getRammable(int damageType) const
{
   for (long i=0; i<mModifiers.getNumber(); i++)
   {
      if (damageType == mModifiers[i].mTargetDamageType)
         return mModifiers[i].mRammable;
   }
   return false;
}

//==============================================================================
//==============================================================================
long BWeaponType::getDeathAnimType() const
{
   if (!mDeathAnimation.isEmpty())
   {
      // There was an issue where the death anim type wasn't defined so in archive
      // builds this returned idle.  For death anims, it's better to return -1 because
      // the anim system will look up the default death anim which is better than idle.
      bool found = false;
      long animType = gVisualManager.getAnimType(mDeathAnimation, found);
      if (found)
         return animType;
   }
   return -1;
}


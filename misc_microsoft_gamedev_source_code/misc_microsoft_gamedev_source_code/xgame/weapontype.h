//==============================================================================
// weapontype.h
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#include "xmlreader.h"

class BWeaponModifier
{
public:

   BWeaponModifier() : mTargetDamageType(-1), mDamagePercentage(1.0f), mDamageRating(1.0f), mReflectDamageFactor(0.0f), mBowlable(false), mRammable(false)
   {}

   BWeaponModifier& operator=(const BWeaponModifier& rhs)
   {      
      mTargetDamageType=rhs.mTargetDamageType;
      mDamagePercentage=rhs.mDamagePercentage;
      mDamageRating=rhs.mDamageRating;
      mReflectDamageFactor=rhs.mReflectDamageFactor;
      mBowlable=rhs.mBowlable;
      mRammable=rhs.mRammable;
      return *this;
   }

   long mTargetDamageType;
   float mDamagePercentage;
   float mDamageRating;
   float mReflectDamageFactor;
   bool mBowlable:1;
   bool mRammable:1;
};

//==============================================================================
// BWeaponType
//==============================================================================
class BWeaponType
{
public:
   BWeaponType();
   ~BWeaponType() {}

   void init(BWeaponType* pBase);
   bool loadWeaponType(BXMLNode root);

   const BSimString& getName() const {return mName;}
   float getDamagePercentage(BEntityID targetID, BVector damageDirection);
   float getDamagePercentage(long damageType) const;
   float getDamageRating(long damageType) const;
   float getShieldedModifier() const { return mShieldedModifier; }

   float getDamageModifier(int damageType) const;
   void setDamageModifier(int damageType, float val);

   float getReflectDamageFactor(long damageType) const;
   bool  getBowlable(int damageType) const;
   bool  getRammable(int damageType) const;

   long  getDeathAnimType() const;

private:
   BSimString mName;
   BSimString mDeathAnimation;
   BDynamicSimArray<BWeaponModifier> mModifiers;
   float mShieldedModifier;
};

//==============================================================================
// ability.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// BAbility
//==============================================================================
class BAbility
{
   public:
      enum
      {
         cTargetNone,
         cTargetLocation,
         cTargetUnit,
         cTargetUnitOrLocation,
      };

      enum
      {
         cMovementModifierAbility,
         cMovementModifierMode,
         cDamageTakenModifierAbility,
         cDodgeModifierAbility,
      };

      BAbility();

      bool load(BXMLNode root);

      void setID(long id) { mID=id; }
      long getID() const { return mID; }

      const BSimString& getName(void) const { return mName; }
      const BSimString& getIconTextureName(void) const { return(mIconTextureName); }

      long getType() const { return mType; }

      float getAmmoCost() const { return mAmmoCost; }

      long getSquadMode() const { return mSquadMode; }
      int getRecoverStart() const { return mRecoverStart; }
      int getRecoverType() const { return mRecoverType; }
      float getRecoverTime() const { return mRecoverTime; }
      float getAbilityDuration() const { return mDuration; }
      float getMovementSpeedModifier() const { return mMovementSpeedModifier; }
      int getMovementModifierType() const { return mMovementModifierType; }
      float getDamageTakenModifier() const { return mDamageTakenModifier; }
      float getAccuracyModifier() const { return mAccuracyModifier; }
      float getDodgeModifier() const { return mDodgeModifier; }

      long getNumberObjects() const { return mObjects.getNumber(); }
      long getObject(long index) const { if(index>=0 && index<mObjects.getNumber()) return mObjects[index]; else return -1; }

      void getDisplayName(BUString& string) const;
      void getDisplayName2(BUString& string) const;
      void getRolloverText(BUString& string) const;

      int  getCircleMenuIconID() const { return mCircleMenuIconID; }
      void setCircleMenuIconID(int id) { mCircleMenuIconID = id; }

      bool getPostAmmoCost();

      int getTargetType() const { return mTargetType; }

      int getRecoverAnimAttachment() const { return mRecoverAnimAttachment; }
      int getRecoverStartAnim() const { return mRecoverStartAnim; }
      int getRecoverEndAnim() const { return mRecoverEndAnim; }

      bool getSprinting() const { return mSprinting; }
      bool getDontInterruptAttack() const { return mDontInterruptAttack; }
      bool getKeepSquadMode() const { return mKeepSquadMode; }
      bool getAttackSquadMode() const { return mAttackSquadMode; }
      bool getCanHeteroCommand() const { return mCanHeteroCommand; }
      bool getNoAbilityReticle() const { return mNoAbilityReticle; }

      float getSmartTargetRange() const { return mSmartTargetRange; }

   protected:
      long mID;
      BSimString mName;
      long mType;
      float mAmmoCost;
      BSmallDynamicSimArray<long> mObjects;
      long mSquadMode;
      int mRecoverStart;
      int mRecoverType;
      float mRecoverTime;
      float mMovementSpeedModifier;
      int mMovementModifierType;
      float mDamageTakenModifier;
      float mAccuracyModifier;
      float mDodgeModifier;
      BSimString mIconTextureName;
      int mCircleMenuIconID;
      long mDisplayNameIndex;
      long mDisplayName2Index;
      long mRolloverTextIndex;
      int mTargetType;
      int mRecoverAnimAttachment;
      int mRecoverStartAnim;
      int mRecoverEndAnim;
      float mDuration;
      float mSmartTargetRange;

      bool mSprinting:1;
      bool mDontInterruptAttack:1;
      bool mKeepSquadMode:1;
      bool mAttackSquadMode:1;
      bool mCanHeteroCommand:1;
      bool mNoAbilityReticle:1;
};
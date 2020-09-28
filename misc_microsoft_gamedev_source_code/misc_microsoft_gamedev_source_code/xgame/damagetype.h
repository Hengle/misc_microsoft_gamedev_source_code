//==============================================================================
// damagetype.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// BDamageType
//==============================================================================
class BDamageType
{
   public:
      BDamageType();
      bool load(BXMLNode root, bool& shieldedOut, bool& useForAttackRatingOut);
      const BSimString& getName(void) const { return mName; }
      void setAttackRatingIndex(int val) { mAttackRatingIndex=val; }
      int getAttackRatingIndex() const { return mAttackRatingIndex; }
      bool getBaseType() const { return mBaseType; }

      BDamageType(const BDamageType& source) { *this=source; }

      BDamageType& operator=(const BDamageType& source)
      {
         if(this==&source)
            return *this;
         mName=source.mName;
         mAttackRatingIndex=source.mAttackRatingIndex;
         mBaseType=source.mBaseType;
         return *this;
      }

   protected:
      BSimString mName;
      int mAttackRatingIndex;
      bool mBaseType;
};
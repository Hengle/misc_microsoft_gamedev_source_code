//==============================================================================
// damagetype.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "damagetype.h"
#include "database.h"
#include "xmlreader.h"

//==============================================================================
// BDamageType::BDamageType
//==============================================================================
BDamageType::BDamageType() :
   mName(),
   mAttackRatingIndex(-1),
   mBaseType(false)
{
}

//==============================================================================
// BDamageType::load
//==============================================================================
bool BDamageType::load(BXMLNode root, bool& shieldedOut, bool& useForAttackRatingOut)
{
   shieldedOut = false;
   useForAttackRatingOut = false;

   root.getText(mName);

   root.getAttribValueAsBool("BaseType", mBaseType);

   bool tempBool;
   if(root.getAttribValueAsBool("Shielded", tempBool))
      shieldedOut = tempBool;

   if(root.getAttribValueAsBool("AttackRating", tempBool))
      useForAttackRatingOut = tempBool;

   return(true);
}
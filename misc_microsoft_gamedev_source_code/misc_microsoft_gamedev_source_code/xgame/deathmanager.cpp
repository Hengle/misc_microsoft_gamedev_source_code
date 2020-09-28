//============================================================================
// deathmanager.cpp
//  
// Copyright (c) 2008 Ensemble Studios
//============================================================================

#include "common.h"
#include "deathmanager.h"
#include "unit.h"
#include "gamedirectories.h"
#include "skullmanager.h"

BDeathManager gDeathManager;

//============================================================================
BDeathSpecial::BDeathSpecial()
{
   mType = -1;
   mCurrentChance = 0.0f;
   mMaxChance = 0.0f;
   mMinChance = 0.0f;
   mChanceGrowth = 0.0f;
}

//============================================================================
void BDeathSpecial::resetChance()
{
   mCurrentChance = mMinChance;
}

//============================================================================
void BDeathSpecial::increaseChance()
{
   mCurrentChance = mCurrentChance + mChanceGrowth;
   if (mCurrentChance > mMaxChance)
      mCurrentChance = mMaxChance;
}

//============================================================================
BDeathManager::BDeathManager()
{
   mDeathSpecials.clear();
}

//============================================================================
void BDeathManager::init()
{
   mDeathSpecials.clear();

   BXMLReader reader;
   if (!reader.load(cDirData, "deathmanager.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return;

   BXMLNode rootNode(reader.getRootNode());
   if (rootNode.getName() != "DeathManager")
   {
      BASSERTM(false, "deathManager.xml had a root node problem and could not be loaded.");
      return;
   }

   long specialCount = rootNode.getNumberChildren();
   mDeathSpecials.reserve(specialCount);
   for (long i=0; i<specialCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      if (node.getName() != "DeathSpecial")
         continue;

      BDeathSpecial special;
      long itemCount = node.getNumberChildren();
      for (long j=0; j < itemCount; j++)
      {
         BXMLNode n2(node.getChild(j));
         if (n2.getName() == "Type")
         {
            BString typeName;
            n2.getText(typeName);
            if (typeName == "RocketDeath")
               special.mType = BDeathSpecial::cDeathSpecialRocket;
         }
         else if (n2.getName() == "MaxChance")
         {
            n2.getTextAsFloat(special.mMaxChance);
         }
         else if (n2.getName() == "MinChance")
         {
            n2.getTextAsFloat(special.mMinChance);
         }
         else if (n2.getName() == "ChanceGrowth")
         {
            n2.getTextAsFloat(special.mChanceGrowth);
         }
      }
      special.mCurrentChance = special.mMaxChance;
      mDeathSpecials.add(special);
   }
}

//============================================================================
//============================================================================
void BDeathManager::reset()
{
   for (int i = 0; i < mDeathSpecials.getNumber(); i++)
   {
      mDeathSpecials[i].mCurrentChance = mDeathSpecials[i].mMaxChance;
   }
}

//============================================================================
bool BDeathManager::checkDeathChanceSpecial(long specialType)
{
   BDeathSpecial *pSpecial = getSpecialChanceInfo(specialType);
   if (pSpecial == NULL)
      return(false);

   if ((specialType == BDeathSpecial::cDeathSpecialRocket) && (gCollectiblesManager.getRocketAllGrunts() == true))
      return(true);

   // [7/1/2008 xemu] roll against our current chance
   float dieRoll = getRandRangeFloat(cSimRand, 0.0f, 1.0f);
   if (dieRoll < pSpecial->mCurrentChance)
   {
      // [7/1/2008 xemu] if we succeed, reset to min
      pSpecial->resetChance();
      return(true);
   }

   // [7/1/2008 xemu] if we fail, charge the chance capacitor
   pSpecial->increaseChance();
   return(false);
      
}

//============================================================================
BDeathSpecial *BDeathManager::getSpecialChanceInfo(long specialType)
{
   if ((specialType >= 0) && (specialType < mDeathSpecials.getNumber()))
      return &mDeathSpecials[specialType];
   return(NULL);
}

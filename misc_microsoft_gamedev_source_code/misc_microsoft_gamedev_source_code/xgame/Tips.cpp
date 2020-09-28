//=============================================================================
// tips.cpp
//
// Copyright (c) 2007 Ensemble Studios
//=============================================================================

// Includes
#include "common.h"
#include "tips.h"
#include "xmlreader.h"
#include "database.h"
#include "config.h"
#include "econfigenum.h"
#include "configsgame.h"
#include "gamedirectories.h"

//=============================================================================
// BTips::BTips
//=============================================================================
BTips::BTips()
{
}

//=============================================================================
// BTips::~BTips
//=============================================================================
BTips::~BTips()
{
}


//=============================================================================
// BTips::reset()
//=============================================================================
void BTips::reset()
{
   mTipStringIndexes.clear();
}

//=============================================================================
// BTips::getRandomTip
//=============================================================================
const BUString* BTips::getRandomTip() const
{
   if (mTipStringIndexes.getNumber() <=0)
      return NULL;

   setRandSeed(cUIRand, GetTickCount());
   long index = getRandRange(cUIRand, 0, mTipStringIndexes.getNumber()-1);

   return &(gDatabase.getLocStringFromIndex(mTipStringIndexes[index]));
}

//=============================================================================
//=============================================================================
void BTips::getRandomLoadingScreenTips(BSmallDynamicArray<const BUString *>& randomTips)
{
   if (mLoadingScreenTips.getNumber() <=0)
      return;

   // get the pointers to the strings
   for (int i=0; i<mLoadingScreenTips.getNumber(); i++)
   {
      const BUString * pString = &(gDatabase.getLocStringFromIndex(mLoadingScreenTips[i]));
      randomTips.add(pString);
   }

   setRandSeed(cUIRand, GetTickCount());

   // shuffle the strings
   long numEntries = randomTips.getNumber();
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cUIRand, 0, maxIndex);
      randomTips.swapIndices(i, randomIndex);
   }
}

//=============================================================================
// BTips::getTip
//=============================================================================
const BUString* BTips::getTip(int i) const
{
   if ( (i<0) || (i>=mTipStringIndexes.getNumber()) )
      return NULL;

   return &(gDatabase.getLocStringFromIndex(mTipStringIndexes[i]));
}

//=============================================================================
// BTips::load
//=============================================================================
bool BTips::load()
{
   BXMLReader reader;
   if(!reader.load(cDirData, "tips.xml"))
      return false;

   const BXMLNode rootNode(reader.getRootNode());

   long num=reader.getRootNode().getNumberChildren();
   for (long i=0; i<num; i++)
   {
      const BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if(name=="Tip")
      {
         long stringID = -1;

         // Get the name
         if (!node.getAttribValueAsLong("_locID", stringID))
            continue;

         long stringIndex = gDatabase.getLocStringIndex(stringID);
         if (stringIndex<0)
            continue;

         mTipStringIndexes.add(stringIndex);

         bool isLoadingScreenTip = false;
         if (node.getAttribValueAsBool("LoadingScreen", isLoadingScreenTip))
            mLoadingScreenTips.add(stringIndex);
      }
   }
   return true;
}

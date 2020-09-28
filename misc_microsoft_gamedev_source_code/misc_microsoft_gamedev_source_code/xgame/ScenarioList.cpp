//=============================================================================
// scenariolist.cpp
//
// Copyright (c) 2007 Ensemble Studios
//=============================================================================

// Includes
#include "common.h"
#include "scenariolist.h"
#include "xmlreader.h"
#include "database.h"
#include "config.h"
#include "econfigenum.h"
#include "configsgame.h"
#include "gamedirectories.h"

//=============================================================================
// BScenarioMap::BScenarioMap
//=============================================================================
BScenarioMap::BScenarioMap() :
   mID(-1)
{
}

//=============================================================================
// BScenarioMap::~BScenarioMap
//=============================================================================
BScenarioMap::~BScenarioMap()
{
}

//=============================================================================
// BScenarioMap::load
//=============================================================================
bool BScenarioMap::load(BXMLNode node)
{
   // Sanity.
   if(!node)
      return(false);

   mMapKeyFrame.set("placeholder");

   // Get the name of this achievement
   long stringID = -1;

   // Get the name
   if (!node.getAttribValueAsLong("NameStringID", stringID))
      stringID=-1;
   mNameStringID = stringID;
   mNameStringIndex = gDatabase.getLocStringIndex(stringID);

   // Get the description
   if (!node.getAttribValueAsLong("InfoStringID", stringID))
      stringID=-1;
   mDescriptionStringIndex = gDatabase.getLocStringIndex(stringID);

   // Max Players
   if (!node.getAttribValueAsLong("MaxPlayers", mMaxPlayers))
      mMaxPlayers=0;

   // Type
   BSimString typeString;
   node.getAttribValueAsString("Type", typeString);

   if (typeString.compare("Final") == 0)
      mType = BScenarioMap::cScenarioTypeFinal;
   else if (typeString.compare("Playtest") == 0)
      mType = BScenarioMap::cScenarioTypePlaytest;
   else if (typeString.compare("Development") == 0)
      mType = BScenarioMap::cScenarioTypeDevelopment;
   else if (typeString.compare("Campaign") == 0)
      mType = BScenarioMap::cScenarioTypeCampaign;
   else
      mType = BScenarioMap::cScenarioTypeUnknown;

   // filename
   node.getAttribValueAsString("File", mFilename);

   // loading screen
   node.getAttribValueAsString("LoadingScreen", mLoadingScreen);

   // name sans extension
   mName = mFilename;
   mName.removeExtension();

   // keyframe
   node.getAttribValueAsString("MapName", mMapKeyFrame);

   // Success.
   return true;
}

//=============================================================================
// BScenarioMap::getMapName
//=============================================================================
const BUString& BScenarioMap::getMapName() const
{
   return gDatabase.getLocStringFromIndex(mNameStringIndex);
}

//=============================================================================
// BScenarioMap::getMapDescription
//=============================================================================
const BUString& BScenarioMap::getMapDescription() const
{
   return gDatabase.getLocStringFromIndex(mDescriptionStringIndex);
}

//=============================================================================
// BScenarioList::BScenarioList
//=============================================================================
BScenarioList::BScenarioList()
{
   mScenarioMaps.clear();
   m1v1SkirmishMaps.clear();
   m2v2SkirmishMaps.clear();
   m3v3SkirmishMaps.clear();

}

//=============================================================================
// BScenarioList::~BScenarioList
//=============================================================================
BScenarioList::~BScenarioList()
{
   reset();
}

//=============================================================================
// BScenarioList::reset()
//=============================================================================
void BScenarioList::reset()
{
   for (int i=0; i<mScenarioMaps.getNumber(); i++)
   {
      BScenarioMap* pMap = mScenarioMaps[i];
      if (pMap)
         delete pMap;
      mScenarioMaps[i] = NULL;
   }
   mScenarioMaps.clear();
   m1v1SkirmishMaps.clear();
   m2v2SkirmishMaps.clear();
   m3v3SkirmishMaps.clear();
}

//=============================================================================
// BScenarioList::getMapInfo
//=============================================================================
const BScenarioMap* BScenarioList::getMapInfo(int i) const
{
   if ( (i<0) || (i>=mScenarioMaps.getNumber()) )
      return NULL;

   return mScenarioMaps[i];
}

//=============================================================================
//=============================================================================
const BScenarioMap* BScenarioList::getMapInfo(const char* pName) const
{
   uint count = mScenarioMaps.getSize();
   for (uint i=0; i < count; ++i)
   {
//-- FIXING PREFIX BUG ID 2130
      const BScenarioMap* pMap = mScenarioMaps[i];
//--
      if (!pMap)
         continue;

      if (pMap->getName().compare(pName) == 0)
         return pMap;
   }

   return NULL;
}

//=============================================================================
// BScenarioList::load
//=============================================================================
bool BScenarioList::load()
{
   BXMLReader reader;
   if(!reader.load(cDirData, "scenariodescriptions.xml"))
      return false;

   const BXMLNode rootNode(reader.getRootNode());

   long num=reader.getRootNode().getNumberChildren();
   for (long i=0; i<num; i++)
   {
      const BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if(name=="ScenarioInfo")
      {
         BScenarioMap* pMap = new BScenarioMap();
         if (!pMap->load(node))
         {
            delete pMap;
         }
         else
         {
            mScenarioMaps.add(pMap);
            int index = mScenarioMaps.getNumber()-1;
            pMap->setID(index);

            if ( pMap->getType() == BScenarioMap::cScenarioTypeFinal)
            {
               switch (pMap->getMaxPlayers())
               {
                  case 2:
                     m1v1SkirmishMaps.add(index);
                     break;
                  case 4:
                     m2v2SkirmishMaps.add(index);
                     break;
                  case 6:
                     m3v3SkirmishMaps.add(index);
                     break;
               }
            }
         }
      }
   }

   return true;
}


//=============================================================================
//=============================================================================
const BScenarioMap* BScenarioList::getPlayerNumberMapInfo(int playerCountType, int i) const
{
   int index = -1;
   switch (playerCountType)
   {
      case cPlayerMapType1v1:
      {
         if ( (i<0) || (i>=m1v1SkirmishMaps.getNumber()))
            return NULL;

         index = m1v1SkirmishMaps.get(i);
      }
      break;

      case cPlayerMapType2v2:
      {
         if ( (i<0) || (i>=m2v2SkirmishMaps.getNumber()))
            return NULL;

         index = m2v2SkirmishMaps.get(i);
      }
      break;

      case cPlayerMapType3v3:
      {
         if ( (i<0) || (i>=m3v3SkirmishMaps.getNumber()))
            return NULL;

         index = m3v3SkirmishMaps.get(i);
      }
      break;
   }

   // handles a -1 index
   return getMapInfo(index);
}

//=============================================================================
//=============================================================================
int BScenarioList::getPlayerNumberMapCount(int playerCountType) const
{ 
   int count = 0;
   switch (playerCountType)
   {
      case cPlayerMapType1v1:
         count = m1v1SkirmishMaps.getNumber(); 
         break;
      case cPlayerMapType2v2:
         count = m2v2SkirmishMaps.getNumber(); 
         break;
      case cPlayerMapType3v3:
         count = m3v3SkirmishMaps.getNumber(); 
         break;
   }

   return count;
}


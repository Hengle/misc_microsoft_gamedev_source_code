//==============================================================================
// civ.cpp
//
// Copyright (c) 1999-2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "civ.h"
#include "database.h"
#include "gamedirectories.h"
#include "ui.h"
#include "xmlreader.h"

//==============================================================================
// BCiv::BCiv
//==============================================================================
BCiv::BCiv(long id) :
   mID(id),
   mDisplayNameStringIndex(-1),
   mCivTech(-1),
   mCommandAckObjectID(-1),
   mRallyPointObjectID(-1),
   mLocalRallyPointObjectID(-1),
   mHullExpansionRadius(0.0f),
   mTerrainPushOffRadius(0.0f),
   mBuildingMagnetRange(0.0f),
   mTransportProtoID(-1),
   mTransportTriggerProtoID(-1),
   mLeaderMenuNameIndex(-1),
   mAlpha(-1),
   mPowerFromHero(false)
{
}

//==============================================================================
// BCiv::~BCiv
//==============================================================================
BCiv::~BCiv()
{
}

//==============================================================================
// BCiv::preload
//==============================================================================
bool BCiv::preload(BXMLNode root)
{
   for(long j=0; j < root.getNumberChildren(); j++)
   {
      BXMLNode node(root.getChild(j));
      const BPackedString name(node.getName());
      BSimString tempStr;
      if(name=="Name")
      {
         node.getText(mName);
         return true;
      }
   }
   return false;
}

//==============================================================================
// BCiv::load
//==============================================================================
bool BCiv::load(BXMLNode root)
{
   root.getAttribValueAsInt("Alpha", mAlpha);

   for(long j=0; j < root.getNumberChildren(); j++)
   {
      BXMLNode node(root.getChild(j));
      const BPackedString name(node.getName());
      BSimString tempStr;
      if(name=="Name")
         node.getText(mName);
      else if(name=="CivTech")
         mCivTech=gDatabase.getProtoTech(node.getTextPtr(tempStr));
      else if (name=="DisplayNameID")
      {
         long id;
         if (node.getTextAsLong(id))
            mDisplayNameStringIndex = gDatabase.getLocStringIndex(id);
      }
      else if (name=="CommandAckObject")
      {
         mCommandAckObjectID = gDatabase.getProtoObject(node.getTextPtr(tempStr));
      }
      else if (name=="RallyPointObject")
      {
         mRallyPointObjectID = gDatabase.getProtoObject(node.getTextPtr(tempStr));
      }
      else if (name=="LocalRallyPointObject")
      {
         mLocalRallyPointObjectID = gDatabase.getProtoObject(node.getTextPtr(tempStr));
      }
      else if(name=="Transport")
      {
         mTransportProtoID = gDatabase.getProtoObject(node.getTextPtr(tempStr));
      }
      else if(name=="TransportTrigger")
      {
         mTransportTriggerProtoID = gDatabase.getProtoObject(node.getTextPtr(tempStr));
      }
      else if(name=="ExpandHull")
      {
         node.getTextAsFloat(mHullExpansionRadius);
      }
      else if(name=="TerrainPushOff")
      {
         node.getTextAsFloat(mTerrainPushOffRadius);
      }
      else if(name=="BuildingMagnetRange")
      {
         node.getTextAsFloat(mBuildingMagnetRange);
      }
      else if(name=="SoundBank")
      {
         node.getText(mSoundBank);
      }
      else if (name=="LeaderMenuNameID")
      {
         long id;
         if(node.getTextAsLong(id))
            mLeaderMenuNameIndex=gDatabase.getLocStringIndex(id);
      }
      else if (name=="PowerFromHero")
      {
         node.getTextAsBool(mPowerFromHero);
      }
      else if (name=="UIControlBackground")
      {
         node.getText(mUIBackground);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
const BUString& BCiv::getDisplayName() const
{
   return gDatabase.getLocStringFromIndex(mDisplayNameStringIndex);
}

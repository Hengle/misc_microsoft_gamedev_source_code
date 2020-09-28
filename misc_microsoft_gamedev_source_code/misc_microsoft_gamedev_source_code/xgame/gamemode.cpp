//==============================================================================
// gamemode.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "common.h"
#include "gamemode.h"
#include "database.h"
#include "xmlreader.h"

//==============================================================================
//==============================================================================
BGameMode::BGameMode() :
   mName(),
   mWorldScript(),
   mPlayerScript(),
   mNPC(),
   mDisplayNameIndex(-1),
   mDescriptionIndex(-1)
{
}

//==============================================================================
//==============================================================================
bool BGameMode::load(BXMLNode root)
{
   BSimString tempStr;
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {                 
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      if (name == "Name")
         node.getText(mName);
      else if (name == "WorldScript")
         node.getText(mWorldScript);
      else if (name == "PlayerScript")
         node.getText(mPlayerScript);
      else if (name == "NPC")
         node.getText(mNPC);
      else if (name == "Tech")
         mTechID = gDatabase.getProtoTech(node.getTextPtr(tempStr));
      else if (name=="DisplayNameID")
      {
         long id;
         if(node.getTextAsLong(id))
            mDisplayNameIndex=gDatabase.getLocStringIndex(id);
      }
      else if (name=="DescriptionID")
      {
         long id;
         if(node.getTextAsLong(id))
            mDescriptionIndex=gDatabase.getLocStringIndex(id);
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
const BUString& BGameMode::getDisplayName() const
{
   return gDatabase.getLocStringFromIndex(mDisplayNameIndex);
}

//==============================================================================
//==============================================================================
const BUString& BGameMode::getDescription() const
{
   return gDatabase.getLocStringFromIndex(mDescriptionIndex);
}

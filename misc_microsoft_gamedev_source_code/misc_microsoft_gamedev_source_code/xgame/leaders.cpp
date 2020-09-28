//=============================================================================
// leaders.cpp
//
// Copyright (c) 2006 Ensemble Studios
//=============================================================================

// Includes
#include "common.h"
#include "leaders.h"
#include "database.h"
#include "xmlreader.h"
#include "config.h"
#include "configsgame.h"

//=============================================================================
// BLeader::BLeader
//=============================================================================
BLeader::BLeader() :
   mIconHandle(cInvalidManagedTextureHandle),
   mAlpha(-1),
   mTest(false),
   mLeaderCivID(-1),
   mLeaderTechID(-1),
   mLeaderPowerID(-1),
   mNameIndex(-1),
   mDescriptionIndex(-1),
   mStartingResources(),
   mStartingSquads(),
   mStartingUnits(),
   mRallyPointOffset(cOriginVector),
   mRepairRate(0.0f),
   mRepairDelay(0),
   mRepairCost(),
   mRepairTime(0.0f),
   mReverseHotDropCost(),
   mPops(),
   mSupportPowers()
{
   mFlashImgKeyFrame.set("placeholder");
   mFlashPortrait.set("img://unknown.ddx"); // we want to see the bad image if it occurs.
}

//=============================================================================
// BLeader::BLeader
//=============================================================================
BLeader::~BLeader()
{
}

//=============================================================================
// BLeader::preload
//=============================================================================
bool BLeader::preload(BXMLNode rootNode)
{
   return rootNode.getAttribValueAsString("Name", mName);
}

//=============================================================================
// BLeader::load
//=============================================================================
void BLeader::load(BXMLNode rootNode)
{
   rootNode.getAttribValueAsString("Name", mName);
   rootNode.getAttribValueAsString("Icon", mIconName);
   rootNode.getAttribValueAsBool("Test", mTest);
   rootNode.getAttribValueAsInt("Alpha", mAlpha);

   for (long j=0; j<rootNode.getNumberChildren(); j++)
   {
      BXMLNode childNode(rootNode.getChild(j));

      const BPackedString name(childNode.getName());

      BSimString tempStr;
      if (name=="Tech")
         mLeaderTechID = gDatabase.getProtoTech(childNode.getTextPtr(tempStr));
      else if (name=="Civ")
         mLeaderCivID = gDatabase.getCivID(childNode.getTextPtr(tempStr));
      else if (name=="Power")
         mLeaderPowerID = gDatabase.getProtoPowerIDByName(childNode.getTextPtr(tempStr));
      else if (name=="NameID")
      {
         long id;
         if(childNode.getTextAsLong(id))
            mNameIndex=gDatabase.getLocStringIndex(id);
      }
      else if (name=="DescriptionID")
      {
         long id;
         if(childNode.getTextAsLong(id))
            mDescriptionIndex=gDatabase.getLocStringIndex(id);
      }
      else if (name=="FlashImg")
      {
         childNode.getTextAsString(mFlashImgKeyFrame);
      }
      else if (name=="FlashPortrait")
      {
         childNode.getTextAsString(mFlashPortrait);
      }
      else if (name=="SupportPower")
      {
         BLeaderSupportPower supportPower;
         childNode.getAttribValueAsLong("IconLocation", supportPower.mIconLocation);

         if (childNode.getAttribValueAsString("TechPrereq", tempStr))
            supportPower.mTechPrereq=gDatabase.getProtoTech(tempStr);

         if (childNode.getNumberChildren())
         {
            for (long k=0; k<childNode.getNumberChildren(); k++)
            {
               BXMLNode powerNode(childNode.getChild(k));
               const BPackedString powerNodeName(powerNode.getName());
               if (powerNodeName == "Power")
               {
                  powerNode.getText(tempStr);
                  if (!tempStr.isEmpty())
                  {
                     long powerID = gDatabase.getProtoPowerIDByName(tempStr);
                     if (powerID != -1)
                        supportPower.mPowers.add(powerID);
                  }
               }
            }
         }

         if (supportPower.mPowers.getNumber() > 0)
            mSupportPowers.add(supportPower);
      }
      else if(name=="StartingSquad")
      {
         BProtoSquadID protoSquadID = gDatabase.getProtoSquad(childNode.getTextPtr(tempStr));
         if (gDatabase.isValidProtoSquad(protoSquadID))
         {
            BVector offset(cOriginVector);
            bool flyIn = false;
            childNode.getAttribValueAsVector("Offset", offset);
            childNode.getAttribValueAsBool("FlyIn", flyIn);

            BStartingSquad startingSquadToAdd;
            startingSquadToAdd.mProtoSquadID = protoSquadID;
            startingSquadToAdd.mOffset = offset;
            startingSquadToAdd.mbFlyInSquad = flyIn;
            mStartingSquads.add(startingSquadToAdd);
         }
      }
      else if(name=="StartingUnit")
      {
         BProtoObjectID protoObjectID = gDatabase.getProtoObject(childNode.getTextPtr(tempStr));
         if (gDatabase.isValidProtoObject(protoObjectID))
         {
            BVector offset(cOriginVector);
            childNode.getAttribValueAsVector("Offset", offset);

            BStartingUnit startingUnitToAdd;
            startingUnitToAdd.mProtoObjectID = protoObjectID;
            startingUnitToAdd.mOffset = offset;

            if (childNode.getAttribValue("BuildOther", &tempStr))
               startingUnitToAdd.mBuildOtherID = gDatabase.getProtoObject(tempStr);
            else
               startingUnitToAdd.mBuildOtherID = cInvalidProtoObjectID;

            bool doppleOnStart = false;
            childNode.getAttribValueAsBool("DoppleOnStart", doppleOnStart);
            startingUnitToAdd.mDoppleOnStart = doppleOnStart;

            mStartingUnits.add(startingUnitToAdd);
         }
      }
      else if(name=="RallyPointOffset")
         childNode.getTextAsVector(mRallyPointOffset);
      else if(name=="RepairRate")
         childNode.getTextAsFloat(mRepairRate);
      else if(name=="RepairDelay")
      {
         float val;
         if(childNode.getTextAsFloat(val))
            mRepairDelay=(DWORD)(val*1000.0f);
      }
      else if(name=="RepairCost")
      {
         BSimString typeName;

         if(childNode.getAttribValue("Type", &typeName))
         {
            long resource=gDatabase.getResource(typeName);
            if(resource!=-1)
            {
               float amount=0.0f;
               childNode.getTextAsFloat(amount);
               mRepairCost.set(resource, amount);
            }
         }
      }
      else if(name=="RepairTime")
         childNode.getTextAsFloat(mRepairTime);
      else if(name=="ReverseHotDropCost")
      {
         BSimString typeName;

         if(childNode.getAttribValue("Type", &typeName))
         {
            long resource=gDatabase.getResource(typeName);
            if(resource!=-1)
            {
               float amount=0.0f;
               childNode.getTextAsFloat(amount);
               mReverseHotDropCost.set(resource, amount);
            }
         }
      }
      else if(name=="Pop")
      {
         BSimString typeName;
         if(childNode.getAttribValue("Type", &typeName))
         {
            BLeaderPop pop;
            pop.mID=gDatabase.getPop(typeName);
            if(pop.mID!=-1)
            {
               childNode.getAttribValueAsFloat("Max", pop.mMax);
               childNode.getTextAsFloat(pop.mCap);

               mPops.add(pop);
            }
         }
      }
      else if(name=="Resource")
      {
         BSimString typeName;

         if(childNode.getAttribValue("Type", &typeName))
         {
            long resource=gDatabase.getResource(typeName);
            if(resource!=-1)
            {
               float amount=0.0f;
               childNode.getTextAsFloat(amount);
               mStartingResources.set(resource, amount);
               mActiveResources.set(resource);
            }
         }
      }
      else if (name=="UIControlBackground")
      {
         childNode.getTextAsString(mUIBackground);
      }

   }
}

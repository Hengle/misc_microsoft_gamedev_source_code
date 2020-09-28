//==============================================================================
// prototech.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "prototech.h"
#include "database.h"
#include "techeffect.h"
#include "visualmanager.h"
#include "xmlreader.h"
#include "soundmanager.h"

GFIMPLEMENTVERSION(BProtoTech, 1);

//==============================================================================
// BProtoTechStatic::BProtoTechStatic
//==============================================================================
BProtoTechStatic::BProtoTechStatic(long id) :
   mID(id),
   mDBID(-1),
   mDisplayNameIndex(-1),
   mRolloverTextIndex(-1),
   mPrereqTextIndex(-1),
   mName(),
   mIcon(),
   mAnimType(-1),
   mTechPrereqs(),
   mUnitPrereqs(),
   mTechEffects(),
   mProtoVisualIndex(-1),
   mCircleMenuIconID(-1),
   mCompletedSoundCue(cInvalidCueIndex),
   mAlpha(-1),
   mStatsProtoID(-1),
   mUINotificationID(-1),
   mHiddenFromStats(false)
{
   for(long i=0; i<mTechEffects.getNumber(); i++)
      delete mTechEffects[i];
   mTechEffects.clear();
}

//==============================================================================
// BProtoTechStatic::~BProtoTechStatic
//==============================================================================
BProtoTechStatic::~BProtoTechStatic()
{
}

//==============================================================================
// BProtoTech::BProtoTech
//==============================================================================
BProtoTech::BProtoTech() :
   mCost(),
   mResearchPoints(0.0f),
   mpStaticData(NULL)
{
   mFlagOwnStaticData=false;
   mFlagUnobtainable=false;
   mFlagUnique=false;
   mFlagShadow=false;
   mFlagOrPrereqs=false;
   mFlagPerpetual=false;
   mFlagForbid=false;
   mFlagNoSound=false;
   mFlagInstant=false;
}

//==============================================================================
// BProtoTech::BProtoTech
//==============================================================================
BProtoTech::BProtoTech(const BProtoTech* pBase)
{
   // Dynamic data   
   mCost=pBase->mCost;
   mResearchPoints=pBase->mResearchPoints;

   //--Flags
   mFlagOwnStaticData=pBase->mFlagOwnStaticData;
   mFlagUnobtainable=pBase->mFlagUnobtainable;
   mFlagUnique=pBase->mFlagUnique;
   mFlagShadow=pBase->mFlagShadow;
   mFlagOrPrereqs=pBase->mFlagOrPrereqs;
   mFlagPerpetual=pBase->mFlagPerpetual;
   mFlagForbid=pBase->mFlagForbid;
   mFlagNoSound=pBase->mFlagNoSound;
   mFlagInstant=pBase->mFlagInstant;

   // Static data
   mpStaticData=pBase->mpStaticData;
   setFlagOwnStaticData(false);
}

//==============================================================================
// BProtoTech::~BProtoTech
//==============================================================================
BProtoTech::~BProtoTech()
{
   if(getFlagOwnStaticData() && mpStaticData)
   {
      delete mpStaticData;
      mpStaticData=NULL;
      setFlagOwnStaticData(false);
   }
}

//==============================================================================
// BProtoTech::init
//==============================================================================
bool BProtoTech::init(long id)
{
   mpStaticData=new BProtoTechStatic(id);
   if(!mpStaticData)
      return false;
   setFlagOwnStaticData(true);
   return true;
}

//==============================================================================
// BProtoTech::load
//==============================================================================
bool BProtoTech::load(BXMLNode root)
{
   BSimString string;
   if(root.getAttribValue("name", &string))
      mpStaticData->mName.set(string);

   root.getAttribValueAsInt("Alpha", mpStaticData->mAlpha);

   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      BSimString tempStr;
      if(name=="Flag")
      {
         BSimString value;
         node.getText(value);
         if(value=="Unobtainable")
            setFlagUnobtainable(true);
         else if(value=="UniqueProtoUnitInstance")
            setFlagUnique(true);
         else if(value=="Shadow")
            setFlagShadow(true);
         else if(value=="OrPrereqs")
            setFlagOrPrereqs(true);
         else if(value=="Perpetual")
            setFlagPerpetual(true);
         else if(value=="NoSound")
            setFlagNoSound(true);
         else if(value=="Instant")
            setFlagInstant(true);
         else if (value == "HiddenFromStats")
            mpStaticData->mHiddenFromStats = true;
      }
      else if(name=="DBID")
         node.getTextAsLong(mpStaticData->mDBID);
      else if(name=="DisplayNameID")
      {
         long id;
         if(node.getTextAsLong(id))
            mpStaticData->mDisplayNameIndex=gDatabase.getLocStringIndex(id);
      }
      else if(name=="RolloverTextID")
      {
         long id;
         if(node.getTextAsLong(id))
           mpStaticData->mRolloverTextIndex=gDatabase.getLocStringIndex(id);
      }
      else if(name=="PrereqTextID")
      {
         long id;
         if(node.getTextAsLong(id))
            mpStaticData->mPrereqTextIndex=gDatabase.getLocStringIndex(id);
      }
      else if(name=="Cost")
         mCost.load(node);
      else if(name=="ResearchPoints")
         node.getTextAsFloat(mResearchPoints);
      else if(name=="Status") //FIXME - Temp to support loading of old tech XML data... Eventually change to a flag in the XML instead.
      {
         if(node.compareText("Unobtainable") == 0)
            setFlagUnobtainable(true);
      }
      else if(name=="Icon")
         node.getText(mpStaticData->mIcon);
      else if(name=="ResearchCompleteSound")
      {
         BString soundCueStr;
         node.getText(soundCueStr);
         mpStaticData->mCompletedSoundCue = gSoundManager.getCueIndex(soundCueStr);
      }
      else if(name == "ResearchAnim")
         mpStaticData->mAnimType = gVisualManager.getAnimType(node.getTextPtr(tempStr));
      else if(name=="Prereqs")
      {
         long childCount=node.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode child(node.getChild(j));
            const BPackedString childName(child.getName());
            if(childName=="TechStatus") //FIXME - Simplify to just "Tech"
            {
               long techID=gDatabase.getProtoTech(child.getTextPtr(tempStr));
               if(techID!=-1)
               {
                  mpStaticData->mTechPrereqs.add(techID);
                  gDatabase.getProtoTech(techID)->addDependentTech(mpStaticData->mID);
               }
            }
            else if(childName=="TypeCount") //FIXME - Simplify to just "Unit"
            {
               BSimString unitName;
               if (child.getAttribValue("unit", &unitName))
               {
                  long unitID=gDatabase.getProtoObject(unitName);
                  if(unitID>0)
                  {
                     if(unitID>65535)
                     {
                        BASSERT(0);
                     }
                     else
                     {
                        long opType=0;
                        BSimString operatorName;
                        if (child.getAttribValue("operator", &operatorName))
                        {
                           if(operatorName=="gt")
                              opType=1;
                           else if(operatorName=="lt")
                              opType=2;
                        }
                        long prereqCount=0;
                        child.getAttribValueAsLong("count", prereqCount);
                        if(prereqCount<-2047)
                        {
                           BASSERT(0);
                           prereqCount=-2047;
                        }
                        else if(prereqCount>2047)
                        {
                           BASSERT(0);
                           prereqCount=2047;
                        }
                        prereqCount+=2047;
                        DWORD unitPrereq=(DWORD)(opType<<28 | prereqCount<<16 | unitID);
                        mpStaticData->mUnitPrereqs.add(unitPrereq);
                        gDatabase.addTechUnitDependecy(mpStaticData->mID, unitID);
                     }
                  }
               }
            }
         }
      }
      else if(name=="Effects")
      {
         long childCount=node.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BTechEffect* pTechEffect=new BTechEffect();
            if(pTechEffect)
            {
               if(!pTechEffect->load(node.getChild(j)))
               {
                  delete pTechEffect;
                  break;
               }
               mpStaticData->mTechEffects.add(pTechEffect);
            }
         }
      }
      else if (name == "StatsObject")
         mpStaticData->mStatsProtoID = gDatabase.getProtoObject(node.getTextPtr(tempStr));
   }

   return true;
}

//==============================================================================
// BProtoTech::applyEffects
//==============================================================================
void BProtoTech::applyEffects(BTechTree*  pTechTree, long unitID, bool noCost)
{
   long count=mpStaticData->mTechEffects.getNumber();
   for(long i=0; i<count; i++)
   {
      BTechEffect* pTechEffect=mpStaticData->mTechEffects[i];
      pTechEffect->apply(pTechTree, unitID, false, noCost);
   }
}

//==============================================================================
// BProtoTech::unapplyEffects
//==============================================================================
void BProtoTech::unapplyEffects(BTechTree*  pTechTree, long unitID)
{
   long count=mpStaticData->mTechEffects.getNumber();
   for(long i=0; i<count; i++)
   {
      BTechEffect* pTechEffect=mpStaticData->mTechEffects[i];
      pTechEffect->apply(pTechTree, unitID, true, false);
   }
}

//==============================================================================
// BProtoTech::getDisplayName
//==============================================================================
void BProtoTech::getDisplayName(BUString& string) const
{
   if(mpStaticData->mDisplayNameIndex==-1)
      string=mpStaticData->mName.getPtr(); // AJL FIXME - Need to return empty string or other value to indicate missing string
   else
      string=gDatabase.getLocStringFromIndex(mpStaticData->mDisplayNameIndex);
}

//==============================================================================
// BProtoTech::getRolloverText
//==============================================================================
void BProtoTech::getRolloverText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mRolloverTextIndex);
}

//==============================================================================
// BProtoTech::getPrereqText
//==============================================================================
void BProtoTech::getPrereqText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mPrereqTextIndex);
}

//==============================================================================
//==============================================================================
bool BProtoTech::save(BStream* pStream, int saveType) const
{
   GFWRITECLASS(pStream, saveType, mCost);
   GFWRITEVAR(pStream, float, mResearchPoints);
   GFWRITEBITBOOL(pStream, mFlagOwnStaticData);
   GFWRITEBITBOOL(pStream, mFlagUnobtainable);
   GFWRITEBITBOOL(pStream, mFlagUnique);
   GFWRITEBITBOOL(pStream, mFlagShadow);
   GFWRITEBITBOOL(pStream, mFlagOrPrereqs);
   GFWRITEBITBOOL(pStream, mFlagPerpetual);
   GFWRITEBITBOOL(pStream, mFlagForbid);
   GFWRITEBITBOOL(pStream, mFlagNoSound);
   GFWRITEBITBOOL(pStream, mFlagInstant);
   return true;
}

//==============================================================================
//==============================================================================
bool BProtoTech::load(BStream* pStream, int saveType)
{
   GFREADCLASS(pStream, saveType, mCost);
   GFREADVAR(pStream, float, mResearchPoints);
   GFREADBITBOOL(pStream, mFlagOwnStaticData);
   GFREADBITBOOL(pStream, mFlagUnobtainable);
   GFREADBITBOOL(pStream, mFlagUnique);
   GFREADBITBOOL(pStream, mFlagShadow);
   GFREADBITBOOL(pStream, mFlagOrPrereqs);
   GFREADBITBOOL(pStream, mFlagPerpetual);
   GFREADBITBOOL(pStream, mFlagForbid);
   GFREADBITBOOL(pStream, mFlagNoSound);
   GFREADBITBOOL(pStream, mFlagInstant);
   return true;
}

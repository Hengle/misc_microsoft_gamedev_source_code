//==============================================================================
// userachievement.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "userachievements.h"
#include "xbox.h"
#include "user.h"
#include "achievementmanager.h"

//Increment this when changing any saving/loading code.  all achievements will be reset.
#define ACHIEVEMENT_CODE_VERSION 01

// Globals

//==============================================================================
// BUserAchievement
//==============================================================================
BUserAchievement::BUserAchievement() : 
   mType(cAT_None),
   mGrantGameTime(0),
   mGranted(false)
{
}

//==============================================================================
//==============================================================================
BUserAchievement::~BUserAchievement()
{
}


//==============================================================================
//==============================================================================
bool BUserAchievement::writeAchievementToStream(BDynamicStream &stream)
{
   if(stream.writeObj((uint8)mGranted) == false)
      return(false);
   if(stream.writeObj(mGrantGameTime) == false)
      return(false);

   return true;
}


//==============================================================================
//==============================================================================
bool BUserAchievement::readAchievementFromStream(BInflateStream* pInfStream)
{
   uint8 granted = 0;
   *pInfStream >> granted;
   mGranted = (granted != 0) ? true : false;
   
   *pInfStream >> mGrantGameTime;

   return true;
}


//==============================================================================
// BUserAccumulatorAchievement
//==============================================================================
BUserAccumulatorAchievement::BUserAccumulatorAchievement() : 
   mQuantity(0)
{
   mType = cAT_Accumulator;
}

//==============================================================================
//==============================================================================
BUserAccumulatorAchievement::~BUserAccumulatorAchievement()
{
}

//==============================================================================
//==============================================================================
bool BUserAccumulatorAchievement::writeAchievementToStream(BDynamicStream &stream)
{
   if( BUserAchievement::writeAchievementToStream(stream) == false )
      return false;
   if(stream.writeObj(mQuantity) == false)
      return(false);

   return true;
}

//==============================================================================
//==============================================================================
bool BUserAccumulatorAchievement::readAchievementFromStream(BInflateStream* pInfStream)
{
   BUserAchievement::readAchievementFromStream(pInfStream);

   *pInfStream >> mQuantity;

   return true;
}





//==============================================================================
// BUserCampaignAchievement
//==============================================================================
BUserCampaignAchievement::BUserCampaignAchievement() :
   mQuantity(0)
{
   mType = cAT_Campaign;
}

//==============================================================================
//==============================================================================
BUserCampaignAchievement::~BUserCampaignAchievement()
{
}

//==============================================================================
//==============================================================================
bool BUserCampaignAchievement::writeAchievementToStream(BDynamicStream &stream)
{
   if( BUserAchievement::writeAchievementToStream(stream) == false )
      return false;
   if(stream.writeObj(mQuantity) == false)
      return(false);
   return true;
}

//==============================================================================
//==============================================================================
bool BUserCampaignAchievement::readAchievementFromStream(BInflateStream* pInfStream)
{
   BUserAchievement::readAchievementFromStream(pInfStream);

   *pInfStream >> mQuantity;

   return true;
}


//==============================================================================
// BUserSkirmishAchievement
//==============================================================================
BUserSkirmishAchievement::BUserSkirmishAchievement()
{
   mType = cAT_Skirmish;
   mMapsCompleted = 0;
   mLeadersCompleted = 0;
   mModesCompleted = 0;
}

//==============================================================================
//==============================================================================
BUserSkirmishAchievement::~BUserSkirmishAchievement()
{
}

//==============================================================================
//==============================================================================
bool BUserSkirmishAchievement::writeAchievementToStream(BDynamicStream &stream)
{
   if( BUserAchievement::writeAchievementToStream(stream) == false )
      return false;

    if(stream.writeObj(mMapsCompleted) == false)
      return(false);
    if(stream.writeObj(mLeadersCompleted) == false)
      return(false);
    if(stream.writeObj(mModesCompleted) == false)
      return(false);

   return true;
}

//==============================================================================
//==============================================================================
bool BUserSkirmishAchievement::readAchievementFromStream(BInflateStream* pInfStream)
{
   BUserAchievement::readAchievementFromStream(pInfStream);

   *pInfStream >> mMapsCompleted;
   *pInfStream >> mLeadersCompleted;
   *pInfStream >> mModesCompleted;

   return true;
}

//==============================================================================
//==============================================================================
bool BUserSkirmishAchievement::getMapCompleted(int i) const
{
   BASSERT(i>=0 && i<16);
   return ((mMapsCompleted & (1 << i))!=0);
}

void BUserSkirmishAchievement::setMapCompleted(int i)
{
   BASSERT(i>=0 && i<16);
   mMapsCompleted |= (1 << i);
}

bool BUserSkirmishAchievement::getLeaderCompleted(int i) const
{
   BASSERT(i>=0 && i<8);
   return ((mLeadersCompleted & (1 << i))!=0);
}

void BUserSkirmishAchievement::setLeaderCompleted(int i)
{
   BASSERT(i>=0 && i<8);
   mLeadersCompleted |= (1 << i);
}

bool BUserSkirmishAchievement::getModeCompleted(int i) const
{
   BASSERT(i>=0 && i<8);
   return ((mModesCompleted & (1 << i))!=0);
}

void BUserSkirmishAchievement::setModeCompleted(int i)
{
   BASSERT(i>=0 && i<8);
   mModesCompleted |= (1 << i);
}




//==============================================================================
// BUserTriggerAchievement
//==============================================================================
BUserTriggerAchievement::BUserTriggerAchievement()
{
   mType = cAT_Trigger;
}

//==============================================================================
//==============================================================================
BUserTriggerAchievement::~BUserTriggerAchievement()
{
}

//==============================================================================
//==============================================================================
bool BUserTriggerAchievement::writeAchievementToStream(BDynamicStream &stream)
{
   if( BUserAchievement::writeAchievementToStream(stream) == false )
      return false;
   return true;
}

//==============================================================================
//==============================================================================
bool BUserTriggerAchievement::readAchievementFromStream(BInflateStream* pInfStream)
{
   BUserAchievement::readAchievementFromStream(pInfStream);
   return true;
}


//==============================================================================
// BUserMapAchievement
//==============================================================================
BUserMapAchievement::BUserMapAchievement()
{
   mType = cAT_Map;
   mMaps.setNumber(0);
   mQuantity.setNumber(0);
}

//==============================================================================
//==============================================================================
BUserMapAchievement::~BUserMapAchievement()
{
   for(int i=0; i<mMaps.getNumber(); i++)
      delete mMaps[i];
   mMaps.setNumber(0);

   mQuantity.setNumber(0);
}


//==============================================================================
//==============================================================================
bool BUserMapAchievement::writeAchievementToStream(BDynamicStream &stream)
{
   if( BUserAchievement::writeAchievementToStream(stream) == false )
      return false;

   // these better be the same
   BASSERT(mMaps.getNumber() == mQuantity.getNumber());

   // fail if they are not the same.
   if ( mMaps.getNumber() != mQuantity.getNumber() )
      return false;

   // maps
   uint count = mMaps.getNumber();
   if(stream.writeObj(count) == false)
      return(false);
   for(uint i=0; i<count; i++)
   {
      if(stream.writeObj(mMaps[i]) == false)
         return(false);
   }

   // map quantities
   count = mQuantity.getNumber();
   if(stream.writeObj(count) == false)
      return(false);
   for(uint i=0; i<count; i++)
   {
      if(stream.writeObj(mQuantity[i]) == false)
         return(false);
   }


   return true;
}

//==============================================================================
//==============================================================================
bool BUserMapAchievement::readAchievementFromStream(BInflateStream* pInfStream)
{
   BUserAchievement::readAchievementFromStream(pInfStream);

   mMaps.setNumber(0);
   mQuantity.setNumber(0);

   // maps
   uint count = 0;
   *pInfStream >> count;
   BString map;
   for (uint i=0; i<count; i++)
   {
      *pInfStream >> map;
      mMaps.add(map);
   }

   // map quantities
   *pInfStream >> count;
   int temp=0;
   for (uint i=0; i<count; i++)
   {
      *pInfStream >> temp;
      mQuantity.add(temp);
   }

   return true;
}

//==============================================================================
// BUserAchievementList
//==============================================================================
BUserAchievementList::BUserAchievementList()
{
}

//==============================================================================
//==============================================================================
void BUserAchievementList::clearAchievments()
{
   for (uint i=0; i< mAchievements.getSize(); i++)
      delete mAchievements[i];
   mAchievements.resize(0);
}

//==============================================================================
//==============================================================================
BUserAchievementList::~BUserAchievementList()
{
   clearAchievments();
}

//==============================================================================
//==============================================================================
void BUserAchievementList::initialize()
{
   mVersion = 0;
   
   clearAchievments();
   
   // load the rules.  this is the static part of the achievement data
   const BDynamicSimArray<BAchievementProto*>& rules = gAchievementManager.getRules();

   mVersion = (ACHIEVEMENT_CODE_VERSION << 16) | gAchievementManager.getVersion();

   uint count = rules.getNumber();
   for (uint i=0; i<count; i++)
   {
      BAchievementProto * pRule = rules[i];

      BUserAchievement* pUserAchievement = NULL;
      switch(pRule->getType())
      {
         case cAT_None:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Accumulator:
            pUserAchievement = new BUserAccumulatorAchievement();
            break;
         case cAT_Campaign:
            pUserAchievement = new BUserCampaignAchievement();
            break;
         case cAT_Skirmish:
            pUserAchievement = new BUserSkirmishAchievement();
            break;
         case cAT_Trigger:
            pUserAchievement = new BUserTriggerAchievement();
            break;
         case cAT_Map:
            pUserAchievement = new BUserMapAchievement();
            break;
         case cAT_Scenario:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Leaders:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Abilities:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Meta:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Misc:
            pUserAchievement = new BUserAchievement();
            break;
         default:
            // [10/1/2008 xemu] hey, this type didn't match, which is not good 
            BASSERT(0);
            break;
      }

      pUserAchievement->setType(pRule->getType());

      mAchievements.add(pUserAchievement);
   }
}

//==============================================================================
//==============================================================================
bool BUserAchievementList::readFromStream( BInflateStream* pInfStream )
{
   // read the version number of the achievements on disk
   uint32 version;
   *pInfStream >> version;

   uint32 dataVersion = version & 0xFFFF;
   uint32 codeVersion = version >> 16;

   //If the versions do not match, this data is outdated, so do not read it in.
   if( (dataVersion != gAchievementManager.getVersion()) || (codeVersion != ACHIEVEMENT_CODE_VERSION) )
      return false;

   mVersion = version;

   // read the number of achievements on disk
   uint count = 0;
   *pInfStream >> count;

   clearAchievments();

   // read all the achievements
   int type = 0;
   for (uint i=0; i<count; i++)
   {
      *pInfStream >> type;

      BUserAchievement* pUserAchievement = NULL;
      switch(type)
      {
         case cAT_None:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Accumulator:
            pUserAchievement = new BUserAccumulatorAchievement();
            break;
         case cAT_Campaign:
            pUserAchievement = new BUserCampaignAchievement();
            break;
         case cAT_Skirmish:
            pUserAchievement = new BUserSkirmishAchievement();
            break;
         case cAT_Trigger:
            pUserAchievement = new BUserTriggerAchievement();
            break;
         case cAT_Map:
            pUserAchievement = new BUserMapAchievement();
            break;
         case cAT_Scenario:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Leaders:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Abilities:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Meta:
            pUserAchievement = new BUserAchievement();
            break;
         case cAT_Misc:
            pUserAchievement = new BUserAchievement();
            break;
         default:
            BASSERT(0);
            break;
      }

      pUserAchievement->setType(type);

      pUserAchievement->readAchievementFromStream(pInfStream);

      mAchievements.add(pUserAchievement);
   }

   return true;
}


//==============================================================================
//==============================================================================
bool BUserAchievementList::writeToStream( BDynamicStream &stream )
{
   // Write the version number out first
   if(stream.writeObj(mVersion) == false)
   return(false);

   // Write the size of the list out
   uint count = mAchievements.getNumber();
   if(stream.writeObj(count) == false)
   return(false);

   // write out each achievement
   for (uint i=0; i<count; i++)
   {
      BUserAchievement *pAchievement = mAchievements[i];
      if(stream.writeObj(pAchievement->getType()) == false)    // save the type for reconstruction.
      return(false);

      if( pAchievement->writeAchievementToStream(stream) == false )
         return false;
   }

   return true;
}

//==============================================================================
//==============================================================================
BUserAchievement* BUserAchievementList::getAchievement(uint i)
{
   if (i >= (uint)mAchievements.getNumber())
      return NULL;

   return mAchievements[i];
}

const BUserAchievement* BUserAchievementList::getAchievement(uint i) const
{
   if (i >= (uint)mAchievements.getNumber())
      return NULL;

   return mAchievements[i];
}

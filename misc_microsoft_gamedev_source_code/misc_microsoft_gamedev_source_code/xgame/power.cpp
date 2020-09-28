//==============================================================================
// power.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "power.h"
#include "powertypes.h"
#include "simtypes.h"
#include "user.h"
#include "usermanager.h"
#include "unitquery.h"
#include "database.h"
#include "world.h"
#include "commandmanager.h"
#include "commands.h"
#include "commandtypes.h"

GFIMPLEMENTVERSION(BPowerUser, 9);
GFIMPLEMENTVERSION(BPower, 22);

//==============================================================================
// Constructor (protected so we cannot allocate the base class by itself.)
//==============================================================================
BPower::BPower()
{
   mID = cInvalidPowerID;
   mType = PowerType::cInvalid;
   mProtoPowerID = cInvalidProtoPowerID;
   initBase();
}

//==============================================================================
//==============================================================================
BPower::~BPower()
{

}

//==============================================================================
//==============================================================================
void BPower::initBase()
{
   mPowerUserID = cInvalidPowerUserID;
   mPowerLevel = 0;
   mMaintenanceSupplies = 0.0f;
   mElapsed = 0;
   mPlayerID = cInvalidPlayerID;
   mOwnerID = cInvalidObjectID;
   mTargetLocation = cOriginVector;
   mFlagDestroy = false;
   mFlagIgnoreAllReqs = false;
   mFlagCheckPowerLocation = true;
}

//==============================================================================
//==============================================================================
BPowerUserID BPower::getPowerUserID()
{
   return mPowerUserID;
}

//==============================================================================
//==============================================================================
void BPower::deletePowerUser()
{
   // Don't bother.
   if (!mPowerUserID.isValid())
      return;

   // Cache off the power user id, and extract the data from it, then invalidate it.
   BPowerUserID powerUserID = mPowerUserID;
   BPlayerID playerID = mPowerUserID.getPlayerID();
   mPowerUserID.invalidate();

   // See if the user that's controlling this power is on this machine.
   BUser* pUser = gUserManager.getUserByPlayerID(playerID);
   if (pUser)
   {
      // Does that user have a power user created?
//-- FIXING PREFIX BUG ID 1585
      const BPowerUser* pPowerUser = pUser->getPowerUser();
//--
      if (!pPowerUser)
         return;

      // Make sure it's the one we want to get rid of.
      if (powerUserID != pPowerUser->getID())
         return;

      // Get rid of that power user.
      pUser->deletePowerUser();
   }
}


//==============================================================================
//==============================================================================
const BSquad* BPower::getOwnerSquad() const
{
   return (gWorld->getSquad(mOwnerID));
}

//==============================================================================
//==============================================================================
BPowerUser::~BPowerUser()
{

}

//==============================================================================
//==============================================================================
void BPowerUser::cancelPower()
{
   gUI.playClickSound();

   if (mFlagIssueCommandOnCancel)
   {
      BPowerInputCommand* c = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand( mpUser->getPlayerID(), cCommandPowerInput );
      if( c )
      {
         // Set up the command.
         long playerID = mpUser->getPlayerID();
         c->setSenders( 1, &playerID );
         c->setSenderType( BCommand::cPlayer );
         c->setRecipientType( BCommand::cPlayer );
         c->setType( BPowerInputCommand::cTypeShutdown );
         c->setPowerUserID(mID);
         c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
         gWorld->getCommandManager()->addCommandToExecute( c );
      }
   }

   shutdown();
}

//==============================================================================
//==============================================================================
bool BPowerUser::save(BStream* pStream, int saveType) const
{
   uint32 val = mID.getID();
   GFWRITEVAR(pStream, uint32, val);
   GFWRITEVAR(pStream, uint, mType);
   GFWRITEBITBOOL(pStream, mInitialized);
   GFWRITEBITBOOL(pStream, mFlagDestroy);
   GFWRITEBITBOOL(pStream, mFlagNoCost);
   GFWRITEBITBOOL(pStream, mFlagCheckPowerLocation);
   GFWRITEVAL(pStream, int8, mProtoPowerID);
   GFWRITEVAL(pStream, int8, mPowerLevel);
   GFWRITEVAR(pStream, BEntityID, mOwnerSquadID);
   GFWRITEVAL(pStream, bool, (mpUser==gUserManager.getPrimaryUser()));
   GFWRITEVAR(pStream, double, mElapsed);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUser::load(BStream* pStream, int saveType)
{
   if (BPowerUser::mGameFileVersion >= 8)
   {
      uint32 val = 0;
      GFREADVAR(pStream, uint32, val);
      mID.setID(val);
      GFREADVAR(pStream, uint, mType);
   }
   GFREADBITBOOL(pStream, mInitialized);
   GFREADBITBOOL(pStream, mFlagDestroy);
   GFREADBITBOOL(pStream, mFlagNoCost);
   GFREADBITBOOL(pStream, mFlagCheckPowerLocation);
   GFREADVAL(pStream, int8, BProtoPowerID, mProtoPowerID);
   GFREADVAL(pStream, int8, BPowerLevel, mPowerLevel);
   GFREADVAR(pStream, BEntityID, mOwnerSquadID);
   bool primaryUser;
   GFREADVAR(pStream, bool, primaryUser);
   if (primaryUser)
      mpUser=gUserManager.getPrimaryUser();
   GFREADVAR(pStream, double, mElapsed);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUser::postLoad(int saveType)
{
   setupUser();
   return true;
}

//==============================================================================
//==============================================================================
bool BPower::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, int, mID);
   GFWRITEVAR(pStream, uint, mType);
   GFWRITEVAR(pStream, BPowerUserID, mPowerUserID);
   GFWRITEVAL(pStream, int8, mProtoPowerID);
   GFWRITEVAL(pStream, int8, mPowerLevel);
   GFWRITEVAR(pStream, float, mMaintenanceSupplies);
   GFWRITEVAR(pStream, double, mElapsed);
   GFWRITEVAL(pStream, int8, mPlayerID);
   GFWRITEVAR(pStream, BEntityID, mOwnerID);
   GFWRITEVECTOR(pStream, mTargetLocation);
   GFWRITEBITBOOL(pStream, mFlagDestroy);
   GFWRITEBITBOOL(pStream, mFlagIgnoreAllReqs);
   GFWRITEBITBOOL(pStream, mFlagCheckPowerLocation);
   return true;
}

//==============================================================================
//==============================================================================
bool BPower::load(BStream* pStream, int saveType)
{
   if (BPower::mGameFileVersion >= 21)
   {
      GFREADVAR(pStream, int, mID);
      GFREADVAR(pStream, uint, mType);
   }
   GFREADVAR(pStream, BPowerUserID, mPowerUserID);
   GFREADVAL(pStream, int8, BProtoPowerID, mProtoPowerID);
   GFREADVAL(pStream, int8, BPowerLevel, mPowerLevel);

   mMaintenanceSupplies = 0.0f;
   if (BPower::mGameFileVersion >= 7)
      GFREADVAR(pStream, float, mMaintenanceSupplies);

   GFREADVAR(pStream, double, mElapsed);
   GFREADVAL(pStream, int8, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BEntityID, mOwnerID);
   GFREADVECTOR(pStream, mTargetLocation);   
   GFREADBITBOOL(pStream, mFlagDestroy);
   GFREADBITBOOL(pStream, mFlagIgnoreAllReqs);
   GFREADBITBOOL(pStream, mFlagCheckPowerLocation);
   return true;
}

//==============================================================================
// unitactionDOT.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "protoobject.h"
#include "unit.h"
#include "unitactionDOT.h"
#include "world.h"
#include "terrainimpactdecal.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionDOT, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionDOT::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionDOT::disconnect()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);

   if (pUnit && mAttachID != cInvalidObjectID)
      pUnit->removeAttachment(mAttachID);

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionDOT::init()
{
   if (!BAction::init())
      return (false);

   mDOTrate=0.0f;
   mDOTduration=0.0f;
   mDOTEffect=-1;
   mAttachLocation = cOriginVector;
   mAttachID=cInvalidObjectID;
   mDamageMultiplier=0.0f;

   mDotStacks.clear();

   mFlagConflictsWithIdle=false;
   mFlagCanHaveMultipleOfType=true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionDOT::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   const BProtoObject* pProtoObject=pUnit->getProtoObject();
   BASSERT(pProtoObject);

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionDOT::update mState", mState);
   #endif

   switch (mState)
   {
      case cStateNone:
         if (mDOTEffect != -1 && mAttachID == cInvalidObjectID)
         {
            // Create object
            mAttachID = gWorld->createEntity(mDOTEffect, false, pUnit->getPlayerID(), pUnit->getPosition(), pUnit->getForward(), pUnit->getRight(), true);
            BObject* pEffectObject = gWorld->getObject(mAttachID);
            if (pEffectObject)
            {
               pUnit->addAttachment(pEffectObject);
               pEffectObject->setCenterOffset(mAttachLocation);
            }
         }

         setState(cStateWorking);
         break;
         
      case cStateWorking:
      {
         long numDots = mDotStacks.getNumber();
         for (int i = numDots - 1; i >= 0; i--)
         {
            // Do some damage
            BDamage damage; // Moved damage var here to make sure it gets fully reset before each call to pUnit->damage
            damage.mDamagePos = pUnit->getPosition();
            damage.mDirection.zero();
            damage.mAttackerID = mDotStacks[i].mSource;
            damage.mAttackerTeamID = mDotStacks[i].mSourceTeamID;
            damage.mDamage = mDOTrate * elapsed;
            damage.mDamageMultiplier = mDamageMultiplier;
            damage.mShieldDamageMultiplier = mDamageMultiplier;
            damage.mpDamageInfo = NULL;
            damage.mIsDOTDamage = true;

#ifdef SYNC_UnitAction
            syncUnitActionData("BUnitActionDOT::update damage.mAttackerID", damage.mAttackerID);
#endif

            pUnit->damage(damage);

            if ((DWORD) gWorld->getGametime() >= mDotStacks[i].mEndTime)
            {
               mDotStacks.erase(i);
            }
         }

         if (mDotStacks.size() == 0)
         {
            #ifdef SYNC_UnitAction
               syncUnitActionCode("BUnitActionDOT::update done");
            #endif

            setState(cStateDone);

            if (mAttachID != cInvalidObjectID)
               pUnit->removeAttachment(mAttachID);
         }

         break;
      }
   }

   return (true);
}


//==============================================================================
//==============================================================================
void BUnitActionDOT::setDOTduration(float duration)
{
   mDOTduration = duration;
}

//==============================================================================
//==============================================================================
bool BUnitActionDOT::operator==(const BUnitActionDOT& rhs)
{
   return (mDOTEffect == rhs.mDOTEffect && mDOTrate == rhs.mDOTrate && mDamageMultiplier == rhs.mDamageMultiplier);
}

//==============================================================================
//==============================================================================
bool BUnitActionDOT::operator==(const BDamage& rhs)
{
   return (mDOTEffect == rhs.mDOTEffect && mDOTrate == rhs.mDOTrate && mDamageMultiplier == rhs.mDamageMultiplier);
}

//==============================================================================
//==============================================================================
void BUnitActionDOT::addStack(BEntityID source, BTeamID sourceTeam, DWORD endTime)
{
   DotInfo info;
   info.mSourceTeamID = sourceTeam;
   info.mEndTime = endTime;
   info.mSource = source;
   mDotStacks.add(info);
}

//==============================================================================
//==============================================================================
void BUnitActionDOT::refreshStack(BEntityID source, BTeamID sourceTeam, DWORD endTime)
{
   bool bFoundStack = false;
   for(long i = 0; i < mDotStacks.getNumber(); i++)
   {
      if(mDotStacks[i].mSource == source && mDotStacks[i].mSourceTeamID == sourceTeam)
      {
         bFoundStack = true;
         mDotStacks[i].mEndTime = endTime;
         break;
      }
   }

   BASSERT(bFoundStack);
}

//==============================================================================
//==============================================================================
bool BUnitActionDOT::hasSource(BEntityID id)
{
   bool retVal = false;
   int count = mDotStacks.size();
   for (int i = 0; i < count; ++i)
   {
      if (mDotStacks[i].mSource == id)
      {
         retVal = true;
         break;
      }
   }
   
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionDOT::hasSource id", id);
      syncUnitActionData("BUnitActionDOT::hasSource retVal", retVal);
   #endif
   
   return retVal;
}

//==============================================================================
//==============================================================================
bool BUnitActionDOT::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   // mDotStacks
   int count = mDotStacks.getNumber();
   GFWRITEVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 200);
   for (long i = 0; i < count; i++)
   {
      GFWRITEVAR(pStream, BEntityID, mDotStacks[i].mSource);
      GFWRITEVAR(pStream, BTeamID, mDotStacks[i].mSourceTeamID);
      GFWRITEVAR(pStream, DWORD, mDotStacks[i].mEndTime);
   }

   GFWRITEVECTOR(pStream, mAttachLocation);
   GFWRITEVAR(pStream, long, mDOTEffect);
   GFWRITEVAR(pStream, BEntityID, mAttachID);
   GFWRITEVAR(pStream, float, mDamageMultiplier);
   GFWRITEVAR(pStream, float, mDOTrate);
   GFWRITEVAR(pStream, float, mDOTduration);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDOT::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   // mDotStacks
   int count;
   GFREADVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 200);
   for (int i=0; i<count; i++)
   {
      BEntityID entityID;
      BTeamID teamID;
      DWORD endTime;
      GFREADVAR(pStream, BEntityID, entityID);
      GFREADVAR(pStream, BTeamID, teamID);
      GFREADVAR(pStream, DWORD, endTime);
      addStack(entityID, teamID, endTime);
   }

   GFREADVECTOR(pStream, mAttachLocation);
   GFREADVAR(pStream, long, mDOTEffect);
   GFREADVAR(pStream, BEntityID, mAttachID);
   GFREADVAR(pStream, float, mDamageMultiplier);
   GFREADVAR(pStream, float, mDOTrate);
   GFREADVAR(pStream, float, mDOTduration);

   return true;
}

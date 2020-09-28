//==============================================================================
// UnitActionScaleLOS.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unit.h"
#include "unitactionScaleLOS.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionScaleLOS, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionScaleLOS::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   //BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionScaleLOS::disconnect()
{
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionScaleLOS::init()
{
   if (!BAction::init())
      return (false);

   mStartValue = 0;
   mFinishValue = 0;
   mFinishTime = 0;
   mDuration = 1;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionScaleLOS::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   //const BProtoObject* pProtoObject=pUnit->getProtoObject();
   //BASSERT(pProtoObject);

   switch (mState)
   {
      case cStateNone:
         setState(cStateWorking);
         break;
         
      case cStateWorking:
      {

         // [7/7/2008 xemu] scale LOS here 
         DWORD gameTime = gWorld->getGametime();
         if (gameTime >= mFinishTime)
         {
            setState(cStateDone);
            break;
         }

         DWORD timeRemaining = mFinishTime - gameTime;

         BASSERT(mDuration != 0);
         float progressFrac = 1.0f - ((float)timeRemaining / (float)mDuration);
         float newLOS = mStartValue + ((mFinishValue - mStartValue) * progressFrac);
         if (newLOS < 4.0f)
            newLOS = 4.0f;
         trace("LOS: %f",newLOS);
         pUnit->setLOSScalar(newLOS);
         break;
      }
   }

   return (true);
}


//==============================================================================
void BUnitActionScaleLOS::setDuration(DWORD d)
{
   mDuration = d;
   mFinishTime = gWorld->getGametime() + d;
}

//==============================================================================
//==============================================================================
bool BUnitActionScaleLOS::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mStartValue);
   GFWRITEVAR(pStream, float, mFinishValue);
   GFWRITEVAR(pStream, DWORD, mFinishTime);
   GFWRITEVAR(pStream, DWORD, mDuration);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionScaleLOS::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mStartValue);
   GFREADVAR(pStream, float, mFinishValue);
   GFREADVAR(pStream, DWORD, mFinishTime);
   GFREADVAR(pStream, DWORD, mDuration);
   return true;
}

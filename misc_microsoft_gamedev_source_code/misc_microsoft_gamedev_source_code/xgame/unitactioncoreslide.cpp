//==============================================================================
// unitactioncorslide.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactioncoreslide.h"
#include "entity.h"
#include "tactic.h"
#include "unit.h"
#include "squad.h"
#include "physics.h"
#include "physicsobject.h"
#include "designobjectsmanager.h"
#include "world.h"
#include "configsgame.h"
#include "mathutil.h"
#include "UnitActionCollisionAttack.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionCoreSlide, 1, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionCoreSlide::init(void)
{
   if (!BAction::init())
      return(false);

   initSlideAreas();

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionCoreSlide::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionCoreSlide::disconnect()
{
   BAction::disconnect();
}


//==============================================================================
//==============================================================================
bool BUnitActionCoreSlide::setState(BActionState state)
{
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   if (!pUnit)
      return false;
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   switch (state)
   {
      case cStateNone:
      case cStateWait:
         pSquad->setFlagUpdateTurnRadius(true);
         pSquad->getSquadAI()->setMode(BSquadAI::cModeNormal);
         break;

      case cStateWorking:
      {
         // Start the slide if its in a slide area
         if (mInArea >= 0 && mInArea < mSlideAreas.getNumber())
         {
            // Calc end pos of slide and set squad to it.  Enable physics / turn radius stuff for movement
            calcDesiredRestPos();
            pSquad->setPosition(mDesiredRestPos);
            pSquad->setLeashPosition(mDesiredRestPos);
            pSquad->setFlagUpdateTurnRadius(false);
            if (pUnit->getPhysicsObject())
               pUnit->getPhysicsObject()->forceActivate();

            // Set in hit-and-run mode to enable collision damage
            pSquad->getSquadAI()->setMode(BSquadAI::cModeHitAndRun);
            BUnitActionCollisionAttack* pUACA = reinterpret_cast<BUnitActionCollisionAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitCollisionAttack));
            if (pUACA)
            {
               pUACA->setFlagIgnoreTarget(true);
               pUACA->setFlagCollideWithFriendlies(true);
               pUACA->setFlagIgnoreAmmo(true);
               pUACA->setFlagUseObstructionForCollisions(true);
            }
         }
         // If not in slide area, just go back to wait state
         else
         {
            return setState(cStateWait);
         }

         break;
      }
   }
   return BAction::setState(state);
}


//==============================================================================
//==============================================================================
bool BUnitActionCoreSlide::update(float elapsed)
{
   BASSERT(mpOwner);
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   // Init slide areas if not itit'd
   if (mSlideAreasInitialized)
      initSlideAreas();

   switch (mState)
   {
      case cStateNone:
      case cStateWait:
      {
         updateAreaLocation();
         break;
      }
      case cStateWorking:
      {
         updateSlide(elapsed);
         break;
      }
   }

   // Debug
   #ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigDebugDodge))
      {
         // Debug render the slide areas
         for (int i = 0; i < mSlideAreas.getNumber(); i++)
         {
            BSlideArea& area = mSlideAreas[i];
            BDynamicSimVectorArray& points = area.mHull.getPoints();
            for (int j = 1; j < points.getNumber(); j++)
            {
               gpDebugPrimitives->addDebugLine(points[j - 1], points[j], cDWORDCyan, cDWORDCyan);
            }
            if (points.getNumber() > 2)
               gpDebugPrimitives->addDebugLine(points[points.getNumber() - 1], points[0], cDWORDCyan, cDWORDCyan);
         }

         // Debug render in/out
         BMatrix mtx;
         pUnit->getWorldMatrix(mtx);
         if (mInArea >= 0 && mInArea < mSlideAreas.getNumber())
            gpDebugPrimitives->addDebugCircle(mtx, pUnit->getObstructionRadius() * 1.5f, cDWORDGreen);
         else
            gpDebugPrimitives->addDebugCircle(mtx, pUnit->getObstructionRadius() * 1.5f, cDWORDRed);
      }
   #endif

   if(!BAction::update(elapsed))
      return (false);
   return (true);
}


//==============================================================================
//==============================================================================
bool BUnitActionCoreSlide::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITECLASSARRAY(pStream, saveType, mSlideAreas, uint8, 100);
   GFWRITEVECTOR(pStream, mDesiredRestPos);
   GFWRITEVAR(pStream, int, mInArea);
   GFWRITEBITBOOL(pStream, mSlideAreasInitialized);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionCoreSlide::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   if (BAction::mGameFileVersion >= 25)
      GFREADCLASSARRAY(pStream, saveType, mSlideAreas, uint8, 100)
   else
   {
      //GFREADARRAY(pStream, BSlideArea, mSlideAreas, uint8, 100);
      uint8 count = 0;
      GFREADVAR(pStream, uint8, count);
      GFVERIFYCOUNT(count, 100);
      if (pStream->skipBytes(sizeof(BSlideArea)*count) != sizeof(BSlideArea)*count)
      {
         {GFERROR("GameFile Error: skipBytes error, type BSlideArea, count %d, on line %s, %d", (int)count, __FILE__,__LINE__);} \
         return false;
      }
   }
   GFREADVECTOR(pStream, mDesiredRestPos);
   GFREADVAR(pStream, int, mInArea);
   GFREADBITBOOL(pStream, mSlideAreasInitialized);

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionCoreSlide::initSlideAreas()
{
   if (mSlideAreasInitialized)
      return;

   // Iterate through design lines we care about
   BDesignObjectManager* pDOM = gWorld->getDesignObjectManager();
   if (!pDOM)
      return;

   mSlideAreas.setNumber(0);

   // Make a slide area for each set of paired lines
   for (uint i = 0; i < pDOM->getDesignLineCount(); i++)
   {
      //Get the line.  Ignore non-slide lines.
      BDesignLine& line = pDOM->getDesignLine(i);
      if ((line.mDesignData.mValue != BDesignObjectData::cCoreSlideLine) || (line.mDesignData.mValues[1] != 1) || (line.mPoints.getNumber() < 2)) //1 = designation for start ramp, 2 = end ramp
         continue;

      int lineID = line.mDesignData.mValues[0];  //mValues[0] stores an identifying number so we can match it to the landing line

      //Find the matching end ramp line
      for (uint j = 0; j < pDOM->getDesignLineCount(); j++)
      {
         //Get the line.  Ignore non-slide lines.
         BDesignLine& line2 = pDOM->getDesignLine(j);
         if ((line.mDesignData.mValue != BDesignObjectData::cCoreSlideLine) || (line2.mDesignData.mValues[0] != lineID) || (line2.mDesignData.mValues[1] != 2) || (line2.mPoints.getNumber() < 2))
            continue;

         //Found the end ramp line, make a new slide area

         // Add new slide area, setting the start line points and creating the hull
         BSlideArea& newSlideArea = mSlideAreas.grow();
         newSlideArea.mStartPt1 = line.mPoints[0];
         newSlideArea.mStartPt2 = line.mPoints[1];

         BVector points[4];
         points[0] = line.mPoints[0];;
         points[1] = line.mPoints[1];
         points[2] = line2.mPoints[0];
         points[3] = line2.mPoints[1];
         newSlideArea.mHull.clear();
         newSlideArea.mHull.addPoints(points, 4);

         break;
      }
   }

   mSlideAreasInitialized = true;
}

//==============================================================================
//==============================================================================
void BUnitActionCoreSlide::updateAreaLocation()
{
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   // Determine if it is inside any slide area
   mInArea = -1;   
   for (int i = 0; i < mSlideAreas.getNumber(); i++)
   {
      BSlideArea& area = mSlideAreas[i];
      if (area.mHull.inside(pUnit->getPosition()))
      {
         mInArea = i;
         return;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionCoreSlide::calcDesiredRestPos()
{
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   mDesiredRestPos = pUnit->getPosition();

   if (mInArea < 0 || mInArea >= mSlideAreas.getNumber())
      return;

   BSlideArea& area = mSlideAreas[mInArea];
   mDesiredRestPos = closestPointOnLine(area.mStartPt1, area.mStartPt2, pUnit->getPosition());
}

//==============================================================================
//==============================================================================
void BUnitActionCoreSlide::updateSlide(float elapsed)
{
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return;

   //BVector currentPos = pUnit->getPosition();
   const float cActionCompleteEpsilon = 0.1f;
   BVector currentPos = pSquad->getTurnRadiusPos();
   float distanceLeft = currentPos.xzDistance(mDesiredRestPos);
   if (distanceLeft < cActionCompleteEpsilon)
   {
      setState(cStateWait);
      return;
   }

   BVector dir = mDesiredRestPos - currentPos;
   dir.y = 0.0f;
   if (!dir.safeNormalize())
      return;

   static float vel = 10.0f;
   float distanceThisFrame = vel * elapsed;
   if (distanceThisFrame > distanceLeft)
      distanceThisFrame = distanceLeft;

   BVector newPos = currentPos + dir * distanceThisFrame;
   pSquad->setTurnRadiusPos(newPos);
}

//==============================================================================
//==============================================================================
bool BSlideArea::save(BStream* pStream, int saveType) const
{
   GFWRITECLASS(pStream, saveType, mHull);
   GFWRITEVECTOR(pStream, mStartPt1);
   GFWRITEVECTOR(pStream, mStartPt2);
   return true;
}

//==============================================================================
//==============================================================================
bool BSlideArea::load(BStream* pStream, int saveType)
{
   GFREADCLASS(pStream, saveType, mHull);
   GFREADVECTOR(pStream, mStartPt1);
   GFREADVECTOR(pStream, mStartPt2);
   return true;
};

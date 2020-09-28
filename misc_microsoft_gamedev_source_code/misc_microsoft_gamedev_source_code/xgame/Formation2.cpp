//==============================================================================
// formation2.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#include "Common.h"
#include "Formation2.h"
#include "Action.h"
#include "ConfigsGame.h"
#include "ObstructionManager.h"
#include "Platoon.h"
#include "ProtoObject.h"
#include "Squad.h"
#include "SquadActionMove.h"
#include "SquadPlotter.h"
#include "World.h"
#include "Unit.h"
#include "UnitQuery.h"
#include "Tactic.h"
#include "mathutil.h"
#include "containers\dynamicarray2d.h"

//#define DEBUGASSIGNMENT

const float cMobOutlierRadiusToSquadRadiusRatio = 2.5f;
const float cMobOutlierRadiusPullInMultiplier = 0.75f;
const float cMobSpacingFactorMin = 1.3f;
const float cMobSpacingFactorMax = 1.8f;
const DWORD cMobReassignTimeout = 2000;
const float cSearchFactor = 2.0f;

GFIMPLEMENTVERSION(BFormation2, 1);

//==============================================================================
//==============================================================================
class BMobFormationCachedData
{
public:
   BEntity* mpEntity;
   uint mPosIndex;
   float mDistanceSqr;
};


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BFormation2, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BFormation2::init()
{
   mPrevForward = cInvalidVector;
   mpOwner=NULL;
   mID=0;
   mType=eTypeStandard;
   mChildren.clear();
   mPositions.clear();
   mRadiusX=0.0f;
   mRadiusZ=0.0f;
   mLineRatio = 1.4f;
   
   mReassignTimer = 0;
//   mPositionErrorAtLastAssignmentSqr = 0.0f;

   mMakeNeeded=false;
   mReassignNeeded=false;
   mDynamicPrioritiesValid=false;
   mUseRandom = true;
   mGetObstructions = true;
   return (true);
}

//==============================================================================
//==============================================================================
bool BFormation2::addChild(BEntityID childID)
{
   mChildren.uniqueAdd(childID);

   mMakeNeeded=true;
   return (true);
}

//==============================================================================
//==============================================================================
bool BFormation2::removeChild(BEntityID childID)
{
   if (!mChildren.remove(childID))
      return (false);

   mMakeNeeded=true;
   return (true);
}

//==============================================================================
//==============================================================================
bool BFormation2::addChildren(const BEntityIDArray &childrenIDs)
{
   bool result = true;
   uint numChildrenToAdd = childrenIDs.getSize();
   for (uint i = 0; i < numChildrenToAdd; i++)
   {
      if (!addChild(childrenIDs[i]))
      {
         result = false;
      }
   }

   return (result);
}

//==============================================================================
//==============================================================================
bool BFormation2::removeChildren()
{
   bool result = true;
   int numChildren = mChildren.getNumber();
   for (int i = (numChildren - 1); i >= 0; i--)
   {
      if (!removeChild(mChildren[i]))
      {
         result = false;
      }
   }

   return (result);
}

//==============================================================================
//==============================================================================
uint BFormation2::getDefaultChildPriority(BEntityID childID) const
{
   const BFormationPosition2* pFP=getFormationPosition(childID);
   if (!pFP)
      return (BFormationPosition2::cInvalidPriority);
   return (pFP->getPriority());
}

//==============================================================================
//==============================================================================
uint BFormation2::getChildPriority(BEntityID childID) const
{
   const BFormationPosition2* pFP=getFormationPosition(childID);
   if (!pFP)
      return (BFormationPosition2::cInvalidPriority);
   if (mDynamicPrioritiesValid)
      return(pFP->getDynamicPriority());
   return (pFP->getPriority());
}

//==============================================================================
//==============================================================================
BEntityID BFormation2::getChildByPriority(uint priority) const
{
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      uint tempPriority;
      if (mDynamicPrioritiesValid)
         tempPriority=mPositions[i].getDynamicPriority();
      else
         tempPriority=mPositions[i].getPriority();

      if (tempPriority == priority)
         return (mPositions[i].getEntityID());
   }
   return (cInvalidObjectID);
}

//==============================================================================
//==============================================================================
void BFormation2::scale(float v)
{
   // Don't scale if only one entity in the formation so the obstruction isn't
   // inflated unnecessarily.
   if (mPositions.getSize() <= 1)
      return;
      
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      BVector foo=mPositions[i].getOffset();
      foo*=v;
      mPositions[i].setOffset(foo);
   }
   mRadiusX*=(float)fabs(v);
   mRadiusZ*=(float)fabs(v);
}

//==============================================================================
//==============================================================================
float BFormation2::getForwardAmount() const
{
   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("BForm2::getForwardAmount:");
   #endif
   //Return the largest forward offset plus the radius of the entity in that slot.
   float totalZOffset = 0.0f;
   float rVal=0.0f;  
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      float fa=mPositions[i].getOffset().z;
      totalZOffset += fa;

//-- FIXING PREFIX BUG ID 4711
      const BEntity* pEntity=gWorld->getEntity(mPositions[i].getEntityID());
//--
      if (pEntity)
      {
         fa+=pEntity->getObstructionRadius();
         #ifdef DEBUGASSIGNMENT
         mpOwner->debug("  EID=%d ObRad=%f plus Z Offset=%f equals %f.", pEntity->getID(), pEntity->getObstructionRadius(), mPositions[i].getOffset().z, fa);
         #endif
      }
      #ifdef DEBUGASSIGNMENT
      else
         mpOwner->debug("  EID=%d is invalid, just using the Z Offset=%f.", pEntity->getID(), fa);
      #endif
      if (fa > rVal)
      {
         #ifdef DEBUGASSIGNMENT
         mpOwner->debug("  That's bigger than %f, using the new one.", rVal);
         #endif
         rVal=fa;
      }
   }

   // Subtract the average z offset.  The platoon position is just the average squad position, so it can
   // be offset in the formation by the average z offset.
   if (mPositions.getSize() > 0)
      rVal -= (totalZOffset / mPositions.getSize());

   //Put an extra small jump forward just to fix any rounding/whatever errors.
   rVal+=1.0f;
   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("  Returning %f.", rVal);
   #endif
   return (rVal);
}

//==============================================================================
//==============================================================================
void BFormation2::makeFormation()
{
   //Undo the make flag.
   mMakeNeeded=false;

   //Set our positions to our children size.
   mPositions.setNumber(mChildren.getSize());
   //Reset the positions.
   for (uint i=0; i < mPositions.getSize(); i++)
      mPositions[i].reset();

   //If we have no children, bail.
   if (mChildren.getSize() == 0)
   {
      mRadiusX=0.0f;
      mRadiusZ=0.0f;
      return;
   }

   //Mmm, hacks.
   switch (mType)
   {
      case eTypeStandard:
      case eTypeFlock:
      case eTypeGaggle:
         makeStandardFormation();
         break;
      case eTypeLine:
         makeLineFormation();
         break;
      case eTypePlatoon:
         makePlatoonLineFormation();
         break;
      case eTypeMob:
         makeMobFormation();
         break;
   }
}

//==============================================================================
//==============================================================================
void BFormation2::makeStandardFormation()
{
   // Singleton Unit squad case - just init
   if (mChildren.getSize() == 1)
   {
      //Get the entity info.
//-- FIXING PREFIX BUG ID 4712
      const BEntity* pEntity=gWorld->getEntity(mChildren[0]);
//--
      mRadiusX=pEntity->getObstructionRadiusX();
      mRadiusZ=pEntity->getObstructionRadiusZ();
      mPositions[0].setEntityID(pEntity->getID());
      mPositions[0].setOffset(cOriginVector);
      mPositions[0].setPriority(0);
      return;
   }

   //Still Dumb.
   mRadiusX=0.0f;
   mRadiusZ=0.0f;
   float largestObstructionRadius=0.0f;
   BVector averageOffset=cOriginVector;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      //Get the entity info.
//-- FIXING PREFIX BUG ID 4714
      const BEntity* pEntity=gWorld->getEntity(mChildren[i]);
//--
      float obstructionRadius=pEntity->getObstructionRadius();
      if (obstructionRadius > largestObstructionRadius)
         largestObstructionRadius=obstructionRadius;
      //Save the entity info into the position.
      mPositions[i].setEntityID(pEntity->getID());

      //Pick a "random" spot and march it out until it's not on top of anything
      //anymore.
      float randX = mUseRandom ? getRandRangeFloat(cSimRand, -1.0f, 1.0f) : 0.0f;
      float randZ = mUseRandom ? getRandRangeFloat(cSimRand, -1.0f, 1.0f) : 0.0f;
      BVector offset(randX, 0.0f, randZ);
      bool collided=true;
      uint count=0;
      while (collided && (count < 10))
      {
         collided=false;
         count++;
         for (uint j=0; j < i; j++)
         {
//-- FIXING PREFIX BUG ID 4713
            const BEntity* pEntity2=gWorld->getEntity(mPositions[j].getEntityID());
//--
            float obstructionRadius2=pEntity2->getObstructionRadius();

            BVector direction=offset-mPositions[j].getOffset();
            float distance=direction.length();
            float obstructionTotal=obstructionRadius+obstructionRadius2;
            if (obstructionTotal > distance)
            {
               float difference=obstructionTotal-distance;
               direction.normalize();
               if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
                  direction*=difference*1.2f;
               else
                  direction*=difference*1.8f;
               offset+=direction;
               collided=true;
            }
         }
      }

      //Save the offset into the position.
      mPositions[i].setOffset(offset);
        
      //Track the average offset.
      averageOffset+=offset;
   }
   averageOffset/=static_cast<float>(mChildren.getSize());

   //"Center" the formation.
   for (uint i=0; i < mPositions.getSize(); i++)
      mPositions[i].adjustOffset(-averageOffset);
   
   
   //If we have a platoon, it's radius is the largest radius of its individual squads.
   float constantScaleFactor = 1.0f;
   if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
   {
      mRadiusX=largestObstructionRadius;
      mRadiusZ=largestObstructionRadius;
   }
   // Else, calculate the radius from adjusted positions.  Doing this before positions
   // are adjusted can result in some error if the offsets aren't evenly distributed
   else
   {
      //Update the radius.
      float maxObsRadiusX = 0.0f;
      float radius=0.0f;
      for (uint i=0; i < mChildren.getSize(); i++)
      {
//-- FIXING PREFIX BUG ID 4715
         const BEntity* pEntity=gWorld->getEntity(mChildren[i]);
//--
         float obstructionRadius=pEntity->getObstructionRadius();
         maxObsRadiusX += obstructionRadius;

         float magnitude=mPositions[i].getOffset().length();
         magnitude+=obstructionRadius;
         if (magnitude > radius)
            radius = magnitude;
      }

      mRadiusX=radius;
      mRadiusZ=radius;

      // Calculate an extra scaling factor to force the formation into a constant
      // size.  In this case the size is a very scientifically arrived at 0.75
      // times the maximum x radius if all obstructions were in a line.
      constantScaleFactor = maxObsRadiusX * 0.75f / radius;
   }

      
   //Scale it for some simple spacing.
   float scaleFactor=1.0f;
   if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
      scaleFactor*=1.3f;
   else
      scaleFactor*=1.1f * constantScaleFactor;
   //If we're a gaggle, up the scale.
   if (mType == eTypeGaggle)
      scaleFactor*=1.3f;
   //Actually scale.      
   scale(scaleFactor);
   
   //Prioritize it.
   prioritizePositions();
}

//==============================================================================
//==============================================================================
void BFormation2::makeLineFormation()
{
   mRadiusX=0.0f;
   mRadiusZ=0.0f;
   BVector averageOffset=cOriginVector;
   float currentX=0.0f;
   float maxPossibleRadiusX = 0.0f;

   for (uint i=0; i < mChildren.getSize(); i++)
   {
      //Get the entity info.
//-- FIXING PREFIX BUG ID 4716
      const BEntity* pEntity=gWorld->getEntity(mChildren[i]);
//--
      float obstructionRadius=pEntity->getObstructionRadius();
      //Save the entity info into the position.
      mPositions[i].setEntityID(pEntity->getID());

      //Create the offset.
      float randX = mUseRandom ? getRandRangeFloat(cSimRand, 0.0f, 1.0f) : 0.0f;
      randX *= obstructionRadius;
      float randZ = mUseRandom ? getRandRangeFloat(cSimRand, -0.6f, 0.6f) : 0.0f;
      randZ *= obstructionRadius;
      BVector offset(currentX + obstructionRadius + randX, 0.0f, randZ); 

      //Save the offset into the position.
      mPositions[i].setOffset(offset);

      //Track the average offset.
      averageOffset+=offset;
      //Increment our currentX.
      currentX=offset.x+obstructionRadius;

      maxPossibleRadiusX += 1.5f * obstructionRadius;
   }
   averageOffset/=static_cast<float>(mChildren.getSize());

   //Radius is half of currentX.
   mRadiusX=currentX * 0.5f;
   mRadiusZ=mRadiusX;

   //"Center" the formation.
   for (uint i=0; i < mPositions.getSize(); i++)
      mPositions[i].adjustOffset(-averageOffset);
      
   //Scale it for some simple spacing.
   if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
      scale(1.3f);
   else
   {
      // Calculate an extra scaling factor to force the formation into a constant
      // size.  In this case the size is a very scientifically arrived at 0.75
      // times the maximum x radius.
      float constantScaleFactor = maxPossibleRadiusX * 0.75f / mRadiusX;
      scale(1.1f * constantScaleFactor);
   }
   
   //Prioritize it.
   prioritizePositions();
}

//==============================================================================
//==============================================================================
bool BFormation2::makeAttackFormation(BSimTarget target)
{
   //MMM, can you say f'in uber hack?
//-- FIXING PREFIX BUG ID 4718
   const BPlatoon* pPlatoon=reinterpret_cast<BPlatoon*>(mpOwner);
//--
   BASSERT(pPlatoon);

//-- FIXING PREFIX BUG ID 4719
   const BEntity* pTarget=gWorld->getEntity(target.getID());
//--
   if (!pTarget)
      return (false);

   BDynamicSimVectorArray foo;
   gSquadPlotter.plotSquads(pPlatoon->getChildList(), pPlatoon->getPlayerID(), pTarget->getID(),
      foo, pTarget->getPosition(), target.getAbilityID(), 0);

   const BDynamicSimArray<BSquadPlotterResult>& plotterResults=gSquadPlotter.getResults();
   if (plotterResults.getNumber() != mChildren.getNumber())
      return (false);

   //Set our positions to our children size.
   mPositions.setNumber(mChildren.getSize());
   //Reset the positions.
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      mPositions[i].reset();
      //Save the entity info into the position.
      mPositions[i].setEntityID(mChildren[i]);
      mPositions[i].setOffset(cOriginVector);
      mPositions[i].setPosition(plotterResults[i].getDesiredPosition());
      mPositions[i].setTransform(false);
   }

   //Prioritize.
   prioritizePositions();
   
   //Save the type.
   mType=eTypeAttack;

   return (true);
}


//==============================================================================
//==============================================================================
void BFormation2::makePlatoonLineFormation()
{
   // Make sure this is a platoon
   if (!mpOwner || (mpOwner->getClassType() != BEntity::cClassTypePlatoon))
      return;

   ////////////////////////////////////////////////////////////////////////////
   // Sort squads into logical lines based on range / depth
   // Everything on the same line doesn't have the same range, it just
   // means we can't fit a squad of lower range between the target and
   // the highest range squad on the same line (because of squad formation depth)
   uint numSquads = mChildren.getNumber();
   static BDynamicSimArray<BFormationCachedSquadData> squadData;
   squadData.setNumber(numSquads);
   static BSimIntArray tempSquadOrder;
   tempSquadOrder.setNumber(0);
   for (uint i = 0; i < numSquads; i++)
   {
      // Gather data about the squads
      BSquad* pSquad = gWorld->getSquad(mChildren[i]);
      BASSERT(pSquad);
      if (pSquad)
      {
         squadData[i].setSquad(pSquad);
         squadData[i].setRange(pSquad->getPathingRange());
         squadData[i].setWidth(pSquad->getObstructionRadiusX());
         squadData[i].setDepth(pSquad->getObstructionRadiusZ());
      }
      else
      {
         squadData[i].setSquad(NULL);
         squadData[i].setRange(0.0f);
         squadData[i].setWidth(0.0f);
         squadData[i].setDepth(0.0f);
      }

      // Sort squads from lowest inner radius to highest inner radius
      if (i == 0)
         tempSquadOrder.add(0);
      else
      {
         float innerRadiusToSort = squadData[i].getRange() - squadData[i].getDepth();

         // Increment insert index while this squad's inner radius larger than those already inserted
         bool outDistanced = false;
         uint insertIndex = 0;
         while ((insertIndex < i) && !outDistanced)
         {
            float innerRadiusToTest = squadData[tempSquadOrder[insertIndex]].getRange() -
                                      squadData[tempSquadOrder[insertIndex]].getDepth();
            if (innerRadiusToSort > innerRadiusToTest)
               insertIndex++;
            else
               outDistanced = true;
         }
         
         // Insert
         tempSquadOrder.insertAtIndex(i, insertIndex);
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Now that the squads are sorted by inner radius, put them
   // into logical lines.  You only add a new line if the 
   // inner radius is greater than the outer radius (+ some apacing)
   // of the line you're testing against.
   float lineSpacing = 1.0f;
   static BDynamicSimLongArray lines;           // number of squads in each line
   static BDynamicSimLongArray originalLineIndices;   // original index of line (used to group for reassigning positions)
   static BDynamicSimFloatArray lineOuterRadii;  // the outer radii of each line
   static BDynamicSimFloatArray lineInnerRadii;  // the inner radii of each line
   lines.resize(0);
   originalLineIndices.resize(0);
   lineOuterRadii.resize(0);
   lineInnerRadii.resize(0);

   // Initialize line data with 1st squad
   lines.add(1);
   originalLineIndices.add(0);
//-- FIXING PREFIX BUG ID 4721
   const BFormationCachedSquadData* pTempResult = &squadData[tempSquadOrder[0]];
//--
   float radiiToTest = pTempResult->getRange() + pTempResult->getDepth() + lineSpacing;
   float tempInnerRadii = Math::Max(0.0f, pTempResult->getRange() - pTempResult->getDepth());
   lineOuterRadii.add(radiiToTest);
   lineInnerRadii.add(tempInnerRadii);

   for (uint i = 1; i < numSquads; i++)
   {
      pTempResult = &squadData[tempSquadOrder[i]];
      float radiiToSort = Math::Max(0.0f, pTempResult->getRange() - pTempResult->getDepth());

      // Go through all lines testing to see which one this gets added to
      bool squadPlaced = false;
      long lineIndex = 0;
      while (!squadPlaced && (lineIndex < lines.getNumber()))
      {
         radiiToTest = lineOuterRadii[lineIndex];

         // If this squad's inner radius is greater than the outer radius of the line we're testing
         // against, promote it to a further line
         if (radiiToSort > radiiToTest)
         {
            lineIndex++;
         }
         // Otherwise, it gets added to the tested line, and the outer radii potentially increases
         else
         {
            lines[lineIndex] = lines[lineIndex] + 1;

            float tempOuterRadii = pTempResult->getRange() + pTempResult->getDepth() + lineSpacing;
            lineOuterRadii[lineIndex] = Math::Max(lineOuterRadii[lineIndex], tempOuterRadii);

            tempInnerRadii = Math::Max(0.0f, pTempResult->getRange() - pTempResult->getDepth());
            lineInnerRadii[lineIndex] = Math::Min(lineInnerRadii[lineIndex], tempInnerRadii);

            squadPlaced = true;
         }
      }

      // If existing line not found, make a new one
      if (!squadPlaced)
      {
         lines.add(1);
         originalLineIndices.add(originalLineIndices.getSize());

         float tempOuterRadii = pTempResult->getRange() + pTempResult->getDepth() + lineSpacing;
         lineOuterRadii.add(tempOuterRadii);

         tempInnerRadii = Math::Max(0.0f, pTempResult->getRange() - pTempResult->getDepth());
         lineInnerRadii.add(tempInnerRadii);
      }
   }

   BASSERT(lines.getSize() == originalLineIndices.getSize());

   ////////////////////////////////////////////////////////////////////////////
   // Now that the squads are divided into lines by range, see if each line
   // is long enough that it needs to be broken up into multiple lines
   uint squadOrderIndex = tempSquadOrder.getSize();
   for (int i = lines.getNumber() - 1; i >= 0; i--)
   {
      if (lines[i] <= 1)
         continue;

      // Lines contains number of squads in each line.  Since working backwards subtract the number
      // of squads in this line to get the index to the squad data.
      squadOrderIndex -= lines[i];

      // Get total width and depth of the squads in this line
      float totalWidth = 0.0f;
      float totalDepth = 0.0f;
      for (int j = 0; j < lines[i]; j++)
      {
         totalWidth += squadData[tempSquadOrder[squadOrderIndex + j]].getWidth() * 2.0f;
         totalDepth = Math::Max(totalDepth, squadData[tempSquadOrder[squadOrderIndex + j]].getDepth() * 2.0f);
      }

      // Calculate how many units should fit on a line
      int numLinesToMake = 0;
      float maxLineWidth = 0.0f;
      calculateLineDimensions(totalWidth, totalDepth, mLineRatio, numLinesToMake, maxLineWidth);

      // If the lines should be broken up then fill each line up to the maxLineWidth then start a new line
      if (numLinesToMake > 1)
      {
         uint lineIndex = i;
         uint numSquadsInLines = lines[i];
         lines[i] = 0;
         float lineWidthFilled = 0.0f;
         for (uint j = 0; j < numSquadsInLines; j++)
         {
            // See if new line needed
            if ((lineWidthFilled > cFloatCompareEpsilon) && ((lineWidthFilled + squadData[tempSquadOrder[squadOrderIndex + j]].getWidth()) > maxLineWidth))
            {
               lineIndex++;
               int origLineIndex = originalLineIndices[i];
               originalLineIndices.insertAtIndex(origLineIndex, lineIndex);
               lines.insertAtIndex(0, lineIndex);
               lineWidthFilled = 0.0f;
            }

            // Add squad to this line
            lineWidthFilled += squadData[tempSquadOrder[squadOrderIndex + j]].getWidth() * 2.0f;
            lines[lineIndex] += 1;
         }
      }
   }

   BASSERT(lines.getSize() == originalLineIndices.getSize());

   ////////////////////////////////////////////////////////////////////////////
   // Now we're sorted into lines, so set the position info
   squadOrderIndex = 0;
   uint lineStartPriority = 0;
   BVector tempOffset(0.0f, 0.0f, 0.0f);
   for (long i = 0; i < lines.getNumber(); i++)
   {
      // Place along line
      for (long j = 0; j < lines[i]; j++)
      {
         int squadIndex = tempSquadOrder[squadOrderIndex + j];
         BSquad *sq = squadData[squadIndex].getSquad();
         BASSERT(sq);

         mPositions[squadIndex].setEntityID(sq->getID());
         mPositions[squadIndex].setOriginalLineIndex(originalLineIndices[i]);
         mPositions[squadIndex].setActualLineIndex(i);

         // Set a temporary offset so the function that sets the real offset knows where the positions should be
         // relative to each other
         tempOffset.x = static_cast<float>(j);
         mPositions[squadIndex].setOffset(tempOffset);

         // Calculate the priority on this line and offset by lineStartPriority
         // Priorities are assigned from the inside of the line out.  Examples
         // 7 units -->   5 3 1 0 2 4 6
         // 6 units -->    5 3 1 0 2 4
         int lineRelativePriority;
         int mid = lines[i] / 2;
         if (j < mid)
            lineRelativePriority = 2 * (mid - j) - 1;
         else
            lineRelativePriority = 2 * (j - mid);
         mPositions[squadIndex].setPriority(lineStartPriority + lineRelativePriority);
      }
      lineStartPriority += lines[i]; // increment lineStartPriority

      // Increment squad order index
      squadOrderIndex += lines[i];
   }

   // Set the position offsets
   adjustPlatoonFormationOffsets();
}

//==============================================================================
//==============================================================================
void BFormation2::makeMobFormation()
{
   // Set reassign timer so it can start counting down.
   mReassignTimer = cMobReassignTimeout;
   
   // For now, just set this to 0 when we do a make since this is designed so each formation
   // position is exactly at it's entity's posiiton.  If that were to change we'd need to go through
   // computePositionErrorSqr to get this initialized correctly.
//   mPositionErrorAtLastAssignmentSqr = 0.0f;
   
   // Single entity
   if (mChildren.getSize() == 1)
   {
      //Get the entity info.
//-- FIXING PREFIX BUG ID 4722
      const BEntity* pEntity = gWorld->getEntity(mChildren[0]);
//--
      mRadiusX = pEntity->getObstructionRadiusX();
      mRadiusZ = pEntity->getObstructionRadiusZ();
      mPositions[0].setEntityID(pEntity->getID());
      mPositions[0].setOffset(cOriginVector);
      mPositions[0].setPriority(0);
      return;
   }

   mRadiusX = 0.0f;
   mRadiusZ = 0.0f;
   
   #ifndef _MOVE4
   BVector averagePosition = cOriginVector;
   #endif

   // Set initial position data and calculate average position
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
      BEntity* pEntity = gWorld->getEntity(mChildren[i]);

      mPositions[i].setEntityID(pEntity->getID());
      BVector entityPos = pEntity->getPosition();
      mPositions[i].setOffset(entityPos);
      
      #ifndef _MOVE4
      averagePosition += entityPos;
      #endif
   }
   
   #ifndef _MOVE4
   averagePosition /= static_cast<float>(mChildren.getSize());

   // Find the entity closest to the average position
   uint closestToCenter = 0;
   float distanceToCenterSqr = FLT_MAX;
   for (uint i = 0; i < mPositions.getSize(); i++)
   {
      float distanceSqr = averagePosition.xzDistanceSqr(mPositions[i].getOffset());
      if (distanceSqr < distanceToCenterSqr)
      {
         closestToCenter = i;
         distanceToCenterSqr = distanceSqr;
      }
   }
   averagePosition = mPositions[closestToCenter].getOffset();
   #else
   // jce [5/12/2008] -- Get the average position as the owner's position since we need our
   // center to correspond to the owner's center since the transform we'll be handed is relative to
   // the owner.
   BVector averagePosition = mpOwner->getPosition();
   #endif

   static BSmallDynamicSimArray<BMobFormationCachedData> sortedPositionData;
   sortedPositionData.resize(0);
   float largestSquadRadius = 0.0f;

   // Move the formation so it's centered around squad closest to center and cache position data sorted by distance to origin
   for (uint i = 0; i < mPositions.getSize(); i++)
   {
      // Move relative to squad closest to center
      mPositions[i].adjustOffset(-averagePosition);

      // Get position data
      BMobFormationCachedData tempPosData;
      tempPosData.mpEntity = gWorld->getEntity(mPositions[i].getEntityID());
      tempPosData.mPosIndex = i;
      BVector offset = mPositions[i].getOffset();
      tempPosData.mDistanceSqr = (offset.x * offset.x + offset.z * offset.z);

      // Insert the position data into its sorted position
      uint insertIndex = 0;
      for ( ; insertIndex < sortedPositionData.getSize(); insertIndex++)
      {
         if (sortedPositionData[insertIndex].mDistanceSqr > tempPosData.mDistanceSqr)
            break;
      }
      sortedPositionData.insertAtIndex(tempPosData, insertIndex);

      // Calculated largest squad radius to be used later
      if (largestSquadRadius < tempPosData.mpEntity->getObstructionRadius())
         largestSquadRadius = tempPosData.mpEntity->getObstructionRadius();
   }

   // Radius used to find squads too far from center
   #ifndef _MOVE4
   float outlierRadius = mPositions.getSize() * largestSquadRadius * cMobOutlierRadiusToSquadRadiusRatio;
   float outlierRadiusSqr = outlierRadius * outlierRadius;
   // Factor used to extend ray from origin through current squad position.  This used to find other squads to move away from.
   float lineExtensionFactor = 100.0f;
   if (gConfig.get(cConfigPlatoonRadius, &lineExtensionFactor))
      lineExtensionFactor *= 2.0f;
   #endif
   float minX = FLT_MAX;
   float minZ = FLT_MAX;
   float maxX = -FLT_MAX;
   float maxZ = -FLT_MAX;

   // Adjust positions of squads so they aren't too close or too far from other squads.
   // Current squad will be moved along the vector from center to the squad's position.
   #ifndef _MOVE4
   averagePosition = cOriginVector;
   #endif
   for (uint i = 0; i < sortedPositionData.getSize(); i++)
   {
      BVector offset = mPositions[sortedPositionData[i].mPosIndex].getOffset();
      float obstructionRadius = sortedPositionData[i].mpEntity->getObstructionRadius();

      
      // jce [5/6/2008] -- let the _MOVE4 flocking stuff take care of this on the fly.
      #ifndef _MOVE4
      uint posIndex = sortedPositionData[i].mPosIndex;

      if (i != 0)
      {
         // Pick a random direction if this squad on top of the center squad
         BVector offsetDirection = offset;
         offsetDirection.y = 0.0f;
         while (offsetDirection.lengthSquared() < cFloatCompareEpsilon)
         {
            offsetDirection.x = getRandRangeFloat(cSimRand, -1.0f, 1.0f);
            offsetDirection.z = getRandRangeFloat(cSimRand, -1.0f, 1.0f);
         }

         // Find farthest out squad along current squad's vector from center
         int farthestSquadIndex = -1;
         float farthestSquadDistanceSqr = -1.0f;
         for (uint j = 0; j < i; j++)
         {
            BEntity* pEntity2 = sortedPositionData[j].mpEntity;
            BVector offset2 = mPositions[sortedPositionData[j].mPosIndex].getOffset();
            float obstructionRadius2 = pEntity2->getObstructionRadius();
            float obstructionTotal = obstructionRadius + obstructionRadius2;
            float obstructionTotalSqr = obstructionTotal * obstructionTotal;

            // Only want squads within obstruction distance of ray from origin through current squad's position
            float distanceSqr = distanceToSegmentSqr(offset2.x, offset2.z, 0.0f, 0.0f, (offsetDirection.x * lineExtensionFactor), (offsetDirection.z * lineExtensionFactor));
            if ((distanceSqr < obstructionTotalSqr) && (sortedPositionData[j].mDistanceSqr > farthestSquadDistanceSqr))
            {
               farthestSquadIndex = static_cast<int>(j);
               farthestSquadDistanceSqr = sortedPositionData[j].mDistanceSqr;
            }
         }

         // Move the current offset along its vector from center so it isn't too close or too far from other squads
         if (farthestSquadIndex != -1)
         {
            float obstructionTotal = obstructionRadius + sortedPositionData[farthestSquadIndex].mpEntity->getObstructionRadius();
            bool tooFarOut = (sortedPositionData[i].mDistanceSqr > outlierRadiusSqr);

            // If farthest out squad is farther than this squad (happens if closer squad was pushed out past this squad) OR
            // farthest out squad is too close to this squad OR
            // this squad is too far away from the center of the formation then adjust its position
            if ((farthestSquadDistanceSqr > sortedPositionData[i].mDistanceSqr) ||
                (sortedPositionData[i].mpEntity->calculateXZDistance(sortedPositionData[farthestSquadIndex].mpEntity) < (obstructionTotal * cMobSpacingFactorMin)) ||
                tooFarOut)
            {
               // Calculate position that is along the vector between the origin and the current position, making sure it is
               // beyond the farthest squad
               offset = offsetDirection;
               offset.normalize();
               float randSpacing = mUseRandom ? getRandRangeFloat(cSimRand, cMobSpacingFactorMin, cMobSpacingFactorMax) : cMobSpacingFactorMin;
               float desiredDistance = sqrt(farthestSquadDistanceSqr) + (obstructionTotal * randSpacing);

               // If squad is far from the center of the formation see if it can be moved closer
               if (tooFarOut)
                  desiredDistance = Math::Max(desiredDistance, (outlierRadius * cMobOutlierRadiusPullInMultiplier));

               offset *= desiredDistance;
            }
         }
      }
      #endif

      // Set new offset
      #ifndef _MOVE4
      mPositions[posIndex].setOffset(offset);
      #endif
      sortedPositionData[i].mDistanceSqr = (offset.x * offset.x + offset.z * offset.z);

      // Calculate extents
      #ifndef _MOVE4
      averagePosition += offset;
      #endif
      minX = Math::Min(minX, (offset.x - obstructionRadius));
      maxX = Math::Max(maxX, (offset.x + obstructionRadius));
      minZ = Math::Min(minZ, (offset.z - obstructionRadius));
      maxZ = Math::Max(maxZ, (offset.z + obstructionRadius));
   }
   averagePosition /= static_cast<float>(mChildren.getSize());


   // Set radius
   mRadiusX = Math::Max((maxX - minX), (maxZ - minZ)) * 0.5f;
   mRadiusZ = mRadiusZ;

   #ifndef _MOVE4
   // Recenter the formation
   for (uint i = 0; i < mPositions.getSize(); i++)
      mPositions[i].adjustOffset(-averagePosition);
   #endif
      
   // Prioritize it
   // Calculate new priorities         
   //mpFormation->calculateDynamicPriorities(forward);

   prioritizePositions();
}


//==============================================================================
//==============================================================================
BVector BFormation2::computeFlockingForce(long formationPosIndex, BVector position, BVector velocity, BEntityIDArray &ignoreList, BVector formationPos, BVector formationDir, BVector spinePoint)
{
   // Get entity.
   BEntity *entity = gWorld->getEntity(mPositions[formationPosIndex].getEntityID());
   if(!entity)
      return(cOriginVector);
   
   BVector force = cOriginVector;

   // Radius.
   //float radius = entity->getObstructionRadius();
   // jce [5/23/2008] -- try using the hypotenuse of their rectangle (hopefully really a square) obstruction
   float radius = sqrtf(entity->getObstructionRadiusX()*entity->getObstructionRadiusX() + entity->getObstructionRadiusZ()*entity->getObstructionRadiusZ());

   // DLM 6/12/08
   // Added "HeroFactor" as an additional push off distance to use for guys we want to be "special" in the formation.    
   float heroFactor = 0.0f;
   
   BVector worldPos = position+formationPos;
   
   BVector obstructionForce = cOriginVector;
   float obstructionFactor = 0.0f;
   if(entity->isClassType(BEntity::cClassTypeSquad))
   {
      BSquad *squad = (BSquad*)entity;

      // Is this guy a hero?  If so, give him the houdini number Hero Factor.
      BUnit *pLeader = squad->getLeaderUnit();
      if (pLeader)
      {
         if (pLeader->getProtoObject() && pLeader->getProtoObject()->getGotoType() == cGotoTypeHero)
            heroFactor = 8.0f;
      }
      
      float searchDist = cSearchFactor*squad->getPathingRadius();
      
      // Sanity check that we don't have zero radius.
      if(searchDist < 0.1f)
         searchDist = 0.1f;
      
      if (mGetObstructions)
      {
         gObsManager.findObstructions(worldPos, false, false, mComputeForceObstructions);
         mGetObstructions = false;
      }
      
      float worstOverlapDist = 0.0f;
      for(long obIndex=0; obIndex<mComputeForceObstructions.getNumber(); obIndex++)
      {
         // Get the obstruction node.
         BOPObstructionNode *ob = mComputeForceObstructions[obIndex];
         if(!ob)
            continue;
            
         // jce [9/10/2008] -- Try ignoring moving things.
         if(ob->mObject && ob->mObject->isMoving())
            continue;
            
         const BOPQuadHull *hull = ob->getHull();
         if(!hull)
         {
            continue;
         }
          
         /*  
         BVector center;
         hull->computeCenter(center);
         
         // jce [5/7/2008] -- manually compute radius since mRadius is bogus on terrain obstructions
         float obDX = center.x - hull->mPoint[0].mX;
         float obDY = center.z - hull->mPoint[0].mZ;
         float obRadius = sqrtf(obDX*obDX + obDY*obDY);
         
         BVector dirToOther = center - worldPos;
         float distToOther = dirToOther.length();
         float overlapDist = searchDist + obRadius - distToOther;
         */
         
         float distToOther = hull->distance(worldPos);
         float overlapDist = searchDist - distToOther;

         if(overlapDist > worstOverlapDist)
            worstOverlapDist = overlapDist;
      }
      
      static float maxForce = 200.0f;
      //static float overlapDistForMax = 0.2f;
      float overlapDistForMax = searchDist*0.5f;
      //obstructionFactor = Math::fSmoothStep(0.0f, 1.0f, worstOverlapDist/overlapDistForMax);
      obstructionFactor = Math::Clamp(worstOverlapDist/overlapDistForMax, 0.0f, 1.0f);
      float obstructionMultiplier = Math::Lerp(0.0f, maxForce, obstructionFactor);

      // jce [5/14/2008] -- experiment: move towards a line segment through the center along the
      // direction of travel instead of towards a single point
      // jce [10/1/2008] -- spine point now calculated outside and passed in
      /*
      static float hackdist=50.0f;
      BVector segPt1 = hackdist * formationDir;
      BVector segPt2 = -hackdist * formationDir;
      BVector spinePoint = cOriginVector;
      closestPointOnSegment(position.x, position.z, segPt1.x, segPt1.z, segPt2.x, segPt2.z, spinePoint.x, spinePoint.z);
      */
      
      // Get distance/direction to center.
      BVector dirToSpinePoint = spinePoint-position;
      dirToSpinePoint.y = 0.0f;
      float distToSpinePoint = dirToSpinePoint.length();
      if(distToSpinePoint > cFloatCompareEpsilon)
      {
         dirToSpinePoint /= distToSpinePoint;
         obstructionForce = obstructionMultiplier*dirToSpinePoint;
      }
   }


   //////////////////////////////////////////////////////////////////////////
   // Move to center.
   BVector centerForce = cOriginVector;
   {

      // Get distance/direction to center.
      BVector dirToCenter = -position;
      dirToCenter.y = 0.0f;

      float distToCenter = dirToCenter.length();
      // Attract more the further away you are.
      float multiplier = Math::fSmoothStep(0.0f, 1.0f, distToCenter/7.5f);

      // Normalize.
      if(distToCenter > cFloatCompareEpsilon)
      {
         dirToCenter *= 1.0f/distToCenter;

         static float cMaxCenterForce = 50.0f;
         centerForce = multiplier * cMaxCenterForce * dirToCenter;
      }
   }

   // Interpolate move to spine vs. move to center based on obstruction factor.
   BVector spineCenterForce = Math::Lerp(centerForce, obstructionForce, obstructionFactor);
   force += spineCenterForce;


   // Now look at others for overlap.
   for(long j=0; j<mPositions.getNumber(); j++)
   {
      if(j == formationPosIndex)
         continue;
      
      // Get the entity.
      BEntity *otherEntity = gWorld->getEntity(mPositions[j].getEntityID());
      if(!otherEntity)
         continue;
      
      // Obstruction radius.   
      float otherRadius = otherEntity->getObstructionRadius();
      
      BVector dirToOther = mPositions[j].getOffset() - position;
      dirToOther.y = 0.0f;
      float distToOther = dirToOther.length();
      //float overlapDist = radius + otherRadius - distToOther;
      
      // Separate by combined radii plus some fudge factor
      //float maxDist = 4.0f + 1.25f*(mPositions[formationPosIndex].getEntityID().asLong()%5);
      //float maxDist = radius + otherRadius + 0.5f*(mPositions[formationPosIndex].getEntityID().asLong()%5);
      float maxDist = 0.5f*(radius + otherRadius) + 0.5f*(mPositions[formationPosIndex].getEntityID().asLong()%5);
      
      // Sanity check that both radii are not 0.
      if(maxDist < 0.1f)
         maxDist = 0.1f;
      
      float overlapForce = (maxDist + radius + otherRadius + heroFactor - distToOther)/maxDist;
      if(overlapForce<0.0f)
         continue;
      if(overlapForce>1.0f)
         overlapForce = 1.0f;
      
      if(distToOther > cFloatCompareEpsilon)
      {
         // Normalize.
         dirToOther /= distToOther;
      }
      else
      {
         // Guys are exactly on top of each other... so pick a random direction to push off by.
         // jce [5/22/2008] -- Maybe this is bogus and needs something smarter.
         dirToOther = cZAxisVector;
         dirToOther.rotateXZ(getRandRangeFloat(cSimRand, 0.0f, cTwoPi));
      }
      static float overlapFactor = 100.0f;
      
      force -= dirToOther*overlapFactor*overlapForce;
      
      // jce [8/26/2008] -- try tapering off the overlap force on us based on obstruction factor so that 
      // pushing off obstructions is prioritized and other positions will push away from us in theory.  
      // Didn't seem to work well -- maybe needs more magic tweaking.
      //force -= dirToOther*overlapFactor*overlapForce*(1.0f-obstructionFactor);
      // jce [9/8/2008] -- trying this at half magnitude, which seems to be more promising
      force -= dirToOther*overlapFactor*overlapForce*(1.0f-0.5f*obstructionFactor);
   }
   
   // Dampening.
   ///*
   static float cDamp = -10.0f;
   BVector dampenForce = cDamp*velocity;
   force += dampenForce;
   //*/

   /*
   float forceMag = force.length();
   const float maxForce = 50.0f;
   if(forceMag > maxForce)
      force *= maxForce/forceMag;
   */
   
   return(force);
}


//==============================================================================
//==============================================================================
void BFormation2::update(BMatrix matrix, const BPath *path /*=NULL*/)
{
   // jce [4/2/2008] -- UBER HACK to adjust formation positions on the fly for mobs.  Basically they
   // try to move towards the center and move off of each other.
   // jce [5/2/2008] -- hacked hack into Phx
   if(mType == eTypeMob && mPositions.getNumber()>1)
   {
      // Update reassign timer.
      DWORD elapsed = gWorld->getLastUpdateLength();
      if(elapsed >= mReassignTimer)
      {
         /*
         // Timer is up, so we check our position error and see if it changed enough from last time for us to 
         // reassign.  This is done to prevent excessive reassignments which can look goofy.
         
         // Get the current error metric.
         float posErrorSqr = computePositionErrorSqr();
         float delta = posErrorSqr-mPositionErrorAtLastAssignmentSqr;
         const float cDeltaPerEntity = 2.0f;
         float threshold = mPositions.getNumber()*cDeltaPerEntity;
         threshold *= threshold;  // error metric is distSqr for faster computation
         if(delta > threshold)
         */
            mReassignNeeded = true;
            
         // Whether we chose to reassign or not, we reset the timer.
         mReassignTimer = cMobReassignTimeout;
      }
      else
      {
         // Just tick off some time from the timer.
         mReassignTimer -= elapsed;
      }
      
      BVector force = cOriginVector;

      BVector formationPos;
      matrix.getTranslation(formationPos);

      BASSERT(!_isnan(formationPos.x));
      BASSERT(!_isnan(formationPos.y));
      BASSERT(!_isnan(formationPos.z));

      BVector formationDir;
      matrix.getForward(formationDir);

      BASSERT(!_isnan(formationDir.x));
      BASSERT(!_isnan(formationDir.y));
      BASSERT(!_isnan(formationDir.z));
      
      // Sanity check the direction.  It really should never be non-normalized and it
      // certainly should never be zero.  Zero = everything else explodes, so we set it to an
      // arbitrary valid vector (z axis) in that case.
      if(!formationDir.safeNormalize())
      {
         #ifdef _MOVE4
         BFAIL("Bogus formation dir");
         #endif
         formationDir = cZAxisVector;
      }
      
      
      // Set up ignore list.
      // Also look for largest radius.
      float largestRadius = 0.0f;
      static BEntityIDArray ignoreList;
      ignoreList.resize(0);
      for(long i=0; i<mPositions.getNumber(); i++)
      {
         // Get entity for this position
         BEntityID id = mPositions[i].getEntityID();
         BEntity *entity = gWorld->getEntity(id);
         if(!entity)
            continue;
         
         // Update largest radius.
         float radius = entity->getObstructionRadius();
         if(radius > largestRadius)
            largestRadius = radius;

         // Add it.
         ignoreList.add(id);
         
         // If a squad, add children.
         if(entity->isClassType(BEntity::cClassTypeSquad))
         {
            BSquad *squad = (BSquad*)entity;
            for(uint j=0; j<squad->getNumberChildren(); j++)
            {
               ignoreList.add(squad->getChild(j));
            }
         }
      }
      
      // jce [9/30/2008] -- Bullet-proof against having a formation full of null entities.  Not sure how or why this can happen but
      // that's an upstream problem we'll have to figure out.
      if(ignoreList.getNumber() <= 0)
      {
         BFAIL("Formation full of null entities.");
         return;
      }
      
      // jce [9/23/2008] -- Begin obstructionmanager for all guys using the 0th one.  Formation should not consist of a mix of crazy movement 
      // types or this will break.  Lots of other stuff wouldn't work right in that case anyway.

      // Get 0th entity in ignore list to use as our example for obmgr::begin
      BEntity *entity = gWorld->getEntity(ignoreList[0]);
      BASSERT(entity);     // jce [9/23/2008] -- given logic above this "can't" be null
      
      // Set up quadtrees.  A litte messy, yes.
      long quadtrees=BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeAllCollidableSquads;
      if(entity->isClassType(BEntity::cClassTypeSquad))
      {
         BSquad *squad = (BSquad*)entity;
         
         // Get the movement type.
         // jce [9/24/2008] -- If there is no protoobject, just assume land which is what the squad code does.  Not sure
         // the is correct though.
         long movementType;
         const BProtoObject *protoObject = squad->getProtoObject();
         if(protoObject)
            movementType = protoObject->getMovementType();
         else
            movementType = cMovementTypeLand;
            
         switch(movementType)
         {
            case cMovementTypeAir:
               quadtrees = BObstructionManager::cIsNewTypeBlockAirMovement;
               break;
            case cMovementTypeFlood:
               quadtrees |= BObstructionManager::cIsNewTypeBlockFloodUnits;
               break;
               
            case cMovementTypeScarab:
               quadtrees |= BObstructionManager::cIsNewTypeBlockScarabUnits;
               break;
               
            default:
               quadtrees |= BObstructionManager::cIsNewTypeBlockLandUnits;
         }
      }
      else if(entity->isClassType(BEntity::cClassTypeUnit))
      {
         // Not sure what to do with units... they shouldn't be in this formation anyway
      }
     
      // Do the begin.
      gObsManager.begin(BObstructionManager::cBeginNone, cSearchFactor*largestRadius, quadtrees, BObstructionManager::cObsNodeTypeAll, entity->getPlayerID(), 
         cDefaultRadiusSofteningFactor, &ignoreList, false);
      

      // Break timestep down into smaller increments to keep integration from going flaky.
      
      //const float cInterval = 0.025f;
      //static bool rk4 = false;

      const float cInterval = 0.1f;
      static bool rk4 = true;
      
      float remainingTime = gWorld->getLastUpdateLengthFloat();
      while(remainingTime > cFloatCompareEpsilon)
      {
         float stepTime = min(remainingTime, cInterval);
         
         // Run through each spot.
         for(long i=0; i<mPositions.getNumber(); i++)
         {
            // Compute spine point.
            BVector spinePoint;
            if(!path || path->getNumberWaypoints() < 2)
            {
               // If there is no path (or a bogus one), the find a spot along the direction of travel.
               static float hackdist=50.0f;
               BVector segPt1 = hackdist * formationDir;
               BVector segPt2 = -hackdist * formationDir;
               spinePoint = cOriginVector;
               closestPointOnSegment(mPositions[i].getOffset().x, mPositions[i].getOffset().z, segPt1.x, segPt1.z, segPt2.x, segPt2.z, spinePoint.x, spinePoint.z);
            }
            else
            {
               // Otherwise, find the closest point on the path 
               
               // Get translate into world space (no rotations for mob)
               BVector worldPos = mPositions[i].getOffset()+formationPos;
               
               // Get closest point on path passed in.
               spinePoint = path->findClosestPoint(worldPos);
               
               // Translate back into local space.
               spinePoint -= formationPos;
            }
            
            if(!rk4)
            {
               // jce [5/21/2008] -- Normal Euler integration.  This is faster but requires smaller step sizes and can
               // blow up easily with high forces (especially with high damping force)
               BVector force = computeFlockingForce(i, mPositions[i].getOffset(), mPositions[i].getFlockVelocity(), ignoreList, formationPos, formationDir, spinePoint);
    
               // Update velocity based on acceleration (based on force)
               //float recipMass = 1.0f/0.25f;
               //BVector acceleration = force*recipMass;
               BVector acceleration = force;
               
               mPositions[i].setFlockVelocity(mPositions[i].getFlockVelocity() + stepTime*acceleration);

               // Update position based on velocity.
               BVector newOffset = mPositions[i].getOffset() + stepTime*mPositions[i].getFlockVelocity();
               mPositions[i].setOffset(newOffset);
            }
            else
            {
               // jce [5/21/2008] -- Runge-Kutta integrator (RK4).  A lot more math than the Euler but is more stable,
               // especially with high damping.  Longer time steps work better as well.
               resetComputeForceObstructions();
               const float cOneSixth = 1.0f/6.0f;
               BVector pos = mPositions[i].getOffset();
               BVector k1Velocity = mPositions[i].getFlockVelocity();
               BVector k1Accel = computeFlockingForce(i, pos, k1Velocity, ignoreList, formationPos, formationDir, spinePoint);
               BVector k2Velocity = k1Velocity + k1Accel*stepTime*0.5f;
               BVector k2Accel = computeFlockingForce(i, pos + k1Velocity*stepTime*0.5f, k2Velocity, ignoreList, formationPos, formationDir, spinePoint);
	            BVector k3Velocity = k1Velocity + k2Accel*stepTime*0.5f;
	            BVector k3Accel = computeFlockingForce(i, pos + k2Velocity*stepTime*0.5f, k3Velocity, ignoreList, formationPos, formationDir, spinePoint);
               BVector k4Velocity = k1Velocity + k3Accel*stepTime;
	            BVector k4Accel = computeFlockingForce(i, pos + k3Velocity*stepTime, k4Velocity, ignoreList, formationPos, formationDir, spinePoint);
               mPositions[i].setFlockVelocity(k1Velocity + (k1Accel + k2Accel*2.0f + k3Accel*2.0f + k4Accel)*stepTime*cOneSixth);
	            mPositions[i].setOffset(pos + (k1Velocity + k2Velocity*2.0f + k3Velocity*2.0f + k4Velocity)*stepTime*cOneSixth);

               BASSERT(!_isnan(mPositions[i].getFlockVelocity().x));
               BASSERT(!_isnan(mPositions[i].getFlockVelocity().y));
               BASSERT(!_isnan(mPositions[i].getFlockVelocity().z));
               BASSERT(!_isnan(mPositions[i].getOffset().x));
               BASSERT(!_isnan(mPositions[i].getOffset().y));
               BASSERT(!_isnan(mPositions[i].getOffset().z));
            }
         }
         
         remainingTime -= stepTime;
      }

      // Done with obstruction manager.
      gObsManager.end();
   }
   

   // Transform.
   transform(matrix);
}

//==============================================================================
//==============================================================================
void BFormation2::transform(BMatrix matrix)
{
   #ifdef DEBUGASSIGNMENT
   if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
      mpOwner->debug("Form2::transformFormation: OwnerID=%s.", mpOwner->getID().getDebugString().getPtr());
   #endif
   //If we need to re-make the formation, do it.
   if (mMakeNeeded)
   {
      #ifdef DEBUGASSIGNMENT
      if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
         mpOwner->debug("  Make needed.");
      #endif
      makeFormation();
      mPrevForward = cInvalidVector;
   }

   //No Rotate check.
   bool noRotate=false;
   //if ((mType == eTypeFlock) || (mType == eTypeGaggle))
   if ((mType == eTypeFlock) || (mType == eTypeMob))
      noRotate=true;

   //Invert the formation offsets for extreme direction changes, but only at the
   //squad level.  Platoons put squads in certain spots that need to be maintained,
   //even across direction changes given the hetero nature of a platoon.
   if (!mPrevForward.almostEqual(cInvalidVector) && !noRotate)
   {
      BVector newForward;
      matrix.getForward(newForward);
      float angleDiff = newForward.angleBetweenVector(mPrevForward);
      if (abs(angleDiff - cPi) < cPiOver2)
      {
         if (mpOwner->getClassType() == BEntity::cClassTypeSquad)
         {
            #ifdef DEBUGASSIGNMENT
            mpOwner->debug("  Squad: inverting offsets.");
            #endif
            //Invert it.
            scale(-1.0f);
            //Reprioritize the positions.
            //TODO: This could make more assumptions and just invert the priority w/o
            //recalculating it, but it's quicker for now to just to do this and this
            //isn't really going to be called that much.
            prioritizePositions();
         }
         else
         {
            #ifdef DEBUGASSIGNMENT
            if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
               mpOwner->debug("  Platoon: setting reassignNeeded to true.");
            #endif
            mReassignNeeded=true;
         }
      }
      #ifdef DEBUGASSIGNMENT
      else if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
         mpOwner->debug("  No inversion needed.");
      #endif
   }
   #ifdef DEBUGASSIGNMENT
   else if (mpOwner->getClassType() == BEntity::cClassTypePlatoon)
      mpOwner->debug("  Skipping inversion check.");
   #endif
   matrix.getForward(mPrevForward);

   //If we're not rotating, strip the orientation now that we've gotten the forward
   //out of the matrix.
   if (noRotate)
      matrix.clearOrientation();

   //Simply go through and transform the position offsets into real positions.
   BVector position;
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      if (mPositions[i].getTransform())
      {
         matrix.transformVectorAsPoint(mPositions[i].getOffset(), position);
         mPositions[i].setPosition(position);
      }
   }
   
   #ifdef _MOVE4
   // jce [5/13/2008] -- Keep redoing priorities on mob as we move around.  Could be too slow in which case
   // we'll have to relocate this and make it smarter.
   if (mType == eTypeMob)
      prioritizePositions();
   #endif

   //Reassign the children as needed.
   if (mReassignNeeded)
   {
      switch (mType)
      {
         case eTypePlatoon:
            reassignPlatoonChildren();
            break;
         #ifndef _MOVE4
         case eTypeMob:
            // No reassignment
            break;
         #endif
         default:
            reassignChildren();
            break;
      }
   }
}

//==============================================================================
//==============================================================================
const BFormationPosition2* BFormation2::getFormationPosition(uint index) const
{
   if (index >= mPositions.getSize())
      return(NULL);
   return(&mPositions[index]);
}

//==============================================================================
//==============================================================================
const BFormationPosition2* BFormation2::getFormationPosition(BEntityID childID) const
{
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      if (mPositions[i].getEntityID() == childID)
         return(&mPositions[i]);
   }
   return(NULL);
}

//==============================================================================
//==============================================================================
BVector BFormation2::getTransformedFormationPositionFast(BEntityID childID, BVector formationPos, BVector formationForward, BVector formationRight) const
{
   // Get the child formation position.  If not found return the formation's position.
   int childIndex = -1;
   for (uint i = 0; i < mPositions.getSize(); i++)
   {
      if (mPositions[i].getEntityID() == childID)
      {
         childIndex = i;
         break;
      }
   }
   if (childIndex == -1)
      return formationPos;

   BMatrix worldMatrix;      

   // FTLOG why isn't this just a flag on the formation by now?
   bool noRotate=false;
   if ((mType == eTypeFlock) || (mType == eTypeMob))
      noRotate=true;
   if (!noRotate)
   {
      BVector up;
      up.assignCrossProduct(formationForward, formationRight);

      worldMatrix.makeOrient(formationForward, up, formationRight);  
   }
   else
      worldMatrix.clearOrientation();

   worldMatrix.setTranslation(formationPos);

   // Transform child position
   BVector newPosition;
   worldMatrix.transformVectorAsPoint(mPositions[childIndex].getOffset(), newPosition);
   return newPosition;
}

//==============================================================================
//==============================================================================
void BFormation2::calculateDynamicPriorities(BVector forward)
{
   BDEBUG_ASSERT(mpOwner->getClassType() == BEntity::cClassTypePlatoon);
   //BDEBUG_ASSERT(mType == eTypeMob);

   mDynamicPrioritiesValid = false;

   // Rotate the formation positions so the forward vector is aligned with the Z axis.  Then
   // comparison of the Z values can be used to sort them.
   BVector right;
   right.assignCrossProduct(cYAxisVector, forward);
   right.normalize();
   BMatrix matrix;
   matrix.makeInverseOrient(forward, cYAxisVector, right);

   static BFormationPriorityEntryArray squads;
   squads.resize(0);
   BVector position;
   for (uint i=0; i < mPositions.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 4734
      const BSquad* pSquad = gWorld->getSquad(mPositions[i].getEntityID());
//--
      if (!pSquad)
         continue;
      squads.grow();
      int squadIndex = squads.getSize() - 1;
      squads[squadIndex].setEntityID(pSquad->getID());
      
      matrix.transformVectorAsPoint(mPositions[i].getOffset(), position);
      squads[squadIndex].setPercentComplete(position.z);
   }

   // Sort them by who's farthest towards the front of the formation
   uint nextPriority = 0;
   while (squads.getSize() > 0)
   {
      BEntityID bestID = cInvalidObjectID;
      float bestPercent = -FLT_MAX;
      uint bestIndex=0;
      for (uint i=0; i < squads.getSize(); i++)
      {
         if (bestPercent < squads[i].getPercentComplete())
         {
            bestID = squads[i].getEntityID();
            bestPercent = squads[i].getPercentComplete();
            bestIndex = i;
         }
      }
      
      // If we didn't get anything
      if (bestID == cInvalidObjectID)
         break;
      
      setDynamicPriority(bestID, nextPriority);
      nextPriority++;
      squads.removeIndex(bestIndex);
   }

   // If we've given *everyone* a dynamic priority, then we're good on those.
   if (squads.getSize() == 0)
      mDynamicPrioritiesValid = true;
}

//==============================================================================
//==============================================================================
void BFormation2::debugRender() const
{
   BMatrix matrix;
   matrix.makeIdentity();
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      BEntityID nEntityID = mPositions[i].getEntityID();
//-- FIXING PREFIX BUG ID 4735
      const BEntity* pEntity=gWorld->getEntity(nEntityID);
//--
      if (!pEntity)
         continue;
   
      // Get position, but placed at terrain height.
      BVector pos = mPositions[i].getPosition();
      gTerrainSimRep.getHeight(pos, true);
      
      // Fudge it above the terrain a bit.
      pos.y += 1.0f;

      matrix.setTranslation(pos);

      if (pEntity->getID().getType() == BEntity::cClassTypeSquad)
      {
         // Draw obstruction boxes
         if (mType == eTypePlatoon)
         {
            BVector offsetPts[4];
            BVector worldPts[4];
            offsetPts[0].set(pEntity->getObstructionRadiusX() * -1.0f, 0.0f, pEntity->getObstructionRadiusZ());
            offsetPts[1].set(pEntity->getObstructionRadiusX(),         0.0f, pEntity->getObstructionRadiusZ());
            offsetPts[2].set(pEntity->getObstructionRadiusX(),         0.0f, pEntity->getObstructionRadiusZ() * -1.0f);
            offsetPts[3].set(pEntity->getObstructionRadiusX() * -1.0f, 0.0f, pEntity->getObstructionRadiusZ() * -1.0f);

            BMatrix matrixWithOrient;
            BVector right;
            right.assignCrossProduct(cYAxisVector, mPrevForward);
            matrixWithOrient.makeOrient(mPrevForward, cYAxisVector, right);  
            matrixWithOrient.setTranslation(pos);
            
            matrixWithOrient.transformVectorListAsPoint(offsetPts, worldPts, 4);

            if (!pEntity->isMoving())
               gTerrainSimRep.addDebugSquareOverTerrain(worldPts[0], worldPts[1], worldPts[2], worldPts[3], cDWORDDarkOrange, 0.5f);
            else
               gTerrainSimRep.addDebugThickSquareOverTerrain(worldPts[0], worldPts[1], worldPts[2], worldPts[3], 0.1f, cDWORDOrange, 0.5f);
         }
         // Draw obstruction circles
         else
         {
            if (!pEntity->isMoving())
               gpDebugPrimitives->addDebugCircle(matrix, pEntity->getObstructionRadius(), cDWORDDarkOrange);
            else
               gpDebugPrimitives->addDebugThickCircle(matrix, pEntity->getObstructionRadius(), 0.1f, cDWORDOrange);
         }

         // Display priority of child
         if (gConfig.isDefined(cConfigDebugPlatoonFormations))
         {
            uint priority = getChildPriority(nEntityID);
            BSimString childIndexAsString;
            childIndexAsString.format("%d", priority);
            BSimString buffer;
            gpDebugPrimitives->addDebugText(childIndexAsString.asANSI(buffer), pEntity->getPosition(), 1.0f, cDWORDYellow);
            // jce [5/13/2008] -- also draw priority at formation position itself
            gpDebugPrimitives->addDebugText(childIndexAsString.asANSI(buffer), pos, 0.5f, cDWORDBlue);
         }
#ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigRenderLineToFormationPos))
         {
            gTerrainSimRep.addDebugLineOverTerrain(pEntity->getPosition(), pos, cDWORDBlack, cDWORDWhite, 0.5f);
         }
#endif
      }
      else
      {
         if (!pEntity->isMoving())
            gpDebugPrimitives->addDebugCircle(matrix, pEntity->getObstructionRadius(), cDWORDPurple);
         else
            gpDebugPrimitives->addDebugThickCircle(matrix, pEntity->getObstructionRadius(), 0.1f, cDWORDMagenta);
      }
   }
}

//==============================================================================
//==============================================================================
void BFormation2::prioritizePositions()
{
   #ifdef _MOVE4
   if(mType == eTypeMob)
   {
      BVector dir = mpOwner->getForward();
      
      // Mob formation doesn't rotate, so we are going to sort positions based on direction of movement.
      static BDynamicSimArray<float> signedDistances;
      signedDistances.resize(mPositions.getSize());
      static BDynamicSimArray<uint> positionIndices;
      positionIndices.resize(mPositions.getSize());
      for(uint i=0; i<mPositions.getSize(); i++)
      {
         float x = mPositions[i].getOffset().x;
         float z = mPositions[i].getOffset().z;
         signedDistances[i] = dir.x*x + dir.z*z;
         
         //mPositions[i].setPriority(i);
         positionIndices[i] = i;
      }
      
      // Sort
      for(uint i=0; i<mPositions.getSize(); i++)
      {
         for(uint j=i+1; j<mPositions.getSize(); j++)
         {
            if(signedDistances[i] < signedDistances[j])
            {
               signedDistances.swapIndices(i, j);
               positionIndices.swapIndices(i, j);
               //uint iPriority = mPositions[i].getPriority();
               //uint jPriority = mPositions[j].getPriority();
               //mPositions[i].setPriority(jPriority);
               //mPositions[j].setPriority(iPriority);
            }
         }
      }
      
      for(uint i=0; i<mPositions.getSize(); i++)
      {
         mPositions[positionIndices[i]].setPriority(i);
      }
   }
   else
   #endif
   {
      //DUMB AND SLOW for now.
      for (uint i=0; i < mPositions.getSize(); i++)
         mPositions[i].setPriority(BFormationPosition2::cInvalidPriority);

      for (uint i=0; i < mPositions.getSize(); i++)
      {
         bool bestValid=false;
         uint bestIndex=0;
         float bestZ=0.0f;
         
         for (uint j=0; j < mPositions.getSize(); j++)
         {
            if (mPositions[j].getPriority() != BFormationPosition2::cInvalidPriority)
               continue;
            if (!bestValid || (mPositions[j].getOffset().z > bestZ))
            {
               bestValid=true;
               bestIndex=j;
               bestZ=mPositions[j].getOffset().z;
            }
         }
         
         if (bestValid)
            mPositions[bestIndex].setPriority(i);
         else
            break;
      }
   }
}


//==============================================================================
//==============================================================================
long findBestSlot(BEntityID entityID, float entityPosX, float entityPosZ, const BFormationPosition2Array &positions, const BDynamicSimArray<bool> usedSlots, float &outBestSlotDistanceSqr)
{
   BASSERT(positions.getNumber() == usedSlots.getNumber());

   // No best slot found yet.
   long bestSlotIndex = -1;
   outBestSlotDistanceSqr = cMaximumFloat;
      
   // Check each slot.
   for(long i=0; i<positions.getNumber(); i++)
   {
      // If it's used, skip it.
      if(usedSlots[i])
         continue;
         
      // Compute xz distance.  This is done point to point because that seems most relevant here (units try to align their
      // centers with the positions not just get "in range" of them.
      float dx = entityPosX - positions[i].getPosition().x;
      float dz = entityPosZ - positions[i].getPosition().z;
      float distSqr = dx*dx + dz*dz;
      
      // Slightly prefer the slot we're already in.
      if(positions[i].getEntityID() == entityID)
      {
         static float cCurrentSlotMultiplier = 0.8f;
         distSqr *= cCurrentSlotMultiplier;
      }
      
      // If it's the best one yet, remember it.
      if(distSqr < outBestSlotDistanceSqr)
      {
         bestSlotIndex = i;
         outBestSlotDistanceSqr = distSqr;
      }
   }
   
   // Return index.
   return(bestSlotIndex);
}


//==============================================================================
//==============================================================================
void BFormation2::reassignChildren()
{
   // jce [5/8/2008] -- Converted/merged age3 version of this which does a way better job of
   // assigning positions by looking at the overall picture instead of just letting guys grab the
   // closest remaining position.
   
   // Mark that we've done the reassign.
   mReassignNeeded=false;
   mDynamicPrioritiesValid=false;
   BASSERT(mPositions.getSize() == mChildren.getSize());

   uint numPositions = mPositions.getNumber();
   if(numPositions <= 0)
      return;


   //Scratch space.
   BDynamicSimArray<bool> usedSlots(numPositions);

//#define TESTNEWREASSIGNER
#define TESTNEWREASSIGNER2
#ifdef TESTNEWREASSIGNER
   // jce [8/26/2008] -- Another attempt: find the unit whose best choice is the furthest away and let him pick first.
   // Then the next worst guy picks from what's left and so on.

   // Assemble a list of pointers we will be pulling items out of.   
   BDynamicSimArray<BEntity *> entitiesToAssign;
   for(uint i=0; i<numPositions; i++)
   {  
      // Get the entity.
      BEntity *entity = gWorld->getEntity(mChildren[i]);
      if(!entity)
      {
         BFAIL("Null entity in formation list");
         continue;
      }
      
      // Put pointer in the list.
      entitiesToAssign.add(entity);
   }

   
   // While there are still guys left to assign...
   while(entitiesToAssign.getNumber())
   {
      // Find whose best slot is the furthest away.
      long worstIndexInEntityArray = -1;
      long worstSlotIndex = -1;
      float worstDistanceSqr = -cMaximumFloat;
      for(long i=0; i<entitiesToAssign.getNumber(); i++)
      {
         // Get entity.
         BEntity *entity = entitiesToAssign[i];
         
         // Find our best slot (of the remaining ones)
         float bestSlotDistanceSqr;
         long bestSlotIndex = findBestSlot(entity->getID(), entity->getPosition().x, entity->getPosition().z, mPositions, usedSlots, bestSlotDistanceSqr);
         
         // Are we the worst one so far?
         if(bestSlotDistanceSqr > worstDistanceSqr)
         {
            // Remember that we have the worst best slot.
            worstDistanceSqr = bestSlotDistanceSqr;
            worstSlotIndex = bestSlotIndex;
            worstIndexInEntityArray = i;
         }
      }
      
      // The worst-off guy now gets his slot.
      if(worstSlotIndex < 0)
      {
         BFAIL("Somehow couldn't get a slot index");
         return;
      }
      mPositions[worstSlotIndex].setEntityID(entitiesToAssign[worstIndexInEntityArray]->getID());
      usedSlots[worstSlotIndex] = true;
      
      // We don't need to consider him any more.
      entitiesToAssign.removeIndex(worstIndexInEntityArray, false);
   }
#else

#ifdef TESTNEWREASSIGNER2
   // jce [9/25/2008] -- Yet another variation.  Let each position (in priority order) pick it's favorite entity.

   // Assemble a list of pointers we will be pulling items out of.   
   BDynamicSimArray<BEntity *> entitiesToAssign;
   for(uint i=0; i<numPositions; i++)
   {  
      // Get the entity.
      BEntity *entity = gWorld->getEntity(mChildren[i]);
      if(!entity)
      {
         BFAIL("Null entity in formation list");
         continue;
      }
      
      // Put pointer in the list.
      entitiesToAssign.add(entity);
   }
   
   // Get a sorted list of indices
   BDynamicSimArray<uint> positionIndices(mPositions.getSize());
   for(uint i=0; i<mPositions.getSize(); i++)
   {
      positionIndices[i] = i;
   }
   for(uint i=0; i<mPositions.getSize(); i++)
   {
      for(uint j=i+1; j<mPositions.getSize(); j++)
      {
         if(mPositions[positionIndices[i]].getPriority() > mPositions[positionIndices[j]].getPriority())
         {
            positionIndices.swapIndices(i, j);
         }
      }
   }

   // Walk each position in priority order.
   for(uint i=0; i<numPositions; i++)
   {
      // Get position.
      BFormationPosition2 &pos = mPositions[positionIndices[i]];
      
      // Find the closest child.
      long bestIndex = -1;
      float bestDistSqr = cMaximumFloat;
      for(long j=0; j<entitiesToAssign.getNumber(); j++)
      {
         // Get distance from entity to position.
         float thisDistSqr = entitiesToAssign[j]->getPosition().xzDistanceSqr(pos.getPosition());
         
         // If it's the best choice so far, save it off.
         if(thisDistSqr < bestDistSqr)
         {
            bestIndex = j;
            bestDistSqr = thisDistSqr;
         }
         
      }
      
      // Assign to this spot.
      pos.setEntityID(entitiesToAssign[bestIndex]->getID());

      // We don't need to consider him any more.
      entitiesToAssign.removeIndex(bestIndex, false);
   }
   
      
#else

   
   //This is slow.  It's currently doing a lot to see what the upper limit of
   //quality is.

   //Calculate the distance between each entity and each formation position.  Formation
   //positions with non-matching types get a MaxFloat distance.
   BDynamicArray2D<float> distanceEntityToSlot(numPositions, numPositions);
   distanceEntityToSlot.setAll(cMaximumFloat);
   
   BDynamicSimArray<BEntity *> entitiesToAssign;

   for(uint i=0; i < numPositions; i++)
   {
      BEntity* entity = gWorld->getEntity(mChildren[i]);
      if (entity == NULL)
         continue;
         
      // Remember pointer in a list.
      entitiesToAssign.add(entity);

      BVector entityPos = entity->getPosition();
      
      for (uint j=0; j < numPositions; j++)
      {
         // jce [5/8/2008] -- this used to have some logic for categories/types/etc but since I cant' find the HW equivalents of that, I'm
         // simplifying for now.  For mismatches between slot/entity it was just setting distance to cMaximumFloat.
         
         // Compute xz distance.  I switched this to being point to point because that seems most relevant here (units try to align their
         // centers with the positions not just get "in range" of them.
         float dx = entityPos.x - mPositions[j].getPosition().x;
         float dz = entityPos.z - mPositions[j].getPosition().z;
         float dist = sqrtf(dx*dx + dz*dz);
         
         // Save off distance to this slot.
         distanceEntityToSlot(i, j) = dist;
      }
   }
   
   //Clear the unit/slot matrix.
   BDynamicArray2D<int> sortedSlotsByEntity(numPositions, numPositions);
   sortedSlotsByEntity.setAll(-1);

   //Go back through the units and build up the list sorted slots.  We rank each position
   //in this simple insertion (re)sort.
   for (i=0; i < numPositions; i++)
   {
      // Clear used slots list.
      usedSlots.setAll(false);

      for (uint j=0; j < numPositions; j++)
      {
         long bestIndex=-1;
         for (uint k=0; k < numPositions; k++)
         {
            if (usedSlots[k] == true)
               continue;
            if ((bestIndex == -1) || (distanceEntityToSlot(i, k) < distanceEntityToSlot(i,bestIndex)))
               bestIndex=k;
         }

         if (bestIndex == -1)
            break;
         sortedSlotsByEntity(i, j)=bestIndex;
         usedSlots[bestIndex]=true;
      }
   }

   //Setup the best unit/slot assignment tracking vars.
   float bestTotalDistance=cMaximumFloat;
   float bestMeanDistance=cMaximumFloat;
   float bestStandardDeviation=cMaximumFloat;
   BDynamicSimArray<int> bestUnits(numPositions);
   bestUnits.setAll(-1);
   BDynamicSimArray<int> bestSlots(numPositions);
   bestSlots.setAll(-1);
   
   // jce [5/21/2008] -- So far, no slots have been assigned.  This is a safety check used to make sure
   // we don't try to go off the array or anything in the case of no slots ever being assigned.  I think this
   // can only occur if input positions are something like NANs.
   bool slotsAssigned = false;

   //Do some trial runs.
   for (uint i=0; i < numPositions; i++)
   {
      BDynamicSimArray<int> tempUnits(numPositions);
      tempUnits.setAll(-1);
      BDynamicSimArray<int> tempSlots(numPositions);
      tempSlots.setAll(-1);

      usedSlots.setAll(false);
      float tempTotalDistance=0.0f;
      long currentUnitIndex=i;
      for (uint j=0; j < numPositions; j++)
      {
         long index=0;
         while (usedSlots[sortedSlotsByEntity(currentUnitIndex,index)] == true)
            index++;
         tempUnits[j]=currentUnitIndex;
         tempSlots[j]=sortedSlotsByEntity(currentUnitIndex,index);
         usedSlots[sortedSlotsByEntity(currentUnitIndex, index)]=true;

         //Add in the distance.
         tempTotalDistance+=distanceEntityToSlot(currentUnitIndex, sortedSlotsByEntity(currentUnitIndex, index));

         currentUnitIndex++;
         if (currentUnitIndex >= (int)numPositions)
            currentUnitIndex=0;
      }

      //Run some calcs.
      float tempMeanDistance=tempTotalDistance/(float)numPositions;
      //Calculate the standard deviation.
      float tempStandardDeviation=0.0f;
      for (uint j=0; j < numPositions; j++)
      {
         float foo=distanceEntityToSlot(tempUnits[j], tempSlots[j])-tempMeanDistance;
         foo*=foo;
         foo=(float)sqrt(foo);
         tempStandardDeviation+=foo;
      }
      tempStandardDeviation/=(float)numPositions;

      if (tempStandardDeviation < bestStandardDeviation)
      {
         #ifdef DEBUGASSIGNMENT
         mpOwner->debug("    BEST so far.");
         #endif
         bestTotalDistance=tempTotalDistance;
         bestMeanDistance=tempMeanDistance;
         bestStandardDeviation=tempStandardDeviation;
         
         for(uint k=0; k<numPositions; k++)
         {
            bestUnits[k] = tempUnits[k];
            bestSlots[k] = tempSlots[k];
         }
         
         // Remember that we assigned slots.
         slotsAssigned = true;
      }
      #ifdef DEBUGASSIGNMENT
      else
         mpOwner->debug("    NOT best so far.");
      #endif
   }

   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("  Best UA has a total distance of %f, mean distance of %f.", bestTotalDistance, bestMeanDistance);
   #endif
   
   // Put the children in the right slots.
   if(slotsAssigned)
   {
      // Use the best slot setup we found.
      for (uint i=0; i < numPositions; i++)
         mPositions[bestSlots[i]].setEntityID(mChildren[bestUnits[i]]);
   }
   else
   {
      // jce [5/21/2008] -- as described in an earlier comment, this should never really happen but it's here to protect against trying to write
      // into bogus array locations in the case of bad input data.
      for (uint i=0; i < numPositions; i++)
         mPositions[i].setEntityID(mChildren[i]);
         
      BFAIL("Assigning formation positions failed, probably have NANs for input positions somehow");
   }

#endif
#endif


/*


   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("Form2::reassignChildren: %d Children.", mChildren.getSize());
   #endif
   mReassignNeeded=false;
   mDynamicPrioritiesValid=false;
   
   //DUMB AND SLOW for now.
   BASSERT(mPositions.getSize() == mChildren.getSize());

   //Blank the entity assignment.
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      #ifdef DEBUGASSIGNMENT
      mpOwner->debug("  Pos[%d]:  Offset=(%5.2f, %5.2f), Pos=(%5.2f, %5.2f), OldEID=%d, Pri=%d.",
         i, mPositions[i].getOffset().x, mPositions[i].getOffset().z,
         mPositions[i].getPosition().x, mPositions[i].getPosition().z,
         mPositions[i].getEntityID().asLong(), mPositions[i].getPriority());
      #endif
      mPositions[i].setEntityID(cInvalidObjectID);
      BASSERT(mPositions[i].getPriority() < mPositions.getSize());
   }
    
   BEntityIDArray children;
   children.setNumber(mChildren.getSize());
   for (uint i=0; i < mChildren.getSize(); i++)
      children[i]=mChildren[i];
   
   //Assume that we have valid priorities set.
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      for (uint j=0; j < mPositions.getSize(); j++)
      {
         if (mPositions[j].getPriority() == i)
         {
            #ifdef DEBUGASSIGNMENT
            mpOwner->debug("  Pos[%d]:  Pri=%d:", j, mPositions[j].getPriority());
            #endif
         
            BEntityID childID=findClosest(children, mPositions[j].getPosition());
            #ifdef DEBUGASSIGNMENT
            mpOwner->debug("    NewEID=%d.", childID.asLong());
            #endif
            BASSERT(childID != cInvalidObjectID);
            mPositions[j].setEntityID(childID);
            children.remove(childID);

            //BEntity* pEntity=gWorld->getEntity(childID);
            //gTerrainSimRep.addDebugLineOverTerrain(pEntity->getPosition(), mPositions[j].getPosition(), cDWORDBlue, cDWORDBlue, 1.5f, BDebugPrimitives::cCategoryPathing, 5.0f);
            break;
         }
      }
   }
   */
   
   // Now that we have new positions assigned, save off the current error amount.  This is
   // the total of the distances between each entity and it's assigned position.
//   mPositionErrorAtLastAssignmentSqr = computePositionErrorSqr();
}

//==============================================================================
//==============================================================================
void BFormation2::reassignPlatoonChildren()
{
   BASSERT(mType == BEntity::cClassTypePlatoon);
   BASSERT(mPositions.getSize() == mChildren.getSize());

   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("Form2::reassignPlatoonChildren: %d Children.", mChildren.getSize());
   #endif
   mReassignNeeded=false;
   mDynamicPrioritiesValid=false;

   // Must preserve the squad line assignments for platoon formations.
   // So gather all children and positions of the same line index and reassign them.
   uint lineIndex = 0;
   uint numChildrenReassigned = 0;
   while (numChildrenReassigned < mChildren.getSize())
   {
      // Find all children with current priority
      static BEntityIDArray children;
      children.resize(0);
      static BDynamicSimLongArray positionIndices;
      positionIndices.resize(0);
      for (uint i = 0; i < mPositions.getSize(); i++)
      {
         if (mPositions[i].getOriginalLineIndex() == lineIndex)
         {
            children.add(mPositions[i].getEntityID());
            positionIndices.add(i);
            mPositions[i].setEntityID(cInvalidObjectID);
         }
      }
      lineIndex++;

      // Sort positions in this line by descending z offset
      for (int i = 0; i < positionIndices.getNumber() - 1; i++)
      {
         for (int j = 0; j < positionIndices.getNumber() - 1 - i; j++)
         {
            if (mPositions[positionIndices[j]].getOffset().z < mPositions[positionIndices[j+1]].getOffset().z)
            {
               int temp = positionIndices[j];
               positionIndices[j] = positionIndices[j+1];
               positionIndices[j+1] = temp;
            }
         }
      }

      // Find best child for each position of this priority
      for (uint i = 0; i < positionIndices.getSize(); i++)
      {
         uint positionIndex = positionIndices[i];
         if (children.getSize() == 1)
         {
            mPositions[positionIndex].setEntityID(children[0]);
         }
         else
         {
            BEntityID childID = findClosest(children, mPositions[positionIndex].getPosition());
            BASSERT(childID != cInvalidObjectID);
            mPositions[positionIndex].setEntityID(childID);
            children.remove(childID);
         }

         numChildrenReassigned++;
      }
   }

   // Adjust the offsets to account for the new positions of the squads
   adjustPlatoonFormationOffsets();
}

//==============================================================================
//==============================================================================
void BFormation2::adjustPlatoonFormationOffsets()
{
   BASSERT(mType == BEntity::cClassTypePlatoon);
   BASSERT(mPositions.getSize() == mChildren.getSize());

   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("Form2::adjustPlatoonFormationOffsets: %d Children.", mChildren.getSize());
   #endif

   // Gather all children and positions of the same actual line index and adjust the offsets.
   uint lineIndex = 0;
   uint numChildrenAdjusted = 0;
   float currentLineDepth = 0.0f;
   float formationDepth = 0.0f;
   float formationWidth = 0.0f;
   while (numChildrenAdjusted < mChildren.getSize())
   {
      // Find all children with current line index
      static BDynamicSimLongArray positionIndices;
      positionIndices.resize(0);
      for (uint i = 0; i < mPositions.getSize(); i++)
      {
         if (mPositions[i].getActualLineIndex() == lineIndex)
         {
            positionIndices.add(i);
         }
      }

      // Sort positions in this line by x offset so the relative order of the line is known
      for (int i = 0; i < positionIndices.getNumber() - 1; i++)
      {
         for (int j = 0; j < positionIndices.getNumber() - 1 - i; j++)
         {
            if (mPositions[positionIndices[j]].getOffset().x > mPositions[positionIndices[j+1]].getOffset().x)
            {
               int temp = positionIndices[j];
               positionIndices[j] = positionIndices[j+1];
               positionIndices[j+1] = temp;
            }
         }
      }

      // Place along line
      float lineWidth = 0.0f;
      float maxSquadDepth = 0.0f;
      float currentX = 0.0f;
      for (uint i = 0; i < positionIndices.getSize(); i++)
      {
         uint positionIndex = positionIndices[i];
         BSquad *sq = gWorld->getSquad(mPositions[positionIndex].getEntityID());
         BASSERT(sq);

         float width = 0.0f;
         float depth = 0.0f;
         if (sq)
         {
            width = sq->getObstructionRadiusX();
            depth = sq->getObstructionRadiusZ();
         }

         float maxWidthDepth = Math::Max(width, depth);

         // TODO - Add back in rand z displacement.  Off for now because we need to know the max
         // squad depth beforehand so we don't offset past max depth.
         /*
         float randZ = mUseRandom ? getRandRangeFloat(cSimRand, -0.3f, 0.3f) : 0.0f;
         randZ *= depth;
         if (!addRand)
            randZ = 0.0f;
         */
         float randZ = 0.0f;


         mPositions[positionIndex].setOffset(BVector(currentX + width, 0.0f, currentLineDepth + randZ - depth));
         lineWidth = currentX + width * 2.0f; // keep track of line width

         // Advance current X past squad just placed + some random spacing
         currentX += (maxWidthDepth * 2.0f);
         float randX = mUseRandom ? getRandRangeFloat(cSimRand, 0.0f, 1.0f) : 0.0f;
         currentX += randX * width;

         // Get max squad depth for advancing to next line
         maxSquadDepth = Math::Max(maxSquadDepth, depth * 2.0f);
      }

      // Center the line horizontally now that line width is known
      formationWidth = Math::Max(formationWidth, lineWidth);
      float halfLineWidth = lineWidth * 0.5f;
      for (uint i = 0; i < positionIndices.getSize(); i++)
      {
         uint positionIndex = positionIndices[i];
         BVector offset = mPositions[positionIndex].getOffset();
         offset.x -= halfLineWidth;
         mPositions[positionIndex].setOffset(offset);
      }

      // Increment squad order index and adjust depth for next line
      formationDepth = currentLineDepth - maxSquadDepth;
      float randDepthFactor = mUseRandom ? getRandRangeFloat(cSimRand, 1.0f, 1.3f) : 1.0f;
      currentLineDepth -= (maxSquadDepth * randDepthFactor + 1.0f);

      numChildrenAdjusted += positionIndices.getSize();
      lineIndex++;
   }

   // Center the formation in z direction
   BVector zOffset(0.0f, 0.0f, formationDepth * -0.5f);
   for (uint i = 0; i < mPositions.getSize(); i++)
   {
      mPositions[i].adjustOffset(zOffset);
   }

   mRadiusX = formationWidth * 0.5f;
   mRadiusZ = formationDepth * -0.5f;
}

//==============================================================================
//==============================================================================
BEntityID BFormation2::findClosest(BEntityIDArray& entities, BVector position)
{
   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("    FindClosest::Position=(%5.2f, %5.2f)", position.x, position.z);
   #endif
   BEntityID rVal=cInvalidObjectID;
   float distance=0.0f;

   //Get the "forward".
   BVector formationForward=mPrevForward;
   formationForward.y=0.0f;
   formationForward.safeNormalize();
   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("      Forward=(%5.2f, %5.2f)", formationForward.x, formationForward.z);
   #endif

   for (uint i=0; i < entities.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 4742
      const BEntity* pEntity=gWorld->getEntity(entities[i]);
//--
      BASSERT(pEntity);
      float tempDistance=pEntity->getPosition().xzDistance(position);
      #ifdef DEBUGASSIGNMENT
      mpOwner->debug("      EID=%d Pos=(%5.2f, %5.2f), Distance=%5.2f", pEntity->getID().asLong(),
         pEntity->getPosition().x, pEntity->getPosition().z, tempDistance);
      #endif

      //Add in a small preference for getting put in a position that's "In Front" of us.
      //Get the direction to this point.
      if (tempDistance > cFloatCompareEpsilon)
      {
         BVector positionDirection=position-pEntity->getPosition();
         positionDirection.y=0.0f;
         positionDirection.safeNormalize();
         //Calculate the angle between ideal and this position.
         float angle=(float)fabs(formationForward.angleBetweenVector(positionDirection));
         float angleModifier=angle/cPi*1.5f;
         tempDistance+=tempDistance*angleModifier;
      }
      #ifdef DEBUGASSIGNMENT
      mpOwner->debug("      PositionDirection=(%5.2f, %5.2f), Angle=%f, AngleModifier=%f",
         positionDirection.x, positionDirection.z, angle, angleModifier);
      mpOwner->debug("      NewDistance=%5.2f", tempDistance);
      #endif

      if ((rVal == cInvalidObjectID) || (distance > tempDistance))
      {
         #ifdef DEBUGASSIGNMENT
         mpOwner->debug("        This is the new best one.");
         #endif
         rVal=pEntity->getID();
         distance=tempDistance;
      }
   }

   #ifdef DEBUGASSIGNMENT
   mpOwner->debug("      Returning %d.", rVal.asLong());
   #endif
   return (rVal);
}

//==============================================================================
// BFormation2::calculateLineDimensions
//==============================================================================
void BFormation2::calculateLineDimensions(float totalWidth, float totalDepth, float ratio, int& nL, float& lineWidth) const
{
   BASSERT(totalWidth > 0.0f);
   BASSERT(totalDepth > 0.0f);
   BASSERT(ratio > 0);

   //Figure out how many lines and columns we need given the number of units
   //passed in.  We use the basic 1:2 ratio (in that the line should be twice as
   //wide as the columns are deep).  First, we figure out what the square size
   //would be and use that to calculate the initial numbers needed.  Then, we
   //calc the major size (e.g. the number of lines is the measure of how long a
   //column is) and then iterate until that's at least twice as big as the other size.
   nL = 0;

   float area = totalWidth * totalDepth;
   int halfSquare = (int) ceil(sqrt(area));
   if (halfSquare <= 0)
      halfSquare = 1;

   nL = (int) (totalWidth / halfSquare);
   if (nL <= 0)
      nL = 1;

   lineWidth = totalDepth * nL * ratio;
   lineWidth = Math::Min(totalWidth / nL, lineWidth); // Don't make extra lines
}

//==============================================================================
//==============================================================================
void BFormation2::setDynamicPriority(BEntityID childID, uint priority)
{
   for (uint i=0; i < mPositions.getSize(); i++)
   {
      if (mPositions[i].getEntityID() == childID)
      {
         mPositions[i].setDynamicPriority(priority);
         return;
      }
   }
}


//==============================================================================
//==============================================================================
float BFormation2::computePositionErrorSqr()
{
   // No error yet.
   float errorSqr = 0.0f;
   
   // Run through positions
   for(long i=0; i < mPositions.getNumber(); i++)
   {
      // Get the associated entity.
      BEntity* entity = gWorld->getEntity(mPositions[i].getEntityID());
      if (entity == NULL)
         continue;

      // Compute xz distance to it's position.
      float dx = entity->getPosition().x - mPositions[i].getPosition().x;
      float dz = entity->getPosition().z - mPositions[i].getPosition().z;
      float distSqr = dx*dx + dz*dz;
      
      // Add to total error
      errorSqr += distSqr;
   }
   
   // Give it back
   return(errorSqr);
}


//==============================================================================
//==============================================================================
bool BFormation2::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPrevForward);
   GFWRITEARRAY(pStream, BEntityID, mChildren, uint8, 200);
   GFWRITECLASSARRAY(pStream, saveType, mPositions, uint8, 200);
   //BEntity* mpOwner;
   GFWRITEVAR(pStream, uint, mID);
   GFWRITEVAR(pStream, float, mRadiusX);
   GFWRITEVAR(pStream, float, mRadiusZ);
   GFWRITEVAR(pStream, float, mLineRatio);
   GFWRITEVAR(pStream, BFormationType, mType);
   GFWRITEVAR(pStream, DWORD, mReassignTimer);
   //BObstructionNodePtrArray mComputeForceObstructions;
   GFWRITEVAL(pStream, bool, (mGetObstructions || mComputeForceObstructions.size() > 0));
   GFWRITEBITBOOL(pStream, mMakeNeeded);
   GFWRITEBITBOOL(pStream, mReassignNeeded);
   GFWRITEBITBOOL(pStream, mDynamicPrioritiesValid);
   GFWRITEBITBOOL(pStream, mUseRandom);
   return true;
}

//==============================================================================
//==============================================================================
bool BFormation2::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mPrevForward);
   GFREADARRAY(pStream, BEntityID, mChildren, uint8, 200);
   GFREADCLASSARRAY(pStream, saveType, mPositions, uint8, 200);
   //BEntity* mpOwner;
   GFREADVAR(pStream, uint, mID);
   GFREADVAR(pStream, float, mRadiusX);
   GFREADVAR(pStream, float, mRadiusZ);
   GFREADVAR(pStream, float, mLineRatio);
   GFREADVAR(pStream, BFormationType, mType);
   GFREADVAR(pStream, DWORD, mReassignTimer);
   //BObstructionNodePtrArray mComputeForceObstructions;
   GFREADBITBOOL(pStream, mGetObstructions);
   GFREADBITBOOL(pStream, mMakeNeeded);
   GFREADBITBOOL(pStream, mReassignNeeded);
   GFREADBITBOOL(pStream, mDynamicPrioritiesValid);
   GFREADBITBOOL(pStream, mUseRandom);
   return true;
}

//==============================================================================
//==============================================================================
bool BFormationPosition2::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mOffset);
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVECTOR(pStream, mFlockVelocity);
   GFWRITEVAR(pStream, BEntityID, mEntityID);
   GFWRITEVAR(pStream, uint16, mPriority);
   GFWRITEVAR(pStream, uint16, mDynamicPriority);
   GFWRITEVAR(pStream, uint16, mOriginalLineIndex);
   GFWRITEVAR(pStream, uint16, mActualLineIndex);
   GFWRITEBITBOOL(pStream, mTransform);
   return true;
}

//==============================================================================
//==============================================================================
bool BFormationPosition2::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mOffset);
   GFREADVECTOR(pStream, mPosition);
   GFREADVECTOR(pStream, mFlockVelocity);
   GFREADVAR(pStream, BEntityID, mEntityID);
   GFREADVAR(pStream, uint16, mPriority);
   GFREADVAR(pStream, uint16, mDynamicPriority);
   GFREADVAR(pStream, uint16, mOriginalLineIndex);
   GFREADVAR(pStream, uint16, mActualLineIndex);
   GFREADBITBOOL(pStream, mTransform);
   return true;
}



//==============================================================================
//==============================================================================

//==============================================================================
// selectionmanager.cpp
//
// Copyright (c) 1999-2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "selectionmanager.h"

#include "camera.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "debugprimitives.h"
#include "displayStats.h"
#include "game.h"
#include "generaleventmanager.h"
#include "mathutil.h"
#include "object.h"
#include "objectmanager.h"
#include "point.h"
#include "protoobject.h"
#include "protosquad.h"
#include "render.h"
#include "simhelper.h"
#include "team.h"
#include "unit.h"
#include "user.h"
#include "world.h"
#include "ability.h"


GFIMPLEMENTVERSION(BSelectionAbility, 1);

//==============================================================================
// BSelectionManager::BSelectionManager
//==============================================================================
BSelectionManager::BSelectionManager() : 
   mpUser(NULL),
   mSubSelectGroupHandle(-1),
   mSubSelectSquadHandle(-1),
   mNumSubSelectGroups(0),
   mSaveSubSelectSort(-1),
   mSaveSubSelectGroupHandle(-1),
   mSelectionChangeCounter(0),
   mSubselectUIStartIndex(0),
   mSubselectUIEndIndex(0)
{
   for (uint i=0; i<cMaxSubSelectGroups; i++)
      mSubSelectTag[i]=false;

   mFlagSubSelectingSquads=false;
   mFlagAbilityAvailable=false;
   mFlagAbilityValid=false;
   mFlagAbilityRecovering=false;
   mFlagAbilityPlayer=false;
}

//==============================================================================
// BSelectionManager::~BSelectionManager
//==============================================================================
BSelectionManager::~BSelectionManager()
{
}


//==============================================================================
// BSelectionManager::~BSelectionManager
//==============================================================================
void BSelectionManager::attachUser( BUser *pUser )
{
   BASSERT(mpUser==NULL);
   mpUser = pUser;
   mSelectionChangeCounter = 0;
}

//==============================================================================
// BSelectionManager::~BSelectionManager
//==============================================================================
void BSelectionManager::detachUser( void )
{
   mpUser = NULL;
}


//==============================================================================
//==============================================================================
void BSelectionManager::recomputeSubSelection()
{
   calculateSelectedSquads();
   calculateSubSelectGroups();
}

//==============================================================================
// BSelectionManager::clearSelections
//==============================================================================
void BSelectionManager::clearSelections()
{
   long num = mSelectedUnits.getNumber();
   for(long i=0; i<num; i++)
      markUnitUnselected(mSelectedUnits[i]);
   mSelectedUnits.setNumber(0);
   calculateSelectedSquads();
   calculateSubSelectGroups();

   mSubselectUIStartIndex = 0;
   mSubselectUIEndIndex = cMaxUISubSelectGroupIndex;   

   mSelectionChangeCounter++;
}

//==============================================================================
// BSelectionManager::selectUnit
//==============================================================================
bool BSelectionManager::selectUnit(BEntityID unitID)
{
   // Do the easy error checking first for the early bail.
   if (isUnitSelected(unitID))
      return(false);

   // Make sure we can get a real unit out of this ID.
   BUnit *pUnit = gWorld->getUnit(unitID);
   if (!pUnit)
      return(false);
   //DCPTODO: Debug Hack.
   //if (pUnit->getPlayerID() != mpUser->getPlayerID())
   //   return(false);
   if (!pUnit->isAlive())
      return(false);

   //flash the unit
   if (!gConfig.isDefined(cConfigNoSelectionHighlight))
      pUnit->setSelectionFlashTime(1.0f);

   // Select me.
   mSelectedUnits.add(unitID);
   markUnitSelected(unitID);

   // Select my squad mates as well, if I have any.
   BSquad *pSquad = pUnit->getParentSquad();
   if (pSquad)
   {

      long numSquadChildren = pSquad->getNumberChildren();
      for (long i=0; i<numSquadChildren; i++)
      {
         BEntityID childUnitID = pSquad->getChild(i);
         if (!isUnitSelected(childUnitID))
         {
            //flash the unit
            if (!gConfig.isDefined(cConfigNoSelectionHighlight))
            {
               BUnit* pChildUnit = gWorld->getUnit(childUnitID);
               if (pChildUnit)
                  pChildUnit->setSelectionFlashTime(1.0f);
            }
            mSelectedUnits.add(childUnitID);
            markUnitSelected(childUnitID);
         }
      }
   }

   // Recalculate the selected squads and return.
   calculateSelectedSquads();
   calculateSubSelectGroups();

   mSelectionChangeCounter++;

   gGeneralEventManager.eventTrigger(BEventDefinitions::cSelectUnits, mpUser->getPlayer()->getID(), &mSelectedUnits, cInvalidObjectID, NULL, unitID); 

   return(true);
}

//==============================================================================
// BSelectionManager::unselectUnit
//==============================================================================
bool BSelectionManager::unselectUnit(BEntityID unitID)
{
   // Easy error checking for the early bail.
   if (!unitID.isValid())
      return (false);
   if (unitID.getType() != BEntity::cClassTypeUnit)
      return (false);
   if (!isUnitSelected(unitID))
      return (false);

   // Unselect the unit.
   markUnitUnselected(unitID);
   long foundUnitIndex = mSelectedUnits.find(unitID);
   if (foundUnitIndex != cInvalidIndex)
   {
      long lastIndex = mSelectedUnits.getNumber() - 1;
      mSelectedUnits[foundUnitIndex] = mSelectedUnits[lastIndex];
      mSelectedUnits.setNumber(lastIndex);
   }
   calculateSelectedSquads();
   calculateSubSelectGroups();

   mSelectionChangeCounter++;

   return (true);
}

//==============================================================================
// BSelectionManager::selectSquad
//==============================================================================
bool BSelectionManager::selectSquad(BEntityID squadID)
{
   bool retVal = false;
//-- FIXING PREFIX BUG ID 3713
   const BSquad* pSquad=gWorld->getSquad(squadID);
//--
   if (pSquad)
   {
      uint unitCount=pSquad->getNumberChildren();
      for (uint j=0; j<unitCount; j++)
         retVal |= selectUnit(pSquad->getChild(j));

   }
   return retVal;
}

//==============================================================================
// Unselect the squad
//==============================================================================
void BSelectionManager::unselectSquad(BEntityID squadID)
{
//-- FIXING PREFIX BUG ID 3714
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--
   if (pSquad)
   {
      uint numChildren = pSquad->getNumberChildren();
      for (uint i = 0; i < numChildren; i++)
         unselectUnit(pSquad->getChild(i));
   }
}

//==============================================================================
// BSelectionManager::selectSquads
//==============================================================================
bool BSelectionManager::selectSquads(BEntityIDArray& squads)
{
   bool retval=false;
   uint count=squads.getSize();
   for (uint i=0; i<count; i++)
      retval |= selectSquad(squads[i]);

   return retval;
}

//==============================================================================
// BSelectionManager::isUnitSelected
//==============================================================================
bool BSelectionManager::isUnitSelected(BEntityID unitID)
{
   if (!unitID.isValid() || unitID.getType() != BEntity::cClassTypeUnit)
      return(false);

   long unitBitPos = (long)unitID.getIndex();
   bool selected = (mIsUnitSelected.getNumber() > unitBitPos && mIsUnitSelected.isBitSet(unitBitPos) != 0);
   return(selected);
}

//==============================================================================
// BSelectionManager::isSquadSelected
//==============================================================================
bool BSelectionManager::isSquadSelected(BEntityID squadID)
{
   if (!squadID.isValid() || squadID.getType() != BEntity::cClassTypeSquad)
      return(false);

   long squadBitPos = (long)squadID.getIndex();
   bool selected = (mIsSquadSelected.getNumber() > squadBitPos && mIsSquadSelected.isBitSet(squadBitPos) != 0);
   return(selected);
}

//==============================================================================
// Unselect all squads that are outside the playable bounds
//==============================================================================
void BSelectionManager::fixupPlayableBoundsSelections()
{
   const BEntityIDArray& selectedSquads = getSelectedSquads();
   int numSelectedSquads = selectedSquads.getNumber();
   for (int i = (numSelectedSquads - 1); i >= 0; i--)
   {
//-- FIXING PREFIX BUG ID 3715
      const BSquad* pSquad = gWorld->getSquad(selectedSquads[i]);
//--
      if (pSquad && pSquad->isOutsidePlayableBounds())
      {
         unselectSquad(selectedSquads[i]);
      }
   }
}

//==============================================================================
// BSelectionManager::markUnitSelected
//==============================================================================
void BSelectionManager::markUnitSelected(BEntityID unitID)
{
   BASSERT(unitID.isValid());
   BASSERT(unitID.getType() == BEntity::cClassTypeUnit);
   
   // Get number of bits currently in the array.
   long numUnitBits = mIsUnitSelected.getNumber();
   long unitBitPos = (long)unitID.getIndex();

   // Expand the number of unit bits if necessary, and clear the new bits we've created.
   if (unitBitPos >= numUnitBits)
   {
      mIsUnitSelected.setNumber(unitBitPos+1);
      for (long i=numUnitBits; i<unitBitPos; i++)
         mIsUnitSelected.clearBit(i);
   }

   // Set the bit for this unitID
   mIsUnitSelected.setBit(unitBitPos);
}


//==============================================================================
// BSelectionManager::markUnitUnselected
//==============================================================================
void BSelectionManager::markUnitUnselected(BEntityID unitID)
{
   BASSERT(unitID.isValid());
   BASSERT(unitID.getType() == BEntity::cClassTypeUnit);

   // Get number of bits currently in the array.
   long unitBitPos = (long)unitID.getIndex();
   BASSERT(unitBitPos < mIsUnitSelected.getNumber());

   if (gWorld)
   {
      BUnit* pUnit = gWorld->getUnit(unitID);
      if (pUnit)
      {
         BSquad* pSquad = pUnit->getParentSquad();
         if (pSquad)
            pSquad->hideSelectionDecal();
      }
   }

   // Clear the bit for this unitID
   mIsUnitSelected.clearBit(unitBitPos);
}

//==============================================================================
// BSelectionManager::populateSelectionsCircle
//==============================================================================
void BSelectionManager::populateSelectionsCircle(BVector cameraLoc, BVector* points, long pointCount, float pixelError, bool drawMode)
{
   if(!drawMode)
   {
      // Start off with nothing in our list.
      mPossibleSelections.setNumber(0);
   }

   if(pointCount<3 || pointCount>64)
      return;

   // FIXME: This is going to be slow.
   BRenderViewParams view(gRender.getViewParams());

   if (mpUser)
   {
      BMatrix mat;
      mpUser->getCamera()->getViewMatrix(mat);
      view.setViewMatrix(mat);
   }

   // Build a list of triangles in screen space and calc center point
   BVector tps[64];

   BVector cp(0.0f,0.0f,1.0f);

   float oon=1.0f/pointCount;

   for(long i=0; i<pointCount; i++)
   {
      BVector& tp=tps[i];
      view.calculateWorldToScreen(points[i], tp.x, tp.y);
      tp.z=1.0f;
      cp.x+=tp.x;
      cp.y+=tp.y;
   }
   tps[pointCount]=tps[0];

   cp.x*=oon;
   cp.y*=oon;

   if(drawMode)
      gpDebugPrimitives->addDebugScreenPolygon(cp, tps, pointCount, cDWORDYellow);

   // Get viewport size
   float viewWidth=(float)view.getViewportWidth();
   float viewHeight=(float)view.getViewportHeight();

   // Calc bounding box of triangles in screen space. Units must be positioned within this box to be considered.
   float top=cMaximumFloat;
   float bottom=0.0f;
   float right=0.0f;
   float left=cMaximumFloat;
   for(long i=0; i<pointCount; i++)
   {
      if(tps[i].y<top)
         top=tps[i].y;
      if(tps[i].y>bottom)
         bottom=tps[i].y;
      if(tps[i].x>right)
         right=tps[i].x;
      if(tps[i].x<left)
         left=tps[i].x;
   }

   if(drawMode)
      gpDebugPrimitives->addDebugScreenRect(left, top, right, bottom, cDWORDBlue);

   BTeamID teamID=mpUser->getTeamID();

   // Run through each object.
   float pixelErrorSqr=pixelError*pixelError;
   long selectionMethod=-1;
   BVector rp;
   rp.z=1.0f;

   BVector up, uc, p, tp;

   for(long listType=0; listType<2; listType++)
   {
      BEntityHandle handle=cInvalidObjectID;
      for(;;)
      {
//-- FIXING PREFIX BUG ID 3710
         const BObject* pObject;
//--
         if(listType==0)
         {
            pObject=gWorld->getNextUnit(handle);
            if(!pObject)
               break;
            if(!pObject->isVisible(teamID))
               continue;
            if(!pObject->isAlive())
               continue;
            if(pObject->isGarrisoned() || pObject->isAttached() || pObject->isHitched())
               continue;
         }
         else
         {
            BDopple* pDopple=gWorld->getNextDopple(handle);
            if(!pDopple)
               break;
            if(pDopple->getForTeamID()!=teamID)
               continue;
            pObject=pDopple;
         }

         //FIXME - Should just look through objects that were rendered last frame instead of all objects in the world.
         const BProtoObject* pProtoObject = pObject->getProtoObject();
         if(!pProtoObject)
            continue;

         if (pObject->getSelectType(teamID) == cSelectTypeNone)
            continue;

         // Not being rendered
         if (pObject->getFlagNoRender())
            continue;

         // Calc unit center point in screen space
         float r=pProtoObject->getPickRadius();
         if(r==0.0f)
            r=pObject->getVisualRadius();

         float offset=pProtoObject->getPickOffset();
         if(offset==0.0f)
            uc=pObject->getVisualCenter();
         else
            uc=pObject->getPosition()+(pObject->getUp()*offset);

         view.calculateWorldToScreen(uc, up.x, up.y);
         up.z=1.0f;

         // Skip if off screen
         if(up.x<0.0f || up.y<0.0f || up.x>viewWidth || up.y>viewHeight)
            continue;

         if (!Math::IsValidFloat(uc.x))
            continue;

         // Calc unit radius in screen space and check against bounding box
         p=uc+(mpUser->getCamera()->getCameraRight()*r);
         view.calculateWorldToScreen(p, rp.x, rp.y);
         float ur=rp.xyDistance(up);
         bool skip=(up.x+ur<left || up.x-ur>right || up.y+ur<top || up.y-ur>bottom);
         if(drawMode)
            gpDebugPrimitives->addDebugScreenCircle(up, ur, ur, (skip?cDWORDRed:cDWORDOrange));
         if(skip)
            continue;

         selectionMethod=-1;

         for(long j=0; j<pointCount; j++)
         {
            bool hit=pointInTriangle(up, cp, tps[j], tps[j+1], cZAxisVector, 0.001f);
            if(hit)
            {
               if(drawMode)
                  gpDebugPrimitives->addDebugScreenTriangle(cp, tps[j], tps[j+1], cDWORDGreen);
               selectionMethod=BSelectionPossibility::cExact;
               break;
            }
         }

         if(selectionMethod==-1)
         {
            float urSqr=ur*ur;
            for(long j=0; j<pointCount; j++)
            {
               tp=closestPointOnTriangle(cp, tps[j], tps[j+1], up);
               float distanceSqr=tp.xyDistanceSqr(up);
               if(distanceSqr<urSqr)
               {
                  if(drawMode)
                     gpDebugPrimitives->addDebugScreenTriangle(cp, tps[j], tps[j+1], cDWORDBlue);
                  selectionMethod=BSelectionPossibility::cExact;
                  break;
               }

               if(pixelError!=0.0f && distanceSqr<urSqr+pixelErrorSqr)
               {
                  if(drawMode)
                     gpDebugPrimitives->addDebugScreenTriangle(cp, tps[j], tps[j+1], cDWORDCyan);
                  selectionMethod=BSelectionPossibility::cClose;
                  break;
               }
            }
         }

         // If not hit, skip.
         if(selectionMethod<0)
            continue;

         if(drawMode)
            gpDebugPrimitives->addDebugScreenCircle(up, ur, ur, (selectionMethod==BSelectionPossibility::cClose ? cDWORDMagenta : cDWORDGreen));

         if(!drawMode)
         {
            // Add to list.
            long newIndex=mPossibleSelections.getNumber();
            mPossibleSelections.setNumber(newIndex+1);
            mPossibleSelections[newIndex].mID=pObject->getID();
            mPossibleSelections[newIndex].mSelectionMethod=(short)selectionMethod;
            mPossibleSelections[newIndex].mDistance=cameraLoc.distanceSqr(uc);
            mPossibleSelections[newIndex].mOutlined=false;//pObject->getFlag(BObject::cViewObscured);
            mPossibleSelections[newIndex].mPriority=pProtoObject->getPickPriority();
         }
      }
   }

   sortPossibleSelectionList();
}

//==============================================================================
// BSelectionManager::populateSelectionsRect
//==============================================================================
void BSelectionManager::populateSelectionsRect(BVector cameraLoc, BVector point, float xSize, float ySize, bool drawMode, bool targetMode)
{
   mPossibleSelections.setNumber(0);

   BRenderViewParams view(gRender.getViewParams());

   if (mpUser)
   {
      BMatrix mat;
      mpUser->getCamera()->getViewMatrix(mat);
      view.setViewMatrix(mat);
   }

   float viewWidth=(float)view.getViewportWidth();
   float viewHeight=(float)view.getViewportHeight();

   float xHalfSize=xSize*0.5f;
   float yHalfSize=ySize*0.5f;

   BVector cp;
   view.calculateWorldToScreen(point, cp.x, cp.y);
   cp.z=1.0f;

   BVector tps[4];
   tps[0].x=cp.x-xHalfSize;
   tps[0].y=cp.y-yHalfSize;
   tps[0].z=1.0f;

   tps[1].x=cp.x+xHalfSize;
   tps[1].y=cp.y-yHalfSize;
   tps[1].z=1.0f;

   tps[2].x=cp.x+xHalfSize;
   tps[2].y=cp.y+yHalfSize;
   tps[2].z=1.0f;

   tps[3].x=cp.x-xHalfSize;
   tps[3].y=cp.y+yHalfSize;
   tps[3].z=1.0f;

   float left=cp.x-xHalfSize;
   float top=cp.y-yHalfSize;
   float right=cp.x+xHalfSize;
   float bottom=cp.y+yHalfSize;

   if(drawMode)
      gpDebugPrimitives->addDebugScreenRect(left, top, right, bottom, cDWORDBlue);

   BPlayer* pPlayer=mpUser->getPlayer();
   BTeamID teamID=pPlayer->getTeamID();
   BTeam* pTeam=pPlayer->getTeam();

   const BBitArray* pVisibleUnitArray=pTeam->getVisibleUnits();
   uint numUnits = (uint) gWorld->getUnitsHighWaterMark();
   uint unitType = BEntity::cClassTypeUnit << 28;

   BVector rp;
   BVector uc;
   BVector up;
   BVector p;

   bool skipSelectTypeCheck = false;
   #ifndef BUILD_FINAL
      if (BDisplayStats::getMode()==BDisplayStats::cDSMSim && BDisplayStats::getType(BDisplayStats::cDSMSim)==BDisplayStats::cDisplayStatObject)
         skipSelectTypeCheck = true;
   #endif

   for(long listType=0; listType<2; listType++)
   {
      BEntityHandle handle=cInvalidObjectID;
      uint i=(uint)-1;
      for(;;)
      {
//-- FIXING PREFIX BUG ID 3712
         const BObject* pObject;
//--
         if(listType==0)
         {
            i++;
            if(i==numUnits)
               break;
            if (!pVisibleUnitArray->isBitSet(i))
               continue;
            pObject = gWorld->getUnit(unitType | i, true);
            if (!pObject)
               continue;
            if(!pObject->isAlive())
               continue;
            if(pObject->isGarrisoned() || pObject->isAttached() || pObject->isHitched())            
               continue;
         }
         else
         {
            BDopple* pDopple=gWorld->getNextDopple(handle);
            if(!pDopple)
               break;
            if(pDopple->getForTeamID()!=teamID)
               continue;
            pObject=pDopple;
         }
         
         const BProtoObject* pProtoObject = pObject->getProtoObject();
         if(!pProtoObject)
            continue;

         if (!skipSelectTypeCheck)
         {
            long selectType = pObject->getSelectType(teamID);
            if((selectType == cSelectTypeNone) || (!targetMode && (selectType == cSelectTypeTarget)))
               continue;
         }

         // Not being rendered.
         if (pObject->getFlagNoRender())
            continue;

         float r=pProtoObject->getPickRadius();
         if(r==0.0f)
            r=pObject->getVisualRadius();

         float offset=pProtoObject->getPickOffset();
         if(offset==0.0f)
            uc=pObject->getVisualCenter();
         else
            uc=pObject->getPosition()+(pObject->getUp()*offset);
        
         view.calculateWorldToScreen(uc, up.x, up.y);
         up.z=1.0f;

         if(up.x<0.0f || up.y<0.0f || up.x>viewWidth || up.y>viewHeight)
            continue;

         if (!Math::IsValidFloat(uc.x))
            continue;

         // Calc unit radius in screen space and check against bounding box
         p=uc+(mpUser->getCamera()->getCameraRight()*r);
         view.calculateWorldToScreen(p, rp.x, rp.y);
         float ur=rp.xyDistance(up);
         bool skip=(up.x+ur<left || up.x-ur>right || up.y+ur<top || up.y-ur>bottom);
         if(skip)
         {
            if(drawMode)
               gpDebugPrimitives->addDebugScreenCircle(up, ur, ur, cDWORDRed);
            continue;
         }

         float ud=cp.distance(up);

         long newIndex=mPossibleSelections.getNumber();
         mPossibleSelections.setNumber(newIndex+1);
         mPossibleSelections[newIndex].mID=pObject->getID();
         mPossibleSelections[newIndex].mSelectionMethod=(short)(ud<ur*0.75f ? BSelectionPossibility::cExact : BSelectionPossibility::cClose);
         mPossibleSelections[newIndex].mDistance=cameraLoc.distanceSqr(uc);
         mPossibleSelections[newIndex].mOutlined=false;
         mPossibleSelections[newIndex].mPriority=pProtoObject->getPickPriority();

         if(drawMode)
            gpDebugPrimitives->addDebugScreenCircle(up, ur, ur, (mPossibleSelections[newIndex].mSelectionMethod==BSelectionPossibility::cClose ? cDWORDMagenta : cDWORDGreen));
      }
   }

   sortPossibleSelectionList();
}

//==============================================================================
// BSelectionManager::sortPossibleSelectionList
//==============================================================================
void BSelectionManager::sortPossibleSelectionList()
{
   // Dumb selection sort.  Hopefully this list isn't that long...
   BSelectionPossibility temp;
   for(long i=0; i<mPossibleSelections.getNumber()-1; i++)
   {
      for(long j=i+1; j<mPossibleSelections.getNumber(); j++)
      {
         // Compare.
         long result=comparePossibleSelections(mPossibleSelections[i], mPossibleSelections[j]);
   
         // Swap if necessary.
         if(result>0)
         {
            temp=mPossibleSelections[i];
            mPossibleSelections[i]=mPossibleSelections[j];
            mPossibleSelections[j]=temp;
         }
      }
   }
}

//==============================================================================
// BSelectionManager::getNextPossibleSelection
//==============================================================================
BEntityID BSelectionManager::getNextPossibleSelection(bool startOfList)
{
   // If nothing in list, return that now.
   if(mPossibleSelections.getNumber()<=0)
      return(cInvalidObjectID);

   // If we were asked for the start of the list, just give that back.
   // Also, we can do this if there is nothing currently selected.
   if(startOfList || getNumberSelectedUnits()<1)
      return(mPossibleSelections[0].mID);

   // Ok look for the (first) currently selected object
   BEntityID currSelected=mSelectedUnits[0];
   long i;
   for(i=0; i<mPossibleSelections.getNumber(); i++)
   {
      if(mPossibleSelections[i].mID==currSelected)
         break;
   }

   // We really want the next thing in the list.
   i++;

   // If past the end, wrap around to 0 (this also handles the "not found" case
   // coming out of the previous loop)
   if(i>=mPossibleSelections.getNumber())
      i=0;

   // Here ya go.
   return(mPossibleSelections[i].mID);
}

//==============================================================================
// BSelectionManager::getPossibleSelection
//==============================================================================
BEntityID BSelectionManager::getPossibleSelection(long index) const
{
   // Check for valid index.
   if(index<0 || index>=mPossibleSelections.getNumber())
      return(cInvalidObjectID);

   // Here ya go.
   return(mPossibleSelections[index].mID);
}

//==============================================================================
// BSelectionManager::calculateSelectedSquads
//==============================================================================
void BSelectionManager::calculateSelectedSquads()
{
   mSelectedSquads.setNumber(0);
   mIsSquadSelected.clear();
 
   long numUnitsSelected = mSelectedUnits.getNumber();
   for (long i=0; i<numUnitsSelected; i++)
   {
      BUnit *pUnit = gWorld->getUnit(mSelectedUnits[i]);
      if (pUnit && pUnit->getParentSquad())
      {
         BEntityID squadID = pUnit->getParentID();
         mSelectedSquads.uniqueAdd(squadID);

         long bitPos = (long)squadID.getIndex();

         // Grow array if necessary.
         if (bitPos >= mIsSquadSelected.getNumber())
         {
            long oldNum = mIsSquadSelected.getNumber();
            mIsSquadSelected.setNumber(bitPos+1);
            for (long i=oldNum; i<bitPos; i++)
               mIsSquadSelected.clearBit(i);
         }

         mIsSquadSelected.setBit(bitPos);
      }         
   }

   // Sort the squad list by goto type and proto ID
   uint numSquads = mSelectedSquads.getSize();
   if (numSquads != 0)
   {
      for (uint i=0; i<numSquads-1; i++)
      {
         for (uint j=i+1; j<numSquads; j++)
         {
            int result1 = getSquadSortValue(mSelectedSquads[i]);
            int result2 = getSquadSortValue(mSelectedSquads[j]);

            // Swap if necessary.
            if (result1 > result2)
            {
               BEntityID temp=mSelectedSquads[i];
               mSelectedSquads[i]=mSelectedSquads[j];
               mSelectedSquads[j]=temp;
            }
         }
      }
   }
}

//==============================================================================
// BSelectionManager::getSquadSortValue
//==============================================================================
int BSelectionManager::getSquadSortValue(BEntityID squadID)
{
//-- FIXING PREFIX BUG ID 3705
   const BSquad* pSquad=gWorld->getSquad(squadID);
//--
   if (pSquad)
   {
      int sort = pSquad->getProtoSquad()->getSubSelectSort();
      //if (pSquad->getSquadMode()==BSquadAI::cModeLockdown || pSquad->getSquadMode()==BSquadAI::cModeAbility)
      if (pSquad->getSquadMode()==BSquadAI::cModeAbility)
         sort++;
      return sort;
   }
   else
      return INT_MAX;
}

//==============================================================================
// BSelectionManager::calculateSubSelectGroups
//==============================================================================
void BSelectionManager::calculateSubSelectGroups()
{   
   uint numSquads = mSelectedSquads.getSize();
   if (numSquads == 0)
   {
      clearCurrentSubSelect();
      mNumSubSelectGroups = 0;
      for (uint i=0; i<cMaxSubSelectGroups; i++)
         mSubSelectGroups[i].clear();
      mFlagAbilityAvailable=false;
      mFlagAbilityValid=false;
      mFlagAbilityRecovering=false;
      mFlagAbilityPlayer=false;
      return;
   }
   
   // Save off the squad sort values for the current and tagged sub select items.
   // We'll use these sort values after the sub select data is recalculated to 
   // reset the current and tagged sub select items.
   int saveSubSelectTagSort[cMaxSubSelectGroups];
   for (uint i=0; i<cMaxSubSelectGroups; i++)
      saveSubSelectTagSort[i]=-1;
   bool anySaveSubSelectTags=false;
   int saveSubSelectSort=-1;
   int saveSubSelectGroupHandle=mSubSelectGroupHandle;
   //int saveSubSelectSquadHandle=mSubSelectSquadHandle;
   if (mSubSelectGroupHandle != -1 && mSubSelectGroupHandle < (int)mNumSubSelectGroups)
   {
      for (int i=0; i<(int)mNumSubSelectGroups; i++)
      {
         if (i==mSubSelectGroupHandle || mSubSelectTag[i])
         {
            const BEntityIDArray& group=mSubSelectGroups[i];
            uint count=group.getSize();
            for (uint j=0; j<count; j++)
            {
               int sortValue=getSquadSortValue(group[j]);
               if (sortValue!=0)
               {
                  if (i==mSubSelectGroupHandle)
                     saveSubSelectSort=sortValue;
                  if (mSubSelectTag[i])
                  {
                     saveSubSelectTagSort[i]=sortValue;
                     anySaveSubSelectTags=true;
                  }
                  break;
               }
            }
         }
      }
   }

   // Recalc sub select groups.
   clearCurrentSubSelect();
   mNumSubSelectGroups = 0;
   for (uint i=0; i<cMaxSubSelectGroups; i++)
      mSubSelectGroups[i].clear();
   mFlagAbilityAvailable=false;
   mFlagAbilityValid=false;
   mFlagAbilityRecovering=false;
   mFlagAbilityPlayer=false;
   if (numSquads == 0)
      return;

   mNumSubSelectGroups = 1;
   uint groupIndex = 0;
   for (uint i=0; i<numSquads; i++)
   {
      BEntityID squadID = mSelectedSquads[i];
      BEntityIDArray& subGroup = mSubSelectGroups[groupIndex];
      if (subGroup.getSize() == 0)
         subGroup.add(squadID);
      else
      {
         int sortValue1 = getSquadSortValue(subGroup[0]);
         int sortValue2 = getSquadSortValue(squadID);
         if (sortValue1 == sortValue2)
            subGroup.add(squadID);
         else
         {
            if (groupIndex == cMaxSubSelectGroups-1)
               break;
            mNumSubSelectGroups++;
            groupIndex++;
            mSubSelectGroups[groupIndex].add(squadID);
         }
      }
   }

   if (mNumSubSelectGroups > 0)
   {
      // Compute the unit ability for each subselect group.
      calculateSubSelectAbilities();

      // Reset the current and tagged sub select items.
      int firstRetaggedGroup=-1;
      if (saveSubSelectSort!=-1 || anySaveSubSelectTags)
      {
         for (int i=0; i<(int)mNumSubSelectGroups; i++)
         {
            const BEntityIDArray& group=mSubSelectGroups[i];
            uint count=group.getSize();
            int sortValue=0;
            for (uint j=0; j<count; j++)
            {
               sortValue=getSquadSortValue(group[j]);
               if (sortValue!=0)
                  break;
            }
            if (mSubSelectGroupHandle==-1 && sortValue==saveSubSelectSort)
            {
               mSubSelectGroupHandle=i;
               if (!anySaveSubSelectTags)
                  break;
            }
            if (anySaveSubSelectTags)
            {
               for (uint k=0; k<cMaxSubSelectGroups; k++)
               {
                  if (sortValue==saveSubSelectTagSort[k])
                  {
                     mSubSelectTag[i]=true;
                     if (firstRetaggedGroup==-1)
                        firstRetaggedGroup=i;
                     break;
                  }
               }
            }
         }
      }
      if (mSubSelectGroupHandle==-1 && saveSubSelectGroupHandle!=-1)
      {
         if (firstRetaggedGroup!=-1)
            mSubSelectGroupHandle=firstRetaggedGroup;
         else
         {
            if (saveSubSelectGroupHandle>=(int)mNumSubSelectGroups)
               mSubSelectGroupHandle=mNumSubSelectGroups-1;
            else
               mSubSelectGroupHandle=saveSubSelectGroupHandle;
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSelectionManager::saveSubSelect()
{
   mSaveSubSelectSort=-1;
   mSaveSubSelectGroupHandle=mSubSelectGroupHandle;
   if (mSubSelectGroupHandle != -1 && mSubSelectGroupHandle < (int)mNumSubSelectGroups)
   {
      const BEntityIDArray& group=mSubSelectGroups[mSubSelectGroupHandle];
      uint count=group.getSize();
      for (uint j=0; j<count; j++)
      {
         int sortValue=getSquadSortValue(group[j]);
         if (sortValue!=0)
         {
            mSaveSubSelectSort=sortValue;
            break;
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSelectionManager::restoreSubSelect()
{
   clearCurrentSubSelect();

   if (mSaveSubSelectSort!=-1)
   {
      for (int i=0; i<(int)mNumSubSelectGroups; i++)
      {
         const BEntityIDArray& group=mSubSelectGroups[i];
         uint count=group.getSize();
         int sortValue=0;
         for (uint j=0; j<count; j++)
         {
            sortValue=getSquadSortValue(group[j]);
            if (sortValue!=0)
               break;
         }
         if (mSubSelectGroupHandle==-1 && sortValue==mSaveSubSelectSort)
            mSubSelectGroupHandle=i;
      }
   }

   if (mSubSelectGroupHandle==-1 && mSaveSubSelectGroupHandle!=-1)
   {
      if (mSaveSubSelectGroupHandle>=(int)mNumSubSelectGroups)
         mSubSelectGroupHandle=mNumSubSelectGroups-1;
      else
         mSubSelectGroupHandle=mSaveSubSelectGroupHandle;
   }
}

//==============================================================================
//==============================================================================
void BSelectionManager::calculateSubSelectAbilities()
{
   mFlagAbilityPlayer = false;
   BPlayerID userPlayerID = mpUser->getPlayerID();
   for (uint i=0; i<mNumSubSelectGroups; i++)
   {
      BSimHelper::calculateSelectionAbility(mSubSelectGroups[i], userPlayerID, mSubSelectAbility[i]);
      if (!mFlagAbilityPlayer && mSubSelectAbility[i].mPlayer)
         mFlagAbilityPlayer = true;
   }
   updateSubSelectAbilities(cInvalidObjectID, cInvalidVector);
}


//==============================================================================
//==============================================================================
void BSelectionManager::updateSubSelectAbilities(BEntityID hoverObject, BVector hoverPoint)
{
   mFlagAbilityAvailable = false;
   mFlagAbilityValid = false;
   mFlagAbilityRecovering = true;

   for (uint i=0; i<mNumSubSelectGroups; i++)
   {
      BSelectionAbility& ssa = mSubSelectAbility[i];
      BSimHelper::updateSelectionAbility(mSubSelectGroups[i], hoverObject, hoverPoint, ssa);

      if (!mFlagAbilityAvailable && ssa.mAbilityID!=-1)
         mFlagAbilityAvailable = true;
      if (!mFlagAbilityValid && ssa.mValid)
         mFlagAbilityValid = true;
      if (mFlagAbilityRecovering && !ssa.mRecovering)
         mFlagAbilityRecovering = false;
   }
}


//==============================================================================
// mrh - 5/22/08
// Removed the core logic into BSimHelper class (but kept the logic in-tact)
// so that other parts of the sim can use this and we don't dupliate code.
//==============================================================================
/*void BSelectionManager::calculateSubSelectAbilities()
{
   mFlagAbilityPlayer=false;
   for (uint i=0; i<mNumSubSelectGroups; i++)
   {
      BSelectionAbility& ability = mSubSelectAbility[i];
      ability.mAbilityID = -1;
      ability.mTargetType = -1;
      ability.mPlayer = false;
      const BEntityIDArray& group=mSubSelectGroups[i];
      uint numSquads = group.getSize();
      for (uint j=0; j<numSquads; j++)
      {
         BSquad* pSquad = gWorld->getSquad(group[j]);
         if (pSquad)
         {
            BAbility* pAbility = NULL;
            uint numUnits = pSquad->getNumberChildren();
            for (uint k=0; k<numUnits; k++)
            {
//-- FIXING PREFIX BUG ID 3704
               const BUnit* pUnit=gWorld->getUnit(pSquad->getChild(k));
//--
               if (pUnit)
               {
                  if (!pUnit->getProtoObject()->getFlagAbilityDisabled())
                  {
                     int abilityID = pUnit->getProtoObject()->getAbilityCommand();
                     if (abilityID != -1)
                     {
                        pAbility = gDatabase.getAbilityFromID(abilityID);
                        ability.mPlayer = (pUnit->getPlayerID() == mpUser->getPlayerID());
                        if (!mFlagAbilityPlayer && ability.mPlayer)
                           mFlagAbilityPlayer = true;
                     }
                  }
                  break;
               }
            }
            if (pAbility)
            {
               ability.mAbilityID = (int8)pAbility->getID();
               ability.mTargetType = (int8)pAbility->getTargetType();
               break;
            }
         }
      }
   }
   updateSubSelectAbilities(cInvalidObjectID, cInvalidVector);
}
*/

//==============================================================================
// mrh - 5/22/08
// Removed the core logic into BSimHelper class (but kept the logic in-tact)
// so that other parts of the sim can use this and we don't dupliate code.
//==============================================================================
/*void BSelectionManager::updateSubSelectAbilities(BEntityID hoverObject, BVector hoverPoint)
{
   mFlagAbilityAvailable = false;
   mFlagAbilityValid = false;
   mFlagAbilityRecovering = true;

   for (uint i=0; i<mNumSubSelectGroups; i++)
   {
      BSelectionAbility& ability = mSubSelectAbility[i];
      ability.mReverse = false;
      ability.mValid = false;
      ability.mTargetUnit = false;
      ability.mRecovering = true;
      const BEntityIDArray& group=mSubSelectGroups[i];
      uint numSquads = group.getSize();
      if (numSquads == 0)
      {
         ability.mAbilityID = -1;
         ability.mTargetType = -1;
      }
      else if (ability.mAbilityID != -1)
      {
         if (ability.mTargetType == BAbility::cTargetNone || ability.mTargetType == BAbility::cTargetLocation || ability.mTargetType == BAbility::cTargetUnitOrLocation)
         {
            if (ability.mAbilityType==cAbilityChangeMode)
            {
               bool allChanging = true;
               bool allChanged = true;
               bool anyChanging = false;
               for (uint j=0; j<numSquads; j++)
               {
                  BSquad* pSquad = gWorld->getSquad(group[j]);
                  if (pSquad)
                  {
                     if (!pSquad->getFlagChangingMode())
                        allChanging = false;
                     else
                        anyChanging = true;
                     if (pSquad->getCurrentOrChangingMode()==BSquadAI::cModeNormal)
                        allChanged = false;
                  }
               }
               if (allChanging || (anyChanging && allChanged))
                  ability.mValid = false;
               // ajl 4/20/08 - Jira 5650 X/Y button change
               //else if (allChanged)
               //   ability.mReverse = true;
               //
            }
            else if (ability.mAbilityType==cAbilityUnload)
            {
               for (uint j=0; j<numSquads; j++)
               {
                  BSquad* pSquad=gWorld->getSquad(group[j]);
                  if (pSquad)
                  {
                     BUnit* pUnit=pSquad->getLeaderUnit();
                     if (pUnit && pUnit->getProtoObject()->getAbilityCommand() == ability.mAbilityID)
                     {
                        if (pUnit->getFirstEntityRefByType(BEntityRef::cTypeContainUnit) != NULL || pUnit->getFirstEntityRefByType(BEntityRef::cTypeAttachObject) != NULL)
                        {
                           ability.mValid = true;
                           break;
                        }
                     }
                  }
               }
            }
            else
               ability.mValid = true;
         }
         if (hoverObject != cInvalidObjectID)
         {
            for (uint j=0; j<numSquads; j++)
            {
               BSquad* pSquad=gWorld->getSquad(group[j]);
               if (pSquad)
               {
                  BProtoAction* pAction = pSquad->getProtoActionForTarget(hoverObject, hoverPoint, gDatabase.getAIDCommand(), false);
                  if(!pAction)
                     continue;
                  ability.mTargetUnit = true;
                  if (ability.mValid && ability.mReverse)
                     ability.mReverse = false;
                  else
                     ability.mValid = true;
                  break;
               }
            }
         }
         for (uint j=0; j<numSquads; j++)
         {
            BSquad* pSquad=gWorld->getSquad(group[j]);
            if (pSquad)
            {
               if (pSquad->getRecoverType() != cRecoverAbility && !pSquad->getFlagUsingTimedAbility())
               {
                  ability.mRecovering=false;
                  break;
               }
            }
         }
      }
      if (!mFlagAbilityAvailable && ability.mAbilityID!=-1)
         mFlagAbilityAvailable=true;
      if (!mFlagAbilityValid && ability.mValid)
         mFlagAbilityValid=true;
      if (mFlagAbilityRecovering && !ability.mRecovering)
         mFlagAbilityRecovering=false;
   }
}
*/

//==============================================================================
// BSelectionManager::comparePossibleSelections
//==============================================================================
long BSelectionManager::comparePossibleSelections(const BSelectionPossibility &sp1, const BSelectionPossibility &sp2)
{
   //return(BSelectionManager::comparePossibleSelections(sp1, sp2));
   long score1=getSelectionScore(sp1);
   long score2=getSelectionScore(sp2);
   if(score1>score2)
      return(-1);
   else if(score1<score2)
      return(1);

   if(sp1.mDistance<sp2.mDistance)
      return(-1);
   else
      return(1);
}

//==============================================================================
// BSelectionManager::getSelectionScore
//==============================================================================
long BSelectionManager::getSelectionScore(const BSelectionPossibility &sp)
{
   switch(sp.mPriority)
   {
      case cPickPriorityBuilding:
         if(sp.mSelectionMethod==BSelectionPossibility::cExact)
            return(2);
         return(1);

      case cPickPriorityResource:
         if(sp.mSelectionMethod==BSelectionPossibility::cExact)
            return(3);
         return(2);

      case cPickPriorityUnit:
         if(sp.mSelectionMethod==BSelectionPossibility::cExact)
            return(9);
         return(8);

      case cPickPriorityRally:
         return(20);
   }

   return(1);
}

//==============================================================================
//==============================================================================
int BSelectionManager::subSelectNext()
{
   mSelectionChangeCounter++;

   if (mNumSubSelectGroups == 0)
      clearCurrentSubSelect();
   else
   {
      if (mFlagSubSelectingSquads)
      {
         mSubSelectSquadHandle++;
         if (mSubSelectSquadHandle >= (int)getNumSubSelectSquads())
            mSubSelectSquadHandle=0;
      }
      else
      {
         mSubSelectGroupHandle++;
         if (mSubSelectGroupHandle >= (int)mNumSubSelectGroups)
         {
            //if (gConfig.isDefined(cConfigExitSubSelectOnCancel))
               mSubSelectGroupHandle=0;
               mSubselectUIStartIndex = 0;
               mSubselectUIEndIndex =cMaxUISubSelectGroupIndex;
            //else
            //   mSubSelectGroupHandle=-1;
         }
         else if (mSubSelectGroupHandle > mSubselectUIEndIndex)
         {
            mSubselectUIEndIndex++;
            mSubselectUIStartIndex++;
         }         
      }
   }
   return mSubSelectGroupHandle;
}

//==============================================================================
//==============================================================================
int BSelectionManager::subSelectPrev()
{
   mSelectionChangeCounter++;

   if (mNumSubSelectGroups == 0)
      clearCurrentSubSelect();
   else
   {
      if (mFlagSubSelectingSquads)
      {
         mSubSelectSquadHandle--;
         if (mSubSelectSquadHandle == -1)
            mSubSelectSquadHandle = getNumSubSelectSquads() - 1;
      }
      else
      {
         mSubSelectGroupHandle--;
         if (mSubSelectGroupHandle < -1)
         {
            mSubSelectGroupHandle = mNumSubSelectGroups - 1;            
            mSubselectUIEndIndex =mNumSubSelectGroups - 1;
            mSubselectUIStartIndex = mSubselectUIEndIndex-cMaxUISubSelectGroupIndex;
         }
         //else if (gConfig.isDefined(cConfigExitSubSelectOnCancel) && mSubSelectGroupHandle==-1)
         else if (mSubSelectGroupHandle==-1)
         {
            mSubSelectGroupHandle = mNumSubSelectGroups - 1;
            mSubselectUIEndIndex =mNumSubSelectGroups - 1;
            mSubselectUIStartIndex = mSubselectUIEndIndex-cMaxUISubSelectGroupIndex;
         }
         else if (mSubSelectGroupHandle < mSubselectUIStartIndex)
         {
            mSubselectUIEndIndex--;
            mSubselectUIStartIndex--;
            if (mSubselectUIStartIndex < 0)
               mSubselectUIStartIndex = 0;
            if (mSubselectUIEndIndex < 0)
               mSubselectUIEndIndex = 0;
         }         
      }
   }
   return mSubSelectGroupHandle;
}

//==============================================================================
//==============================================================================
void BSelectionManager::subSelectBySquad()
{
   if (mSubSelectGroupHandle==-1)
      return;
   mFlagSubSelectingSquads=true;
   mSubSelectSquadHandle=0;
   mSelectionChangeCounter++;
}

//==============================================================================
//==============================================================================
void BSelectionManager::subSelectByType()
{
   mFlagSubSelectingSquads=false;
   mSubSelectSquadHandle=-1;
   mSelectionChangeCounter++;
}

//==============================================================================
//==============================================================================
void BSelectionManager::subSelectTag()
{
   if (mSubSelectGroupHandle==-1)
      return;
   mSubSelectTag[mSubSelectGroupHandle]=!mSubSelectTag[mSubSelectGroupHandle];
}

//==============================================================================
//==============================================================================
void BSelectionManager::subSelectSelect()
{
   if (mSubSelectGroupHandle==-1)
      return;
   BEntityIDArray squadList;
   if (!getSubSelectedSquads(squadList))
      return;
   clearSelections();
   selectSquads(squadList);
}

//==============================================================================
//==============================================================================
uint BSelectionManager::getNumSubSelectSquads() const
{
   if (mSubSelectGroupHandle==-1)
      return 0;
   return mSubSelectGroups[mSubSelectGroupHandle].getSize();
}

//==============================================================================
//==============================================================================
BEntityID BSelectionManager::getSubSelectSquad(uint index) const
{
   if (mSubSelectGroupHandle==-1)
      return cInvalidObjectID;
   return mSubSelectGroups[mSubSelectGroupHandle][index];
}

//==============================================================================
//==============================================================================
bool BSelectionManager::isSubSelected(BEntityID squadID) const
{
   if (mSubSelectGroupHandle==-1)
      return false;
   if (mFlagSubSelectingSquads)
   {
      if (squadID == getSubSelectSquad(mSubSelectSquadHandle))
         return true;
   }
   else
   {
      if (mSubSelectGroups[mSubSelectGroupHandle].find(squadID)!=cInvalidIndex)
         return true;
      for (int i=0; i<cMaxSubSelectGroups; i++)
      {
         if (i!=mSubSelectGroupHandle && mSubSelectTag[i] && mSubSelectGroups[i].find(squadID)!=cInvalidIndex)
            return true;
      }
   }
   return false;
}

//==============================================================================
//==============================================================================
void BSelectionManager::clearCurrentSubSelect()
{ 
   mSubSelectGroupHandle=-1;
   
   mSubselectUIStartIndex=0;
   mSubselectUIEndIndex=cMaxUISubSelectGroupIndex;

   for (uint i=0; i<cMaxSubSelectGroups; i++)
      mSubSelectTag[i]=false;
   mSubSelectSquadHandle=-1;
   mFlagSubSelectingSquads=false;
   mSelectionChangeCounter++;
}

//==============================================================================
//==============================================================================
bool BSelectionManager::getSubSelectedUnits(BEntityIDArray& unitList) const
{
   if (mSubSelectGroupHandle == -1 || mSubSelectGroupHandle >= (int)mNumSubSelectGroups)
   {
      unitList = mSelectedUnits;
      return true;
   }

   unitList.clear();

   if (mFlagSubSelectingSquads)
   {
      BEntityID squadID = getSubSelectSquad(mSubSelectSquadHandle);
      if (squadID == cInvalidObjectID)
         return false;
//-- FIXING PREFIX BUG ID 3707
      const BSquad* pSquad=gWorld->getSquad(squadID);
//--
      if (!pSquad)
         return false;
      int unitCount=pSquad->getNumberChildren();
      for (int j=0; j<unitCount; j++)
         unitList.add(pSquad->getChild(j));
   }
   else
   {
      for (int i=0; i<cMaxSubSelectGroups; i++)
      {
         if (i==mSubSelectGroupHandle || mSubSelectTag[i])
         {
            const BEntityIDArray& group=mSubSelectGroups[i];
            uint squadCount=group.getSize();
            for (uint i=0; i<squadCount; i++)
            {
//-- FIXING PREFIX BUG ID 3708
               const BSquad* pSquad=gWorld->getSquad(group[i]);
//--
               if (!pSquad)
                  continue;
               int unitCount=pSquad->getNumberChildren();
               for (int j=0; j<unitCount; j++)
                  unitList.add(pSquad->getChild(j));
            }
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BSelectionManager::getSubSelectedSquads(BEntityIDArray& squadList) const
{
   if (mSubSelectGroupHandle == -1 || mSubSelectGroupHandle >= (int)mNumSubSelectGroups)
   {
      squadList = mSelectedSquads;
      return true;
   }

   squadList.clear();

   if (mFlagSubSelectingSquads)
   {
      BEntityID squadID = getSubSelectSquad(mSubSelectSquadHandle);
      if (squadID == cInvalidObjectID)
         return false;
      squadList.add(squadID);
   }
   else
   {
      for (int i=0; i<cMaxSubSelectGroups; i++)
      {
         if (i==mSubSelectGroupHandle || mSubSelectTag[i])
         {
            const BEntityIDArray& group=mSubSelectGroups[i];
            uint count=group.getSize();
            for (uint i=0; i<count; i++)
               squadList.add(group[i]);
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
BSelectionAbility BSelectionManager::getSubSelectAbility() const
{
   if (mSubSelectGroupHandle==-1)
   {
      if (mNumSubSelectGroups == 1)
         return mSubSelectAbility[0];
      else
         return BSelectionAbility();
   }
   return mSubSelectAbility[mSubSelectGroupHandle];
}

//==============================================================================
//==============================================================================
bool BSelectionManager::getSubSelectAbilityValid() const
{
   if (mSubSelectGroupHandle==-1)
   {
      if (mNumSubSelectGroups == 1)
         return mSubSelectAbility[0].mValid;
      else
         return mFlagAbilityValid;
   }
   return mSubSelectAbility[mSubSelectGroupHandle].mValid;
}

//==============================================================================
//==============================================================================
bool BSelectionManager::getSubSelectAbilityRecovering() const
{
   if (mSubSelectGroupHandle==-1)
   {
      if (mNumSubSelectGroups == 1)
         return mSubSelectAbility[0].mRecovering;
      else
         return mFlagAbilityRecovering;
   }
   return mSubSelectAbility[mSubSelectGroupHandle].mRecovering;
}


//==============================================================================
//==============================================================================
bool BSelectionManager::getSubSelectAbilityPlayer() const
{
   if (mSubSelectGroupHandle==-1)
   {
      if (mNumSubSelectGroups == 1)
         return mSubSelectAbility[0].mPlayer;
      else
         return mFlagAbilityPlayer;
   }
   return mSubSelectAbility[mSubSelectGroupHandle].mPlayer;
}


//==============================================================================
// BSelectionAbility::save
//==============================================================================
bool BSelectionAbility::save(BStream* pStream, int saveType) const
{ 
   GFWRITEVAR(pStream, BAbilityID, mAbilityID); 
   GFWRITEVAR(pStream, BAbilityType, mAbilityType); 
   GFWRITEVAR(pStream, BAbilityTargetType, mTargetType);
   GFWRITEBITBOOL(pStream, mReverse);
   GFWRITEBITBOOL(pStream, mValid);
   GFWRITEBITBOOL(pStream, mTargetUnit);
   GFWRITEBITBOOL(pStream, mRecovering);
   GFWRITEBITBOOL(pStream, mPlayer);
   return true; 
}


//==============================================================================
// BSelectionAbility::load
//==============================================================================
bool BSelectionAbility::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BAbilityID, mAbilityID); 
   GFREADVAR(pStream, BAbilityType, mAbilityType); 
   GFREADVAR(pStream, BAbilityTargetType, mTargetType);
   GFREADBITBOOL(pStream, mReverse);
   GFREADBITBOOL(pStream, mValid);
   GFREADBITBOOL(pStream, mTargetUnit);
   GFREADBITBOOL(pStream, mRecovering);
   GFREADBITBOOL(pStream, mPlayer);
   return true;
}

//============================================================================
// damagetemplate.cpp
//
// Copyright (c) 2004-2007 Ensemble Studios
//============================================================================

#include "common.h"
#include "damagetemplate.h"
#include "damageaction.h"
#include "damagetemplatemanager.h"

#include "grannymanager.h"
#include "grannymodel.h"
#include "shape.h"
#include "gamedirectories.h"
#include "database.h"
#include "particlegateway.h"
#include "visual.h"
#include "unit.h"
#include "grannyinstance.h"
#include "physicsinfo.h"
#include "physicsobjectblueprint.h"
#include "world.h"
#include "reloadManager.h"

#include <Physics/Collide/Shape/Convex/Box/hkpBoxShape.h>
#include <Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h>
#include <Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppUtility.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h>
#include <Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>



static BDynamicSimArray<BSimString> gImpactPointNames;
static BDynamicSimArray<BSimString> gTempNames;


//-- scratch pad for use in calculating damge template functions
BDynamicBYTEArray BDamageTemplate::mScratchPad;

GFIMPLEMENTVERSION(BDamageTracker, 1);


//============================================================================
// BDamageTracker
//============================================================================
BDamageTracker::BDamageTracker() :
   mDamageTemplateID(-1),
   mAliveImpactPointCount(0),
   mCurrentHP(0.0f),
   mTimeToClearParts(0.0f)
{
}

//============================================================================
// BDamageTracker::~BDamageTracker
//============================================================================
BDamageTracker::~BDamageTracker()
{
   for(long i = 0; i < mImpactPointTrackers.getNumber(); i++)
   {
      delete mImpactPointTrackers[i];
      mImpactPointTrackers[i] = NULL;
   }
   mImpactPointTrackers.clear();
}

//============================================================================
// BDamageTracker::init
//============================================================================
bool BDamageTracker::init(long damageTemplateID)
{
//   syncUnitDetailCode("BDamageTracker::init");

   const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(damageTemplateID);
   if(!pDT)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BDamageTracker::init no pDT");
      #endif
      return false;
   }

   for(long i = 0; i < mImpactPointTrackers.getNumber(); i++)
   {
      delete mImpactPointTrackers[i];
      mImpactPointTrackers[i] = NULL;
   }
   mImpactPointTrackers.clear();

   long count = pDT->getImpactPointCount();
   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BDamageTracker::init count", count);
   #endif
   for(long i = 0; i < count; i++)
   {
      BImpactPointTracker* pDestIPT = new BImpactPointTracker();
      if(!pDestIPT)
      {
         #ifdef SYNC_UnitDetail
            syncUnitDetailCode("BDamageTracker::init new BImpactPointTracker failed");
         #endif
         return false;
      }

      if(pDT->getImpactPointByIndex(i))
      {
         #ifdef SYNC_UnitDetail
            syncUnitDetailCode("BDamageTracker::init pDT->getImpactPointByIndex");
         #endif
         pDestIPT->mImpactPointIndex = pDT->getImpactPointByIndex(i)->getIndex();
      }

      mImpactPointTrackers.add(pDestIPT);
   }

   //syncUnitDetailData("   impact point trackers added=", pDT->getImpactPointCount());

   mDamageTemplateID = damageTemplateID;

   mAliveImpactPointCount = count;

   mCurrentHP = 1.0f;

   mTimeToClearParts = 0.0f;
   mRecentThrownParts.clear();


   return true;
}


//============================================================================
// BDamageTracker::reset
//============================================================================
void BDamageTracker::reset()
{
   long count = mImpactPointTrackers.getNumber();
   for(long i = 0; i < count; i++)
   {
      mImpactPointTrackers[i]->mNumEventsTriggered = 0;
   }

   mAliveImpactPointCount = count;
   mCurrentHP = 1.0f;
}

//============================================================================
// BDamageTracker::isDestroyed
//============================================================================
bool BDamageTracker::isDestroyed(long impactPointIndex) const
{
//   syncUnitDetailCode("BDamageTracker::isDestroyed");

   // MS 6/24/2005: special handling for "default" bone... yeah...
   if(impactPointIndex == -1)
      return false;

   const BImpactPointTracker* pIPT = getImpactPointTracker(impactPointIndex);
   if(!pIPT)
      return true;

   const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(mDamageTemplateID);
   if(!pDT)
      return true;

   const BDamageImpactPoint* pIP = pDT->getImpactPoint(impactPointIndex);
   if(!pIP)
      return true;

   //syncUnitDetailData("   number of parts destroyed=", pIPT->mNumEventsTriggered);
   //syncUnitDetailData("   partRef count=", pIPR->getPartReferenceCount());

   if(pIPT->mNumEventsTriggered >= pIP->getEventCount())
      return true;

   return false;
}

//============================================================================
// BDamageTracker::getImpactPointTracker
//============================================================================
BImpactPointTracker* BDamageTracker::getImpactPointTracker(long index) const
{
   BImpactPointTracker* pDefaultIPT = NULL;
   for(long i = 0; i < mImpactPointTrackers.getNumber(); i++)
   {
      BImpactPointTracker* pIPT = mImpactPointTrackers.get(i);
      if(pIPT)
      {
         if(pIPT->mImpactPointIndex == index)
            return pIPT;
         else if(pIPT->mImpactPointIndex == -1) // the "default" bone
            pDefaultIPT = pIPT;
      }
   }

   // if we didn't find one, use the default bone (this will return NULL if
   // that wasn't found either)
   return pDefaultIPT;
}

//============================================================================
// BDamageTracker::impactPointHit
//============================================================================
long BDamageTracker::impactPointHit(long impactPointIndex)
{
   //syncUnitDetailCode("BDamageTracker::impactPointHit");

   if(isDestroyed(impactPointIndex))
      return -1;

   BImpactPointTracker* pIPT = getImpactPointTracker(impactPointIndex);
   if(!pIPT)
      return -1;

   const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(mDamageTemplateID);
   if(!pDT)
      return -1;

   const BDamageImpactPoint* pIPR = pDT->getImpactPoint(impactPointIndex);
   if(!pIPR)
      return -1;

   pIPT->mNumEventsTriggered++;

   if(pIPT->mNumEventsTriggered == pIPR->getEventCount())
      mAliveImpactPointCount--;

   //syncUnitDetailData("   impactPointIndex=", impactPointIndex);
   //syncUnitDetailData("   number of parts destroyed=", pIPT->mNumEventsTriggered);

   return(pIPT->mNumEventsTriggered - 1);
}


//============================================================================
// BDamageTracker::getAliveEventCountForImpactPoint
//============================================================================
long BDamageTracker::getAliveEventCountForImpactPoint(long impactPointIndex)
{
   if(isDestroyed(impactPointIndex))
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BDamageTracker::getAliveEventCountForImpactPoint isDestroyed");
      #endif
      return 0;
   }

//-- FIXING PREFIX BUG ID 2409
   const BImpactPointTracker* pIPT = getImpactPointTracker(impactPointIndex);
//--
   if(!pIPT)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BDamageTracker::getAliveEventCountForImpactPoint no pIPT");
      #endif
      return 0;
   }

   const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(mDamageTemplateID);
   if(!pDT)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BDamageTracker::getAliveEventCountForImpactPoint no pDT");
      #endif
      return 0;
   }

   const BDamageImpactPoint* pIPR = pDT->getImpactPoint(impactPointIndex);
   if(!pIPR)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BDamageTracker::getAliveEventCountForImpactPoint no pIPR");
      #endif
      return 0;
   }

   long eventCount = pIPR->getEventCount();
   long aliveEventCount = eventCount - pIPT->mNumEventsTriggered;

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BDamageTracker::getAliveEventCountForImpactPoint eventCount", eventCount);
      syncUnitDetailData("BDamageTracker::getAliveEventCountForImpactPoint mNumEventsTriggered", pIPT->mNumEventsTriggered);
   #endif

   return(aliveEventCount);
}



//============================================================================
// BDamageTracker::updatePercentageBaseDamage
//============================================================================
void BDamageTracker::updatePercentageBaseDamage(float newHP, const BDamageTemplate* pTemplate, BUnit *pUnit, const BVector* pOverrideForceDir, float force, bool onlyFinalEvent)
{
   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BDamageTracker::updatePercentageBaseDamage mCurrentHP", mCurrentHP);
      syncUnitDetailData("BDamageTracker::updatePercentageBaseDamage newHP", newHP);
   #endif

   if(mCurrentHP == newHP)
      return;

   if(newHP < 0.0f)
      newHP = 0.0f;

   if(mCurrentHP == newHP)
      return;


   updateModelRenderMaskIfNeeded(newHP, mCurrentHP, pUnit);


   long totalPercentageBasedEvents = pTemplate->getPercentageBasedEventCount();

   long newPercentageBasedEvents = ceil(newHP * (totalPercentageBasedEvents - 1));
   long currentPercentageBasedEvents = ceil(mCurrentHP * (totalPercentageBasedEvents - 1));

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BDamageTracker::updatePercentageBaseDamage currentPercentageBasedEvents", currentPercentageBasedEvents);
      syncUnitDetailData("BDamageTracker::updatePercentageBaseDamage newPercentageBasedEvents", newPercentageBasedEvents);
   #endif

   if(newPercentageBasedEvents < currentPercentageBasedEvents)
   {
      // We are taking damage - need to execute events
      //
      for(long i = currentPercentageBasedEvents; i > newPercentageBasedEvents; i--)
      {
         const BDamageEvent* pEvent = pTemplate->getPercentageBasedEvent(totalPercentageBasedEvents - i);

         if(pEvent)
         {
            // Execute all actions on this event
            long actioncount = pEvent->getActionCount();

            #ifdef SYNC_UnitDetail
               syncUnitDetailData("BDamageTracker::updatePercentageBaseDamage actioncount", actioncount);
            #endif

            for (long actionindex = 0; actionindex < actioncount; actionindex++)
            {
               const BDamageAction* pAction = pEvent->getAction(actionindex);

               // VAT: 11/06/08 - hack so that we don't do unnecessary damage actions
               // if we've specified that we only want final events, 
               // we only execute final events or swap / hide part events 
               // (so that final events can be processed correctly)
               if (onlyFinalEvent && (i != newPercentageBasedEvents + 1) && pAction->getType() != BDamageAction::cSwapPart && pAction->getType() != BDamageAction::cHidePart)
                  continue;

               pAction->execute(pUnit, true, NULL, force, true, pOverrideForceDir);
            }
         }
      }
   }
   else if (newPercentageBasedEvents > currentPercentageBasedEvents)
   {
      // We are healing - need to restore events
      //
      for(long i = currentPercentageBasedEvents; i < newPercentageBasedEvents; i++)
      {
         const BDamageEvent* pEvent = pTemplate->getPercentageBasedEvent(totalPercentageBasedEvents - i - 1);

         if(pEvent)
         {
            // Undo Execute all actions on this event
            long actioncount = pEvent->getActionCount();

            #ifdef SYNC_UnitDetail
               syncUnitDetailData("BDamageTracker::updatePercentageBaseDamage actioncount", actioncount);
            #endif

            for (long actionindex = 0; actionindex < actioncount; actionindex++)
            {
               const BDamageAction* pAction = pEvent->getAction(actionindex);

               pAction->undoExecuteSilent(pUnit);
            }
         }
      }
   }

   mCurrentHP = newHP;
}


//============================================================================
// BDamageTracker::updatePercentageBaseDamageSilent
//============================================================================
void BDamageTracker::updatePercentageBaseDamageSilent(float newHP, const BDamageTemplate* pTemplate, BUnit *pUnit)
{
   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BDamageTracker::updatePercentageBaseDamageSilent mCurrentHP", mCurrentHP);
      syncUnitDetailData("BDamageTracker::updatePercentageBaseDamageSilent newHP", newHP);
   #endif

   if(mCurrentHP == newHP)
      return;

   if(newHP < 0.0f)
      newHP = 0.0f;

   if(mCurrentHP == newHP)
      return;

   if(mCurrentHP > newHP)
   {
      updateModelRenderMaskIfNeeded(newHP, mCurrentHP, pUnit);
   }

   long totalPercentageBasedEvents = pTemplate->getPercentageBasedEventCount();

   long newPercentageBasedEvents = ceil(newHP * (totalPercentageBasedEvents - 1));
   long currentPercentageBasedEvents = ceil(mCurrentHP * (totalPercentageBasedEvents - 1));

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BDamageTracker::updatePercentageBaseDamageSilent currentPercentageBasedEvents", currentPercentageBasedEvents);
      syncUnitDetailData("BDamageTracker::updatePercentageBaseDamageSilent newPercentageBasedEvents", newPercentageBasedEvents);
   #endif

   if(newPercentageBasedEvents < currentPercentageBasedEvents)
   {
      // We are taking damage - need to execute events
      //
      for(long i = currentPercentageBasedEvents; i > newPercentageBasedEvents; i--)
      {
         const BDamageEvent* pEvent = pTemplate->getPercentageBasedEvent(totalPercentageBasedEvents - i);

         if(pEvent)
         {
            // Execute all actions on this event
            long actioncount = pEvent->getActionCount();

            #ifdef SYNC_UnitDetail
               syncUnitDetailData("BDamageTracker::updatePercentageBaseDamageSilent actioncount", actioncount);
            #endif

            for (long actionindex = 0; actionindex < actioncount; actionindex++)
            {
               const BDamageAction* pAction = pEvent->getAction(actionindex);

               pAction->doExecuteSilent(pUnit);
            }
         }
      }
   }
   else if (newPercentageBasedEvents > currentPercentageBasedEvents)
   {
      // We are healing - need to restore events
      //
      for(long i = currentPercentageBasedEvents; i < newPercentageBasedEvents; i++)
      {
         const BDamageEvent* pEvent = pTemplate->getPercentageBasedEvent(totalPercentageBasedEvents - i - 1);

         if(pEvent)
         {
            // Undo Execute all actions on this event
            long actioncount = pEvent->getActionCount();

            #ifdef SYNC_UnitDetail
               syncUnitDetailData("BDamageTracker::updatePercentageBaseDamageSilent actioncount", actioncount);
            #endif

            for (long actionindex = 0; actionindex < actioncount; actionindex++)
            {
               const BDamageAction* pAction = pEvent->getAction(actionindex);

               pAction->undoExecuteSilent(pUnit);
            }
         }
      }
   }

   if(mCurrentHP < newHP)
   {
      updateModelRenderMaskIfNeeded(newHP, mCurrentHP, pUnit);
   }

   // If we are completly heal them heal all the parts and fix impact points
   if(newHP == 1.0f)
   {
      BVisual *pVisual = pUnit->getVisual();
      if(pVisual)
      {
         // Remove all user attachments
         int numAttachments = pVisual->mAttachments.getNumber();
         for (int i = numAttachments - 1; i >= 0; i--)
         {
            BVisualItem* pCurAttachment = pVisual->mAttachments[i];
            if(pCurAttachment && pCurAttachment->getFlag(BVisualItem::cFlagUser))
            {
               BVisualItem::releaseInstance(pCurAttachment);
               pVisual->mAttachments.removeIndex(i, false);
            }
         }

      }

      reset();
   }

   mCurrentHP = newHP;
}



//============================================================================
// BDamageTracker::updateModelRenderMaskIfNeeded
//============================================================================
void BDamageTracker::updateModelRenderMaskIfNeeded(float newHP, float currentHP, BUnit *pUnit)
{
   // This will change the render mask between the intact undamage state and the damage version
   // that has yet to be damage.

   if((mCurrentHP == 1.0) && (newHP < 1.0f))
   {      
      BVisual *pVisual = pUnit->getVisual();
      if(pVisual)
      {
         BGrannyInstance *pGrannyInstance = pVisual->getGrannyInstance();
         if (pGrannyInstance)
            pGrannyInstance->setMeshRenderMaskToDamageNoHitsState();
      }
   }
   else if((mCurrentHP < 1.0) && (newHP == 1.0f))
   {
      BVisual *pVisual = pUnit->getVisual();
      if(pVisual)
      {
         BGrannyInstance *pGrannyInstance = pVisual->getGrannyInstance();
         if (pGrannyInstance)
            pGrannyInstance->setMeshRenderMaskToUndamageState();
      }
   }
}

//============================================================================
// BDamageTemplate::receiveEvent
//============================================================================
void BDamageTracker::shatterDeath(const BDamageTemplate* pTemplate, BUnit *pUnit, const BVector* pOverrideForceDir, float force)
{
   if (!pUnit || !pTemplate)
      return;

   updateModelRenderMaskIfNeeded(0.0f, mCurrentHP, pUnit);

   const BDamageEvent* pEvent = pTemplate->getShatterDeathEvent();

   if(pEvent)
   {
      // Execute all actions on this event
      long actioncount = pEvent->getActionCount();

#ifdef SYNC_UnitDetail
      syncUnitDetailData("BDamageTracker::shatterDeath actioncount", actioncount);
#endif

      for (long actionindex = 0; actionindex < actioncount; actionindex++)
      {
         const BDamageAction* pAction = pEvent->getAction(actionindex);

         pAction->execute(pUnit, true, NULL, force, true, pOverrideForceDir);
      }
   }

   mCurrentHP = 0.0f;
}

//============================================================================
// BDamageTemplate::updateThrownParts
//============================================================================
void BDamageTracker::updateThrownParts(float elapsed)
{
   if (mTimeToClearParts > 0.0f)
   {
      mTimeToClearParts -= elapsed;
      if (mTimeToClearParts <= 0.0f)
         mRecentThrownParts.clear();
   }
}

//============================================================================
// BDamageTemplate::addThrownPart
//============================================================================
void BDamageTracker::addThrownPart(BEntityID newPart)
{
   mRecentThrownParts.add(newPart);
   mTimeToClearParts = 1.0f;
}


/*
//=============================================================================
// BDamageTracker::fillMeshRenderMask
//=============================================================================
void BDamageTracker::fillMeshRenderMask(BBitArray& mask)
{
   const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(mDamageTemplateID);
   if(!pDT)
      return;

   long impactPointCount = pDT->getImpactPointCount();
   for(long i = 0; i < impactPointCount && i < mImpactPointTrackers.getNumber(); i++)
   {
      const BImpactPointTracker* pIPT = mImpactPointTrackers[i];
      if(!pIPT)
         return;

      const BDamageImpactPoint* pIPR = pDT->getImpactPointByIndex(i);
      if(!pIPR)
         return;

      long partcount = pIPR->getPartReferenceCount();

      for (long partindex = 0; partindex < partcount && partindex < pIPT->mNumEventsTriggered; partindex++)
      {
         const BDamageAction *pPart = pIPR->getPartReference(partindex);
         if (pPart)
         {
            long meshIndex = pPart->getMeshIndex();

            if (mask.getNumber() > meshIndex && mask.isBitSet(meshIndex))
               mask.clearBit(meshIndex);
         }
      }
   }
}
*/

//============================================================================
//============================================================================
bool BDamageTracker::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, long, mDamageTemplateID);
   GFWRITECLASSPTRARRAY(pStream, saveType, BImpactPointTracker, mImpactPointTrackers, uint8, 200);
   GFWRITEVAR(pStream, long, mAliveImpactPointCount);
   GFWRITEVAR(pStream, float, mCurrentHP);
   return true;
}

//============================================================================
//============================================================================
bool BDamageTracker::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, long, mDamageTemplateID);
   GFREADCLASSPTRARRAY(pStream, saveType, BImpactPointTracker, mImpactPointTrackers, uint8, 200);
   GFREADVAR(pStream, long, mAliveImpactPointCount);
   GFREADVAR(pStream, float, mCurrentHP);
   return true;
}

//============================================================================
//============================================================================
bool BImpactPointTracker::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, long, mImpactPointIndex);
   GFWRITEVAR(pStream, long, mNumEventsTriggered);
   return true;
}

//============================================================================
//============================================================================
bool BImpactPointTracker::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, long, mImpactPointIndex);
   GFREADVAR(pStream, long, mNumEventsTriggered);
   return true;
}

//============================================================================
// BDamageEvent
//============================================================================

//============================================================================
// BDamageEvent::BDamageEvent
//============================================================================
BDamageEvent::BDamageEvent()
{
   reset();
}


//============================================================================
// BDamageEvent::~BDamageEvent
//============================================================================
BDamageEvent::~BDamageEvent()
{
   reset();
}


//============================================================================
// BDamageEvent::getAction
//============================================================================
const BDamageAction* BDamageEvent::getAction( long index ) const
{
   if (index < 0 || index >= mActions.getNumber())
      return NULL;

   return mActions.get(index);
}


//============================================================================
// BDamageEvent::reset
//============================================================================
void BDamageEvent::reset( void )
{
   long count = mActions.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mActions.get(i);
   }
}


//============================================================================
// BDamageEvent::load
//============================================================================
bool BDamageEvent::load(const BXMLNode &root, long modelIndex, bool onlyAdditive)
{
   // Read all actions for this event
   //-- we assume that only actions are children of this type of node
   //
   long childCount = root.getNumberChildren();
   for (long i = 0; i < childCount; i++)
   {
      BXMLNode child(root.getChild(i));

      BDamageAction *pDamageAction = NULL;

      BXMLAttribute attrib;
      if (child.getAttribute("type", &attrib))
      {
         BSimString typeName;
         attrib.getValue(typeName);

         if (typeName.compare(L"swappart")==0)
         {
            if (onlyAdditive)
            {
               gConsoleOutput.warning("BDamageEvent::load - %s was trying to add a swappart damage action in a damage basis that only supports additive actions.", root.getReader()->getFilename().getPtr());
               BFAIL("This damage basis only supports additive damage actions! Cannot add swappart! Check the XFS output for more info.");
               continue;
            }
            pDamageAction = new BDamageActionSwapPart();
         }
         else if (typeName.compare(L"hidepart")==0)
         {
            if (onlyAdditive)
            {
               gConsoleOutput.warning("BDamageEvent::load - %s was trying to add a hidepart damage action in a damage basis that only supports additive actions.", root.getReader()->getFilename().getPtr());
               BFAIL("This damage basis only supports additive damage actions! Cannot add hidepart! Check the XFS output for more info.");
               continue;
            }
            pDamageAction = new BDamageActionHidePart();
         }
         else if (typeName.compare(L"multiframetextureindex")==0)
         {
            if (onlyAdditive)
            {
               gConsoleOutput.warning("BDamageEvent::load - %s was trying to add a multiframetextureindex damage action in a damage basis that only tsupports additive actions.", root.getReader()->getFilename().getPtr());
               BFAIL("This damage basis only supports additive damage actions! Cannot add multiframetextureindex! Check the XFS output for more info.");
               continue;
            }
            pDamageAction = new BDamageActionMultiframeTextureIndex();
         }
         else if (typeName.compare(L"throwpart")==0)
         {
            pDamageAction = new BDamageActionThrowPart();
         }
         else if (typeName.compare(L"throwattachment")==0)
         {
            pDamageAction = new BDamageActionThrowAttachment();
         }
         else if (typeName.compare(L"attachparticle")==0)
         {
            pDamageAction = new BDamageActionAttachParticle();
         }
         else if (typeName.compare(L"playsound")==0)
         {
            pDamageAction = new BDamageActionPlaySound();
         }
         else
         {
            BGrannyModel *pGrannyModel = gGrannyManager.getModel(modelIndex, true);
            BString errorMsg;
            BSimString text;
            errorMsg.format(BString("Invalid type \"%s\" in part declaration for model \"%s\", this probably means the bone was not defined in the unit's anim XML."), BStrConv::toA(typeName), BStrConv::toA(pGrannyModel->getFilename()));
            child.logInfo(BStrConv::toA(errorMsg));
            return (false);
         }
      }

      if (pDamageAction)
      {
         if (!pDamageAction->load(child, modelIndex))
         {
            child.logInfo("Unable to load part reference : %s", BStrConv::toA(child.getName()));
            delete pDamageAction;
            continue;
         }

         mActions.add(pDamageAction);
      }
   }

   return(true);
}






//============================================================================
// BDamageImpactPoint
//============================================================================

//============================================================================
// BDamageImpactPoint::BDamageImpactPoint
//============================================================================
BDamageImpactPoint::BDamageImpactPoint() 
{
  reset();
}


//============================================================================
// BDamageImpactPoint::~BDamageImpactPoint
//============================================================================
BDamageImpactPoint::~BDamageImpactPoint()
{
   reset();
}


//============================================================================
// BDamageImpactPoint::addEvent
//============================================================================
long BDamageImpactPoint::addEvent(BDamageEvent *pEvent)
{
   return mEvents.add(pEvent);
}



//============================================================================
// BDamageImpactPoint::reset
//============================================================================
void BDamageImpactPoint::reset( void )
{
   long count = mEvents.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mEvents.get(i);
   }

   mEvents.clear();

   mIndex = -1;
   mPrereqID = -1;
   mReplaceShapeID = -1;
   mDisableAfterDestruction = false;
   mFadeAfterMotionStops = true;
   mMotionStopFadeTime = 1.0f;
   mMotionStopFadeDelay = 3.5f;
   mImpactEffectOffset = 0.5f;
   mSplashEffectHandle = NULL;
}

//============================================================================
// BDamageImpactPoint::load
//============================================================================
bool BDamageImpactPoint::load( const BXMLNode &root, long modelIndex)
{
  
   BXMLAttribute attrib;
   if(!root.getAttribute("bone", &attrib))
   {
      root.logInfo("ImpactPoint definitions must specify a bone! : %s", BStrConv::toA(root.getName()));
      gConsoleOutput.output(cMsgError, "BDamageImpactPoint::load: ImpactPoint definitions must specify a bone! : %s", BStrConv::toA(root.getName()));
      return (false);
   }


   //-- is this the special 'default' bone
   BSimString text;
   if (attrib.getValue(text).compare(L"default") == 0)
   {
      setIndex(-1);
   }
   else
   {
      BGrannyModel *pGrannyModel = gGrannyManager.getModel(modelIndex, true);

      BSimString text;
      long index = -1;

      if(pGrannyModel) 
      {
         index = pGrannyModel->getBoneHandle(BStrConv::toA(attrib.getValue(text)));
      }

      if (index == -1 )
      {
         BSimString name;
         root.logInfo("Couldn't find bone: %s", BStrConv::toA(attrib.getValue(name)));
         gConsoleOutput.output(cMsgError, "BDamageImpactPoint::load: Couldn't find bone: %s", BStrConv::toA(attrib.getValue(name)));
         return (false);
      }



      setIndex(index);


      //-- there may be a pre-req on this impact point
      //-- perhaps we have declared and impact type
      BSimString stringValue;
      if (root.getAttribValue("prereq", &stringValue))
      {
         mPrereqID = gTempNames.add(stringValue);
      }


      //-- we may have an optional replacement shape 
      if (root.getAttribValue("shapeReplace", &stringValue))
      {
         if (!stringValue.isEmpty())
            mReplaceShapeID = gPhysics->getShapeManager().getOrCreate(stringValue); 
      }

      //-- we may have an optional disableAfterDestruction flag
      //-- the default value for this flag is false
      long lValue = 0;
      root.getAttribValueAsLong("disableAfterDestruction", lValue);
      if (lValue == 1)
         mDisableAfterDestruction = true;
      else
         mDisableAfterDestruction = false;


      if(root.getAttribute("noFadeAfterMotionStops"))
         mFadeAfterMotionStops = false;

      root.getAttribValueAsFloat("motionStopFadeTime", mMotionStopFadeTime);
      root.getAttribValueAsFloat("motionStopFadeDelay", mMotionStopFadeDelay);
      root.getAttribValueAsFloat("impactEffectOffset", mImpactEffectOffset);

      if(root.getAttribValue("partImpactSoundSet", &stringValue) && !stringValue.isEmpty())
         mPartImpactSoundSet = stringValue;
      if(root.getAttribValue("heavyPartImpactSoundSet", &stringValue) && !stringValue.isEmpty())
         mHeavyPartImpactSoundSet = stringValue;

      // SAT change
      /*
      if(root.getAttribValue("splashEffect", &stringValue))
      {
         if(pString)
            gModelManager->getParticleSystemManager()->getParticleSet(stringValue, &mSplashEffectHandle);
      }
      */
   }
  


   //-- now process each event
   //-- we assume that only events are children of this type of node
   long childCount = root.getNumberChildren();
   for (long i = 0; i < childCount; i++)
   {
      BXMLNode child(root.getChild(i));
      BDamageEvent *pDamageEvent = new BDamageEvent();
      if (pDamageEvent)
      {
         if (!pDamageEvent->load(child, modelIndex))
         {
            child.logInfo("Unable to load part event : %s", BStrConv::toA(child.getName()));
            delete pDamageEvent;
            continue;
         }
   
         // scan to see if this has a throw part damage action
         int actionCount = pDamageEvent->getActionCount();
         for (int j = 0; j < actionCount; ++j)
         {
            const BDamageActionThrowPart* pThrowPartAction = (const BDamageActionThrowPart*)pDamageEvent->getAction(j);
            if (pThrowPartAction)
               mFinalThrowPartEventIndex = i;
         }

         mEvents.add(pDamageEvent);
      }
   }

   return (true);
}

//============================================================================
// BDamageImpactPoint::getEvent
//============================================================================
const BDamageEvent* BDamageImpactPoint::getEvent( long index ) const
{
   if (index < 0 || index >= mEvents.getNumber())
      return NULL;

   return mEvents.get(index);
}

//============================================================================
// BDamageTemplate
//============================================================================

//============================================================================
// BDamageTemplate::BDamageTemplate
//============================================================================
BDamageTemplate::BDamageTemplate() :
   mModelIndex(-1),
   mIsLoaded(false),
   mManagerIndex(-1),
   mShatterDeathEvent(NULL)
{
   reset();
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit();
#endif
}


//============================================================================
// BDamageTemplate::~BDamageTemplate
//============================================================================
BDamageTemplate::~BDamageTemplate()
{
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);
#endif

   reset();

#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit();
#endif
}

//============================================================================
// BDamageTemplate::getImpactPoint
//============================================================================
const BDamageImpactPoint * BDamageTemplate::getImpactPoint( long impactIndex ) const
{
//-- FIXING PREFIX BUG ID 2412
   const BDamageImpactPoint* pDefaultIPR = NULL;
//--

   long count = mImpactPoints.getNumber();
   for(long i = 0; i < count; i++)
   {
      BDamageImpactPoint* pIPR = mImpactPoints.get(i);
      if(pIPR)
      {
         if(pIPR->getIndex() == impactIndex)
            return pIPR;
         else if(pIPR->getIndex() == -1) // the "default" bone
            pDefaultIPR = pIPR;
      }
   }

   // if we didn't find one, use the default bone (this will return NULL if
   // that wasn't found either)
   return pDefaultIPR;
}

//============================================================================
// BDamageTemplate::getImpactPointByIndex
//============================================================================
const BDamageImpactPoint * BDamageTemplate::getImpactPointByIndex( long index ) const
{
   if(!mImpactPoints.validIndex(index))
      return NULL;

   return mImpactPoints[index];
}


//============================================================================
// BDamageTemplate::getPercentageBasedEvent
//============================================================================
const BDamageEvent* BDamageTemplate::getPercentageBasedEvent( long index ) const
{
   if(!mPercentageBasedEvents.validIndex(index))
      return NULL;

   return mPercentageBasedEvents[index];
}


//============================================================================
// BDamageTemplate::getCryoPercentageBasedEvent
//============================================================================
const BDamageEvent* BDamageTemplate::getCryoPercentageBasedEvent( long index ) const
{
   if(!mCryoPercentageBasedEvents.validIndex(index))
      return NULL;

   return mCryoPercentageBasedEvents[index];
}


//============================================================================
// BDamageTemplate::chooseClosestNonDestroyedImpactPoint
//============================================================================
long BDamageTemplate::chooseClosestImpactPoint( const BDamageTracker* pTracker, BVisual* pVisual, BVector& impactPosModelSpace, bool includeDestroyed ) const
{
   if (!includeDestroyed && !pTracker)
      return (-1);

   //-- we want to count all the impact points that are still valid
   long numvalid = 0;
   long count = mImpactPoints.getNumber();

   if(!includeDestroyed && count != pTracker->getImpactPointTrackerCount())
      return -1;

   //-- reinitialize the scratchpad 
   mScratchPad.setNumber(count);
   mScratchPad.setNumber(0);

   for (long i = 0; i < count; i++)
   {
      if (mImpactPoints[i])
      {
         //-- if this impact point has already been completely hit
         if (!includeDestroyed && pTracker->isDestroyed(mImpactPoints[i]->getIndex()))
            continue;

         //syncUnitDetailData("BDamageTemplate::chooseRandomImpactPoint, adding i=", i);

         numvalid++;
         mScratchPad.add(BYTE(i));
      }
   }

   //syncUnitDetailData("BDamageTemplate::chooseRandomImpactPoint, numvalid=", numvalid);

   if (numvalid <= 0)
      return (-1);



   // Find the closest to the given position
   float closestDistSqr = cMaximumFloat;
   long closestIndex = -1;

   for(int i = 0; i < numvalid; i++)
   {
      long newIndex = mScratchPad[i];

      BVector bonePosition;
      pVisual->getBone(mImpactPoints[newIndex]->getIndex(), &bonePosition);

      float newDistanceSqr = impactPosModelSpace.distanceSqr(bonePosition);

      if(newDistanceSqr < closestDistSqr)
      {
         closestDistSqr = newDistanceSqr;
         closestIndex = newIndex;
      }
   }


   if(closestIndex == -1)
      return -1;
   else
      return mImpactPoints.get(closestIndex)->getIndex();
}


//============================================================================
// BDamageTemplate::reset
//============================================================================
void BDamageTemplate::reset( void )
{
   long count = mImpactPoints.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mImpactPoints.get(i);
   }
   mImpactPoints.clear();

   count = mPercentageBasedEvents.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mPercentageBasedEvents.get(i);
   }
   mPercentageBasedEvents.clear();

   count = mCryoPercentageBasedEvents.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mCryoPercentageBasedEvents.get(i);
   }
   mCryoPercentageBasedEvents.clear();

   if (mShatterDeathEvent)
   {
      delete mShatterDeathEvent;
      mShatterDeathEvent = NULL;
   }

   mIsLoaded = false;
}


//============================================================================
// BDamageTemplate::load
//============================================================================
bool BDamageTemplate::load()
{
#ifdef SYNC_UnitDetail
   syncUnitDetailData("BDamageTemplate::load mIsLoaded", mIsLoaded);
   syncUnitDetailData("BDamageTemplate::load mFileName", mFileName.getPtr());
#endif
   if (mIsLoaded)
      return true;

   mIsLoaded = true;

   BSimString mFileNameWithExtension = mFileName;
   strPathAddExtension(mFileNameWithExtension, "dmg");

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BDamageTemplate::load full file path", mFileNameWithExtension);
   #endif


   // Reloading setup
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);

   BReloadManager::BPathArray paths;
   BString fullpath;
   gFileManager.constructQualifiedPath(cDirArt, mFileNameWithExtension, fullpath);
   paths.pushBack(fullpath);
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle);
#endif

   BXMLReader reader;
   bool ok = reader.load(cDirArt, mFileNameWithExtension);
   if (!ok)
   {
       #ifdef SYNC_UnitDetail
         syncUnitDetailData("BDamageTemplate::load failed to load file", mFileNameWithExtension);
      #endif
      BString errMsg;
      errMsg.format("BDamageTemplate ERROR: %s didn't load.", mFileName.getPtr());
      {setBlogError(4154); blogerrortrace("%s", errMsg.getPtr());}
      return false;
   }

   //-- Get the root node.
   BXMLNode rootNode(reader.getRootNode());
   if (rootNode.getNumberChildren() <= 0)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BDamageTemplate::load no root node");
      #endif
      BString errMsg;
      errMsg.format("BDamageTemplate ERROR: %s has not root node.", mFileName.getPtr());
      {setBlogError(4155); blogerrortrace("%s", errMsg.getPtr());}
      return false;
   }

   if (!load(rootNode, mModelIndex))
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BDamageTemplate::load failed");
      #endif
      return false;
   }

   return true;
}


//============================================================================
// BDamageTemplate::reload
//============================================================================
bool BDamageTemplate::reload()
{
   if (mFileName.isEmpty())
      return false;
      
   reset();

   load();  


#ifndef BUILD_FINAL
   gDamageTemplateManager.reInitDamageTrackers(mManagerIndex);
#endif

   return true;
}


//============================================================================
// BDamageTemplate::load
//============================================================================
bool BDamageTemplate::load(const BXMLNode &root, long modelIndex)
{
   //-- make sure we are clear
   reset();

   //-- does this look like a damage template?
   if (root.getName().compare("damagetemplate") != 0)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BDamageTemplate::load invalid root");
      #endif

      BSimString text;
      root.logInfo("Invalid node specified as root of damage template: %s", BStrConv::toA(root.getText(text)));
      return (false);
   }

   long numChildren = root.getNumberChildren();
   for(long i = 0; i < numChildren; i++)
   {
      BXMLNode childNode(root.getChild(i));
      if (!childNode)
      {
         BSimString text;
         #ifdef SYNC_UnitDetail
            syncUnitDetailData("BDamageTemplate::load couldn't get model node", BStrConv::toA(root.getText(text)));
         #endif

         root.logInfo("Unable to get the model node from this damage template : %s", BStrConv::toA(root.getText(text)));
         return (false);
      }

      const BPackedString childNodeName(childNode.getName());
      if(childNodeName=="percentagebased")
      {
         //-- now process each event
         //-- we assume that only events are children of this type of node
         long childCount = childNode.getNumberChildren();
         for (long i = 0; i < childCount; i++)
         {
            BXMLNode child(childNode.getChild(i));
            BDamageEvent *pDamageEvent = new BDamageEvent();
            if (pDamageEvent)
            {
               if (!pDamageEvent->load(child, modelIndex))
               {
                  child.logInfo("Unable to load event : %s", BStrConv::toA(child.getName()));
                  delete pDamageEvent;
                  continue;
               }

               mPercentageBasedEvents.add(pDamageEvent);
            }
         }
      }
      else if(childNodeName=="cryopercentagebased")
      {
         //-- now process each event
         //-- we assume that only events are children of this type of node
         long childCount = childNode.getNumberChildren();
         for (long i = 0; i < childCount; i++)
         {
            BXMLNode child(childNode.getChild(i));
            BDamageEvent *pDamageEvent = new BDamageEvent();
            if (pDamageEvent)
            {
               if (!pDamageEvent->load(child, modelIndex, true))
               {
                  child.logInfo("Unable to load event : %s", BStrConv::toA(child.getName()));
                  delete pDamageEvent;
                  continue;
               }

               mCryoPercentageBasedEvents.add(pDamageEvent);
            }
         }
      }
      else if (childNodeName=="impactpointbased")
      {
         loadImpactPoints(childNode, modelIndex, mImpactPoints);
      }
      else if (childNodeName=="shatterdeathevent")
      {
         mShatterDeathEvent = new BDamageEvent();
         if (mShatterDeathEvent)
         {
            if (!mShatterDeathEvent->load(childNode, modelIndex, false))
            {
               childNode.logInfo("Unable to load event : %s", BStrConv::toA(childNode.getName()));
               delete mShatterDeathEvent;
               mShatterDeathEvent = NULL;
            }
         }
      }
   }
   return (true);
}

//============================================================================
// BDamageTemplate::loadImpactPoints
//============================================================================
bool BDamageTemplate::loadImpactPoints(const BXMLNode &rootNode, long modelIndex, BDynamicSimArray<BDamageImpactPoint*> &impactPoints)
{
   gImpactPointNames.setNumber(0);
   gTempNames.setNumber(0);

   long impactPointCount = rootNode.getNumberChildren();
   for (long i=0; i < impactPointCount; i++)
   {
      BXMLNode impactPointNode(rootNode.getChild(i));
      
      BDamageImpactPoint *pRef = new BDamageImpactPoint();
      if (pRef)
      {
         if (!pRef->load(impactPointNode, modelIndex))
         {
            impactPointNode.logInfo("Unable to load impact point reference : %s", BStrConv::toA(impactPointNode.getName()));
            delete pRef;
            continue;
         }

         impactPoints.add(pRef);

         //-- there may be an optional name for this impact point
         //-- we store this so we can turn it into an index later
         //-- we also store this in a place where we can free the memory
         //-- after this load process
         BSimString impactPointName;
         if (!impactPointNode.getAttribValue("name", &impactPointName))
         {
            //--  just use the bone name
            if (!impactPointNode.getAttribValue("bone", &impactPointName))
            {
               impactPointNode.logInfo("Unable to create name for impact point");
               continue;
            }
           
         }
 
         gImpactPointNames.add(impactPointName);
         
      }

   }


   //-- now fix up any name references
   impactPointCount = impactPoints.getNumber();
   for (long i=0; i < impactPointCount; i++)
   {
      BDamageImpactPoint *pRef = impactPoints.get(i);
      //-- fixup the prereq id
      //-- the current id contains a lookup into the temporary names list
      //-- that can be used to find the correct entry in the impact point names list
      if (pRef)
      {
         if (pRef->getPrereqID() != -1)
         {
            pRef->setPrereqID(gImpactPointNames.find(gTempNames.get(pRef->getPrereqID())));
         }
      }
    
   }

   gImpactPointNames.clear();
   gTempNames.clear();


   return true;
}


//============================================================================
// BDamageTemplate::receiveEvent
//============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BDamageTemplate::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (event.mEventClass == cEventClassReloadNotify)
   {
//-- FIXING PREFIX BUG ID 2413
      const BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
//--
      gConsoleOutput.status("Reloading damage template file: %s", pPayload->mPath.getPtr());

      // Reload animation
      reload();
   }

   return false;
}
#endif
//============================================================================
// eof: damagetemplate.cpp
//============================================================================



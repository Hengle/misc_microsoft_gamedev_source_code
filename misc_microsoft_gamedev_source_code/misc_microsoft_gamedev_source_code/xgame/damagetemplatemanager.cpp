//============================================================================
// damagetemplatemananger.cpp
//
// Copyright (c) 2004-2007 Ensemble Studios
//============================================================================

#include "common.h"
#include "damagetemplate.h"
#include "damagetemplatemanager.h"

#include "world.h"
#include "visual.h"
#include "grannyinstance.h"
#include "physicsinfo.h"
#include "physicsobjectblueprint.h"

#include <Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppUtility.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h>
#include <Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h>

BDamageTemplateManager gDamageTemplateManager;


//============================================================================
// BDamageTemplateManager
//============================================================================

//============================================================================
// BDamageTemplateManager::BDamageTemplateManager
//============================================================================
BDamageTemplateManager::BDamageTemplateManager()
{
   reset();
}


//============================================================================
// BDamageTemplateManager::~BDamageTemplateManager
//============================================================================
BDamageTemplateManager::~BDamageTemplateManager()
{
   reset();
}


//============================================================================
// BDamageTemplateManager::init
//============================================================================
void BDamageTemplateManager::init(BWorld *pWorld)
{
   if(mBreakOffPartsUtil)
   {
      mBreakOffPartsUtil->removeReference();
   }

   BPhysicsWorld *pPhysicsWorld = pWorld->getPhysicsWorld();

   pPhysicsWorld->markForWrite(true);
	mBreakOffPartsUtil = new hkpBreakOffPartsUtil( pPhysicsWorld->getHavokWorld(), this );

   pPhysicsWorld->markForWrite(false);
}


//============================================================================
// BDamageTemplateManager::reset
//============================================================================
void BDamageTemplateManager::reset( void )
{
   long count = mDamageTemplates.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mDamageTemplates.get(i);
   }

   mDamageTemplates.clear();
   mDamageNameTable.clearAll();

   // Remove reference
   if (mBreakOffPartsUtil)
   {
      mBreakOffPartsUtil->removeReference();
      mBreakOffPartsUtil = NULL;
   }
}

//============================================================================
// BDamageTemplateManager::getDamageTemplate
//============================================================================
const BDamageTemplate * BDamageTemplateManager::getDamageTemplate(long id) const
{
   #ifdef SYNC_UnitDetail
      if (!gSaveGame.isSaving() && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncUnitDetailData("BDamageTemplateManager::getDamageTemplate id", id);
      }
   #endif

   if (id < 0 || id >= mDamageTemplates.getNumber())
      return (NULL);

   BDamageTemplate *pTemplate = mDamageTemplates.get(id);

   /*
   // Make sure it's loaded
   if ((pTemplate != NULL) && (pTemplate->isLoaded() == false))
   {
      pTemplate->load();
   }
   */

   #ifdef SYNC_UnitDetail
      if (!gSaveGame.isSaving() && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncUnitDetailData("BDamageTemplateManager::getDamageTemplate template name", pTemplate ? pTemplate->getFileName().getPtr() : "NULL");
      }
   #endif

   return pTemplate;
}



//============================================================================
// BDamageTemplateManager::findDamageTemplate
//============================================================================
long BDamageTemplateManager::findDamageTemplate(const BCHAR_T* pFileName)
{
   #ifdef SYNC_UnitDetail
   if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
   {
      syncUnitDetailData("BDamageTemplateManager::findDamageTemplate pFileName", pFileName);
   }
   #endif
   
   // Clean path.
   BSimString standardizedFilename;
   standardizedFilename.standardizePathFrom(pFileName);

   // Look up in name table.
   short index=-1;
   bool found=mDamageNameTable.find(standardizedFilename, &index);

   #ifdef SYNC_UnitDetail
   if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
   {
      syncUnitDetailData("BDamageTemplateManager::findDamageTemplate mDamageNameTable.size", mDamageNameTable.numTags());
      syncUnitDetailData("BDamageTemplateManager::findDamageTemplate found", found);
   }
   #endif

   // If we found a match, give back the index.
   if(found)
      return(index);

   // No match, give back -1.
   return(-1);
}

//============================================================================
// BDamageTemplateManager::createDamageTemplate
//============================================================================
long BDamageTemplateManager::createDamageTemplate(const BCHAR_T* pFileName, long modelindex)
{
   #ifdef SYNC_UnitDetail
      if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncUnitDetailData("BDamageTemplateManager::createDamageTemplate file name", pFileName);
         syncUnitDetailData("BDamageTemplateManager::createDamageTemplate modelindex", modelindex);
      }
   #endif

   // Allocate Damage.
   BDamageTemplate *pTemplate = new BDamageTemplate();
   if (!pTemplate)
   {
      BFAIL("Could not allocate damage template!");
      return (-1);
   }

   // Clean path.
   BSimString standardizedFilename;
   standardizedFilename.standardizePathFrom(pFileName);


   // Set data so it can be loaded on demand later
   pTemplate->setFileName(standardizedFilename);
   pTemplate->setModelIndex(modelindex);
   
   // Load the file
   if(!pTemplate->load())
   {
      #ifdef SYNC_UnitDetail
         if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncUnitDetailCode("BDamageTemplateManager::createDamageTemplate template load failed");
         }
      #endif
      delete pTemplate;      
      return(-1);
   }


   // Add to array.
   long index = mDamageTemplates.add(pTemplate);
#ifdef SYNC_UnitDetail
   if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
   {
      syncUnitDetailData("BDamageTemplateManager::createDamageTemplate file name", pFileName);
      syncUnitDetailData("BDamageTemplateManager::createDamageTemplate index", index);
   }
#endif

   pTemplate->setManagerIndex(index);

   // Add to hash table.
   mDamageNameTable.add(standardizedFilename, static_cast<short>(index));

   // Hand back index.
   return(index);
}

//============================================================================
// BDamageTemplateManager::getOrCreateDamageTemplate
//============================================================================
long BDamageTemplateManager::getOrCreateDamageTemplate(const BCHAR_T* pFileName, long modelindex)
{
   #ifdef SYNC_World
      if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncWorldData("BDamageTemplateManager::getOrCreateDamageTemplate", pFileName);
         syncWorldData("BDamageTemplateManager::getOrCreateDamageTemplate modelindex", modelindex);
      }
   #endif

   //SCOPEDSAMPLE(BDamageTemplateManager_getOrCreateDamageTemplate)
   // Check param.
   if(!pFileName || (modelindex == -1))
   {
      return(-1);
   }

   // Look for existing.
   long index=findDamageTemplate(pFileName);

   #ifdef SYNC_World
      if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncWorldData("BDamageTemplateManager::getOrCreateDamageTemplate index", index);
      }
   #endif

   // None existing, must create.
   if(index<0)
   {
      index=createDamageTemplate(pFileName, modelindex);
      #ifdef SYNC_World
         if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncWorldData("BDamageTemplateManager::getOrCreateDamageTemplate new index", index);
         }
      #endif
   }

   // Give back result.
   return(index);
}


#ifndef BUILD_FINAL
//============================================================================
// BDamageTemplateManager::reInitDamageTrackers
//============================================================================
void BDamageTemplateManager::reInitDamageTrackers(long index)
{
   // Iterate through all units and update their damage tracker if it is using the same template
   //

   //-- reset handle
   BEntityHandle handle = cInvalidObjectID;
   BUnit* pUnit = gWorld->getNextUnit(handle);
   while (pUnit)
   {
      if(pUnit->getProtoID() != gDatabase.getPOIDPhysicsThrownObject())
      {
         const BDamageTracker *pDamageTracker = pUnit->getDamageTracker();
         if(pDamageTracker && (pDamageTracker->getDamageTemplateID() == index))
         {
            pUnit->getProtoID();
            pUnit->reInitDamageTracker();
         }
      }

      pUnit = gWorld->getNextUnit(handle);
   }
}
#endif




//==============================================================================
// BDamageTemplateManager::breakOffSubPart
//==============================================================================
hkResult BDamageTemplateManager::breakOffSubPart( const ContactImpulseLimitBreachedEvent& event, hkArray<hkpShapeKey>& keysBrokenOffOut, hkpPhysicsSystem& systemOut ) 
{
	//	
	//	Remove the broken pieces from the car or landscape
	//
	hkpShapeKey brokenPieceKey = event.m_brokenPieceKey;
	hkpRigidBody* breakingBody = event.m_breakingBody;
	const hkpShape* newShape = HK_NULL;
	{
		hkpShape* shape = const_cast<hkpShape*>(breakingBody->getCollidable()->getShape());
		switch (shape->m_type )
		{
		case HK_SHAPE_LIST:
			{
				hkpListShape* list = static_cast<hkpListShape*>(shape);
				newShape = list->m_childInfo[brokenPieceKey].m_shape;

				hkpBreakOffPartsUtil::removeKeysFromListShape( breakingBody, &brokenPieceKey, 1 );

				keysBrokenOffOut.pushBack( brokenPieceKey );
				break;
			}
		case HK_SHAPE_MOPP:
			{
				hkpMoppBvTreeShape* moppTree = static_cast<hkpMoppBvTreeShape*>(shape);
				hkpShapeCollection* collection = const_cast<hkpShapeCollection*>(moppTree->getShapeCollection());

				BASSERTM(collection->getType() == HK_SHAPE_LIST, "The container must be a list shape.");
				hkpListShape* list = static_cast<hkpListShape*>(collection);
				newShape = list->m_childInfo[brokenPieceKey].m_shape;

				hkpBreakOffPartsUtil::removeKeysFromListShape( breakingBody, &brokenPieceKey, 1 );

				keysBrokenOffOut.pushBack( brokenPieceKey );
				break;
			}
		default:
         BASSERTM(false, "This shape is not implemented yet." );
			return HK_FAILURE;
		}


#if 0
      // Rebuild inertia tensors.
      hkpMassProperties massProperties;
      //hkpInertiaTensorComputer::computeShapeVolumeMassProperties(shape, 175.0f, massProperties);

      hkTransform transform;
      transform.setIdentity();
      hkAabb aabb;
      shape->getAabb (transform, 0.0f, aabb);

      /*
      // Compute composite bounding box and center offset
      //AABB box(AABB::eInitExpand);
      //centerOffset.zero();


      long num = shape->getNumChildShapes();
      for (long i=0; i < num; i++)
      {
         if(shape->isChildEnabled(i))
         {
            const hkpShape *pChildShape = shape->getChildShapeInl(i);


            //box.expand((const BVec3&)pPart->mMin);
            //box.expand((const BVec3&)pPart->mMax);
         }
      }
      */


      /*
      hkVector4 extends = ((aabb.m_max - aabb.m_min) / 2.0f);
      hkVector4 centerOfMass = ((aabb.m_max + aabb.m_min) / 2.0f);
      float mass = 175.0f;


      hkMatrix3 inertialTensor;
      inertialTensor.setIdentity();
      inertialTensor.setDiagonal(extends(0) * mass, extends(1) * mass, extends(2) * mass);

      breakingBody->setInertiaLocal(inertialTensor);
      breakingBody->setCenterOfMassLocal(centerOfMass);
      breakingBody->setMass(mass);
      */
#endif
	}



   const hkpShape* parentShape = const_cast<const hkpShape*>(breakingBody->getCollidable()->getShape());
   const BObject *pParentObject = (BObject*) parentShape->getUserData();
   
   gWorld->addPhysicsBreakOffObjectAsync(pParentObject, newShape);

	return HK_SUCCESS;
}

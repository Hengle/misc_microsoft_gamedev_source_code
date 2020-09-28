//============================================================================
//
//  physicsinfomanager.cpp
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================

#include "common.h"
#include "physicsinfomanager.h"
#include "physicsinfo.h"
#include "workdirsetup.h" 
#include "gamedirectories.h"
#include "physics.h"
#include "physicsobject.h"
#include "physics\Dynamics\Action\hkpAction.h"
#include "physicsgroundvehicleaction.h"


BPhysicsInfoManager gPhysicsInfoManager;

//============================================================================
// BPhysicsInfoManager::BPhysicsInfoManager
//============================================================================
BPhysicsInfoManager::BPhysicsInfoManager()
{
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit(cThreadIndexSim, true);
#endif
}


//============================================================================
// BPhysicsInfoManager::~BPhysicsInfoManager
//============================================================================
BPhysicsInfoManager::~BPhysicsInfoManager()
{
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);
   eventReceiverDeinit(true);
#endif
   cleanup();
}


//============================================================================
// BPhysicsInfoManager::init
//============================================================================
bool BPhysicsInfoManager::init(void)
{
   // Nothing yet.
#ifdef ENABLE_RELOAD_MANAGER
   BReloadManager::BPathArray paths;
   BString fullpath;
   gFileManager.constructQualifiedPath(cDirPhysics, "*.physics", fullpath);
   paths.pushBack(fullpath);
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle);
#endif   
   // Success.
   return(true);
}


//============================================================================
// BPhysicsInfoManager::cleanup
//============================================================================
void BPhysicsInfoManager::cleanup(void)
{
   // Kill off physics infos.
   for(long i=0; i<mPhysicsInfos.getNumber(); i++)
   {
      delete mPhysicsInfos[i];
      mPhysicsInfos[i]=NULL;
   }
   mPhysicsInfos.setNumber(0);

   // Clear hash table.
   mNameTable.clearAll();
}


//============================================================================
// BPhysicsInfoManager::getOrCreate
//============================================================================
long BPhysicsInfoManager::getOrCreate(const BCHAR_T *filename, bool forceLoad)
{
   // Sanity check.
   if(!filename)
   {
      BFAIL("Null filename passed to getOrCreate.");
      return(-1);
   }

   // Look for existing physics info.
   long index=find(filename);

   // If found, return it.
   if(index>=0)
      return(index);

   // Create.
   index=create(filename);
   if(index<0)
      return(-1);

   // If we want to force load this physics info, go ahead and do that now.
   if(forceLoad)
      mPhysicsInfos[index]->load(cDirPhysics);

   return(index);
}


//============================================================================
// BPhysicsInfoManager::find
//============================================================================
long BPhysicsInfoManager::find(const BCHAR_T *filename)
{
   // Look up in name table.
   long index=-1;
   bool found=mNameTable.find(filename, &index);

   // If we found a match, give back the index.
   if(found)
      return(index);

   // No match, give back -1.
   return(-1);
}


//============================================================================
// BPhysicsInfoManager::create
//============================================================================
long BPhysicsInfoManager::create(const BCHAR_T *filename)
{
   // Sanity check.
   if(!filename)
   {
      BFAIL("Null filename passed to create.");
      return(-1);
   }

   // Allocate new physics info.
   BPhysicsInfo *physicsInfo = new BPhysicsInfo;
   if(!physicsInfo)
   {
      BFAIL("Could not allocate new physics info.");
      return(-1);
   }

   // Set data.
   physicsInfo->setFilename(filename);

   // Add to list.
   long index=mPhysicsInfos.add(physicsInfo);
   if(index<0)
   {
      BFAIL("Could not allocate space in array for new physics info.");
      delete physicsInfo;
      return(-1);
   }

   // Put this into the hash table.
   long foundVal=0;
   mNameTable.add(filename, index, foundVal);

   physicsInfo->setID(index);

   // Hand back index.
   return(index);
}


//============================================================================
// BPhysicsInfoManager::get
//============================================================================
BPhysicsInfo *BPhysicsInfoManager::get(long index, bool load)
{
   if(index<0 || index>=mPhysicsInfos.getNumber())
      return(NULL);

   // Make sure physics inbfo is loaded if requested.
   if(load)
      mPhysicsInfos[index]->load(cDirPhysics);

   // Hand back pointer.
   return(mPhysicsInfos[index]);
}


//============================================================================
// BPhysicsInfoManager::unloadAll
//============================================================================
void BPhysicsInfoManager::unloadAll(void)
{
   for(long i=0; i<mPhysicsInfos.getNumber(); i++)
      mPhysicsInfos[i]->unload();
}


//============================================================================
//============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BPhysicsInfoManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassReloadNotify:
      {
//-- FIXING PREFIX BUG ID 1582
         const BReloadManager::BNotificationPayload* pPayload = (BReloadManager::BNotificationPayload*)event.mpPayload;
//--
         if (pPayload)
         {
            BString filename = pPayload->mPath;
            filename.removePath();
            filename.removeExtension();

            long id = find(filename);
            BPhysicsInfo* pInfo = get(id, false);
            if (pInfo)
            {
               gConsoleOutput.status("Reloading physics file <%s>", filename.asNative());

               BPhysicsVehicleInfo* pOldData = pInfo->getVehicleInfo();
               pInfo->unload();
               pInfo->load(cDirPhysics);
               BPhysicsVehicleInfo* pNewData = pInfo->getVehicleInfo();
               resetPhysicsInfo(pInfo, pOldData, pNewData);
            }
         }
         break;
      }
   }
   
   return false;      
}
#endif

//============================================================================
//============================================================================
void BPhysicsInfoManager::resetPhysicsInfo(BPhysicsInfo* pInfo, BPhysicsVehicleInfo* pOldData, BPhysicsVehicleInfo* pNewData)
{
   // Note - this really only supports reloads for physics object that use the groundVehicleAction for now

   //-- now walk the known physics objects and update them with the new physics info
   //-- if they are affected by this reload
   BPointerList<BPhysicsObject> &list = BPhysicsObject::getPhysicsObjectList();
   BHandle hItem = NULL;
   BPhysicsObject *pItem = list.getHead(hItem);
   while (pItem)
   {
      hkpAction* pVehicleAction = NULL;
      if (pItem->getNumActions() > 0)
         pVehicleAction = pItem->getAction(0);

      if (pVehicleAction && pVehicleAction->getUserData() == BPhysicsInfo::cGround) // hack
      {
         BPhysicsGroundVehicleAction* pPGVA = reinterpret_cast<BPhysicsGroundVehicleAction*>(pVehicleAction);
//-- FIXING PREFIX BUG ID 1583
         const BPhysicsVehicleInfo* pCurrentData = pPGVA->getVehicleInfo();
//--
         if (pCurrentData == pOldData)
         {
            pPGVA->setVehicleInfo(pNewData);
            pItem->setCenterOffset(pInfo->getCenterOffset());
            if (pItem->getRigidBody())
               pItem->getRigidBody()->activate();
         }
      }

      //-- grab the next one
      pItem = list.getNext(hItem);
   }
}

//============================================================================
// eof: physicsinfomanager.cpp
//============================================================================

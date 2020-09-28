//==============================================================================
// reloadManager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xsystem.h"
#include "reloadManager.h"
#include "consoleOutput.h"

// xcore
#include "functor\funBind.h"

//==============================================================================
// Globals
//==============================================================================
BReloadManager gReloadManager;

//==============================================================================
// BReloadManager::BReloadManager
//==============================================================================
BReloadManager::BReloadManager() :
   mPrevFlushTickCount(0)
{
}

//==============================================================================
// BReloadManager::~BReloadManager
//==============================================================================
BReloadManager::~BReloadManager()
{
}

//==============================================================================
// BReloadManager::clearHelper
//==============================================================================
void BReloadManager::clearHelper(void)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex()==cThreadIndexMisc);
   
   mClients.clear();
   mDeferredNotifications.clear();
}

//==============================================================================
// BReloadManager::clear
//==============================================================================
void BReloadManager::clear(void)
{
   gEventDispatcher.submitFunctor(
      cThreadIndexMisc,  
      BEventDispatcher::BFunctor(this, &BReloadManager::clearHelper), true );
}

//==============================================================================
// BReloadManager::createPathArray
//==============================================================================
BReloadManager::BPathArray BReloadManager::createPathArray(long dirID, const BString& filename)
{
   BString fullname;
   gFileManager.constructQualifiedPath(dirID, filename, fullname);
   
   BReloadManager::BPathArray paths;
   paths.pushBack(fullname);
   
   return paths;
}

//==============================================================================
// BReloadManager::registerClient
//==============================================================================
void BReloadManager::registerClient(const BPathArray& paths, DWORD flags, BEventReceiverHandle handle, uint eventClass, uint data, uint id)
{
#ifdef ENABLE_RELOAD_MANAGER
   BDEBUG_ASSERT(handle != cInvalidEventReceiverHandle);
   
   const BClientDesc* pClientDesc = new BClientDesc(paths, flags, handle, eventClass, data, id);
      
   gEventDispatcher.submitFunctor(
      cThreadIndexMisc,  
      GF::Bind<0>(BProcessRegisterClientFunctor(this, &BReloadManager::processRegisterClient), pClientDesc) );
#endif
}

//==============================================================================
// BReloadManager::registerFunctor
//==============================================================================
void BReloadManager::registerFunctor(const BPathArray& paths, DWORD flags, BThreadIndex threadIndex, const BNotificationFunctor& functor, uint data, uint id)
{
#ifdef ENABLE_RELOAD_MANAGER
   BDEBUG_ASSERT(threadIndex != cThreadIndexInvalid);
   
   const BClientDesc* pClientDesc = new BClientDesc(paths, flags, threadIndex, functor, data, id);

   gEventDispatcher.submitFunctor(
      cThreadIndexMisc,  
      GF::Bind<0>(BProcessRegisterClientFunctor(this, &BReloadManager::processRegisterClient), pClientDesc) );
#endif
}

//==============================================================================
// BReloadManager::deregisterClient
//==============================================================================
void BReloadManager::deregisterClient(BEventReceiverHandle handle, uint id, bool wait)
{
#ifdef ENABLE_RELOAD_MANAGER
   gEventDispatcher.submitFunctor(
      cThreadIndexMisc,  
      GF::Bind<0>(BProcessDeregisterClientFunctor(this, &BReloadManager::processDeregisterClient), BDeregisterClientData(handle, id)), wait );
#endif
}

//==============================================================================
// BReloadManager::changeNotify
//==============================================================================
void BReloadManager::changeNotify(const char* pPath)
{
#ifdef ENABLE_RELOAD_MANAGER
   gEventDispatcher.submitFunctor(
      cThreadIndexMisc,  
      GF::Bind<0>(BProcessChangeNotifyFunctor(this, &BReloadManager::processChangeNotify), BString(pPath)) );
#endif
}

//==============================================================================
// BReloadManager::flushNotifications
//==============================================================================
void BReloadManager::flushNotifications(void)
{
#ifdef ENABLE_RELOAD_MANAGER
   const DWORD curTickCount = GetTickCount();
   
   if ((curTickCount - mPrevFlushTickCount) < 500)
      return;
   
   mPrevFlushTickCount = curTickCount;
   
   gEventDispatcher.submitFunctor(
      cThreadIndexMisc,  
      BEventDispatcher::BFunctor(this, &BReloadManager::processFlushNotifications), false, false, false );   
#endif
}

//==============================================================================
// BReloadManager::processRegisterClient
//==============================================================================
void BReloadManager::processRegisterClient(const BClientDesc* pClientDesc)
{
#ifdef ENABLE_RELOAD_MANAGER
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex()==cThreadIndexMisc);
   
   if (mClients.find(*pClientDesc) == cInvalidIndex)
   {
      mClients.pushBack(*pClientDesc);
      
      if (mClients.getSize() == 1024)
      {
         gConsoleOutput.debug("BReloadManager::processRegisterClient: Client array size is is getting very big!");
      }
   }
   
   delete pClientDesc;
#endif
}

//==============================================================================
// BReloadManager::processDeregisterClient
//==============================================================================
void BReloadManager::processDeregisterClient(const BDeregisterClientData& deregisterClientData)
{
#ifdef ENABLE_RELOAD_MANAGER
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex()==cThreadIndexMisc);
   
   for (int i = 0; i < static_cast<int>(mClients.size()); i++)
   {
      if ( (mClients[i].mHandle == deregisterClientData.mHandle) && 
           (((deregisterClientData.mID != UINT_MAX) && (mClients[i].mID == deregisterClientData.mID)) || (deregisterClientData.mID == UINT_MAX))
         )
      {
         mClients.erase(i);
         i--;
      }
   }
#endif
}

//==============================================================================
// BReloadManager::canonicalizePathname
//==============================================================================
void BReloadManager::canonicalizePathname(BString& path)
{
   path.standardizePath();
   
   BString extension;
   strPathGetExtension(path, extension);
   if (extension == ".xmb")
      strPathRemoveExtension(path);
   
   int len = path.length();
   
   // umm, hack?
   if (strncmp(path.getPtr(), "game:\\", 6) == 0)
   {
      if (len > 6)
         path.crop(6, len - 1);
      else
         path.empty();
   }
   else if (strncmp(path.getPtr(), "e:\\", 3) == 0)
   {
      if (len > 3)
         path.crop(3, len - 1);
      else
         path.empty();
   }
}

//==============================================================================
// BReloadManager::matchPath
//==============================================================================
bool BReloadManager::matchPath(const BClientDesc& client, const BString& path)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex()==cThreadIndexMisc);
   
   BString srcPathname, srcFilename;
   strPathSplit(path, srcPathname, srcFilename);   
         
   BString dstPathname, dstFilename;
   
   for (uint pathIndex = 0; pathIndex < client.mPaths.size(); pathIndex++)
   {
      strPathSplit(client.mPaths[pathIndex], dstPathname, dstFilename);
            
      dstFilename.toLower();
      canonicalizePathname(dstPathname);
                  
      if (!wildcmp(dstFilename.getPtr(), srcFilename.getPtr()))
         continue;
                  
      if (client.mFlags & cFlagSubDirs)            
      {
         if (strncmp(srcPathname.getPtr(), dstPathname.getPtr(), dstPathname.length()) != 0)
            continue;
      }
      else
      {
         if (srcPathname != dstPathname)
            continue;
      }
      
      return true;
   }
   
   return false;
}

//==============================================================================
// BReloadManager::sendChangeNotification
//==============================================================================
void BReloadManager::sendChangeNotification(const BClientDesc& client, const BString& path)
{
#ifdef ENABLE_RELOAD_MANAGER
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex()==cThreadIndexMisc);
   
   trace("BReloadManager::sendChangeNotification: Async Notify: Client: 0x%I64X Data: %u ID: %u Class: %u Path: %s", 
      client.mHandle, 
      client.mData,
      client.mID,
      client.mEventClass,
      path.getPtr());

   if (client.mHandle != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.send(
         cInvalidEventReceiverHandle, client.mHandle, client.mEventClass,
         client.mData, client.mID, 
         new BNotificationPayload(path, client.mData, client.mID), 
         (client.mFlags & cFlagSynchronous) ? BEventDispatcher::cSendSynchronousDispatch : 0 );
   }
   else
   {
      gEventDispatcher.submitFunctor(
         client.mThreadIndex, 
         GF::Bind<0, 1, 2>(client.mNotifyFunctor, path, client.mData, client.mID),
         false,
         (client.mFlags & cFlagSynchronous) != 0 );
   }         
#endif
}

//==============================================================================
// BReloadManager::processChangeNotify
//==============================================================================
void BReloadManager::processChangeNotify(const BString& path)
{
#ifdef ENABLE_RELOAD_MANAGER
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex()==cThreadIndexMisc);
   
   BString canonicalizedPath(path);
   canonicalizePathname(canonicalizedPath);

   if (!mDeferredNotifications.contains(canonicalizedPath))
      mDeferredNotifications.pushBack(canonicalizedPath);
#endif
}

//==============================================================================
// BReloadManager::processFlushNotifications
//==============================================================================
void BReloadManager::processFlushNotifications(void)
{
#ifdef ENABLE_RELOAD_MANAGER
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex()==cThreadIndexMisc);
      
   for (uint notificationIndex = 0; notificationIndex < mDeferredNotifications.size(); notificationIndex++)
   {
      const BString& canonicalizedPath = mDeferredNotifications[notificationIndex];
      
      uint totalSends = 0;
      
      for (uint i = 0; i < mClients.size(); i++)
      {
         if (matchPath(mClients[i], canonicalizedPath))
         {
            totalSends++;
            sendChangeNotification(mClients[i], canonicalizedPath);
         }
      }         
      
      trace("BReloadManager::processChangeNotify: Path: %s Total Client Matches: %u", canonicalizedPath.getPtr(), totalSends); 
   }     
   
   mDeferredNotifications.resize(0);
#endif
}

//==============================================================================
// BFileWatcher::BFileWatcher
//==============================================================================
BFileWatcher::BFileWatcher(BThreadIndex clientThreadIndex) : 
   BEventReceiver(),
   mDirtyFlags(0)
{
   eventReceiverInit(clientThreadIndex, true);
}

//==============================================================================
// BFileWatcher::~BFileWatcher
//==============================================================================
BFileWatcher::~BFileWatcher()
{
   gReloadManager.deregisterClient(mEventHandle);

   eventReceiverDeinit(true);
}

//==============================================================================
// BFileWatcher::clear
//==============================================================================
void BFileWatcher::clear(bool wait)
{
   BDEBUG_ASSERT(gEventDispatcher.getHandleThreadIndex(mEventHandle) == gEventDispatcher.getThreadIndex());
   
   gReloadManager.deregisterClient(mEventHandle, wait);

   mPaths.clear();
}

//==============================================================================
// BFileWatcher::add
//==============================================================================
BFileWatcher::BPathHandle BFileWatcher::add(BReloadManager::BPathArray& paths, DWORD flags)
{
   BDEBUG_ASSERT(gEventDispatcher.getHandleThreadIndex(mEventHandle) == gEventDispatcher.getThreadIndex());
   
   const BPathHandle handle = (BPathHandle)mPaths.getSize();
   if (handle >= cMaxPaths)
      return -1;

   mPaths.pushBack(paths);

   gReloadManager.registerClient(
      mPaths.back(), 
      flags, 
      mEventHandle, cEventClassReloadNotify, 0, handle);

   return handle;
}

//==============================================================================
// BFileWatcher::add
//==============================================================================
BFileWatcher::BPathHandle BFileWatcher::add(long dirID, const BString& filename, DWORD flags)
{
   BDEBUG_ASSERT(gEventDispatcher.getHandleThreadIndex(mEventHandle) == gEventDispatcher.getThreadIndex());
   
   return add(BReloadManager::createPathArray(dirID, filename), flags);   
}

//==============================================================================
// BFileWatcher::getIsDirty
//==============================================================================
bool BFileWatcher::getIsDirty(BFileWatcher::BPathHandle handle)
{
   BDEBUG_ASSERT(gEventDispatcher.getHandleThreadIndex(mEventHandle) == gEventDispatcher.getThreadIndex());
   
   BDEBUG_ASSERT((handle >= 0) && (handle < (int)mPaths.getSize()));

   const DWORD bitMask = 1U << handle;

   const bool dirty = (mDirtyFlags & bitMask) != 0;
   mDirtyFlags &= ~bitMask;

   return dirty;
}

//==============================================================================
// BFileWatcher::getAreAnyDirty
//==============================================================================
bool BFileWatcher::getAreAnyDirty(void)
{
   BDEBUG_ASSERT(gEventDispatcher.getHandleThreadIndex(mEventHandle) == gEventDispatcher.getThreadIndex());
   
   const bool result = (mDirtyFlags != 0);

   mDirtyFlags = 0;

   return result;  
}

//==============================================================================
// BFileWatcher::receiveEvent
//==============================================================================
bool BFileWatcher::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassReloadNotify:
      {
//-- FIXING PREFIX BUG ID 530
         const BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
//--

         if (pPayload->mID < mPaths.getSize())
         {
            const uint bitMask = 1U << pPayload->mID;
            mDirtyFlags |= bitMask;
         }

         break;
      }
   }

   return false;
}

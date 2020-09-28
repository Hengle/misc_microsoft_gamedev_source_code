//==============================================================================
// reloadManager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xcore
#include "threading\eventDispatcher.h"

//==============================================================================
// class BReloadManager
//==============================================================================
class BReloadManager
{
   BReloadManager(const BReloadManager&);
   BReloadManager& operator= (const BReloadManager&);
   
public:
   BReloadManager();
   ~BReloadManager();
   
   typedef BDynamicArray<BString> BPathArray;
   
   enum
   {
      cFlagSubDirs = 1,
      cFlagSynchronous = 2,
   };
   
   // createPathArray() is a helper method to simplify creating single entry path arrays. 
   static BPathArray createPathArray(long dirID, const BString& filename);
      
   void registerClient(const BPathArray& paths, DWORD flags, BEventReceiverHandle handle, uint eventClass = cEventClassReloadNotify, uint data = 0, uint id = UINT_MAX);
   
   void registerClient(long dirID, const BString& filename, DWORD flags, BEventReceiverHandle handle, uint eventClass = cEventClassReloadNotify, uint data = 0, uint id = UINT_MAX)
   {
      registerClient(createPathArray(dirID, filename), flags, handle, eventClass, data, id);
   }
               
   void deregisterClient(BEventReceiverHandle handle, uint id = UINT_MAX, bool wait = false);
   
   // If the functor points to a class method the object instance cannot go out of scope!
   typedef GF::Functor<void, TYPELIST_3(const BString&, uint, uint)> BNotificationFunctor;   
   void registerFunctor(const BPathArray& paths, DWORD flags, BThreadIndex threadIndex, const BNotificationFunctor& functor, uint data = 0, uint id = UINT_MAX);
   
   void changeNotify(const char* pPath);
   void flushNotifications(void);
   
   class BNotificationPayload : public BEventPayload
   {
   public:
      BNotificationPayload(const BString& path, uint data, uint id) : mPath(path), mData(data), mID(id) { }
      
      BString mPath;  
      uint mData;
      uint mID;
      
   private:
      virtual void deleteThis(bool delivered) { delivered; delete this; }
   };
   
   void clear(void);
      
private:   
   class BClientDesc
   {
   public:
      BClientDesc() { }
      
      BClientDesc(const BPathArray& paths, DWORD flags, BEventReceiverHandle handle, uint eventClass, uint data, uint id) :
         mPaths(paths), mFlags(flags), mHandle(handle), mEventClass(static_cast<uint16>(eventClass)), mData(data), mID(id), mThreadIndex(cThreadIndexInvalid)
      {
      }
      
      BClientDesc(const BPathArray& paths, DWORD flags, BThreadIndex threadIndex, const BNotificationFunctor& functor, uint data, uint id) :
         mPaths(paths), mFlags(flags), mHandle(cInvalidEventReceiverHandle), mNotifyFunctor(functor), mData(data), mID(id), mThreadIndex(threadIndex), mEventClass(0)
      {
      }
      
      BPathArray mPaths;

      BEventReceiverHandle mHandle;
      
      BThreadIndex mThreadIndex;
      BNotificationFunctor mNotifyFunctor;
                  
      uint mID;
      uint mData;
      uint16 mEventClass;
      DWORD mFlags;
      
      bool operator== (const BClientDesc& rhs) const
      {
         // Functors can't be compared!
         return 
            (mPaths        == rhs.mPaths) &&
            (mHandle       == rhs.mHandle) &&
            (mThreadIndex  == rhs.mThreadIndex) &&
            (mID           == rhs.mID) &&
            (mData         == rhs.mData) &&
            (mEventClass   == rhs.mEventClass) &&
            (mFlags        == rhs.mFlags);
      }
   };
   
   typedef BDynamicArray<BClientDesc> BClientDescArray;
   BClientDescArray mClients;
   
   typedef BDynamicArray<BString> BStringArray;
   BStringArray mDeferredNotifications;
   
   DWORD mPrevFlushTickCount;
  
   typedef GF::Functor<void, TYPELIST_1(BString)> BProcessChangeNotifyFunctor;   
   void processChangeNotify(const BString& path);
   
   typedef GF::Functor<void, TYPELIST_1(const BClientDesc*)> BProcessRegisterClientFunctor;   
   void processRegisterClient(const BClientDesc* pClientDesc);
   
   class BDeregisterClientData
   {
   public:
      BDeregisterClientData(BEventReceiverHandle handle, uint id) : mHandle(handle), mID(id)
      {
      }
      
      BEventReceiverHandle mHandle;
      uint mID;
   };
   
   typedef GF::Functor<void, TYPELIST_1(BDeregisterClientData)> BProcessDeregisterClientFunctor;   
   void processDeregisterClient(const BDeregisterClientData& deregisterClientData);

   void clearHelper(void);
   bool matchPath(const BClientDesc& client, const BString& path);
   void sendChangeNotification(const BClientDesc& client, const BString& path);
   void processFlushNotifications(void);
   static void canonicalizePathname(BString& path);
};

//==============================================================================
// externs
//==============================================================================
extern BReloadManager gReloadManager;

//==============================================================================
// class BFileWatcher
// This is a single threaded API!
//==============================================================================
class BFileWatcher : public BEventReceiver
{
public:
   BFileWatcher(BThreadIndex clientThreadIndex = cThreadIndexInvalid);
   ~BFileWatcher();
   
   // This method is very slow if wait is true. It waits until the reload manager's helper thread 
   // processes the request to deregister the client.
   // If wait is false, it's possible (but unlikely) that you could receive invalid reload requests (should be harmless).
   void clear(bool wait = true);
   
   typedef int BPathHandle;
      
   // Returns -1 on error.   
   BPathHandle add(BReloadManager::BPathArray& paths, DWORD flags = BReloadManager::cFlagSynchronous);
      
   // Returns -1 on error.   
   BPathHandle add(long dirID, const BString& filename, DWORD flags = BReloadManager::cFlagSynchronous);
   
   int getNumPaths(void) const { return mPaths.getSize(); }
      
   const BReloadManager::BPathArray& getPathArray(BPathHandle handle) const { return mPaths[handle]; }
      
   bool getIsDirty(BPathHandle handle);
   
   bool getAreAnyDirty(void);
             
private:
   typedef BDynamicArray<BReloadManager::BPathArray> BPathArray;
   BPathArray mPaths;
   
   enum { cMaxPaths = 32 };
   DWORD mDirtyFlags;
      
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   
   BFileWatcher(const BFileWatcher&);
   BFileWatcher& operator= (const BFileWatcher&);
};


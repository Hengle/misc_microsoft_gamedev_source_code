//-------------------------------------------------------------------------------------------------
//
// File: buildServer.h
// Copyright (c) 2007, Ensemble Studios
//
//-------------------------------------------------------------------------------------------------
#pragma once
#include "containers\hashMap.h"
#include "containers\queue.h"
#include "threading\monitor.h"
#include "containers\nameValueMap.h"
#include "buildSystemProtocol.h"
#include "..\shared\tcpClient.h"

typedef uint64 BBuildTaskID;
const uint64 cInvalidBuildTaskID = UINT64_MAX;

class BBuildTaskDesc
{
public:
   BBuildTaskDesc()
   {
      clear();
   }

   void clear()
   {
      mExec.empty();
      mPath.empty();
      mArgs.empty();
      mOutputFilenames.clear();
      mCompletionCallbackData = 0;
      mpCompletionCallbackDataPtr = NULL;
      mpCompletionCallback = NULL;
      mAllClients = false;
   }

   BString                 mExec;
   BString                 mPath;
   BString                 mArgs;
   BDynamicArray<BString>  mOutputFilenames;
      
   uint64                  mCompletionCallbackData;
   void*                   mpCompletionCallbackDataPtr;
   
   bool                    mAllClients;

   // Completion callback will be called from a worker thread!
   enum
   { 
      cFailedExecutingResult     = -100, 
      cCanceledResult            = -200,
      cFailedWritingFileResult   = -300
   };
   
   // Callback will be called from any thread, within the server's monitor lock.
   typedef void (*BCompletionCallbackPtr)(const BBuildTaskDesc& desc, int resultCode, uint numMissingFiles, const BDynamicArray<BString>* pFilesWritten);
   BCompletionCallbackPtr  mpCompletionCallback;
}; 

class BBuildServer
{
   BBuildServer(const BBuildServer&);
   BBuildServer& operator= (const BBuildServer&);
   
public:   
   BBuildServer();
   ~BBuildServer();

   bool           init(const BDynamicArray<BString>& clients);
   bool           deinit();
   
   bool           getInitialized() const { return mInitialized; }
      
   uint           getMaxActiveProcesses(void) const { return mMaxActiveProcesses; }
   bool           setMaxActiveProcesses(uint maxProcesses);
   
   bool           addTask(const BBuildTaskDesc& task);
   
   bool           waitForAllTasks();
   bool           cancelAllTasks();
   
   bool           submitAutoUpdate(void);

   struct BStatus
   {
      BStatus() 
      {
         clear();
      }
      
      void clear()
      {
         Utils::ClearObj(*this);
      }
      
      uint mNumHelpers;
      uint mTotalCPUs;
      uint mActiveTasks;
      uint mQueuedTasks;
      uint mPendingTaskCompletions;
   };
   
   bool           getStatus(BStatus& status);
   
private:
   enum 
   { 
      cMaxHelpers          = 32,
      cMaxHelperProcesses  = 8
   };
   
   friend class BHelperThread;
   
   enum eHelperState
   {
      cStateInvalid,
      cStateFailed,
      cStateDisconnecting,
      cStateDisconnected,

      cStateConnecting,

      cStateIdle,
      cStateBusy,
   };
         
   class BHelperThread
   {
      BHelperThread(const BHelperThread&);
      BHelperThread& operator= (const BHelperThread&);
      
   public:
      BHelperThread() { clear(); }
                  
      HANDLE init(BBuildServer* pServer, uint helperIndex, const char* pAddress);
         
   private:      
      BBuildServer*  mpServer;
      uint           mHelperIndex;
            
      BTcpClient     mClient;
      BString        mAddress;
      
      DWORD          mLastKeepAliveSent;
      DWORD          mLastKeepAliveReceived;
      
      BBuildTaskID   mActiveProcesses[cMaxHelperProcesses];
            
      uint           mProtocolVersion;
      uint           mBuildSystemVersion;
      uint           mNumCPUs;
                  
      void clear()
      {  
         mpServer = NULL;
         mHelperIndex = 0;
         mAddress.empty();
         mLastKeepAliveSent = 0;
         mLastKeepAliveReceived = 0;
         
         for (uint i = 0; i < cMaxHelperProcesses; i++)
            mActiveProcesses[i] = cInvalidBuildTaskID;
                  
         mProtocolVersion = 0;
         mBuildSystemVersion = 0;
         mNumCPUs = 0;
      }
      
      void updateState(BBuildServer::eHelperState state);
      bool sendPacket(BuildSystemProtocol::ePacketTypes packetType, const BNameValueMap& nameValueMap, bool wait = true);
      bool sendPacket(BuildSystemProtocol::ePacketTypes packetType, void* pData, uint dataLen, bool wait = true);
      bool receivePacket(bool& receivedPacked, BuildSystemProtocol::BPacketHeader& header, BByteArray& data, bool waitForData);
      bool tickConnection();
      bool processHelloPacket(const BNameValueMap& helloPacketData);
      bool processByePacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData);
      bool processProcessCompletedPacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData);
      bool processPackets(BuildSystemProtocol::ePacketTypes packetType, bool& receivedPacket, BByteArray* pPacketData, BNameValueMap* pNameValueMap);
      bool waitForPacket(BuildSystemProtocol::ePacketTypes packetType, BByteArray* pPacketData, BNameValueMap* pNameValueMap);
      void disconnectAndExit(bool failed);
      uint getNumProcessesActive() const;
      uint getNumProcessesAvailable() const;
      int getNextAvailableProcess() const;
      
      bool cancelAllTasks();
      void finalizeAllTasks(bool resubmit);
      void finalizeTask(BBuildTaskID buildTaskID, int result, bool resubmit, uint numMissingFiles, const BDynamicArray<BString>* pFilesWritten);
      bool startNewProcess();
      unsigned threadMethod();
      unsigned threadMethodGuarded();
            
      void logMessage(const char* pMsg, ...);
      void logErrorMessage(const char* pMsg, ...);
            
      static unsigned __stdcall threadFunc(void* pArguments);
   };
   
   DWORD                      mDebugPrefix;
   
   BMonitor                   mMonitor;
   
   volatile bool              mInitialized;
   volatile bool              mCancelAll;
   volatile bool              mExiting;
   
   uint                       mNumActiveHelperThreads;
   BHelperThread              mHelpers[cMaxHelpers];
   
   struct BHelperStatus
   {
      BHelperStatus() { clear(); }
      
      void clear()
      {
         mThreadHandle = INVALID_HANDLE_VALUE;
         mState = cStateInvalid;
         mNumCPUs = 0;
      }
      
      HANDLE         mThreadHandle;
      eHelperState   mState;
      uint           mNumCPUs;
   };
   BHelperStatus        mHelperStatus[cMaxHelpers];
      
   struct BBuildTask
   {  
      BBuildTask() { clear(); }
      
      void clear()
      {
         mDesc.clear();
         mID = cInvalidBuildTaskID;
         mHelperIndex = 0;
      }
      
      BBuildTaskDesc mDesc;
      BBuildTaskID   mID;  
      
      uint           mHelperIndex;
   };
  
   BDynamicArray<BBuildTask>  mActiveTasks;
         
   BBuildTaskID mNextBuildTaskID;
   
   uint findActiveTaskSlot();
   uint getNumActiveTasks() const;
   uint getNumQueuedTasks() const;
   uint getNumConnectingHelpers() const;
   uint getNumActiveHelpers() const;
   
   typedef BQueue<BBuildTaskDesc> BTaskQueue;
   
   BTaskQueue     mTaskQueue;
   BTaskQueue     mHelperTaskQueue[cMaxHelpers];
   
   bool           mSubmitAutoUpdate;
   
   uint           mMaxActiveProcesses;
   
   uint           mNumPendingTaskCompletions;
   
   static uint64 createBuildTaskID(uint64 nextID, uint processIndex, uint activeTaskSlot);
   static uint getActiveTaskSlotIndex(BBuildTaskID buildTaskID);
   static uint getProcessIndex(BBuildTaskID buildTaskID);

   static BOOL waitUntilNoHelpers(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL waitWhileAnyConnecting(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL waitUntilTaskOrExitingOrAutoUpdateOrCancelingAll(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL waitUntilExitingOrAutoUpdate(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL waitUntilExitingOrAutoUpdateOrCancelingAll(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL waitUntilNoActiveTasksOrNoHelpers(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL waitUntilAllTasksDoneOrNoHelpers(void* pCallbackDataPtr, uint64 callbackData);
};


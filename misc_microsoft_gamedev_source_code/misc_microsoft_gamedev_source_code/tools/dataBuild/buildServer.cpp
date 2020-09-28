//-------------------------------------------------------------------------------------------------
//
// File: buildServer.cpp
// Copyright (c) 2007, Ensemble Studios
//
//-------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "buildServer.h"
#include "containers\staticArray.h"
#include "file\win32File.h"

#define SEND_KEEPALIVE_PACKETS 1

#ifdef BUILD_DEBUG
   const uint cConnectionTimeoutTime = 6000;
#else
   const uint cConnectionTimeoutTime = 120;
#endif

const uint cMaxQueuedTasks = 2048;

const uint cActiveTaskSlotShift = 48U;
const uint cProcessIndexShift = 40U;

uint64 BBuildServer::createBuildTaskID(uint64 nextID, uint processIndex, uint activeTaskSlot)
{
   return nextID | (((uint64)activeTaskSlot) << cActiveTaskSlotShift) | (((uint64)processIndex) << cProcessIndexShift);
}

uint BBuildServer::getActiveTaskSlotIndex(BBuildTaskID buildTaskID)
{
   return (uint)((buildTaskID >> cActiveTaskSlotShift) & 0xFFFF);
}

uint BBuildServer::getProcessIndex(BBuildTaskID buildTaskID)
{
   return (uint)((buildTaskID >> cProcessIndexShift) & 0xFF);
}

const uint cDebugPrefix = 0xAABB1234;

BBuildServer::BBuildServer() :   
   mMonitor(256, false),
   mNumActiveHelperThreads(0),
   mInitialized(false),
   mCancelAll(false),
   mExiting(false),
   mNextBuildTaskID(0),
   mMaxActiveProcesses(8),
   mTaskQueue(cMaxQueuedTasks),
   mDebugPrefix(cDebugPrefix),
   mSubmitAutoUpdate(false),
   mNumPendingTaskCompletions(0)
{
   for (uint i = 0; i < cMaxHelpers; i++)
      mHelperTaskQueue[i].resize(256);
}

BBuildServer::~BBuildServer()
{
   deinit();
}

BOOL BBuildServer::waitWhileAnyConnecting(void* pCallbackDataPtr, uint64 callbackData)
{
   callbackData;
   BBuildServer* pServer = static_cast<BBuildServer*>(pCallbackDataPtr);
          
   if (!pServer->getNumActiveHelpers())            
      return TRUE;
      
   if (!pServer->getNumConnectingHelpers())
      return TRUE;
      
   return FALSE;
}

bool BBuildServer::init(const BDynamicArray<BString>& clients)
{
   if ((mInitialized) || (clients.isEmpty()))
      return false;

   mMonitor.enter();
   
   mInitialized = true;
   mCancelAll = false;
   mNextBuildTaskID = 0;
   mSubmitAutoUpdate = false;
   mNumPendingTaskCompletions = 0;
   
   mTaskQueue.clear();
   for (uint i = 0; i < cMaxHelpers; i++)
      mHelperTaskQueue[i].clear();

   for (uint i = 0; i < cMaxHelpers; i++)
      mHelperStatus[i].clear();
      
   mActiveTasks.resize(0);
         
   mNumActiveHelperThreads = Math::Min<uint>(clients.getSize(), cMaxHelpers);
   
   for (uint i = 0; i < mNumActiveHelperThreads; i++)   
      mHelperStatus[i].mThreadHandle = mHelpers[i].init(this, i, clients[i].getPtr());
      
   gConsoleOutput.printf("BBuildServer::init: Waiting for connections\n");
   
   mMonitor.waitForCondition(waitWhileAnyConnecting, this);
   
   if (!getNumActiveHelpers())
   {
      gConsoleOutput.error("BBuildServer::init: No clients\n");
      
      mMonitor.leave();
      
      deinit();
      
      return false;
   }
   
   gConsoleOutput.printf("BBuildServer::init: %u connection(s) established\n", getNumActiveHelpers());
         
   mMonitor.leave();
   return true;
}

bool BBuildServer::deinit()
{
   if (!mInitialized)
      return false;
            
   if (mNumActiveHelperThreads)
   {
      {
         BScopedMonitor lock(mMonitor);
         mExiting = true;
      }
            
      HANDLE handles[cMaxHelpers];
      for (uint i = 0; i < mNumActiveHelperThreads; i++)
         handles[i] = mHelperStatus[i].mThreadHandle;
         
      WaitForMultipleObjects(mNumActiveHelperThreads, handles, TRUE, INFINITE);
                  
      mExiting = false;
      
      for (uint i = 0; i < mNumActiveHelperThreads; i++)
      {
         CloseHandle(mHelperStatus[i].mThreadHandle);
         mHelperStatus[i].clear();
      }
   }
   
   cancelAllTasks();
   
   mInitialized = false;
   
   return true;
}

bool BBuildServer::setMaxActiveProcesses(uint maxProcesses)
{
   if (!mInitialized)
   {
      mMaxActiveProcesses = Math::Clamp<uint>(maxProcesses, 1U, cMaxHelperProcesses);
      return true;
   }
   
   if (!waitForAllTasks())
      return true;

   BScopedMonitor lock(mMonitor);

   mMaxActiveProcesses = Math::Clamp<uint>(maxProcesses, 1U, cMaxHelperProcesses);

   return true;
}

bool BBuildServer::addTask(const BBuildTaskDesc& task)
{
   if (!mInitialized)
      return false;
      
   if (!task.mpCompletionCallback)
      return false;
      
   BScopedMonitor lock(mMonitor);
   
   if (task.mAllClients)
   {
      for (uint i = 0; i < cMaxHelpers; i++)
      {
         if (mHelperStatus[i].mState >= cStateConnecting)
         {
            if (!mHelperTaskQueue[i].pushBack(task))
            {
               mHelperTaskQueue[i].resize(mHelperTaskQueue[i].getMaxSize() * 2U);
               if (!mHelperTaskQueue[i].pushBack(task))
                  return false;
            }
         }
      }
   }
   else if (!mTaskQueue.pushBack(task))
   {
      mTaskQueue.resize(mTaskQueue.getMaxSize() * 2U);
      return mTaskQueue.pushBack(task);
   }
   
   return true;
}

bool BBuildServer::submitAutoUpdate(void)
{
   if (!mInitialized)
      return false;
   
   if (!waitForAllTasks())
      return false;
   
   BScopedMonitor lock(mMonitor);
   
   mSubmitAutoUpdate = true;
   
   mMonitor.waitForCondition(waitUntilNoHelpers, this);
   
   mSubmitAutoUpdate = false;
   
   return true;   
}

BOOL BBuildServer::waitUntilNoHelpers(void* pCallbackDataPtr, uint64 callbackData)
{
   callbackData;
   BBuildServer* pServer = static_cast<BBuildServer*>(pCallbackDataPtr);
   BDEBUG_ASSERT(pServer->mDebugPrefix == cDebugPrefix);

   if (!pServer->getNumActiveHelpers())
      return TRUE;

   return FALSE;
}

BOOL BBuildServer::waitUntilTaskOrExitingOrAutoUpdateOrCancelingAll(void* pCallbackDataPtr, uint64 callbackData)
{
   BBuildServer* pServer = static_cast<BBuildServer*>(pCallbackDataPtr);
   BDEBUG_ASSERT(pServer->mDebugPrefix == cDebugPrefix);
   
   const uint helperIndex = (uint)callbackData;
   BDEBUG_ASSERT(helperIndex < cMaxHelpers);

   if ((pServer->mExiting) || (pServer->mCancelAll) || (pServer->mSubmitAutoUpdate))
      return TRUE;

   if (pServer->mTaskQueue.getSize() != 0) 
      return TRUE;
      
   if (pServer->mHelperTaskQueue[helperIndex].getSize() != 0)
      return TRUE;

   return FALSE;
}

BOOL BBuildServer::waitUntilExitingOrAutoUpdate(void* pCallbackDataPtr, uint64 callbackData)
{
   callbackData;
   BBuildServer* pServer = static_cast<BBuildServer*>(pCallbackDataPtr);
   BDEBUG_ASSERT(pServer->mDebugPrefix == cDebugPrefix);

   if ((pServer->mExiting) || (pServer->mSubmitAutoUpdate))
      return TRUE;

   return FALSE;
}

BOOL BBuildServer::waitUntilExitingOrAutoUpdateOrCancelingAll(void* pCallbackDataPtr, uint64 callbackData)
{
   callbackData;
   BBuildServer* pServer = static_cast<BBuildServer*>(pCallbackDataPtr);
   BDEBUG_ASSERT(pServer->mDebugPrefix == cDebugPrefix);

   if ((pServer->mExiting) || (pServer->mCancelAll) || (pServer->mSubmitAutoUpdate))
      return TRUE;

   return FALSE;
}

BOOL BBuildServer::waitUntilNoActiveTasksOrNoHelpers(void* pCallbackDataPtr, uint64 callbackData)
{
   callbackData;
   BBuildServer* pServer = static_cast<BBuildServer*>(pCallbackDataPtr);
   BDEBUG_ASSERT(pServer->mDebugPrefix == cDebugPrefix);

   if (!pServer->getNumActiveHelpers())
      return TRUE;

   if ((!pServer->getNumActiveTasks()) && (!pServer->mNumPendingTaskCompletions))
      return TRUE;

   return FALSE;
}

BOOL BBuildServer::waitUntilAllTasksDoneOrNoHelpers(void* pCallbackDataPtr, uint64 callbackData)
{
   callbackData;
   BBuildServer* pServer = static_cast<BBuildServer*>(pCallbackDataPtr);
   BDEBUG_ASSERT(pServer->mDebugPrefix == cDebugPrefix);

   if (!pServer->getNumActiveHelpers())
      return TRUE;
      
   if ((!pServer->getNumQueuedTasks()) && (!pServer->getNumActiveTasks()) && (!pServer->mNumPendingTaskCompletions))
      return TRUE;
   
   return FALSE;
}

bool BBuildServer::waitForAllTasks()
{
   if (!mInitialized)
      return false;
   
   BScopedMonitor lock(mMonitor);
   
   mMonitor.waitForCondition(waitUntilAllTasksDoneOrNoHelpers, this);
      
   if ((!getNumQueuedTasks()) && (!getNumActiveTasks()) && (!mNumPendingTaskCompletions))
      return true;
   
   return false;
}

bool BBuildServer::cancelAllTasks()
{
   if (!mInitialized)
      return false;
   
   BScopedMonitor lock(mMonitor);
   
   mCancelAll = true;

   mMonitor.waitForCondition(waitUntilNoActiveTasksOrNoHelpers, this);
   
   mCancelAll = false;
   
   for (uint i = 0; i < mActiveTasks.getSize(); i++)
   {
      if (mActiveTasks[i].mID != cInvalidBuildTaskID)
      {
         BBuildTaskDesc& desc = mActiveTasks[i].mDesc;
         
         (*desc.mpCompletionCallback)(desc, BBuildTaskDesc::cCanceledResult, 0, NULL);
         
         mActiveTasks[i].clear();
      }
   }
   
   while (mTaskQueue.getSize())
   {
      BBuildTaskDesc desc;
      mTaskQueue.popFront(desc);
      
      (*desc.mpCompletionCallback)(desc, BBuildTaskDesc::cCanceledResult, 0, NULL);
   }
   
   for (uint i = 0; i < cMaxHelpers; i++)
   {
      BTaskQueue& taskQueue = mHelperTaskQueue[i];
      
      while (taskQueue.getSize())
      {
         BBuildTaskDesc desc;
         taskQueue.popFront(desc);

         (*desc.mpCompletionCallback)(desc, BBuildTaskDesc::cCanceledResult, 0, NULL);
      }
   }
   
   return true;
}

bool BBuildServer::getStatus(BStatus& status)
{
   if (!mInitialized)
      return false;

   BScopedMonitor lock(mMonitor);
   
   status.clear();
   
   status.mNumHelpers = getNumActiveHelpers();
   status.mActiveTasks = getNumActiveTasks();
   status.mQueuedTasks = getNumQueuedTasks();
   status.mPendingTaskCompletions = mNumPendingTaskCompletions;
   
   for (uint i = 0; i < mNumActiveHelperThreads; i++)
      if (mHelperStatus[i].mState >= cStateIdle)
         status.mTotalCPUs += mHelperStatus[i].mNumCPUs;
   
   return true;
}

uint BBuildServer::findActiveTaskSlot() 
{
   for (uint i = 0; i < mActiveTasks.getSize(); i++)
      if (mActiveTasks[i].mID == cInvalidBuildTaskID)
         return i;

   mActiveTasks.grow();

   return mActiveTasks.getSize() - 1;
}

uint BBuildServer::getNumActiveTasks() const
{
   uint total = 0;
   for (uint i = 0; i < mActiveTasks.getSize(); i++)
      if (mActiveTasks[i].mID != cInvalidBuildTaskID)
         total++;
   return total;            
}

uint BBuildServer::getNumQueuedTasks() const
{
   uint totalQueuedTasks = mTaskQueue.getSize();

   for (uint i = 0; i < cMaxHelpers; i++)
      totalQueuedTasks += mHelperTaskQueue[i].getSize(); 
      
   return totalQueuedTasks;      
}      

uint BBuildServer::getNumConnectingHelpers() const
{
   uint total = 0;
   for (uint i = 0; i < cMaxHelpers; i++)
      if (mHelperStatus[i].mState == cStateConnecting)
         total++;
   return total;            
}

uint BBuildServer::getNumActiveHelpers() const
{
   uint total = 0;
   for (uint i = 0; i < cMaxHelpers; i++)
      if (mHelperStatus[i].mState >= cStateConnecting)
         total++;
   return total;            
}

//-------------------------------------------------------------------------------------------------

HANDLE BBuildServer::BHelperThread::init(BBuildServer* pServer, uint helperIndex, const char* pAddress)
{
   clear();
   
   mpServer = pServer;
   mAddress.set(pAddress);
   mHelperIndex = helperIndex;
   
   updateState(cStateConnecting);
   
   uintptr_t result = _beginthreadex(NULL, 0, threadFunc, this, 0, NULL);
   BVERIFY(result != 0);
   
   return (HANDLE)result;
}
   
void BBuildServer::BHelperThread::updateState(eHelperState state)
{  
   BScopedMonitor lock(mpServer->mMonitor);
   
   mpServer->mHelperStatus[mHelperIndex].mState = state;
   mpServer->mHelperStatus[mHelperIndex].mNumCPUs = mNumCPUs;
}

bool BBuildServer::BHelperThread::sendPacket(BuildSystemProtocol::ePacketTypes packetType, const BNameValueMap& nameValueMap, bool wait)
{
   BStaticArray<BYTE, 4096> buf;
   
   const uint dataSize = nameValueMap.getSerializeSize();
   buf.resize(dataSize);
   
   uchar* pEnd = (uchar*)nameValueMap.serialize(buf.getPtr());
   const uint serializeSize = pEnd - buf.getPtr();
   BVERIFY(dataSize == serializeSize);
   
   BuildSystemProtocol::BPacketHeader packetHeader(serializeSize, calcCRC32(buf.getPtr(), buf.getSize()), (BYTE)packetType, BuildSystemProtocol::BPacketHeader::cFlagDataIsNameValueMap);
   
   eTcpClientError status = mClient.send(&packetHeader, sizeof(packetHeader), false);
   if (status != cTCE_Success)
   {
      logErrorMessage("sendPacket: send() failed\n");
      return false;
   }
   
   status = mClient.send(buf.getPtr(), buf.getSize(), true);
   if (status != cTCE_Success)
   {
      logErrorMessage("sendPacket: send() failed\n");
      return false;
   }
   
   if (wait)
   {
      eTcpClientError status = mClient.sendFlush(true);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: sendFlush() failed\n");
         return false;
      }
   }         
   
   mLastKeepAliveSent = GetTickCount();
      
   return true;
}

bool BBuildServer::BHelperThread::sendPacket(BuildSystemProtocol::ePacketTypes packetType, void* pData, uint dataLen, bool wait)
{
   BStaticArray<BYTE, 4096> buf;

   uint crc32 = 0;
   if ((pData) && (dataLen)) 
      crc32 = calcCRC32(pData, dataLen);
      
   BuildSystemProtocol::BPacketHeader packetHeader(dataLen, crc32, (BYTE)packetType, 0);

   if ((pData) && (dataLen))
   {
      eTcpClientError status = mClient.send(&packetHeader, sizeof(packetHeader), false);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: send() failed\n");
         return false;
      }

      status = mClient.send(pData, dataLen, true);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: send() failed\n");
         return false;      
      }
   }  
   else
   {
      eTcpClientError status = mClient.send(&packetHeader, sizeof(packetHeader), true);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: send() failed\n");
         return false;
      }
   }
            
   if (wait)
   {
      eTcpClientError status = mClient.sendFlush(INFINITE);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: sendFlush() failed\n");
         return false;
      }
   }         
   
   mLastKeepAliveSent = GetTickCount();

   return true;
}

bool BBuildServer::BHelperThread::receivePacket(bool& receivedPacked, BuildSystemProtocol::BPacketHeader& header, BByteArray& data, bool waitForData)
{
   receivedPacked = false;
               
   uint bytesReceived;
   if (mClient.receive(&header, sizeof(header), bytesReceived, BTcpClient::cReceivePeek) < 0)
   {
      logErrorMessage("receivePacket: receive() failed\n");
      return false;
   }

   if (bytesReceived < sizeof(header))
   {
      if (mClient.getConnectionStatus() == BTcpClient::cClosedGracefully)
      {
         logErrorMessage("receivePacket: Connection closed prematurely\n");
         return false;
      }
      
      return true;
   }      
   
   if (header.mSig != BuildSystemProtocol::BPacketHeader::cSig)
   {
      logErrorMessage("receivePacket: Bad packet header sig\n");
      return false;
   }    
      
   if (header.mHeaderCRC16 != header.computeHeaderCRC16())   
   {
      logErrorMessage("receivePacket: Bad packet header CRC\n");
      return false;
   }
   
   if (waitForData)
   {
      uint totalDataSize = header.mDataSize;
      if (totalDataSize > 128U * 1024U * 1024U)
      {
         logErrorMessage("receivePacket: Packet data is too big: %i\n", totalDataSize);
         return false;
      }
                  
      uint bytesAvailable;
      if (mClient.getReceiveBytesAvail(bytesAvailable, false) < 0)
      {
         logErrorMessage("receivePacket: getReceiveBytesAvail() failed\n");
         return false;
      }
      
      if (bytesAvailable < (totalDataSize + sizeof(header)))
      {
         if (mClient.getConnectionStatus() == BTcpClient::cClosedGracefully)
         {
            logErrorMessage("receivePacket: Connection closed prematurely\n");
            return false;
         }
         else
            return true;
      }
      
      if (mClient.receive(&header, sizeof(header), bytesReceived, 0) < 0)
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }
      
      if (bytesReceived != sizeof(header))
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }
      
      data.resize(totalDataSize);   
      if (mClient.receive(data.getPtr(), totalDataSize, bytesReceived, 0) < 0)
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }
      
      if (bytesReceived != totalDataSize)
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }
      
      uint crc32 = cInitCRC32;
      if (totalDataSize)      
         crc32 = calcCRC32(data.getPtr(), totalDataSize);
         
      if (crc32 != header.mDataCRC32)
      {
         logErrorMessage("receivePacket: Bad data CRC\n");
         return false;
      }
   }
   else
   {
      if (mClient.receive(&header, sizeof(header), bytesReceived, 0) < 0)
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }
      
      if (bytesReceived != sizeof(header))
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;  
      }
   }
   
   receivedPacked = true;
  
   return true;
}

bool BBuildServer::BHelperThread::tickConnection()
{
#if SEND_KEEPALIVE_PACKETS
   const DWORD curTime = GetTickCount();
   
   const DWORD timeSinceLastPacketSent = (curTime - mLastKeepAliveSent) / 1000U;
   if (timeSinceLastPacketSent > 10)
   {
      if (!sendPacket(BuildSystemProtocol::cPTKeepAlive, NULL, 0, false))
         return false;
   }
   
   DWORD timeSinceLastPacketReceived = (curTime - mLastKeepAliveReceived) / 1000U;
   if (timeSinceLastPacketReceived > cConnectionTimeoutTime)
   {
      logErrorMessage("BBuildServer::BHelperThread::tickConnection: Client has not sent any packets in the last 60 seconds - disconnecting! (0x%08X 0x%08X)\n", curTime, mLastKeepAliveReceived);
      disconnectAndExit(true);
   }
#endif   
   
   return true;
}

bool BBuildServer::BHelperThread::processHelloPacket(const BNameValueMap& helloPacketData)
{
   if ( (!helloPacketData.get("ProtocolVersion", mProtocolVersion)) ||
        (!helloPacketData.get("SystemVersion", mBuildSystemVersion)) ||
        (!helloPacketData.get("NumCPUs", mNumCPUs)) )
   {
      logErrorMessage("BBuildServer::BHelperThread::processHelloPacket: Invalid hello packet");
      return false;
   }
   
   if (mProtocolVersion != BuildSystemProtocol::cBuildProtocolVersion)
   {  
      logErrorMessage("BBuildServer::BHelperThread::processHelloPacket: Unsupported protocol version");
      return false;
   }
   
   if (mBuildSystemVersion < BuildSystemProtocol::cBuildSystemVersion)
   {  
      logErrorMessage("BBuildServer::BHelperThread::processHelloPacket: Outdated build system version");
      return false;
   }
         
   return true;
}

bool BBuildServer::BHelperThread::processByePacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData)
{
   packetHeader;
   packetData;
   
   const bool failed = getNumProcessesActive() ? true : false;
   
   disconnectAndExit(failed);
      
   return true;
}

bool BBuildServer::BHelperThread::processProcessCompletedPacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData)
{
   if ((packetHeader.mFlags & BuildSystemProtocol::BPacketHeader::cFlagDataIsNameValueMap) == 0)
   {
      logErrorMessage("processProcessCompletedPacket: Invalid packet\n");
      return false;
   }
   
   BNameValueMap nameValueMap;
   if (!nameValueMap.deserialize(packetData.getPtr(), packetData.getSize()))
   {
      logErrorMessage("processProcessCompletedPacket: nameValueMap deserialization failed\n");
      return false;
   }
   
   uint64 buildTaskID = cInvalidBuildTaskID;
   if (!nameValueMap.get("ID", buildTaskID))
   {
      logErrorMessage("processProcessCompletedPacket: Packet missing ID\n");
      return false;
   }
     
   int processIndex = -1;
   for (uint i = 0; i < cMaxHelperProcesses; i++)
   {
      if (buildTaskID == mActiveProcesses[i])
      {
         processIndex = i;
         break;
      }
   }
   if (processIndex < 0)
   {
      logErrorMessage("processProcessCompletedPacket: Received invalid process ID\n");
      return false;
   }
   
   int result;
   if (!nameValueMap.get("Result", result))
   {
      logErrorMessage("processProcessCompletedPacket: Packet missing result\n");
      return false;
   }
   
   uint numFiles;
   if (!nameValueMap.get("NumFiles", numFiles))
   {
      logErrorMessage("processProcessCompletedPacket: Packet missing NumFiles\n");
      return false;
   }
   
   uint numMissingFiles;
   if (!nameValueMap.get("NumMissingFiles", numMissingFiles))
   {
      logErrorMessage("processProcessCompletedPacket: Packet missing NumMissingFiles\n");
      return false;
   }
   
   BDynamicArray<BString> fileNames(numFiles);
   BDynamicArray<uint64> fileSizes(numFiles);
   BDynamicArray<uint32> fileCRCs(numFiles);
   uint64 totalBytes = 0;
   
   for (uint i = 0; i < numFiles; i++)
   {
      if ( (!nameValueMap.get(BFixedString256(cVarArg, "Name%i", i), fileNames[i])) ||
           (!nameValueMap.get(BFixedString256(cVarArg, "Size%i", i), fileSizes[i])) ||
           (!nameValueMap.get(BFixedString256(cVarArg, "CRC%i", i), fileCRCs[i])) )
      {
         logErrorMessage("processProcessCompletedPacket: Invalid packet\n");
         return false;
      }          
      
      totalBytes += fileSizes[i];
   }
   
   logMessage("BBuildServer::BHelperThread::processProcessCompletedPacket: Process %i, ID 0x%08I64X, Result: %i\n",
      processIndex,
      buildTaskID,
      result);
      
   const uint cBufSize = 256U * 1024U;
   BByteArray buf(cBufSize);
   
   for (uint i = 0; i < numFiles; i++)
   {
      //BString filename(fileNames[i] + "_temp");
      const BString& filename = fileNames[i];
      
      BWin32File file;
      if (!file.create(filename.getPtr(), BWin32File::cWriteAccess | BWin32File::cSequentialAccess | BWin32File::cCreateAlways))
      {
         logErrorMessage("BBuildServer::BHelperThread::processProcessCompletedPacket: failed creating file: %s\n", filename.getPtr());
         result = BBuildTaskDesc::cFailedWritingFileResult;
      }
      else
      {
         logMessage("BBuildServer::BHelperThread::processProcessCompletedPacket: Creating file: %s\n", filename.getPtr());
      }
      
      uint32 curCRC32 = cInitCRC32;
      uint64 curNumBytes = 0;
      
      while (curNumBytes < fileSizes[i])
      {
         const uint64 bytesRemaining = fileSizes[i] - curNumBytes;
         const uint bytesToReceive = (uint)Math::Min<uint64>(bytesRemaining, cBufSize);
               
         uint receiveLen;
         if (mClient.receive(buf.getPtr(), bytesToReceive, receiveLen, BTcpClient::cReceiveWait) < 0)
         {
            logErrorMessage("BBuildServer::BHelperThread::processProcessCompletedPacket: receive() failed: %s\n", filename.getPtr());
            return false;
         }
         if (receiveLen < bytesToReceive)
         {
            logErrorMessage("BBuildServer::BHelperThread::processProcessCompletedPacket: Connection closed prematurely: %s\n", filename.getPtr());
            return false;
         }
         
         curCRC32 = calcCRC32(buf.getPtr(), bytesToReceive, curCRC32);
         curNumBytes += bytesToReceive;
         
         if (file.isOpen())
         {
            if (!file.write(buf.getPtr(), bytesToReceive))
            {
               file.close();
               
               logErrorMessage("BBuildServer::BHelperThread::processProcessCompletedPacket: Failed creating file: %s\n", filename.getPtr());
               result = BBuildTaskDesc::cFailedWritingFileResult;     
            }
         }
      }
      
      if (file.isOpen())
      {
         if (!file.close())
         {
            logErrorMessage("BBuildServer::BHelperThread::processProcessCompletedPacket: Failed closing file: %s\n", filename.getPtr());
            result = BBuildTaskDesc::cFailedWritingFileResult;
         }
      }
      
      if (curCRC32 != fileCRCs[i])
      {
         logErrorMessage("BBuildServer::BHelperThread::processProcessCompletedPacket: Bad file data CRC: %s\n", filename.getPtr());
         return false;
      }
   }
         
   finalizeTask(buildTaskID, result, false, numMissingFiles, &fileNames);

   return true;
}

bool BBuildServer::BHelperThread::processPackets(
   BuildSystemProtocol::ePacketTypes packetType, bool& receivedRequestedPacket, BByteArray* pPacketData, BNameValueMap* pNameValueMap)
{
   receivedRequestedPacket = false;
   
   for ( ; ; )
   {
      BuildSystemProtocol::BPacketHeader packetHeader;
      BByteArray packetData;
      bool receivedPacket;
      if (!receivePacket(receivedPacket, packetHeader, packetData, true))
         return false;
      
      if (!receivedPacket)
         break;
      
      mLastKeepAliveReceived = GetTickCount();
      
      if ((packetType != BuildSystemProtocol::cPTInvalid) && (packetHeader.mType == packetType))
      {
         if (!packetHeader.mDataSize)
         {
            if (pPacketData) pPacketData->clear();
            if (pNameValueMap) pNameValueMap->clear();
         }
         else
         {
            if (pPacketData)
               *pPacketData = packetData;
            
            if (pNameValueMap)               
            {
               if ((packetHeader.mFlags & BuildSystemProtocol::BPacketHeader::cFlagDataIsNameValueMap) == 0)
               {
                  logErrorMessage("processPackets: Received expected packet type, but cFlagDataIsNameValueMap is not set\n");
                  return false;
               }
                  
               if (!pNameValueMap->deserialize(packetData.getPtr(), packetData.getSize()))
               {
                  logErrorMessage("processPackets: nameValueMap deserialization failed\n");
                  return false;
               }
            }                  
         }               
         
         receivedRequestedPacket = true;
         break;
      }
      
      switch (packetHeader.mType)
      {
         case BuildSystemProtocol::cPTKeepAlive:
         {
            break;
         }
         case BuildSystemProtocol::cPTBye:
         {
            if (!processByePacket(packetHeader, packetData))
               return false;
            break;
         }
         case BuildSystemProtocol::cPTProcessCompleted:
         {
            if (!processProcessCompletedPacket(packetHeader, packetData))
               return false;
            break;
         }
         default:
         {
            logErrorMessage("processPackets: Received unexpected/invalid packet type\n");  
            return false;
         }
      }
   }         
   
   return true;
}

bool BBuildServer::BHelperThread::waitForPacket(BuildSystemProtocol::ePacketTypes packetType, BByteArray* pPacketData, BNameValueMap* pNameValueMap)
{
   for ( ; ; )
   {
      bool receivedPacket;
      if (!processPackets(packetType, receivedPacket, pPacketData, pNameValueMap))
         return false;
      if (receivedPacket)
         return true;
      
      bool receiveIsPending;
      HANDLE receiveHandle = mClient.getReceiveEvent(&receiveIsPending).getHandle();
            
      bool sendIsPending;
      HANDLE sendHandle = mClient.getSendEvent(&sendIsPending).getHandle();
      
      HANDLE handles[2];
      handles[0] = receiveHandle;
      handles[1] = sendHandle;
      
      mpServer->mMonitor.enter();

      int waitResult = mpServer->mMonitor.waitForCondition(BBuildServer::waitUntilExitingOrAutoUpdate, mpServer, 0, 2, handles, 10000);
      
      const bool exiting = mpServer->mExiting;
      const bool autoUpdate = mpServer->mSubmitAutoUpdate;

      mpServer->mMonitor.leave();
      
      if (waitResult < 0)
      {
         if (!tickConnection())
            return false;
      }
      else if (waitResult == 0)
      {
         // condition function is true
         if (exiting)
         {
            disconnectAndExit(false);
         }
         else if (autoUpdate)
         {
            if (!sendPacket(BuildSystemProtocol::cPTAutoUpdate, NULL, 0, true))
               return false;

            disconnectAndExit(false);
         }
      }
   }   

   return false;
}

void BBuildServer::BHelperThread::disconnectAndExit(bool failed)
{
   updateState(cStateDisconnecting);
   
   if (mClient.getConnectionStatus() == BTcpClient::cOpen)
   {
      if (!failed)
      {
         if (!cancelAllTasks())
            logErrorMessage("BBuildServer::BHelperThread::disconnectAndExit: Failed canceling all tasks\n");
      }            
      
      if (!sendPacket(BuildSystemProtocol::cPTBye, NULL, 0, true))
         logErrorMessage("BBuildServer::BHelperThread::disconnectAndExit: Failed sending bye packet\n");
   }
   
   mClient.close(10000, 10000);
   
   finalizeAllTasks(true);
      
   updateState(failed ? BBuildServer::cStateFailed : BBuildServer::cStateDisconnected);
   
   _endthreadex(failed ? FALSE : TRUE);
}

uint BBuildServer::BHelperThread::getNumProcessesActive() const
{
   uint total = 0;
   for (uint i = 0; i < cMaxHelperProcesses; i++)
      if (mActiveProcesses[i] != cInvalidBuildTaskID)
         total++;
   return total;
}

uint BBuildServer::BHelperThread::getNumProcessesAvailable() const
{
   uint total = 0;
   for (uint i = 0; i < cMaxHelperProcesses; i++)
      if (mActiveProcesses[i] == cInvalidBuildTaskID)
         total++;
   return total;
}

int BBuildServer::BHelperThread::getNextAvailableProcess() const
{
   for (uint i = 0; i < cMaxHelperProcesses; i++)
      if (mActiveProcesses[i] == cInvalidBuildTaskID)
         return i;
   return -1;
}

bool BBuildServer::BHelperThread::cancelAllTasks()
{
   logMessage("BBuildServer::BHelperThread::cancelAllTasks: Canceling all tasks\n");
   
   if (!getNumProcessesActive())
      return true;
   
   if (!sendPacket(BuildSystemProtocol::cPTKillAllProcesses, NULL, 0, true))
      return false;
      
   if (!waitForPacket(BuildSystemProtocol::cPTKillAllProcessesReply, NULL, NULL))
      return false;
   
   finalizeAllTasks(true);
   
   logMessage("BBuildServer::BHelperThread::cancelAllTasks: Cancelation completed\n");
      
   return true;
}

void BBuildServer::BHelperThread::finalizeAllTasks(bool resubmit)
{
   for (uint i = 0; i < cMaxHelperProcesses; i++)
      if (mActiveProcesses[i] != cInvalidBuildTaskID)
         finalizeTask(mActiveProcesses[i], BBuildTaskDesc::cCanceledResult, resubmit, 0, NULL);
}

void BBuildServer::BHelperThread::finalizeTask(BBuildTaskID buildTaskID, int result, bool resubmit, uint numMissingFiles, const BDynamicArray<BString>* pFilesWritten)
{
   logMessage("Finalizing task: ID 0x%08I64X, Result: %i, Resubmit: %i\n", buildTaskID, result, resubmit);
   
   const uint activeTaskSlot = getActiveTaskSlotIndex(buildTaskID);
   const uint processIndex = getProcessIndex(buildTaskID);
   
   mpServer->mMonitor.enter();
   
   BBuildTask& buildTask = mpServer->mActiveTasks[activeTaskSlot];
   BDEBUG_ASSERT(buildTask.mID == buildTaskID);
   BDEBUG_ASSERT(buildTask.mHelperIndex == mHelperIndex);
   
   BBuildTaskDesc desc(buildTask.mDesc);
         
   if (resubmit)
   {
      if (desc.mAllClients)
         resubmit = false;
      else 
         mpServer->mTaskQueue.pushBack(desc);
   }
      
   mActiveProcesses[processIndex] = cInvalidBuildTaskID;
   
   buildTask.clear();
   
   if (!resubmit)
      mpServer->mNumPendingTaskCompletions++;
   
   mpServer->mMonitor.leave();
   
   if (!resubmit)
   {
      BDEBUG_ASSERT(desc.mpCompletionCallback);
      desc.mpCompletionCallback(desc, result, numMissingFiles, pFilesWritten);
   
      mpServer->mMonitor.enter();
      
      mpServer->mNumPendingTaskCompletions--;

      mpServer->mMonitor.leave();
   }      
}

bool BBuildServer::BHelperThread::startNewProcess()
{
   BBuildTaskDesc desc;
   
   bool success = false;
   
   if (mpServer->mHelperTaskQueue[mHelperIndex].getSize())
      success = mpServer->mHelperTaskQueue[mHelperIndex].popFront(desc);
   else
      success = mpServer->mTaskQueue.popFront(desc);
      
   BDEBUG_ASSERT(success);
      
   const uint processIndex = getNextAvailableProcess();
   const uint activeTaskSlot = mpServer->findActiveTaskSlot();
   const BBuildTaskID buildTaskID = createBuildTaskID(mpServer->mNextBuildTaskID++, processIndex, activeTaskSlot);
   
   mActiveProcesses[processIndex] = buildTaskID;
   
   BBuildTask& buildTask = mpServer->mActiveTasks[activeTaskSlot];

   buildTask.mDesc = desc;
   buildTask.mID = buildTaskID;
   buildTask.mHelperIndex = mHelperIndex;
         
   mpServer->mMonitor.leave();
   
   logMessage("BBuildServer::BHelperThread::startNewProcess: Starting new process: ID 0x%08I64X, exec: \"%s\", args: \"%s\", path: \"%s\" NumOutFiles: %u\n",
      buildTaskID,
      desc.mExec.getPtr(),
      desc.mPath.getPtr(),
      desc.mArgs.getPtr(),
      desc.mOutputFilenames.getSize());
   
   BNameValueMap nameValueMap;
   nameValueMap.add("ID", buildTaskID);
   nameValueMap.add("Exec", desc.mExec.getPtr());
   nameValueMap.add("Path", desc.mPath.getPtr());
   nameValueMap.add("Args", desc.mArgs.getPtr());
   nameValueMap.add("NumOutFiles", desc.mOutputFilenames.getSize());
   for (uint i = 0; i < desc.mOutputFilenames.getSize(); i++)
   {
      BFixedString256 outputName(cVarArg, "OutFile%i", i);
      nameValueMap.add(outputName.getPtr(), desc.mOutputFilenames[i].getPtr());
   }
   nameValueMap.sort();
   
   if (!sendPacket(BuildSystemProtocol::cPTExecuteProcess, nameValueMap, true))
      return false;
   
   if (!waitForPacket(BuildSystemProtocol::cPTExecuteProcessReply, NULL, &nameValueMap))
      return false;

   int result;
   nameValueMap.get("Result", result);
   if (!result)
   {
      logErrorMessage("startNewProcess: Client failed executing process \"%s\"\n", desc.mExec.getPtr());
      
      finalizeTask(buildTaskID, BBuildTaskDesc::cFailedExecutingResult, false, 0, NULL);
   }      
      
   return true;
}

unsigned BBuildServer::BHelperThread::threadMethod()
{
   updateState(cStateConnecting);
   
   logMessage("BBuildServer::BHelperThread::threadMethod: Attempting to connect to client %s\n", mAddress.getPtr());
   
   eTcpClientError status = mClient.connect(mAddress.getPtr(), BuildSystemProtocol::cBuildSystemPort, BTcpClient::cDefaultBufSize, true, 10000);
   
   if (status != cTCE_Success)
   {
      logErrorMessage("BBuildServer::BHelperThread::threadMethod: Failed connecting to client %s\n", mAddress.getPtr());
      disconnectAndExit(true);
   }
   
   logMessage("BBuildServer::BHelperThread::threadMethod: Connecting to client %s, sending hello packet\n", mAddress.getPtr());
         
   mClient.setSendTimeout(120*1000);
   mClient.setReceiveTimeout(120*1000);
   
   BNameValueMap nameValueMap;
   nameValueMap.add("ProtocolVersion", (uint16)BuildSystemProtocol::cBuildProtocolVersion);
   nameValueMap.add("SystemVersion", (uint16)BuildSystemProtocol::cBuildSystemVersion);
   
   mLastKeepAliveSent = GetTickCount();
   
   if (!sendPacket(BuildSystemProtocol::cPTHello, nameValueMap))
      disconnectAndExit(true);
   
   mLastKeepAliveReceived = mLastKeepAliveSent;
   
   if (!waitForPacket(BuildSystemProtocol::cPTHello, NULL, &nameValueMap))
      disconnectAndExit(true);
   
   if (!processHelloPacket(nameValueMap))
      disconnectAndExit(true);
   
   updateState(cStateIdle);
   
   logMessage("BBuildServer::BHelperThread::threadMethod: Successfully connected to client %s\n", mAddress.getPtr());
         
   for ( ; ; )
   {
      bool receivedPacket;
      if (!processPackets(BuildSystemProtocol::cPTInvalid, receivedPacket, NULL, NULL))
         break;
      BDEBUG_ASSERT(!receivedPacket);
                        
      bool receiveIsPending;
      HANDLE receiveHandle = mClient.getReceiveEvent(&receiveIsPending).getHandle();

      bool sendIsPending;
      HANDLE sendHandle = mClient.getSendEvent(&sendIsPending).getHandle();

      HANDLE handles[2];
      handles[0] = receiveHandle;
      handles[1] = sendHandle;
      
      BDEBUG_ASSERT(mpServer->mMonitor.getCurLockCount() == 0);
                                 
      mpServer->mMonitor.enter();
      
      const uint numProcessesActive = getNumProcessesActive();
      BDEBUG_ASSERT(numProcessesActive <= mpServer->mMaxActiveProcesses);
      const uint numProcessesAvailable = Math::Min(mNumCPUs, mpServer->mMaxActiveProcesses) - numProcessesActive;
      
      updateState(numProcessesActive ? cStateBusy : cStateIdle);
      
      int waitResult = mpServer->mMonitor.waitForCondition(
         numProcessesAvailable ? BBuildServer::waitUntilTaskOrExitingOrAutoUpdateOrCancelingAll : BBuildServer::waitUntilExitingOrAutoUpdateOrCancelingAll, 
         mpServer, mHelperIndex, 2, handles, 10000);
      
      const bool taskAvailable   = (mpServer->mTaskQueue.getSize() > 0) || (mpServer->mHelperTaskQueue[mHelperIndex].getSize() > 0);
      const bool exiting         = mpServer->mExiting;
      const bool cancelAll       = mpServer->mCancelAll;
      const bool autoUpdate      = mpServer->mSubmitAutoUpdate;
                                                            
      if (waitResult == -1)
      {
         // wait timed out
         mpServer->mMonitor.leave();
                     
         if (!tickConnection())
            break;
      }
      else if (waitResult == 0)
      {
         // condition function returned true
         if (exiting)
         {
            mpServer->mMonitor.leave();
            
            disconnectAndExit(false);
         }
         else if (cancelAll)
         {
            mpServer->mMonitor.leave();
            
            if (!cancelAllTasks())
               break;
         }
         else if (autoUpdate)
         {
            mpServer->mMonitor.leave();
            
            if (!sendPacket(BuildSystemProtocol::cPTAutoUpdate, NULL, 0, true))
               break;
               
            disconnectAndExit(false);
         }
         else if ((numProcessesAvailable) && (taskAvailable))
         {
            if (!startNewProcess())
               break;
         }   
         else
         {
            mpServer->mMonitor.leave();
         }
      }
      else
      {
         mpServer->mMonitor.leave();
      }
      
      BDEBUG_ASSERT(mpServer->mMonitor.getCurLockCount() == 0);
   }

   disconnectAndExit(true);
   
   return 0;
}

unsigned BBuildServer::BHelperThread::threadMethodGuarded()
{
   unsigned result = 0;
   
#ifndef BUILD_DEBUG   
   __try
#endif   
   {
      result = threadMethod();
   }
#ifndef BUILD_DEBUG   
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      logErrorMessage("BBuildServer::BHelperThread::threadMethod: Unhandled exception!\n");

      finalizeAllTasks(true);

      _endthreadex(2);
   }
#endif

   return result;
}

void BBuildServer::BHelperThread::logMessage(const char* pMsg, ...)
{
   const uint cMaxTextBufSize = 4096;
   
   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);
   
   gConsoleOutput.printf("[%02u:%s] %s", mHelperIndex, mAddress.getPtr(), buf);
}

void BBuildServer::BHelperThread::logErrorMessage(const char* pMsg, ...)
{
   const uint cMaxTextBufSize = 4096;

   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.error("[%02u:%s] %s", mHelperIndex, mAddress.getPtr(), buf);
}

unsigned __stdcall BBuildServer::BHelperThread::threadFunc(void* pArguments)
{
   return ((BBuildServer::BHelperThread*)pArguments)->threadMethodGuarded();
}

// File: buildClient.cpp
#include "xcore.h"
#include "buildClient.h"
#include "clientAdvertisement.h"
#include "utils\spawn.h"

#include "containers\staticArray.h"
#include "file\win32FileUtils.h"
#include "file\win32File.h"

#define SEND_KEEPALIVE_PACKETS 1

#ifdef BUILD_DEBUG
   const uint cConnectionTimeoutTime = 6000;
#else
   const uint cConnectionTimeoutTime = 120;
#endif

BBuildClient::BBuildClient() :
   mNumCPUs(1),
   mEnableExecUpdate(false),
   mHelperThread(INVALID_HANDLE_VALUE),
   mLastKeepAliveSent(0),
   mLastKeepAliveReceived(0),
   mServerProtocolVersion(0),
   mServerSystemVersion(0),
   mReceivedAutoUpdatePacket(false)
{
   Utils::ClearObj(mServerSocket);
   Utils::ClearObj(mServerAddress);
   
   clearProcesses();
   
   SYSTEM_INFO sysInfo;
   Utils::ClearObj(sysInfo);
   GetSystemInfo(&sysInfo);
   
   mNumCPUs = sysInfo.dwNumberOfProcessors;
}

BBuildClient::~BBuildClient()
{
}

void BBuildClient::clearProcesses()
{
   for (uint i = 0; i < cMaxHelperProcesses; i++)
      mProcesses[i].clear();
}

uint BBuildClient::getNumActiveProcesses() const
{
   uint total = 0;
   for (uint i = 0; i < cMaxHelperProcesses; i++)
      if (INVALID_HANDLE_VALUE != mProcesses[i].mHandle)
         total++;
   return total;
}

bool BBuildClient::getFreeProcessIndex(uint& index) const
{
   index = 0;
   
   for (uint i = 0; i < cMaxHelperProcesses; i++)
   {
      if (INVALID_HANDLE_VALUE == mProcesses[i].mHandle)
      {
         index = i;
         return true;
      }
   }
   
   return false;
}

bool BBuildClient::findProcessByHandle(HANDLE handle, uint& index) const
{
   index = 0;

   for (uint i = 0; i < cMaxHelperProcesses; i++)
   {
      if (handle == mProcesses[i].mHandle)
      {
         index = i;
         return true;
      }
   }

   return false;
}

bool BBuildClient::sendPacket(BuildSystemProtocol::ePacketTypes packetType, const BNameValueMap& nameValueMap, bool wait)
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

bool BBuildClient::sendPacket(BuildSystemProtocol::ePacketTypes packetType, void* pData, uint dataLen, bool wait)
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

bool BBuildClient::receivePacket(bool& receivedPacked, BuildSystemProtocol::BPacketHeader& header, BByteArray& data, bool waitForData)
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
      logErrorMessage("receivePacket: Bad packet header sig (type %u, expected 0x%08X, received 0x%08X)\n", header.mType, BuildSystemProtocol::BPacketHeader::cSig, header.mSig);
      return false;
   }    

   const uint actualHeaderCRC16 = header.computeHeaderCRC16();
   if (header.mHeaderCRC16 != actualHeaderCRC16)
   {
      logErrorMessage("receivePacket: Bad packet header CRC (type %u, expected 0x%04X, received 0x%04X)\n", header.mType, header.mHeaderCRC16, actualHeaderCRC16);
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
         logErrorMessage("receivePacket: Bad data CRC (type %u, expected 0x%08X, received 0x%08X\n", header.mType, header.mDataCRC32, crc32);
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

bool BBuildClient::tickConnection()
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
      logErrorMessage("BBuildClient::tickConnection: Server has not sent any packets in the last 2 minutes - disconnecting! (0x%08X 0x%08X)\n", curTime, mLastKeepAliveReceived);
      disconnectAndExit(true);
   }
#endif

   return true;
}

bool BBuildClient::processByePacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData)
{
   packetHeader;
   packetData;
   
   logMessage("BBuildClient::processByePacket: Received bye packet\n");
   
   disconnectAndExit(false);
   return true;
}

bool BBuildClient::processAutoUpdatePacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData)
{
   packetHeader;
   packetData;

   logMessage("BBuildClient::processAutoUpdatePacket: Received auto update packet\n");
   
   mReceivedAutoUpdatePacket = true;

   disconnectAndExit(false);
   return true;
}

bool BBuildClient::processKillAllProcessesPacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData)
{
   packetHeader;
   packetData;
   
   terminateAllProcesses();
   
   if (!sendPacket(BuildSystemProtocol::cPTKillAllProcessesReply, NULL, 0, true))
      return false;
   
   return true;
}

bool BBuildClient::processExecuteProcessPacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData)
{
   if ((packetHeader.mFlags & BuildSystemProtocol::BPacketHeader::cFlagDataIsNameValueMap) == 0)
   {
      logErrorMessage("BBuildClient::processExecuteProcessPacket: Invalid packet\n");
      return false;
   }
   
   BNameValueMap nameValueMap;
   if (!nameValueMap.deserialize(packetData.getPtr(), packetData.getSize()))
   {
      logErrorMessage("BBuildClient::processExecuteProcessPacket: Failed deserializing nameValueMap\n");
      return false;
   }

   uint64 ID;
   uint numOutFiles;
   BString exec, path, args;
   if ( (!nameValueMap.get("ID", ID)) ||
        (!nameValueMap.get("Exec", exec)) ||
        (!nameValueMap.get("Path", path)) ||
        (!nameValueMap.get("Args", args)) ||
        (!nameValueMap.get("NumOutFiles", numOutFiles)) )
   {
      logErrorMessage("BBuildClient::processExecuteProcessPacket: Invalid packet\n");
      return false;
   }
   
   BDynamicArray<BString> outputFilenames(numOutFiles);
   for (uint i = 0; i < numOutFiles; i++)
   {
      if (!nameValueMap.get(BFixedString256(cVarArg, "OutFile%i", i), outputFilenames[i]))
      {
         logErrorMessage("BBuildClient::processExecuteProcessPacket: Invalid packet\n");
         return false;
      }
      
      BWin32FileUtils::createDirectories(outputFilenames[i]);
   }
   
   uint processIndex;
   if (!getFreeProcessIndex(processIndex))
   {
      logErrorMessage("BBuildClient::processExecuteProcessPacket: Out of process slots\n");
      return false;
   }
   
   logMessage("Executing process %u, ID 0x%08I64X, exec \"%s\", args: \"%s\", dir: \"%s\"\n", processIndex, ID, exec.getPtr(), args.getPtr(), path.getPtr());
   
   BProcessState& processState = mProcesses[processIndex];
   processState.clear();
   processState.mID = ID;
   processState.mArgs = args;
   processState.mExec = exec;
   processState.mCurDir = path;
   processState.mOutputFiles = outputFilenames;

   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   Utils::ClearObj(si);
   si.cb = sizeof(si);
   
   Utils::ClearObj(pi);
   
   BOOL succeeded = FALSE;
   if (!BWin32FileUtils::doesFileExist(exec))
   {
      //gConsoleOutput.printf("1: %s\n", args.getPtr());
      
      succeeded = CreateProcessA( 
         NULL,   
         (LPSTR)args.getPtr(),
         NULL,             // Process handle not inheritable. 
         NULL,             // Thread handle not inheritable. 
         FALSE,            // Set handle inheritance to FALSE. 
         0,                // No creation flags. 
         NULL,             // Use parent's environment block. 
         (LPSTR)path.getPtr(),    // Use parent's starting directory. 
         &si,              // Pointer to STARTUPINFO structure.
         &pi );            // Pointer to PROCESS_INFORMATION structure.
   }
   else
   {
      //gConsoleOutput.printf("2: %s %s\n", exec.getPtr(), args.getPtr());
      
      succeeded = CreateProcessA( 
         (LPSTR)exec.getPtr(),   
         (LPSTR)args.getPtr(),
         NULL,             // Process handle not inheritable. 
         NULL,             // Thread handle not inheritable. 
         FALSE,            // Set handle inheritance to FALSE. 
         0,                // No creation flags. 
         NULL,             // Use parent's environment block. 
         (LPSTR)path.getPtr(),    // Use parent's starting directory. 
         &si,              // Pointer to STARTUPINFO structure.
         &pi );            // Pointer to PROCESS_INFORMATION structure.
   }         
   
   CloseHandle(pi.hThread);

   processState.mHandle = pi.hProcess;
         
   int result = TRUE;
   if (!succeeded)
   {
      logErrorMessage("Failed executing process: \"%s\", args: \"%s\", dir: \"%s\"\n", exec.getPtr(), args.getPtr(), path.getPtr());
      
      result = FALSE;
      
      CloseHandle(processState.mHandle);
      
      processState.clear();      
   }
      
   nameValueMap.clear();

   nameValueMap.add("Result", result);
   
   if (!sendPacket(BuildSystemProtocol::cPTExecuteProcessReply, nameValueMap, true))
      return false;
   
   return true;
}

struct BOutputFileInfo
{
   BString  mFilename;
   uint64   mFilesize;
   uint32   mCRC32;
};
typedef BDynamicArray<BOutputFileInfo> BOutputFileInfoArray;

bool BBuildClient::handleProcessCompletion(HANDLE handle)
{
   logMessage("\n");
   
   uint processIndex;
   if (!findProcessByHandle(handle, processIndex))
   {
      logErrorMessage("BBuildClient::handleProcessCompletion: Invalid handle\n");
      return false;
   }
   
   BProcessState& processState = mProcesses[processIndex];
   
   DWORD exitCode = 1;
   GetExitCodeProcess(handle, &exitCode);
         
   CloseHandle(handle);
   processState.mHandle = INVALID_HANDLE_VALUE;
   
   logMessage("BBuildClient::handleProcessCompletion: Process: %u, ID: 0x%08I64X, exec: \"%s\" args: \"%s\" completed with exitcode %u\n", processIndex, processState.mID, processState.mExec.getPtr(), processState.mArgs.getPtr(), exitCode);
   
   BNameValueMap nameValueMap;
   nameValueMap.add("ID", processState.mID);
   nameValueMap.add("Result", (int)exitCode);
   
   BOutputFileInfoArray outputFiles;

   const uint cBufSize = 512U * 1024U;   
   BByteArray buf(cBufSize);
   
   uint numMissingFiles = 0;
   for (uint i = 0; i < processState.mOutputFiles.getSize(); i++)
   {
      if (!BWin32FileUtils::doesFileExist(processState.mOutputFiles[i].getPtr()))
      {
         logErrorMessage("BBuildClient::handleProcessCompletion: Expected output file does not exist: \"%s\", exec: \"%s\", args: \"%s\"\n", processState.mOutputFiles[i].getPtr(), processState.mExec.getPtr(), processState.mArgs.getPtr());
         numMissingFiles++;
         continue;
      }
                  
      BWin32File file;
      if (!file.open(processState.mOutputFiles[i].getPtr(), BWin32File::cSequentialAccess))
      {
         logErrorMessage("BBuildClient::handleProcessCompletion: Failed opening file: \"%s\", exec: \"%s\", args: \"%s\"\n", processState.mOutputFiles[i].getPtr(), processState.mExec.getPtr(), processState.mArgs.getPtr());
         numMissingFiles++;
         continue;
      }
            
      uint64 fileSize = 0;
      file.getSize(fileSize);
      
      uint crc32 = cInitCRC32;
                  
      uint64 bytesLeft = fileSize;
      while (bytesLeft)
      {
         const uint bytesToRead = (uint)Math::Min<uint64>(bytesLeft, buf.getSize());
         
         if (file.read(buf.getPtr(), bytesToRead) != bytesToRead)
         {
            logErrorMessage("BBuildClient::handleProcessCompletion: Failed reading from file: %s, exec: \"%s\", args: \"%s\"\n", processState.mOutputFiles[i].getPtr(), processState.mExec.getPtr(), processState.mArgs.getPtr());
            numMissingFiles++;
            continue;
         }
         
         crc32 = calcCRC32(buf.getPtr(), bytesToRead, crc32);
         
         bytesLeft -= bytesToRead;
      }
      
      file.close();
      
      logMessage("Read file: %s, Size: %I64u, CRC32: 0x%08X\n", processState.mOutputFiles[i].getPtr(), fileSize, crc32);
      
      const uint outputFileIndex = outputFiles.getSize();
      
      BOutputFileInfo& outputFileInfo = outputFiles.grow();
      outputFileInfo.mFilename = processState.mOutputFiles[i];
      outputFileInfo.mFilesize = fileSize;
      outputFileInfo.mCRC32 = crc32;
      
      nameValueMap.add(BFixedString256(cVarArg, "Name%i", outputFileIndex), outputFileInfo.mFilename);
      nameValueMap.add(BFixedString256(cVarArg, "Size%i", outputFileIndex), outputFileInfo.mFilesize);
      nameValueMap.add(BFixedString256(cVarArg, "CRC%i", outputFileIndex), outputFileInfo.mCRC32);

// HACK HACK      
//uint checkCRC;
//nameValueMap.get(BFixedString256(cVarArg, "CRC%i", outputFileIndex), checkCRC);
//BVERIFY(checkCRC == outputFileInfo.mCRC32);
   }
   
   nameValueMap.add("NumFiles", outputFiles.getSize());
   nameValueMap.add("NumMissingFiles", numMissingFiles);
   nameValueMap.sort();

   if (!sendPacket(BuildSystemProtocol::cPTProcessCompleted, nameValueMap, false))
      return false;
   
   for (uint i = 0; i < outputFiles.getSize(); i++)
   {
      BWin32File file;
      if (!file.open(outputFiles[i].mFilename.getPtr(), BWin32File::cSequentialAccess))
      {
         logErrorMessage("BBuildClient::handleProcessCompletion: Failed opening file: \"%s\", exec: \"%s\", args: \"%s\"\n", outputFiles[i].mFilename.getPtr(), processState.mExec.getPtr(), processState.mArgs.getPtr());
         disconnectAndExit(true);
      }

      uint64 fileSize;
      file.getSize(fileSize);
      
      if (fileSize != outputFiles[i].mFilesize)
      {
         logErrorMessage("BBuildClient::handleProcessCompletion: File size has changed: \"%s\", exec: \"%s\", args: \"%s\"\n", outputFiles[i].mFilename.getPtr(), processState.mExec.getPtr(), processState.mArgs.getPtr());
         disconnectAndExit(true);
      }
      
      uint crc32 = cInitCRC32;
      
      uint64 bytesLeft = fileSize;
      while (bytesLeft)
      {
         const uint bytesToRead = (uint)Math::Min<uint64>(bytesLeft, buf.getSize());

         if (file.read(buf.getPtr(), bytesToRead) != bytesToRead)
         {
            logErrorMessage("BBuildClient::handleProcessCompletion: Failed reading from file: \"%s\", exec: \"%s\", args: \"%s\"\n", outputFiles[i].mFilename.getPtr(), processState.mExec.getPtr(), processState.mArgs.getPtr());
            disconnectAndExit(true);
         }
         
         if (cTCE_Success != mClient.send(buf.getPtr(), bytesToRead, false))
         {
            logErrorMessage("BBuildClient::handleProcessCompletion: send() failed!\n");
            disconnectAndExit(true);
         }
         
         crc32 = calcCRC32(buf.getPtr(), bytesToRead, crc32);
         
         bytesLeft -= bytesToRead;
      }  
      
      if (crc32 != outputFiles[i].mCRC32)
      {
         logErrorMessage("BBuildClient::handleProcessCompletion: File data has changed while sending to server: \"%s\", exec: \"%s\", args: \"%s\"\n", outputFiles[i].mFilename.getPtr(), processState.mExec.getPtr(), processState.mArgs.getPtr());
         disconnectAndExit(true);
      }
   }            
   
   if (cTCE_Success != mClient.sendFlush(INFINITE)) 
   {
      logErrorMessage("BBuildClient::handleProcessCompletion: sendFlush() failed!\n");
      disconnectAndExit(true);
   }
   
   processState.clear();

   return true;
}

bool BBuildClient::processPackets(
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
         case BuildSystemProtocol::cPTAutoUpdate:
         {
            if (!processAutoUpdatePacket(packetHeader, packetData))
               return false;
            break;
         }
         case BuildSystemProtocol::cPTKillAllProcesses:
         {
            if (!processKillAllProcessesPacket(packetHeader, packetData))
               return false;
            break;
         }
         case BuildSystemProtocol::cPTExecuteProcess:
         {
            if (!processExecuteProcessPacket(packetHeader, packetData))
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

bool BBuildClient::waitForPacket(BuildSystemProtocol::ePacketTypes packetType, BByteArray* pPacketData, BNameValueMap* pNameValueMap)
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

      const uint cMaxHandles = 64;
      HANDLE handles[cMaxHandles];
      handles[0] = receiveHandle;
      handles[1] = sendHandle;
      handles[2] = mExitEvent.getHandle();
      uint totalHandles = 3;
      
      const uint firstProcessHandle = totalHandles;
      for (uint i = 0; i < cMaxHelperProcesses; i++)
         if (mProcesses[i].mHandle != INVALID_HANDLE_VALUE)
            handles[totalHandles++] = mProcesses[i].mHandle;
                  
      uint result = WaitForMultipleObjects(totalHandles, handles, FALSE, 10000);
      
      if (WAIT_TIMEOUT == result)
      {
         if (!tickConnection())
            return false;
      }
      else if ((result >= WAIT_OBJECT_0) && (result < (WAIT_OBJECT_0 + totalHandles)))
      {
         const uint handleIndex = result - WAIT_OBJECT_0;
         
         BVERIFY(handleIndex < totalHandles);
         
         if (handleIndex == 2)
         {
            disconnectAndExit(false);
         }
         else if (handleIndex >= firstProcessHandle)
         {
            if (!handleProcessCompletion(handles[handleIndex]))
               return false;
         }
      }
      else
      {
         BVERIFY(false);
      }
   }   

   return false;
}

bool BBuildClient::terminateAllProcesses()
{  
   for (uint i = 0; i < cMaxHelperProcesses; i++)
   {
      if (INVALID_HANDLE_VALUE != mProcesses[i].mHandle)
      {
         logErrorMessage("Terminating process: %s\n", mProcesses[i].mExec.getPtr());
         
         TerminateProcess(mProcesses[i].mHandle, 100);
         mProcesses[i].clear();
      }
   }
   
   return true;
}

void BBuildClient::disconnectAndExit(bool failed)
{
   if (failed)
      logErrorMessage("BBuildClient::disconnectAndExit: Disconnecting (failure)\n");
   else
      logMessage("BBuildClient::disconnectAndExit: Disconnecting (normal)\n");
   
   if (mClient.getConnectionStatus() == BTcpClient::cOpen)
   {
      if (!sendPacket(BuildSystemProtocol::cPTBye, NULL, 0, true))
         logErrorMessage("BBuildClient::disconnectAndExit: Failed sending bye packet\n");
   }
   
   mClient.close(10000, 10000);
   
   terminateAllProcesses();

   _endthreadex(failed ? FALSE : TRUE);
}

unsigned BBuildClient::threadMethod()
{
   if (mClient.attach(mServerSocket, BTcpClient::cDefaultBufSize, true) < 0)
   {
      logErrorMessage("BBuildClient::threadMethod: attach() failed!");
      disconnectAndExit(true);
   }

   BNameValueMap nameValueMap;
   nameValueMap.add("NumCPUs", mNumCPUs);
   nameValueMap.add("ProtocolVersion", BuildSystemProtocol::cBuildProtocolVersion);
   nameValueMap.add("SystemVersion", BuildSystemProtocol::cBuildSystemVersion);

   char buf[256];
   DWORD bufSize = sizeof(buf);         
   if (!GetComputerNameA(buf, &bufSize))
      buf[0] = '\0';
   nameValueMap.add("ComputerName", buf);
   
   bufSize = sizeof(buf);
   if (!GetUserNameA(buf, &bufSize))
      buf[0] = '\0';
   nameValueMap.add("UserName", buf);
      
   if (!sendPacket(BuildSystemProtocol::cPTHello, nameValueMap, true))
      disconnectAndExit(true);
               
   if (!waitForPacket(BuildSystemProtocol::cPTHello, NULL, &nameValueMap))
      disconnectAndExit(true);
      
   logMessage("BBuildClient::threadMethod: Received hello packet\n");
      
   if ( (!nameValueMap.get("ProtocolVersion", mServerProtocolVersion)) ||
        (!nameValueMap.get("SystemVersion", mServerSystemVersion)) )
   {
      logErrorMessage("BBuildClient::threadMethod: Invalid hello packet\n");
      disconnectAndExit(true);
   }        
   
   if (mServerProtocolVersion != BuildSystemProtocol::cBuildProtocolVersion)
   {
      logErrorMessage("BBuildClient::threadMethod: Unsupported protocol version\n");
      disconnectAndExit(true);
   }
   
   if (mServerSystemVersion < BuildSystemProtocol::cBuildSystemVersion)
   {
      logErrorMessage("BBuildClient::threadMethod: Unsupported build system version\n");
      disconnectAndExit(true);
   }
   
   logMessage("BBuildClient::threadMethod: Hello packet OK\n");
   
   for ( ; ; )
   {
      if (!waitForPacket(BuildSystemProtocol::cPTInvalid, NULL, NULL))
         break;
   }
   
   disconnectAndExit(true);
   
   return 1;
}

unsigned __stdcall BBuildClient::threadFunc(void* pArguments)
{
   return ((BBuildClient*)pArguments)->threadMethod();
}

void BBuildClient::terminateHelperThread()
{
   if (mHelperThread == INVALID_HANDLE_VALUE)
      return;
   
   gConsoleOutput.warning("BBuildClient::terminateHelperThread: Waiting to terminate helper thread\n");
      
   mExitEvent.set();
   
   WaitForSingleObject(mHelperThread, INFINITE);
   
   mExitEvent.reset();
   
   gConsoleOutput.warning("BBuildClient::terminateHelperThread: Wait completed\n");
}

bool BBuildClient::handleConnection()
{
   logMessage("BBuildClient::handleConnection: Incoming connection\n");
   
   terminateHelperThread();
      
   uintptr_t result = _beginthreadex(NULL, 0, threadFunc, this, 0, NULL);
   BVERIFY(result != 0);

   mHelperThread = (HANDLE)result;
   
   return true;
}

bool BBuildClient::parseCommandLineArgs(BCommandLineParser::BStringArray& args)
{
   bool clientFlag = false;
   
   const BCLParam clParams[] =
   {
      {"enableExecUpdate",    cCLParamTypeFlag,             &mEnableExecUpdate },
      {"listenAddress",       cCLParamTypeBStringPtr,       &mListenAddress },
      {"client",              cCLParamTypeFlag,             &clientFlag },
      { NULL } 
   };

   BCommandLineParser parser(clParams);

   const bool success = parser.parse(args, false, false);
   return success;
}

bool BBuildClient::checkExecutable(bool force)
{
   char buf[MAX_PATH];
   if (!GetModuleFileNameA(GetModuleHandle(NULL), buf, sizeof(buf)))
      return false;
   
   BString masterExec(buf);
   masterExec.standardizePath();
      
   BString path, name;
   strPathSplit(masterExec, path, name);
   
   const bool isRunningClient = (name.findLeft("client.exe") >= 0);
                        
   BString clientExec(path);
   clientExec += "client.exe";
   
   if (isRunningClient)
   {
      masterExec = path;
#ifdef BUILD_DEBUG
      masterExec += "dataBuildD.exe";
#else      
      masterExec += "dataBuild.exe";
#endif      
   }
   
   gConsoleOutput.printf("Master executable: %s\nClient executable: %s\n", masterExec.getPtr(), clientExec.getPtr());
      
   WIN32_FILE_ATTRIBUTE_DATA masterExecAttr;
   Utils::ClearObj(masterExecAttr);
   DWORD masterExecStatus = GetFileAttributesEx(masterExec, GetFileExInfoStandard, &masterExecAttr);
   
   WIN32_FILE_ATTRIBUTE_DATA clientExecAttr;
   Utils::ClearObj(clientExecAttr);
   DWORD clientExecStatus = GetFileAttributesEx(clientExec, GetFileExInfoStandard, &clientExecAttr);
   
   bool clientIsUpToDate = false;
   if ((masterExecStatus) && (clientExecStatus))
      clientIsUpToDate = (Utils::FileTimeToUInt64(clientExecAttr.ftLastWriteTime) >= Utils::FileTimeToUInt64(masterExecAttr.ftLastWriteTime));
  
   if (clientIsUpToDate) 
      gConsoleOutput.printf("Client executable %s is up to date.\n", clientExec.getPtr());
   else
      gConsoleOutput.warning("Client executable %s is NOT up of date!\n", clientExec.getPtr());
                
   if (force) 
      clientIsUpToDate = false;      
                    
   if (isRunningClient)
   {
      if (!clientIsUpToDate)
      {
         gConsoleOutput.warning("Executing process: %s\n", masterExec.getPtr());
         
         BSpawnCommand spawnCommand;
         spawnCommand.set(masterExec, mCommandLineArgs);
         spawnCommand.run(_P_OVERLAY);

         gConsoleOutput.error("Failed executing process: %s\n", masterExec.getPtr());
         return false;
      }
   }
   else
   {
      if (!clientIsUpToDate)
      {
         SetFileAttributes(clientExec, FILE_ATTRIBUTE_NORMAL);
         
         uint tries;
         for (tries = 0; tries < 10; tries++)
         {
            if (BWin32FileUtils::copyFile(masterExec, clientExec, false))
               break;
            Sleep(1000);
         }
         
         if (tries == 10)
         {
            gConsoleOutput.error("Unable to copy file %s to %s after 10 tries!\n", masterExec.getPtr(), clientExec.getPtr());
            return false;
         }
         gConsoleOutput.warning("Copied file %s to %s\n", masterExec.getPtr(), clientExec.getPtr());
      }
      
      gConsoleOutput.warning("Executing process: %s\n", clientExec.getPtr());
 
      BSpawnCommand spawnCommand;
      spawnCommand.set(clientExec, mCommandLineArgs);
      spawnCommand.run(_P_OVERLAY);
      
      gConsoleOutput.error("Failed executing process: %s\n", clientExec.getPtr());
      return false;
   }
         
   return true;
}

bool BBuildClient::handleAutoUpdate(void)
{
   mReceivedAutoUpdatePacket = false;

   BClientAdvertisementManager::destroy();

   mListener.close();
         
   if (!checkExecutable(true))
      gConsoleOutput.error("handleAutoUpdate: checkExecutable() failed!");
   else
      gConsoleOutput.error("handleAutoUpdate: checkExecutable() returned!\n");
   
   return false;
}

bool BBuildClient::runInternal(BCommandLineParser::BStringArray& args)
{
   mCommandLineArgs = args;
   
   if (!parseCommandLineArgs(args))
      return false;
      
   if ((mEnableExecUpdate) && (!IsDebuggerPresent()))
   {
      if (!checkExecutable(false))
         return false;
   }
   
   BTcpClient::winsockSetup();
   
   BString actualListenAddress;
   if (cTCE_Success != mListener.listen(mListenAddress.length() ? mListenAddress.getPtr() : NULL, BuildSystemProtocol::cBuildSystemPort, &actualListenAddress))
   {
      logErrorMessage("BBuildClient::run: listen() failed!");
      
      BTcpClient::winsockCleanup();
      
      return false;  
   }
   
   BClientAdvertisementManager::create(actualListenAddress);
         
   gConsoleOutput.printf("Listing for connections at: %s:%u\n", actualListenAddress.getPtr(), BuildSystemProtocol::cBuildSystemPort);
   
   bool status = false;
   
   for ( ; ; )
   {
      eTcpClientError result = mListener.accept(&mServerSocket, &mServerAddress, 1000);
      
      if (result == cTCE_Success)
         handleConnection();
      else if (result != cTCE_WaitTimedOut)
      {
         logErrorMessage("BBuildClient::run: accept() failed\n");
         break;
      }
      
      if (mReceivedAutoUpdatePacket)
      {
         if (!handleAutoUpdate())
            break;
      }
   }
      
   mListener.close();
   BTcpClient::winsockCleanup();
   
   return status;
}

static BOOL WINAPI ConsoleControlHandlerRoutine(DWORD dwCtrlType)
{
   dwCtrlType;
   BClientAdvertisementManager::destroy();
   return FALSE;
}

bool BBuildClient::run(BCommandLineParser::BStringArray& args)
{
   bool status = false;
   
   SetConsoleCtrlHandler(ConsoleControlHandlerRoutine, TRUE);
   
   __try
   {
      status = runInternal(args);
   }
   __finally
   {
      BClientAdvertisementManager::destroy();
   }
   
   return status;
}

void BBuildClient::logMessage(const char* pMsg, ...)
{
   const uint cMaxTextBufSize = 4096;

   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.printf("%s", buf);
}

void BBuildClient::logErrorMessage(const char* pMsg, ...)
{
   const uint cMaxTextBufSize = 4096;

   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.error("%s", buf);
}